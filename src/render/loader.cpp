#include <charconv>
#include <cstring>
#include <fstream>
#include <prism/render/loader.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace prism::render {

using namespace math;

// ── OBJ Loader ────────────────────────────────────────────────────────────────

std::vector<Triangle> load_obj(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file)
        return {};

    std::vector<Vec3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;
    std::vector<Triangle> triangles;

    positions.emplace_back(Vec3(0.0f, 0.0f, 0.0f)); // 1-indexed padding
    texcoords.emplace_back(Vec2(0.0f, 0.0f));
    normals.emplace_back(Vec3(0.0f, 1.0f, 0.0f));

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            Vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (token == "vt") {
            Vec2 t;
            ss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (token == "vn") {
            Vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (token == "f") {
            // Parse face: "v/vt/vn" or "v//vn" or "v/vt" or "v"
            struct FaceVert {
                int vi{0}, ti{0}, ni{0};
            };
            std::vector<FaceVert> face_verts;
            std::string part;
            while (ss >> part) {
                FaceVert fv;
                int slash1 = -1, slash2 = -1;
                for (int i = 0; i < static_cast<int>(part.size()); ++i) {
                    if (part[i] == '/') {
                        if (slash1 < 0)
                            slash1 = i;
                        else
                            slash2 = i;
                    }
                }
                fv.vi = std::atoi(part.c_str());
                if (slash1 >= 0 && slash1 + 1 < static_cast<int>(part.size()) &&
                    (slash2 < 0 || part[slash1 + 1] != '/')) {
                    fv.ti = std::atoi(part.c_str() + slash1 + 1);
                }
                if (slash2 >= 0) {
                    fv.ni = std::atoi(part.c_str() + slash2 + 1);
                }
                // Handle negative indices
                if (fv.vi < 0)
                    fv.vi = static_cast<int>(positions.size()) + fv.vi;
                if (fv.ti < 0)
                    fv.ti = static_cast<int>(texcoords.size()) + fv.ti;
                if (fv.ni < 0)
                    fv.ni = static_cast<int>(normals.size()) + fv.ni;
                face_verts.push_back(fv);
            }

            // Triangulate fan from vert 0
            for (size_t i = 1; i + 1 < face_verts.size(); ++i) {
                Triangle t;
                int indices[3] = {0, static_cast<int>(i), static_cast<int>(i + 1)};
                for (int k = 0; k < 3; ++k) {
                    const auto& fv = face_verts[indices[k]];
                    int vi = (fv.vi >= 1 && fv.vi < static_cast<int>(positions.size())) ? fv.vi : 0;
                    int ti = (fv.ti >= 1 && fv.ti < static_cast<int>(texcoords.size())) ? fv.ti : 0;
                    int ni = (fv.ni >= 1 && fv.ni < static_cast<int>(normals.size())) ? fv.ni : 0;
                    t.v[k].position = positions[vi];
                    t.v[k].uv = texcoords[ti];
                    t.v[k].normal = normals[ni];
                    t.v[k].color = Vec3(1.0f, 1.0f, 1.0f);
                }
                // If no normals, compute face normal
                if (face_verts[0].ni == 0) {
                    Vec3 e1 = t.v[1].position - t.v[0].position;
                    Vec3 e2 = t.v[2].position - t.v[0].position;
                    Vec3 n = normalize(cross(e1, e2));
                    for (int k = 0; k < 3; ++k)
                        t.v[k].normal = n;
                }
                triangles.push_back(t);
            }
        }
    }

    return triangles;
}

// ── PPM Loader (P6 binary) ────────────────────────────────────────────────────

TextureData load_ppm(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        return {};

    std::string magic;
    file >> magic;
    if (magic != "P6")
        return {};

    int width = 0, height = 0, max_val = 0;
    file >> width >> height >> max_val;
    if (width <= 0 || height <= 0 || max_val <= 0)
        return {};
    file.get(); // consume newline

    TextureData td;
    td.width = width;
    td.height = height;
    td.pixels.resize(static_cast<size_t>(width) * height);

    std::vector<uint8_t> rgb(static_cast<size_t>(width) * height * 3);
    file.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));

    float scale = (max_val != 255) ? (255.0f / static_cast<float>(max_val)) : 1.0f;
    for (size_t i = 0; i < td.pixels.size(); ++i) {
        uint8_t r = static_cast<uint8_t>(rgb[i * 3 + 0] * scale);
        uint8_t g = static_cast<uint8_t>(rgb[i * 3 + 1] * scale);
        uint8_t b = static_cast<uint8_t>(rgb[i * 3 + 2] * scale);
        td.pixels[i] =
            (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    }
    return td;
}

// ── Checkerboard ───────────────────────────────────────────────────────────────

TextureData make_checkerboard(int size, uint32_t c0, uint32_t c1) {
    TextureData td;
    td.width = size;
    td.height = size;
    td.pixels.resize(static_cast<size_t>(size) * size);
    int cell = size / 8;
    if (cell < 1)
        cell = 1;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool checker = ((x / cell) + (y / cell)) % 2 == 0;
            td.pixels[static_cast<size_t>(y) * size + x] = checker ? c0 : c1;
        }
    }
    return td;
}

// ── Procedural Stone, Floor, Crystal, Metal ───────────────────────────────────

