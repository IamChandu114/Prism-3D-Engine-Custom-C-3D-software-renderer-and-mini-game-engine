#pragma once

#include <prism/math/mat4.hpp>
#include <prism/math/vec2.hpp>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>

namespace prism::render {

// Distinct types representing vertices across coordinate systems
struct ModelSpaceVertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
};

struct WorldSpaceVertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
};

struct ViewSpaceVertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
};

struct ClipSpaceVertex {
    math::Vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
};

struct NDCSpaceVertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
    float inv_w{1.0f};
};

struct ScreenSpaceVertex {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float inv_w{1.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
};

// 1. Model -> World
[[nodiscard]] inline WorldSpaceVertex
transform_model_to_world(const ModelSpaceVertex& v, const math::Mat4& model_matrix) noexcept {
    WorldSpaceVertex out;
    out.position = (model_matrix * math::Vec4(v.position, 1.0f)).xyz();
    // Assuming uniform scale or pure rotation for normal
    out.normal = math::normalize((model_matrix * math::Vec4(v.normal, 0.0f)).xyz());
    out.uv = v.uv;
    return out;
}

// 2. World -> View
[[nodiscard]] inline ViewSpaceVertex
transform_world_to_view(const WorldSpaceVertex& v, const math::Mat4& view_matrix) noexcept {
    ViewSpaceVertex out;
    out.position = (view_matrix * math::Vec4(v.position, 1.0f)).xyz();
    out.normal = math::normalize((view_matrix * math::Vec4(v.normal, 0.0f)).xyz());
    out.uv = v.uv;
    return out;
}

// 3. View -> Clip
[[nodiscard]] inline ClipSpaceVertex
transform_view_to_clip(const ViewSpaceVertex& v, const math::Mat4& proj_matrix) noexcept {
    ClipSpaceVertex out;
    out.position = proj_matrix * math::Vec4(v.position, 1.0f);
    out.normal = v.normal;
    out.uv = v.uv;
    return out;
}

// 4. Clip -> NDC (Perspective Division)
[[nodiscard]] inline NDCSpaceVertex transform_clip_to_ndc(const ClipSpaceVertex& v) noexcept {
    NDCSpaceVertex out;
    float inv_w = (std::abs(v.position.w) > 1e-6f) ? (1.0f / v.position.w) : 1.0f;
    out.position = math::Vec3(v.position.x * inv_w, v.position.y * inv_w, v.position.z * inv_w);
    out.inv_w = inv_w;
    out.normal = v.normal;
    out.uv = v.uv;
    return out;
}

// 5. NDC -> Screen
[[nodiscard]] inline ScreenSpaceVertex transform_ndc_to_screen(const NDCSpaceVertex& v, int width,
                                                               int height) noexcept {
    ScreenSpaceVertex out;
    // Map [-1, 1] to [0, width - 1]
    out.x = (v.position.x + 1.0f) * 0.5f * static_cast<float>(width - 1);
    // Invert Y for screen display (0 is top)
    out.y = (1.0f - (v.position.y + 1.0f) * 0.5f) * static_cast<float>(height - 1);
    // Map depth [-1, 1] to [0, 1]
    out.z = (v.position.z + 1.0f) * 0.5f;
    out.inv_w = v.inv_w;
    out.normal = v.normal;
    out.uv = v.uv;
    return out;
}

} // namespace prism::render
