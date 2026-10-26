#include <cstring>
#include <gtest/gtest.h>
#include <prism/game/dungeon_logic.hpp>
#include <prism/math/bvh.hpp>
#include <prism/math/geometry.hpp>
#include <prism/math/intersection.hpp>
#include <prism/math/vec3.hpp>

using namespace prism;
using namespace prism::math;
using namespace prism::game;

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
        if (!target.active)
            return false;
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
    player.position = Vec3(0.0f, 0.5f, 0.9f);

    GameEntity wall;
    wall.position = Vec3(0.0f, 0.5f, 1.5f);
    wall.extents = Vec3(1.0f, 1.0f, 0.5f);

    EXPECT_TRUE(intersect_sphere_aabb(Sphere(player.position, player.radius), wall.bounds()));
    bool collided = player.resolve_wall_collision(wall.bounds());
    EXPECT_TRUE(collided);
    EXPECT_FALSE(intersect_sphere_aabb(Sphere(player.position, player.radius), wall.bounds()));
    EXPECT_LE(player.position.z, 1.0f - player.radius + 1e-4f);
}

TEST(GameLayerTest, RaycastPicksUpCollectible) {
    Player player;
    player.position = Vec3(0.0f, 0.5f, 0.0f);

    GameEntity gem;
    gem.position = Vec3(0.0f, 0.5f, 2.0f);
    gem.extents = Vec3(0.3f, 0.3f, 0.3f);
    gem.active = true;

    Ray look_ray(player.position, Vec3(0.0f, 0.0f, 1.0f));

    bool picked = player.interact_raycast(look_ray, gem, 3.0f);
    EXPECT_TRUE(picked);
    EXPECT_FALSE(gem.active);
    EXPECT_EQ(player.score, 1);

    EXPECT_FALSE(player.interact_raycast(look_ray, gem, 3.0f));
    EXPECT_EQ(player.score, 1);
}

TEST(GameLayerTest, RaycastOutOfRangeFails) {
    Player player;
    player.position = Vec3(0.0f, 0.5f, 0.0f);

    GameEntity gem;
    gem.position = Vec3(0.0f, 0.5f, 5.0f);
    gem.extents = Vec3(0.3f, 0.3f, 0.3f);
    gem.active = true;

    Ray look_ray(player.position, Vec3(0.0f, 0.0f, 1.0f));

    bool picked = player.interact_raycast(look_ray, gem, 2.0f);
    EXPECT_FALSE(picked);
    EXPECT_TRUE(gem.active);
    EXPECT_EQ(player.score, 0);
}

TEST(GameLayerTest, EnemyLineOfSightTransitionsToChase) {
    Enemy enemy;
    enemy.position = Vec3(0.0f, 0.5f, 4.0f);
    Vec3 player_pos(0.0f, 0.5f, 0.0f);

    // Empty wall BVH: direct unobstructed line of sight
    BVH wall_bvh;
    wall_bvh.build({});

    EXPECT_EQ(enemy.state, EnemyState::Patrol);
    enemy.update(0.1f, player_pos, wall_bvh);
    EXPECT_EQ(enemy.state, EnemyState::Chase);
    EXPECT_LT(enemy.position.z, 4.0f); // moved towards player
}

TEST(GameLayerTest, EnemyLineOfSightBlockedByWallRemainsPatrol) {
    Enemy enemy;
    enemy.position = Vec3(0.0f, 0.5f, 4.0f);
    Vec3 player_pos(0.0f, 0.5f, 0.0f);

    // Wall at z=2 blocking line of sight
    BVH wall_bvh;
    std::vector<BVHPrimitive> walls;
    walls.emplace_back(AABB(Vec3(-1.0f, 0.0f, 1.5f), Vec3(1.0f, 2.0f, 2.5f)), 0);
    wall_bvh.build(walls);

    EXPECT_EQ(enemy.state, EnemyState::Patrol);
    enemy.update(0.1f, player_pos, wall_bvh);
    EXPECT_EQ(enemy.state, EnemyState::Patrol); // remains patrol because ray was blocked!
}

TEST(GameLayerTest, ScriptedInputDeterminismHash) {
    auto run_sim = [](float forward_step) -> uint64_t {
        Player player;
        Enemy enemy;
        BVH bvh;
        bvh.build({});

        constexpr float dt = 1.0f / 60.0f;
        for (int frame = 0; frame < 60; ++frame) {
            player.position.z += forward_step * dt;
            enemy.update(dt, player.position, bvh);
        }

        // Hash final positions
        uint64_t h = 14695981039346656037ull;
        auto hash_float = [&h](float v) {
            uint32_t bits;
            std::memcpy(&bits, &v, sizeof(bits));
            h ^= bits;
            h *= 1099511628211ull;
        };
        hash_float(player.position.x);
        hash_float(player.position.z);
        hash_float(enemy.position.x);
        hash_float(enemy.position.z);
        return h;
    };

    uint64_t hash1 = run_sim(1.5f);
    uint64_t hash2 = run_sim(1.5f);

    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, 0u);
}
