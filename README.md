# Prism: From-Scratch C++17 3D Software Renderer & Engine

Prism is a high-performance 3D software rasterizer and mini-engine written completely from scratch in modern C++17. Without relying on OpenGL, DirectX, GLM, or external math libraries, Prism implements the entire graphics mathematics, geometric clipping, barycentric rasterization, Blinn-Phong lighting, camera control, and spatial acceleration pipelines manually. Built with a strict focus on cache ergonomics, SIMD optimization, and mathematical rigor, Prism demonstrates core engine architecture and 3D mathematics proficiency for game engine software engineering.

## Architecture

```mermaid
graph TD
    subgraph Core ["Engine Core & Platform"]
        App[Window & Framebuffer Presentation (SDL2)]
        Input[Input & Key Event System]
        Clock[Fixed Timestep Accumulator (60 Hz)]
    end

    subgraph Math ["Prism Math Foundation"]
        Vectors[Vec2 / Vec3 / Vec4]
        Matrices[Mat3 / Mat4 (Column-Major)]
        Quaternions[Quat (Slerp / Nlerp / Rodrigues)]
        Geometry[Barycentric / Frustum / Planes]
        Intersections[Möller–Trumbore / Slab / Sphere]
    end

    subgraph Rasterizer ["Graphics Pipeline"]
        VertexStage[Vertex Transform & Shader]
        ClipStage[Homogeneous Sutherland-Hodgman Clip]
        NDCStage[Perspective Divide (1/w)]
        RasterStage[Barycentric Edge Function Rasterizer]
        PixelStage[Depth Buffer Test & Shading]
    end

    subgraph Scene ["Game & Simulation"]
        Entities[Entity List]
        Collisions[Swept AABB Collision & Sliding]
        Raycasts[Line-of-Sight AI Visibility]
    end

    App --> Rasterizer
    Math --> Rasterizer
    Math --> Scene
    Clock --> Scene
    Input --> Scene
    Scene --> Rasterizer
```

## Features
- **Zero Third-Party Math / Graphics**: All vector, matrix, quaternion, and intersection mathematics implemented from scratch.
- **Column-Major SIMD-Ready Matrices**: Transforms, projections, view matrix, and fast-path affine matrix inverses ($[R \mid T]^{-1} = [R^T \mid -R^T T]$).
- **Gimbal Lock Immunity**: Full quaternion orientation and continuous SLERP / NLERP interpolation.
- **Robust 3D Geometric Intersections**: Möller–Trumbore ray-triangle, Kay-Kajiya slab ray-AABB, and ray-sphere tests.
- **Automated Verification**: Rigorous GoogleTest unit test suite covering edge cases, singular matrices, and degenerate vectors.

## Verified Test Suite
- Current Status: **84 / 84 Tests Passing (100% Green)**
- Warnings as Errors: `-Wall -Wextra -Wpedantic -Werror` enabled across all targets.

## Building and Testing

### Prerequisites
- CMake 3.20+
- C++17 capable compiler (GCC 11+, Clang 14+, or MSVC 2019+)
- SDL2 development libraries

### Linux / WSL Build
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DPRISM_BUILD_TESTS=ON

# Compile
cmake --build build -j

# Execute Test Suite
ctest --test-dir build --output-on-failure
```

### Windows Build
```powershell
cmake -B build -DPRISM_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Benchmarks
*Performance measurements will be populated during Stage 5 performance engineering using deterministic camera paths on fixed triangle loads.*

| Scene Load | Baseline (Scalar) | Frustum Culled | BVH Accelerated | AVX / SIMD | Multithreaded Tiles |
|:---|:---:|:---:|:---:|:---:|:---:|
| 10k Triangles | TBD | TBD | TBD | TBD | TBD |
| 100k Triangles | TBD | TBD | TBD | TBD | TBD |
| 500k Triangles | TBD | TBD | TBD | TBD | TBD |

## Mathematical Reference
Comprehensive derivations and worked numeric examples for the perspective projection matrix, look-at view matrix, quaternion SLERP, and Möller–Trumbore ray intersection are detailed in [docs/math.md](docs/math.md).

## What I Learned
- **Homogeneous Coordinates & Perspective Division**: Why $w_{\text{clip}} = -z_{\text{view}}$ in right-handed systems, and how the non-linear depth mapping preserves precision near the camera.
- **Affine Inversion**: How decomposing transformation matrices into rotation and translation blocks eliminates costly $4 \times 4$ Cramer's rule inversions.
- **Gimbal Lock Singularities**: How Euler angle pitch at $\pm 90^\circ$ causes rotational degrees of freedom to collapse into a single axis, and how unit quaternions avoid this singularity.

## Known Limitations & Failure Analysis
- Depth precision distribution in standard $[-1, 1]$ NDC exhibits non-linear clustering near the near plane. Reverse-Z ($[1, 0]$ depth buffer mapping with floating-point depth) will be analyzed during Stage 5 to mitigate depth fighting.
