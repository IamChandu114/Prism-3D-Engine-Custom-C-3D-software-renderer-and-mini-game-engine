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
- Frustum culling using Gribb-Hartmann plane extraction and AABB intersection test ($2.01\times$ speedup).
- Vectorized SIMD (SSE/AVX) 4-wide edge function evaluation with portable scalar fallback and unit tests verifying numerical equivalence.
- Bounding Volume Hierarchy (BVH) with Surface Area Heuristic (SAH) and fast spatial queries (`query_frustum`, `query_raycast`, `query_sphere`).
- Structure-of-Arrays (SoA) vertex layout with batched 4-wide SIMD coordinate transformations.
- Zero-heap allocation during rendering via pre-allocated scratch buffers (`tests/test_memory.cpp`).

### Stage 6: Tile-Based Multithreading & Memory Engineering
- **Tile-Based Rasterization**: Screen partitioned into $64 \times 64$ pixel bins ($20 \times 12 = 240$ tiles at 720p). Each tile fits within the CPU L1 data cache (16 KB color + 16 KB depth $\le$ 32 KB L1D), avoiding cache thrashing
- **Persistent Zero-Heap Thread Pool**: Custom lock-free/latch worker thread pool that reuses threads across frames with zero heap allocations per dispatch.
- **Zero-Allocation Steady-State Loop**: Verified via global `operator new` interception that 0 heap allocations occur during continuous rendering.
- **Real-Time Scaling**: Achieves **519.1 FPS** on 10k triangles and **30.9 FPS** on 500k triangles at 1280x720 with Blinn-Phong shading.

---

## Empirical Performance Benchmarks

Measured on native Linux x86_64 (`/home/chand/prism_bench/` on WSL2 Ubuntu 24.04, GCC 13.3 `-O3`, 12th Gen Intel Core i5-1235U, 12 threads) rendering procedural 3D cube grids at **1280 × 720** resolution with Blinn-Phong shading and depth testing:

### 1. 10,000 Triangles (10,008 Tris / 834 Cube Instances)
| Optimization Stage | Avg Frame Time | Avg FPS | P95 Frame Time | Speedup vs Baseline | Peak Memory |
|:---|---:|---:|---:|---:|---:|
| **a) Baseline** (Scalar, 1-Thread, No Culling) | 7.56 ms | 132.3 FPS | 20.06 ms | 1.00x | 19.2 MB |
| **b) + Frustum Culling** (Gribb-Hartmann) | 3.76 ms | 265.9 FPS | 6.12 ms | **2.01x** | 19.5 MB |
| **c) + BVH Spatial Hierarchy** | 3.52 ms | 284.4 FPS | 6.26 ms | **2.15x** | 19.5 MB |
| **d) + SoA Vertex Layout & Transform** | 3.10 ms | 322.1 FPS | 5.01 ms | **2.43x** | 19.5 MB |
| **f) + Tiled Multithreaded (8 Threads)** | **1.93 ms** | **519.1 FPS** | **2.65 ms** | **3.92x** | 20.8 MB |

### 2. 100,000 Triangles (100,008 Tris / 8,334 Cube Instances)
| Optimization Stage | Avg Frame Time | Avg FPS | P95 Frame Time | Speedup vs Baseline | Peak Memory |
|:---|---:|---:|---:|---:|---:|
| **a) Baseline** (Scalar, 1-Thread, No Culling) | 19.23 ms | 52.0 FPS | 28.84 ms | 1.00x | 20.8 MB |
| **b) + Frustum Culling** (Gribb-Hartmann) | 15.17 ms | 65.9 FPS | 24.36 ms | **1.27x** | 20.8 MB |
| **c) + BVH Spatial Hierarchy** | 15.33 ms | 65.2 FPS | 24.99 ms | **1.25x** | 20.8 MB |
| **f) + Tiled Multithreaded (12 Threads)** | **7.25 ms** | **138.0 FPS** | **10.04 ms** | **2.65x** | 24.6 MB |

### 3. 500,000 Triangles (500,004 Tris / 41,667 Cube Instances)
| Optimization Stage | Avg Frame Time | Avg FPS | P95 Frame Time | Speedup vs Baseline | Peak Memory |
|:---|---:|---:|---:|---:|---:|
| **a) Baseline** (Scalar, 1-Thread, No Culling) | 90.46 ms | 11.1 FPS | 123.79 ms | 1.00x | 24.6 MB |
| **b) + Frustum Culling** (Gribb-Hartmann) | 56.68 ms | 17.6 FPS | 87.64 ms | **1.60x** | 24.6 MB |
| **f) + Tiled Multithreaded (4 Threads)** | **32.34 ms** | **30.9 FPS** | **42.74 ms** | **2.80x** | 40.3 MB |

*Full CSV dataset with complete thread sweeps (1, 2, 4, 8, 12 threads) available in [`benchmark_results.csv`](benchmark_results.csv) and [`docs/performance.md`](docs/performance.md).*

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

# Run full unit test suite (112 tests)
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

# 4. Comprehensive optimization benchmark suite (Stage 5-6)
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
- **112 / 112 Unit Tests Passing (100% Green)** across vectors, matrices, transforms, quaternions, geometry, intersections, framebuffers, coordinate pipelines, rasterization, game physics, SIMD equivalence, BVH queries, SoA transforms, and zero-heap memory verification.
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
