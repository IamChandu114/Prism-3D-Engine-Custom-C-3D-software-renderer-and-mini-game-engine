#pragma once

#include <prism/math/vec2.hpp>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>
#include <prism/math/geometry.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/pipeline.hpp>
#include <prism/render/camera.hpp>
#include <vector>
#include <cstdint>
#include <functional>

namespace prism::render {

// Light types
enum class LightType { Directional, Point };

struct Light {
    LightType type{LightType::Directional};
    math::Vec3 direction_or_position{0.0f, -1.0f, 0.0f}; // normalized dir or world pos
    math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float radius{10.0f}; // for point lights
};

// Vertex with all attributes for rasterization
struct Vertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec2 uv{0.0f, 0.0f};
    math::Vec3 color{1.0f, 1.0f, 1.0f};
};

struct Triangle {
    Vertex v[3];
};

// Simple material
struct Material {
    math::Vec3 ambient{0.1f, 0.1f, 0.1f};
    math::Vec3 diffuse{0.8f, 0.8f, 0.8f};
    math::Vec3 specular{0.5f, 0.5f, 0.5f};
    float shininess{32.0f};
    uint32_t* texture_data{nullptr};
    int texture_width{0};
    int texture_height{0};
};

enum class ShadingMode { Flat, Gouraud, Phong };

struct RenderConfig {
    ShadingMode shading{ShadingMode::Phong};
    bool backface_culling{true};
    bool depth_test{true};
    bool wireframe{false};
};

// Draw wireframe triangle via Bresenham
void draw_wireframe_triangle(Framebuffer& fb,
                             const ScreenSpaceVertex& v0,
                             const ScreenSpaceVertex& v1,
                             const ScreenSpaceVertex& v2,
                             uint32_t color) noexcept;

// Barycentric rasterizer with depth test, perspective-correct interpolation, Blinn-Phong
void rasterize_triangle(Framebuffer& fb,
                        const ScreenSpaceVertex& v0,
                        const ScreenSpaceVertex& v1,
                        const ScreenSpaceVertex& v2,
                        const Material& material,
                        const std::vector<Light>& lights,
                        const math::Vec3& camera_pos,
                        ShadingMode shading) noexcept;

// Full pipeline: triangles -> framebuffer
void draw_triangles(Framebuffer& fb,
                    const std::vector<Triangle>& triangles,
                    const math::Mat4& model_matrix,
                    const Camera& camera,
                    const Material& material,
                    const std::vector<Light>& lights,
                    const RenderConfig& config) noexcept;

// Draw a wireframe-only mesh (Stage 2 mode)
void draw_wireframe(Framebuffer& fb,
                    const std::vector<Triangle>& triangles,
                    const math::Mat4& model_matrix,
                    const Camera& camera,
                    uint32_t color) noexcept;

// Build a unit cube mesh (12 triangles)
[[nodiscard]] std::vector<Triangle> make_cube() noexcept;

} // namespace prism::render
