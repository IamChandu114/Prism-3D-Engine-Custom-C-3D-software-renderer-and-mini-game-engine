#include <gtest/gtest.h>
#include <prism/math/constants.hpp>
#include <prism/math/transform.hpp>
#include <prism/render/pipeline.hpp>

using namespace prism::math;
using namespace prism::render;

constexpr float kEpsilon = 1e-4f;

TEST(PipelineTest, ExplicitNamedSpacesTransformation) {
    // 1. Model space vertex
    ModelSpaceVertex v_model{Vec3(0.0f, 0.0f, 0.0f)};

    // Matrices
    Mat4 model_matrix = translate(Vec3(0.0f, 0.0f, -5.0f));
    Mat4 view_matrix =
        look_at(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 proj_matrix = perspective(deg_to_rad(90.0f), 1.0f, 1.0f, 100.0f);

    // 2. Transform to World Space
    WorldSpaceVertex v_world = transform_model_to_world(v_model, model_matrix);
    EXPECT_NEAR(v_world.position.x, 0.0f, kEpsilon);
    EXPECT_NEAR(v_world.position.y, 0.0f, kEpsilon);
    EXPECT_NEAR(v_world.position.z, -5.0f, kEpsilon);

    // 3. Transform to View Space
    ViewSpaceVertex v_view = transform_world_to_view(v_world, view_matrix);
    EXPECT_NEAR(v_view.position.x, 0.0f, kEpsilon);
    EXPECT_NEAR(v_view.position.y, 0.0f, kEpsilon);
    EXPECT_NEAR(v_view.position.z, -5.0f, kEpsilon);

    // 4. Transform to Clip Space
    ClipSpaceVertex v_clip = transform_view_to_clip(v_view, proj_matrix);
    EXPECT_NEAR(v_clip.position.w, 5.0f, kEpsilon);

    // 5. Perspective Division to NDC Space
    NDCSpaceVertex v_ndc = transform_clip_to_ndc(v_clip);
    EXPECT_NEAR(v_ndc.position.x, 0.0f, kEpsilon);
    EXPECT_NEAR(v_ndc.position.y, 0.0f, kEpsilon);
    // At z = -5 with near = 1, far = 100:
    // z_ndc should be between -1.0 and 1.0
    EXPECT_GT(v_ndc.position.z, -1.0f);
    EXPECT_LT(v_ndc.position.z, 1.0f);

    // 6. Viewport Transform to Screen Space
    int screen_width = 800;
    int screen_height = 600;
    ScreenSpaceVertex v_screen = transform_ndc_to_screen(v_ndc, screen_width, screen_height);
    EXPECT_NEAR(v_screen.x, 399.5f, 0.5f);
    EXPECT_NEAR(v_screen.y, 299.5f, 0.5f);
}
