#include <algorithm>
#include <cmath>
#include <cstring>
#include <prism/math/constants.hpp>
#include <prism/render/rasterizer.hpp>

namespace prism::render {

using namespace math;

// ── Colour helpers ────────────────────────────────────────────────────────────

static inline uint32_t vec3_to_rgba(Vec3 c) noexcept {
    uint8_t r = static_cast<uint8_t>(clamp(c.x, 0.0f, 1.0f) * 255.0f);
    uint8_t g = static_cast<uint8_t>(clamp(c.y, 0.0f, 1.0f) * 255.0f);
    uint8_t b = static_cast<uint8_t>(clamp(c.z, 0.0f, 1.0f) * 255.0f);
    return (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}

// ── Blinn-Phong shading ───────────────────────────────────────────────────────

static Vec3 blinn_phong(const Vec3& frag_pos, const Vec3& frag_normal, const Vec3& view_dir,
                        const Material& mat, const std::vector<Light>& lights) noexcept {
    Vec3 n = normalize(frag_normal);
    Vec3 result = mat.ambient;

    for (const Light& light : lights) {
        Vec3 l_dir;
        float attenuation = 1.0f;

        if (light.type == LightType::Directional) {
            l_dir = normalize(-light.direction_or_position);
        } else {
            Vec3 lv = light.direction_or_position - frag_pos;
            float dist = length(lv);
            if (dist < kEps)
                continue;
            l_dir = lv * (1.0f / dist);
            float d = std::max(dist, kEps);
            attenuation = 1.0f / (1.0f + (d / light.radius) * (d / light.radius));
        }

        float ndotl = std::max(dot(n, l_dir), 0.0f);
        Vec3 diffuse = mat.diffuse * ndotl;

        Vec3 h = normalize(l_dir + view_dir);
        float ndoth = std::max(dot(n, h), 0.0f);
        Vec3 specular = mat.specular * std::pow(ndoth, mat.shininess);

        result = result + (diffuse + specular) * light.color * (light.intensity * attenuation);
    }

    return result;
}

// ── Texture sampling (bilinear) ───────────────────────────────────────────────

static Vec3 sample_texture_bilinear(const uint32_t* tex, int w, int h, float u, float v) noexcept {
    u = u - std::floor(u); // wrap
    v = v - std::floor(v);
    float fx = u * static_cast<float>(w - 1);
    float fy = v * static_cast<float>(h - 1);
    int ix = static_cast<int>(fx);
    int iy = static_cast<int>(fy);
    float tx = fx - static_cast<float>(ix);
    float ty = fy - static_cast<float>(iy);

    int ix1 = std::min(ix + 1, w - 1);
    int iy1 = std::min(iy + 1, h - 1);

    auto fetch = [&](int px, int py) -> Vec3 {
        uint32_t c = tex[static_cast<size_t>(py) * w + px];
        return Vec3(((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f);
    };

    Vec3 c00 = fetch(ix, iy);
    Vec3 c10 = fetch(ix1, iy);
    Vec3 c01 = fetch(ix, iy1);
    Vec3 c11 = fetch(ix1, iy1);

    Vec3 top = lerp(c00, c10, tx);
    Vec3 bottom = lerp(c01, c11, tx);
    return lerp(top, bottom, ty);
}

// ── Edge function ─────────────────────────────────────────────────────────────

static inline float edge_function(float ax, float ay, float bx, float by, float px,
                                  float py) noexcept {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

// ── Wireframe ─────────────────────────────────────────────────────────────────

void draw_wireframe_triangle(Framebuffer& fb, const ScreenSpaceVertex& v0,
                             const ScreenSpaceVertex& v1, const ScreenSpaceVertex& v2,
                             uint32_t color) noexcept {
    fb.draw_line(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v1.x),
                 static_cast<int>(v1.y), color);
    fb.draw_line(static_cast<int>(v1.x), static_cast<int>(v1.y), static_cast<int>(v2.x),
                 static_cast<int>(v2.y), color);
    fb.draw_line(static_cast<int>(v2.x), static_cast<int>(v2.y), static_cast<int>(v0.x),
                 static_cast<int>(v0.y), color);
}

// ── Triangle rasterizer ───────────────────────────────────────────────────────

void rasterize_triangle(Framebuffer& fb, const ScreenSpaceVertex& v0, const ScreenSpaceVertex& v1,
                        const ScreenSpaceVertex& v2, const Material& material,
                        const std::vector<Light>& lights, const Vec3& camera_pos,
                        ShadingMode shading) noexcept {
    // Bounding box clamped to screen
    int x_min = std::max(0, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
    int x_max = std::min(fb.width() - 1, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    int y_min = std::max(0, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
    int y_max =
        std::min(fb.height() - 1, static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))));

    if (x_min > x_max || y_min > y_max)
        return;

    float area = edge_function(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
    if (area <= 0.0f)
        return; // Degenerate or back-facing (already culled above)

    float inv_area = 1.0f / area;

    // Flat shading: pre-compute face normal from interpolated vertex normals
    Vec3 flat_normal = normalize(v0.normal + v1.normal + v2.normal);
    Vec3 face_center =
        (Vec3(v0.x, v0.y, v0.z) + Vec3(v1.x, v1.y, v1.z) + Vec3(v2.x, v2.y, v2.z)) * (1.0f / 3.0f);
    Vec3 flat_view_dir = normalize(camera_pos - face_center);
    uint32_t flat_color = 0;
    if (shading == ShadingMode::Flat) {
        Vec3 lit = blinn_phong(face_center, flat_normal, flat_view_dir, material, lights);
        flat_color = vec3_to_rgba(lit);
    }

    // Gouraud shading: pre-compute lighting at the 3 vertices
    Vec3 gouraud_c0, gouraud_c1, gouraud_c2;
    if (shading == ShadingMode::Gouraud) {
        Vec3 p0(v0.x, v0.y, v0.z), p1(v1.x, v1.y, v1.z), p2(v2.x, v2.y, v2.z);
        gouraud_c0 = blinn_phong(p0, v0.normal, normalize(camera_pos - p0), material, lights);
        gouraud_c1 = blinn_phong(p1, v1.normal, normalize(camera_pos - p1), material, lights);
        gouraud_c2 = blinn_phong(p2, v2.normal, normalize(camera_pos - p2), material, lights);
    }

    for (int y = y_min; y <= y_max; ++y) {
        for (int x = x_min; x <= x_max; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float py = static_cast<float>(y) + 0.5f;

            float w0 = edge_function(v1.x, v1.y, v2.x, v2.y, px, py) * inv_area;
            float w1 = edge_function(v2.x, v2.y, v0.x, v0.y, px, py) * inv_area;
            float w2 = edge_function(v0.x, v0.y, v1.x, v1.y, px, py) * inv_area;

            // Top-left fill rule: pixel inside if all weights >= 0, with tie-breaking
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            // Perspective-correct interpolation using 1/w
            float iw0 = w0 * v0.inv_w;
            float iw1 = w1 * v1.inv_w;
            float iw2 = w2 * v2.inv_w;
            float iw_sum = iw0 + iw1 + iw2;
            if (iw_sum < kEps)
                continue;
            float inv_iw = 1.0f / iw_sum;
            float c0 = iw0 * inv_iw;
            float c1 = iw1 * inv_iw;
            float c2 = iw2 * inv_iw;

            // Depth interpolation (linear in screen space is fine for depth)
            float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

            // Depth test
            if (depth >= fb.get_depth(x, y))
                continue;

            // Shading
            uint32_t pixel_color;
            if (shading == ShadingMode::Flat) {
                pixel_color = flat_color;
            } else if (shading == ShadingMode::Gouraud) {
                Vec3 lit = gouraud_c0 * c0 + gouraud_c1 * c1 + gouraud_c2 * c2;
                if (material.texture_data) {
                    Vec2 uv = v0.uv * c0 + v1.uv * c1 + v2.uv * c2;
                    lit =
                        lit * sample_texture_bilinear(material.texture_data, material.texture_width,
                                                      material.texture_height, uv.x, uv.y);
                }
                pixel_color = vec3_to_rgba(lit);
            } else {
                // Perspective-correct normal and UV (Phong)
                Vec3 n = normalize(v0.normal * c0 + v1.normal * c1 + v2.normal * c2);
                Vec2 uv = v0.uv * c0 + v1.uv * c1 + v2.uv * c2;

                Vec3 frag_pos = Vec3(static_cast<float>(x), static_cast<float>(y), depth);
                Vec3 view_dir = normalize(camera_pos - frag_pos);

                Vec3 base_color = material.diffuse;
                if (material.texture_data) {
                    base_color = base_color * sample_texture_bilinear(
                                                  material.texture_data, material.texture_width,
                                                  material.texture_height, uv.x, uv.y);
                }

                Material frag_mat = material;
                frag_mat.diffuse = base_color;
                Vec3 lit = blinn_phong(frag_pos, n, view_dir, frag_mat, lights);
                pixel_color = vec3_to_rgba(lit);
            }

            fb.set_pixel(x, y, pixel_color);
            float* depth_buf = fb.depth_data();
            depth_buf[static_cast<size_t>(y) * fb.width() + x] = depth;
        }
    }
}

// ── Sutherland-Hodgman Clip ───────────────────────────────────────────────────

// Clip a list of ClipSpaceVertex against a single plane: w + component >= 0 or w - component >= 0
using ClipList = std::vector<ClipSpaceVertex>;

static void clip_against_plane(ClipList& input, ClipList& output, int comp, float sign) noexcept {
    output.clear();
    if (input.empty())
        return;
    auto inside = [&](const ClipSpaceVertex& v) {
        // sign * v[comp] <= v.w
        float component = (comp == 0) ? v.position.x : (comp == 1) ? v.position.y : v.position.z;
        return sign * component <= v.position.w;
    };
    auto lerp_vertex = [&](const ClipSpaceVertex& a, const ClipSpaceVertex& b,
                           float t) -> ClipSpaceVertex {
        ClipSpaceVertex out;
        out.position =
            Vec4(lerp(a.position.x, b.position.x, t), lerp(a.position.y, b.position.y, t),
                 lerp(a.position.z, b.position.z, t), lerp(a.position.w, b.position.w, t));
        out.normal = normalize(lerp(a.normal, b.normal, t));
        out.uv = lerp(a.uv, b.uv, t);
        return out;
    };

    for (size_t i = 0; i < input.size(); ++i) {
        const ClipSpaceVertex& cur = input[i];
        const ClipSpaceVertex& prev = input[(i + input.size() - 1) % input.size()];
        bool cur_inside = inside(cur);
        bool prev_inside = inside(prev);
        if (cur_inside) {
            if (!prev_inside) {
                // Enter: add intersection
                float comp_prev = (comp == 0)   ? prev.position.x
                                  : (comp == 1) ? prev.position.y
                                                : prev.position.z;
                float comp_cur = (comp == 0)   ? cur.position.x
                                 : (comp == 1) ? cur.position.y
                                               : cur.position.z;
                float t =
                    (prev.position.w - sign * comp_prev) /
                    ((prev.position.w - sign * comp_prev) - (cur.position.w - sign * comp_cur));
                output.push_back(lerp_vertex(prev, cur, t));
            }
            output.push_back(cur);
        } else if (prev_inside) {
            // Exit: add intersection
            float comp_prev = (comp == 0)   ? prev.position.x
                              : (comp == 1) ? prev.position.y
                                            : prev.position.z;
            float comp_cur = (comp == 0)   ? cur.position.x
                             : (comp == 1) ? cur.position.y
                                           : cur.position.z;
            float t = (prev.position.w - sign * comp_prev) /
                      ((prev.position.w - sign * comp_prev) - (cur.position.w - sign * comp_cur));
            output.push_back(lerp_vertex(prev, cur, t));
        }
    }
}

// ── Full pipeline ─────────────────────────────────────────────────────────────

void draw_triangles(Framebuffer& fb, const std::vector<Triangle>& triangles,
                    const Mat4& model_matrix, const Camera& camera, const Material& material,
                    const std::vector<Light>& lights, const RenderConfig& config) noexcept {
    Mat4 view = camera.view_matrix();
    Mat4 proj = camera.proj_matrix();
    int W = fb.width();
    int H = fb.height();

    ClipList clip_a, clip_b;
    clip_a.reserve(8);
    clip_b.reserve(8);

    for (const Triangle& tri : triangles) {
        // --- Model → World → View → Clip ---
        ClipSpaceVertex cv[3];
        for (int k = 0; k < 3; ++k) {
            Vec4 wpos = model_matrix * Vec4(tri.v[k].position, 1.0f);
            Vec4 clip = proj * (view * wpos);
            cv[k].position = clip;
            cv[k].normal = normalize((model_matrix * Vec4(tri.v[k].normal, 0.0f)).xyz());
            cv[k].uv = tri.v[k].uv;
        }

        // --- Back-face cull in clip space ---
        if (config.backface_culling) {
            Vec3 a = cv[0].position.xyz();
            Vec3 b = cv[1].position.xyz();
            Vec3 c = cv[2].position.xyz();
            Vec3 normal_clip = cross(b - a, c - a);
            // If normal_clip.z <= 0, triangle faces away
            if (normal_clip.z <= 0.0f)
                continue;
        }

        // --- Sutherland-Hodgman clip against 6 planes (w+x, w-x, w+y, w-y, w+z, w-z) ---
        clip_a.assign(cv, cv + 3);
        clip_against_plane(clip_a, clip_b, 0, +1.0f); // x <= w  (right)
        clip_against_plane(clip_b, clip_a, 0, -1.0f); // -x <= w (left)
        clip_against_plane(clip_a, clip_b, 1, +1.0f); // y <= w  (top)
        clip_against_plane(clip_b, clip_a, 1, -1.0f); // -y <= w (bottom)
        clip_against_plane(clip_a, clip_b, 2, +1.0f); // z <= w  (far)
        clip_against_plane(clip_b, clip_a, 2, -1.0f); // -z <= w (near)

        if (clip_a.size() < 3)
            continue;

        // --- Fan-triangulate the clipped polygon ---
        for (size_t i = 1; i + 1 < clip_a.size(); ++i) {
            const ClipSpaceVertex* pts[3] = {&clip_a[0], &clip_a[i], &clip_a[i + 1]};

            // --- NDC ---
            NDCSpaceVertex nv[3];
            for (int k = 0; k < 3; ++k) {
                nv[k] = transform_clip_to_ndc(*pts[k]);
            }

            // --- Screen ---
            ScreenSpaceVertex sv[3];
            for (int k = 0; k < 3; ++k) {
                sv[k] = transform_ndc_to_screen(nv[k], W, H);
            }

            if (config.wireframe) {
                draw_wireframe_triangle(fb, sv[0], sv[1], sv[2], 0xFFFFFFFF);
            } else {
                rasterize_triangle(fb, sv[0], sv[1], sv[2], material, lights, camera.position,
                                   config.shading);
            }
        }
    }
}

void draw_wireframe(Framebuffer& fb, const std::vector<Triangle>& triangles,
                    const Mat4& model_matrix, const Camera& camera, uint32_t color) noexcept {
    RenderConfig cfg;
    cfg.wireframe = true;
    cfg.backface_culling = false;
    Material mat;
    std::vector<Light> no_lights;
    draw_triangles(fb, triangles, model_matrix, camera, mat, no_lights, cfg);
    (void)color; // color unused; wireframe uses white for now
}

// ── Mesh builders ─────────────────────────────────────────────────────────────

std::vector<Triangle> make_cube() noexcept {
    std::vector<Triangle> tris;
    tris.reserve(12);

    // 8 corners of a unit cube centred at origin
    Vec3 corners[8] = {
        {-0.5f, -0.5f, -0.5f}, {+0.5f, -0.5f, -0.5f}, {+0.5f, +0.5f, -0.5f}, {-0.5f, +0.5f, -0.5f},
        {-0.5f, -0.5f, +0.5f}, {+0.5f, -0.5f, +0.5f}, {+0.5f, +0.5f, +0.5f}, {-0.5f, +0.5f, +0.5f},
    };

    // Face normal, corner indices (CCW when viewed from outside), UVs
    struct FaceDesc {
        Vec3 normal;
        int idx[4];
    };

    FaceDesc faces[6] = {
        {{0, 0, -1}, {0, 3, 2, 1}}, // back  -Z
        {{0, 0, +1}, {4, 5, 6, 7}}, // front +Z
        {{-1, 0, 0}, {0, 4, 7, 3}}, // left  -X
        {{+1, 0, 0}, {1, 2, 6, 5}}, // right +X
        {{0, -1, 0}, {0, 1, 5, 4}}, // bottom-Y
        {{0, +1, 0}, {3, 7, 6, 2}}, // top   +Y
    };

    Vec2 face_uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    for (auto& f : faces) {
        // Two triangles per face
        int order0[3] = {0, 1, 2};
        int order1[3] = {0, 2, 3};
        for (auto* o : {order0, order1}) {
            Triangle t;
            for (int k = 0; k < 3; ++k) {
                t.v[k].position = corners[f.idx[o[k]]];
                t.v[k].normal = f.normal;
                t.v[k].uv = face_uvs[o[k]];
                t.v[k].color = Vec3(1.0f, 1.0f, 1.0f);
            }
            tris.push_back(t);
        }
    }
    return tris;
}

} // namespace prism::render