static inline float hash2d(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - static_cast<float>((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /
                       1073741824.0f) *
               0.5f +
           0.5f;
}

TextureData make_stone_wall_texture(int size) {
    TextureData td;
    td.width = size;
    td.height = size;
    td.pixels.resize(static_cast<size_t>(size) * size);

    int rows = 4;
    int cols = 4;
    int brick_h = size / rows;
    int brick_w = size / cols;
    int mortar = std::max(2, size / 32);

    for (int y = 0; y < size; ++y) {
        int row = y / brick_h;
        int in_y = y % brick_h;
        int shift = (row % 2 == 1) ? (brick_w / 2) : 0;

        for (int x = 0; x < size; ++x) {
            int shifted_x = (x + shift) % size;
            int col = shifted_x / brick_w;
            int in_x = shifted_x % brick_w;

            bool is_mortar = (in_y < mortar) || (in_x < mortar);
            float noise = hash2d(x, y) * 0.25f - 0.125f;

            uint8_t r, g, b;
            if (is_mortar) {
                r = static_cast<uint8_t>(std::clamp(45.0f + noise * 50.0f, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(48.0f + noise * 50.0f, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(55.0f + noise * 50.0f, 0.0f, 255.0f));
            } else {
                float brick_seed = hash2d(col, row);
                float base = 130.0f + brick_seed * 40.0f + noise * 70.0f;

                if (in_x < mortar * 2 || in_y < mortar * 2)
                    base += 35.0f;
                if (in_x > brick_w - mortar * 2 || in_y > brick_h - mortar * 2)
                    base -= 40.0f;

                r = static_cast<uint8_t>(std::clamp(base * 0.95f, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(base * 0.98f, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(base * 1.10f, 0.0f, 255.0f));
            }
            td.pixels[static_cast<size_t>(y) * size + x] = (0xFFu << 24) |
                                                           (static_cast<uint32_t>(r) << 16) |
                                                           (static_cast<uint32_t>(g) << 8) | b;
        }
    }
    return td;
}

TextureData make_flagstone_floor_texture(int size) {
    TextureData td;
    td.width = size;
    td.height = size;
    td.pixels.resize(static_cast<size_t>(size) * size);

    int tile_size = size / 4;
    int groove = std::max(2, size / 32);

    for (int y = 0; y < size; ++y) {
        int ty = y / tile_size;
        int in_y = y % tile_size;

        for (int x = 0; x < size; ++x) {
            int tx = x / tile_size;
            int in_x = x % tile_size;

            bool is_groove = (in_y < groove) || (in_x < groove);
            float noise = hash2d(x, y) * 0.3f - 0.15f;

            uint8_t r, g, b;
            if (is_groove) {
                r = 30;
                g = 32;
                b = 38;
            } else {
                float tile_seed = hash2d(tx, ty);
                float val = 90.0f + tile_seed * 30.0f + noise * 60.0f;

                int dist_border = std::min({in_x, tile_size - in_x, in_y, tile_size - in_y});
                if (dist_border < groove * 2)
                    val -= 25.0f;

                r = static_cast<uint8_t>(std::clamp(val * 0.92f, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(val * 0.95f, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(val * 1.05f, 0.0f, 255.0f));
            }
            td.pixels[static_cast<size_t>(y) * size + x] = (0xFFu << 24) |
                                                           (static_cast<uint32_t>(r) << 16) |
                                                           (static_cast<uint32_t>(g) << 8) | b;
        }
    }
    return td;
}

TextureData make_glowing_crystal_texture(int size) {
    TextureData td;
    td.width = size;
    td.height = size;
    td.pixels.resize(static_cast<size_t>(size) * size);

    float cx = size * 0.5f;
    float cy = size * 0.5f;
    float max_r = size * 0.5f;

    for (int y = 0; y < size; ++y) {
        float dy = y - cy;
        for (int x = 0; x < size; ++x) {
            float dx = x - cx;
            float dist = std::sqrt(dx * dx + dy * dy) / max_r;
            float angle = std::atan2(dy, dx);

            float facets = 0.5f + 0.5f * std::cos(angle * 6.0f);
            float inner_core = std::clamp(1.0f - dist * 1.2f, 0.0f, 1.0f);

            float r_val = 255.0f * (inner_core * 0.8f + facets * 0.3f);
            float g_val = 200.0f * (inner_core * 0.9f + facets * 0.4f);
            float b_val = 80.0f + 175.0f * inner_core;

            uint8_t r = static_cast<uint8_t>(std::clamp(r_val, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(std::clamp(g_val, 0.0f, 255.0f));
            uint8_t b = static_cast<uint8_t>(std::clamp(b_val, 0.0f, 255.0f));

            td.pixels[static_cast<size_t>(y) * size + x] = (0xFFu << 24) |
                                                           (static_cast<uint32_t>(r) << 16) |
                                                           (static_cast<uint32_t>(g) << 8) | b;
        }
    }
    return td;
}

TextureData make_scifi_metal_texture(int size) {
    TextureData td;
    td.width = size;
    td.height = size;
    td.pixels.resize(static_cast<size_t>(size) * size);

    int border = std::max(2, size / 16);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float grain = hash2d(x / 2, y) * 0.15f;
            float base = 150.0f + grain * 255.0f;

            bool is_rivet = ((x == border * 2 || x == size - border * 2) &&
                             (y == border * 2 || y == size - border * 2));

            if (x < border || x >= size - border || y < border || y >= size - border) {
                base -= 50.0f;
            } else if (is_rivet) {
                base = 240.0f;
            }

            uint8_t r = static_cast<uint8_t>(std::clamp(base * 0.95f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(std::clamp(base * 1.0f, 0.0f, 255.0f));
            uint8_t b = static_cast<uint8_t>(std::clamp(base * 1.15f, 0.0f, 255.0f));

            td.pixels[static_cast<size_t>(y) * size + x] = (0xFFu << 24) |
                                                           (static_cast<uint32_t>(r) << 16) |
                                                           (static_cast<uint32_t>(g) << 8) | b;
        }
    }
    return td;
}

} // namespace prism::render
