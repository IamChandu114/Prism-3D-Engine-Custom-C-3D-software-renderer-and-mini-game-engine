# Prism: 3D Software Renderer & Mini Game Engine in Modern C++17

[![Prism CI](https://github.com/chand/Prism/actions/workflows/ci.yml/badge.svg)](https://github.com/chand/Prism/actions/workflows/ci.yml)
[![Standard: C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Zero-Dependencies](https://img.shields.io/badge/Dependencies-Zero%20(SDL2%20Present%20Only)-orange.svg)]()

> A from-scratch, zero-dependency 3D software rasterizer, mathematical foundation, and mini game engine built in C++17. Developed to prove demonstrated proficiency in 3D mathematics, computer graphics pipelines, and CPU performance engineering for games software engineering internship interviews.

---

## The Pitch
Modern game developers often rely on high-level APIs like DirectX 12, Vulkan, or Unreal Engine without fully understanding the underlying hardware and mathematical abstractions. **Prism** strips away all black boxes:
- **No OpenGL, DirectX, Vulkan, or Metal.**
- **No GLM, Eigen, DirectXMath, or external linear algebra libraries.**
- **Every dot product, matrix inverse, quaternion slerp, Cohen-Sutherland clip, Sutherland-Hodgman polygon clip, barycentric edge function, depth test, and Blinn-Phong reflection is written entirely from scratch in clean, modern C++17.**
- **SDL2 is used strictly for window creation, keyboard/mouse input polling, and presenting a raw CPU pixel buffer.**

---

## Architecture Overview

```mermaid
graph TD
    subgraph Math Foundation
        Vec[Vectors: Vec2, Vec3, Vec4]
        Mat[Column-Major Mat3, Mat4]
        Quat[Quaternions & Slerp/Nlerp]
        Geom[Planes, AABB, Sphere, Ray, Frustum]
        MT[Möller-Trumbore & Slab Intersections]
    end

    subgraph Pipeline Stages
        M2W[Model to World]
        W2V[World to View LookAt]
        V2C[View to Clip Perspective]
        SHClip[Sutherland-Hodgman 6-Plane Homogeneous Clip]
        NDC[Perspective Divide to NDC]
        Screen[Viewport Transform to Screen]
    end

    subgraph Rasterization Core
        Bary[Barycentric Edge Function & Top-Left Rule]
        ZBuf[Z-Buffer Early-Depth Test]
        Persp[Perspective-Correct 1/w Attribute Interpolation]
        Shade[Blinn-Phong Lighting & Bilinear Texture Filter]
        FB[Contiguous RGBA32 Color & Float32 Depth Buffer]
    end

    subgraph Applications
        Wire[Wireframe Demo]
        Rast[Rasterizer Demo]
        Dungeon[3D Dungeon Crawler Game]
        Bench[Optimization Benchmark Suite]
    end

    Math Foundation --> Pipeline Stages
    Pipeline Stages --> Rasterization Core
    Rasterization Core --> Applications
```

---

## Key Features Across Stages

### Stage 1: Mathematical Foundations
- `Vec2`, `Vec3`, `Vec4` with full SIMD-ready vector operations, cross products, dot products, reflections.
- `Mat3`, `Mat4` column-major matrices with explicit basis-vector access, analytical determinants, general Gauss-Jordan inverses, and fast-path affine inverses ($O(1)$ transpose rotation + inverted translation).
- Unit quaternions ($\mathbb{H}$) with axis-angle construction, shortest-path Slerp and Nlerp, Euler conversions, and gimbal lock avoidance.
- Analytical geometry: AABB, Bounding Sphere, Oriented Plane, Ray, and View Frustum with Gribb-Hartmann extraction.
- Intersections: Möller-Trumbore ray-triangle, Kay-Kajiya slab ray-AABB, analytical ray-sphere, sphere-AABB, and AABB-AABB.

### Stage 2: Memory & Coordinate Pipelines
- Contiguous `Framebuffer` with RGBA32 color buffer, 32-bit float depth buffer, and 5x7 ASCII bitmap font HUD overlay.
- Bresenham line algorithm with pure integer error accumulation and Cohen-Sutherland outcode viewport clipping.
- Explicit type-safe coordinate space pipeline: `ModelSpaceVertex` $\to$ `WorldSpaceVertex` $\to$ `ViewSpaceVertex` $\to$ `ClipSpaceVertex` $\to$ `NDCSpaceVertex` $\to$ `ScreenSpaceVertex`.
- Headless execution mode (`--headless N`) saving deterministic PPM/PNG frame sequences for testing and CI.

### Stage 3: The 3D Software Rasterizer
- Juan Pineda barycentric edge function rasterization with subpixel precision and the top-left fill rule (zero seam cracking or double rasterization).
- Floating-point Z-buffering with early-depth rejection.
- Homogeneous Sutherland-Hodgman clipping against all 6 frustum planes (preventing near-plane singularities).
- Back-face culling via signed screen-space polygon area.
- Shading modes: Flat shading, Gouraud shading, and per-fragment Blinn-Phong specular reflection with multiple directional and point lights.
- Texture mapping with bilinear filtering and wrapping modes.
- Wavefront `.obj` mesh loader and PPM texture parser.

### Stage 4: 3D Playable Game Layer
- **Prism: Dungeon Crawler**:
  - Full first-person camera controller with WASD movement, mouse/arrow look, and pitch clamping.
  - Procedural 3D dungeon maze with textured stone walls and checkered floors.
  - Floating, rotating crystal collectibles with attached dynamic point lights that cast real-time colored Blinn-Phong illumination on surrounding stone corridors.
  - Sphere-AABB collision detection and slide resolution (preventing walking through walls).
  - Forward raycasting interaction for collecting crystals.
  - Real-time HUD: mini-map with player position and facing indicator, crosshair, score, and FPS counter.

### Stage 5: Performance Engineering Suite
- Frustum culling using Gribb-Hartmann plane extraction and AABB intersection test.
- Vectorized SIMD (SSE/AVX) 4-wide edge function evaluation with portable scalar fallback and unit tests verifying numerical equivalence.
- Multithreaded striped rasterizer dividing the screen into parallel horizontal bands with zero data race or lock contention.
- Zero-heap allocation during rendering via pre-allocated scratch buffers.

---

## Empirical Performance Benchmarks

Measured on Linux x86_64 (WSL2 Ubuntu 24.04, GCC 13.3 `-O3`) rendering 27 rotating cubes (324 triangles) at 640x480 resolution with Blinn-Phong lighting:

| Optimization Stage | Total Time (60 frames) | Avg Frame Time | Avg FPS | Speedup vs Baseline | Notes |
|:---|---:|---:|---:|---:|:---|
| **1. Baseline** (Scalar, Single-Thread, No Culling) | 6.0 ms | 0.10 ms | 10,003 FPS | 1.00x | Pure scalar software pipeline |
| **2. + Frustum Culling** (Gribb-Hartmann extraction) | 5.3 ms | 0.09 ms | 11,262 FPS | **1.13x** | $O(1)$ rejection of off-screen objects |
| **3. + Multi-threaded Horizontal Stripes** (4 Threads) | 26.2 ms | 0.44 ms | 2,289 FPS | 0.23x | Thread dispatch overhead dominates on sub-millisecond workloads |

*Note on Multithreading: On sub-millisecond workloads (0.10 ms/frame), thread creation and dispatch overhead (~0.1 ms) exceeds the workload duration—a classic demonstration of Amdahl's Law and the necessity of persistent thread pools for micro-tasks.*

---

## Quickstart: Building & Running

### Prerequisites
- C++17 compliant compiler (GCC 10+, Clang 11+, or MSVC 2019+)
- CMake 3.20+
- SDL2 (automatically fetched via CMake FetchContent if not installed locally)

### Build Instructions (Linux / macOS / Windows WSL)
```bash
# Clone the repository
git clone https://github.com/chand/Prism.git
cd Prism

# Configure Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DPRISM_BUILD_TESTS=ON \
  -DPRISM_BUILD_APPS=ON \
  -DPRISM_WARNINGS_AS_ERRORS=ON

# Build all targets in parallel
cmake --build build -j$(nproc)

# Run full unit test suite (102 tests)
./build/tests/prism_math_tests
```

### Running Applications
```bash
# 1. Wireframe spinning cube demo (Stage 2)
./build/apps/wireframe_demo

# 2. Fully lit and textured software rasterizer demo (Stage 3)
./build/apps/rasterizer_demo

# 3. Playable 3D Dungeon Crawler (Stage 4)
./build/apps/dungeon_crawler

# 4. Optimization benchmark suite (Stage 5)
./build/apps/benchmark
```

### Headless Mode & GIF Generation
All applications support `--headless <N>` to run without an SDL window and output a sequence of PPM images for CI runners or demo exports:
```bash
./build/apps/dungeon_crawler --headless 60
python3 scripts/generate_gif.py --pattern "dungeon_frame_*.ppm" --output dungeon_demo.gif --fps 30
```

---

## Testing & Quality Assurance
- **102 / 102 Unit Tests Passing (100% Green)** across vectors, matrices, transforms, quaternions, geometry, intersections, framebuffers, coordinate pipelines, rasterization, game physics, and SIMD equivalence.
- **Strict Compiler Warnings**: Built with `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang) and `/W4 /WX` (MSVC).
- **Sanitizers**: Verified with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) with zero leaks and zero undefined behaviors.
- **Automated CI**: GitHub Actions workflow builds and tests on every commit.

---

## What I Learned
1. **The Crucial Role of Homogeneous Clipping**: Discarding or clipping geometry in NDC space after the perspective divide leads to division-by-zero singularities when vertices lie at $Z \le 0$. Homogeneous 4D clipping against $W \pm X, W \pm Y, W \pm Z$ is mathematically essential.
2. **Perspective-Correct Interpolation**: Screen-space linear interpolation of texture coordinates produces severe affine warping. Interpolating $U/W$ and $V/W$ and dividing by the interpolated $1/W$ is necessary to recover true 3D attributes.
3. **The Reality of Multithreading Overhead**: Parallelizing workloads that complete in 0.1 ms with standard asynchronous dispatch leads to severe thread management overhead. High-performance game engines require lock-free persistent job systems.

---

## Known Limitations & Failure Analysis
1. **Mipmapping & Anisotropic Filtering**: Textures currently sample mip-level 0 using bilinear interpolation. Highly minified textures at steep oblique angles can exhibit aliasing. Future work will implement derivative-based mip-level selection ($ddx / ddy$).
2. **Dynamic Shadows / Shadow Mapping**: Lighting uses analytical Blinn-Phong equations without shadow passes. Adding a software depth-map shadow buffer pass would allow self-shadowing in the dungeon crawler.
3. **Job System vs OS Threads**: The current multithreaded rasterizer uses standard futures. A lock-free fiber-based job system would eliminate the dispatch latency observed in micro-benchmarks.

---

## Interview Guide
A comprehensive study guide covering **25 interview questions with complete mathematical derivations and model answers** across all 5 stages is available in:
👉 [`docs/interview_prep.md`](docs/interview_prep.md)
