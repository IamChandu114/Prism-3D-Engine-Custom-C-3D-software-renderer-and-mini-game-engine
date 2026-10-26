#include <gtest/gtest.h>
#include <prism/math/transform.hpp>
#include <prism/render/pipeline.hpp>
#include <prism/render/soa_vertex.hpp>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

TEST(SoATest, TransformsMatchAoSToFloatingPointPrecision) {
    Mat4 model =
        translate(Vec3(2.5f, -1.0f, 4.0f)) * rotate_y(0.75f) * scale(Vec3(1.2f, 0.8f, 1.5f));
    Mat4 view = look_at(Vec3(0, 2, 5), Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4 proj = perspective(deg_to_rad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    Mat4 mvp = proj * (view * model);

    // Create 5 test vertices (AoS)
    std::vector<Vertex> aos_verts = {
        {Vec3(-0.5f, -0.5f, 0.0f), Vec3(0, 0, 1), Vec2(0.0f, 0.0f), Vec3(1, 1, 1)},
        {Vec3(0.5f, -0.5f, 0.0f), Vec3(0, 0, 1), Vec2(1.0f, 0.0f), Vec3(1, 1, 1)},
        {Vec3(0.0f, 0.5f, 0.0f), Vec3(0, 0, 1), Vec2(0.5f, 1.0f), Vec3(1, 1, 1)},
        {Vec3(1.0f, 2.0f, 3.0f), normalize(Vec3(1, 1, 1)), Vec2(0.2f, 0.8f), Vec3(1, 1, 1)},
        {Vec3(-2.0f, 0.0f, 1.0f), normalize(Vec3(-1, 0, 2)), Vec2(0.9f, 0.1f), Vec3(1, 1, 1)},
    };

    // Populate SoA
    SoAVertexBuffer soa_buf;
    for (const auto& v : aos_verts) {
        soa_buf.add_vertex(v);
    }

    SoAClipBuffer soa_clip;
    transform_soa_vertices(soa_buf, mvp, model, soa_clip);

    ASSERT_EQ(soa_clip.size(), aos_verts.size());

    // Compare each vertex against AoS pipeline transform
    for (size_t i = 0; i < aos_verts.size(); ++i) {
        Vec4 wpos = model * Vec4(aos_verts[i].position, 1.0f);
        Vec4 clip = proj * (view * wpos);
        Vec3 norm = normalize((model * Vec4(aos_verts[i].normal, 0.0f)).xyz());

        EXPECT_NEAR(soa_clip.cx[i], clip.x, 1e-5f);
        EXPECT_NEAR(soa_clip.cy[i], clip.y, 1e-5f);
        EXPECT_NEAR(soa_clip.cz[i], clip.z, 1e-5f);
        EXPECT_NEAR(soa_clip.cw[i], clip.w, 1e-5f);

        EXPECT_NEAR(soa_clip.nx[i], norm.x, 1e-5f);
        EXPECT_NEAR(soa_clip.ny[i], norm.y, 1e-5f);
        EXPECT_NEAR(soa_clip.nz[i], norm.z, 1e-5f);

        EXPECT_FLOAT_EQ(soa_clip.u[i], aos_verts[i].uv.x);
        EXPECT_FLOAT_EQ(soa_clip.v[i], aos_verts[i].uv.y);
    }
}
