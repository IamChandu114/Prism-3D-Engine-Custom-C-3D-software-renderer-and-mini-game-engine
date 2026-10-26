#pragma once

#include <prism/math/constants.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>
#include <prism/math/vec3.hpp>

namespace prism::render {

struct Camera {
    math::Vec3 position{0.0f, 0.0f, 5.0f};
    math::Quat orientation{math::Quat::identity()};
    float fov_y{math::deg_to_rad(70.0f)};
    float aspect{16.0f / 9.0f};
    float near_z{0.1f};
    float far_z{1000.0f};

    [[nodiscard]] math::Vec3 forward() const noexcept {
        return orientation.rotate(math::Vec3(0.0f, 0.0f, -1.0f));
    }
    [[nodiscard]] math::Vec3 right() const noexcept {
        return orientation.rotate(math::Vec3(1.0f, 0.0f, 0.0f));
    }
    [[nodiscard]] math::Vec3 up() const noexcept {
        return orientation.rotate(math::Vec3(0.0f, 1.0f, 0.0f));
    }

    [[nodiscard]] math::Mat4 view_matrix() const noexcept {
        return math::look_at(position, position + forward(), up());
    }

    [[nodiscard]] math::Mat4 proj_matrix() const noexcept {
        return math::perspective(fov_y, aspect, near_z, far_z);
    }
};

} // namespace prism::render
