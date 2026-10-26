#pragma once

#include <prism/math/bvh.hpp>
#include <prism/math/geometry.hpp>
#include <prism/math/intersection.hpp>
#include <prism/math/vec3.hpp>
#include <string>
#include <vector>

namespace prism::game {

using namespace math;

enum class GameState { Playing, Won, Lost };
enum class EnemyState { Patrol, Chase };

struct Enemy {
    Vec3 position{4.0f, 0.5f, 4.0f};
    Vec3 patrol_target{4.0f, 0.5f, -4.0f};
    Vec3 patrol_origin{4.0f, 0.5f, 4.0f};
    EnemyState state{EnemyState::Patrol};
    float speed{1.8f};
    float radius{0.4f};

    void update(float dt, const Vec3& player_pos, const BVH& wall_bvh) {
        Vec3 to_player = player_pos - position;
        float dist_to_player = length(to_player);

        // Line-of-sight raycast to player
        bool has_los = false;
        if (dist_to_player > 0.01f && dist_to_player < 10.0f) {
            Ray los_ray(position, to_player * (1.0f / dist_to_player));
            bool wall_hit = false;
            wall_bvh.query_ray(los_ray, [&](int /*id*/, float t0, float /*t1*/) {
                if (t0 > 0.1f && t0 < dist_to_player) {
                    wall_hit = true;
                }
            });
            has_los = !wall_hit;
        }

        if (has_los) {
            state = EnemyState::Chase;
        } else if (dist_to_player > 12.0f) {
            state = EnemyState::Patrol;
        }

        // Movement
        Vec3 target = (state == EnemyState::Chase) ? player_pos : patrol_target;
        Vec3 move_vec = target - position;
        move_vec.y = 0.0f; // stay on ground plane
        float d = length(move_vec);

        if (d > 0.05f) {
            position = position + (move_vec * (1.0f / d)) * (speed * dt);
        } else if (state == EnemyState::Patrol) {
            // Swap patrol target
            patrol_target = (length(patrol_target - patrol_origin) < 0.1f) ? Vec3(4.0f, 0.5f, -4.0f)
                                                                           : patrol_origin;
        }
    }
};

} // namespace prism::game
