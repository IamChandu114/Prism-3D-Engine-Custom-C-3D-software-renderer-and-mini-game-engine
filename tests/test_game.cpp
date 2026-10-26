#include <gtest/gtest.h>
#include <prism/math/vec3.hpp>
#include <prism/math/geometry.hpp>
#include <prism/math/intersection.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/rasterizer.hpp>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

// Game entity structure
struct GameEntity {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 extents{0.5f, 0.5f, 0.5f}; // half-extents
    bool active{true};

    [[nodiscard]] AABB bounds() const noexcept {
        return AABB(position - extents, position + extents);
    }
};

// Player with sphere collision
struct Player {
    Vec3 position{0.0f, 0.5f, 0.0f};
    float radius{0.35f};
    int score{0};

    bool resolve_wall_collision(const AABB& wall) noexcept {
        Sphere player_sphere(position, radius);
        if (intersect_sphere_aabb(player_sphere, wall)) {
            // Push player out along axis of least penetration
            Vec3 closest(clamp(position.x, wall.min.x, wall.max.x),
                         clamp(position.y, wall.min.y, wall.max.y),
                         clamp(position.z, wall.min.z, wall.max.z));
            Vec3 diff = position - closest;
            float dist = length(diff);
            if (dist > kEps) {
                Vec3 normal = diff * (1.0f / dist);
                float penetration = radius - dist;
                position = position + normal * penetration;
            } else {
                position.y = wall.max.y + radius;
            }
            return true;
        }
        return false;
    }

    bool interact_raycast(const Ray& ray, GameEntity& target, float max_dist) noexcept {
        if (!target.active) return false;
        Sphere target_sphere(target.position, target.extents.x);
        float t1 = 0.0f, t2 = 0.0f;
        if (intersect_ray_sphere(ray, target_sphere, t1, t2)) {
            float t_hit = (t1 > 0.0f) ? t1 : t2;
            if (t_hit > 0.0f && t_hit <= max_dist) {
                target.active = false;
                ++score;
                return true;
            }
        }
        return false;
    }
};

TEST(GameLayerTest, PlayerCollidesAndResolvesWall) {
    Player player;
    player.position = Vec3(0.0f, 0.5f, 0.9f); // near wall at z=1.0

    GameEntity wall;
    wall.position = Vec3(0.0f, 0.5f, 1.5f);
    wall.extents  = Vec3(1.0f, 1.0f, 0.5f); // wall z from 1.0 to 2.0

    // Before collision resolution, player sphere intersects wall
    EXPECT_TRUE(intersect_sphere_aabb(Sphere(player.position, player.radius), wall.bounds()));

    // Resolve collision
    bool collided = player.resolve_wall_collision(wall.bounds());
    EXPECT_TRUE(collided);

    // After resolution, player sphere must no longer intersect wall
    EXPECT_FALSE(intersect_sphere_aabb(Sphere(player.position, player.radius), wall.bounds()));
    EXPECT_LE(player.position.z, 1.0f - player.radius + 1e-4f);
}

TEST(GameLayerTest, RaycastPicksUpCollectible) {
    Player player;
    player.position = Vec3(0.0f, 0.5f, 0.0f);

    GameEntity gem;
    gem.position = Vec3(0.0f, 0.5f, 2.0f);
    gem.extents  = Vec3(0.3f, 0.3f, 0.3f);
    gem.active   = true;

    Ray look_ray(player.position, Vec3(0.0f, 0.0f, 1.0f));

    // Target is 2.0 units ahead; should succeed within range 3.0
    bool picked = player.interact_raycast(look_ray, gem, 3.0f);
    EXPECT_TRUE(picked);
    EXPECT_FALSE(gem.active);
    EXPECT_EQ(player.score, 1);

    // Second interact attempt fails because already inactive
    EXPECT_FALSE(player.interact_raycast(look_ray, gem, 3.0f));
    EXPECT_EQ(player.score, 1);
}

TEST(GameLayerTest, RaycastOutOfRangeFails) {
    Player player;
    player.position = Vec3(0.0f, 0.5f, 0.0f);

    GameEntity gem;
    gem.position = Vec3(0.0f, 0.5f, 5.0f);
    gem.extents  = Vec3(0.3f, 0.3f, 0.3f);
    gem.active   = true;

    Ray look_ray(player.position, Vec3(0.0f, 0.0f, 1.0f));

    // Max distance is 2.0, gem is at 5.0 -> out of range
    bool picked = player.interact_raycast(look_ray, gem, 2.0f);
    EXPECT_FALSE(picked);
    EXPECT_TRUE(gem.active);
    EXPECT_EQ(player.score, 0);
}
