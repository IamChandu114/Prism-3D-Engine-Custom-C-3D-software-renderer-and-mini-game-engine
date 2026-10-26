# Performance Benchmarks and Profiling Log - Prism

This document tracks empirical measurements for the Prism 3D software renderer across development and optimization stages. All figures recorded here are measured directly on x86_64 Linux (WSL2 / Ubuntu 24.04).

## Benchmark Methodology
- **Platform**: Linux x86_64 (WSL2 Ubuntu 24.04, GCC 13.3 `-O3`)
- **Resolution**: 640x480 RGBA32 Color + Float32 Depth Buffer
- **Workload**: 60 frames deterministic camera pass, 27 rotating 3D cubes (324 triangles) with Blinn-Phong directional shading, back-face culling, and z-buffering.
- **Measurement Tool**: High-resolution clock (`std::chrono::high_resolution_clock`) in `apps/benchmark`.

---

## Empirical Benchmark Results

| Optimization Stage | Total Time (60 frames) | Avg Frame Time | Avg FPS | Speedup vs Baseline | Notes |
|:---|---:|---:|---:|---:|:---|
| **1. Baseline** (Scalar, Single-Thread, No Culling) | 6.0 ms | 0.10 ms | 10,003 FPS | 1.00x | Pure scalar software pipeline |
| **2. + Frustum Culling** (Gribb-Hartmann extraction) | 5.3 ms | 0.09 ms | 11,262 FPS | **1.13x** | $O(1)$ rejection of off-screen objects |
| **3. + Multi-threaded Horizontal Stripes** (4 Threads) | 26.2 ms | 0.44 ms | 2,289 FPS | 0.23x | Thread dispatch overhead dominates on sub-millisecond workloads |

---

## Performance Engineering Analysis

### 1. Frustum Culling Speedup (1.13x)
- **Mechanism**: The 6 clipping planes are extracted from the view-projection matrix using the Gribb-Hartmann method during camera update:
  $$\pi_{\text{left}} = \text{row}_3 + \text{row}_0, \quad \pi_{\text{right}} = \text{row}_3 - \text{row}_0, \quad \dots$$
- **Effect**: Meshes outside the camera frustum are culled via an AABB-Frustum plane test before any vertex transformation, clipping, or rasterization.
- **Result**: Reduces vertex and triangle transform load by ~15-25% depending on view angle, yielding an immediate 1.13x speedup on a 27-cube scene.

### 2. Micro-Workload Granularity and Amdahl's Law
- **Observation**: For workloads that complete in **0.10 ms per frame**, creating or dispatching threads across `std::async` introduces thread synchronization and context-switching overhead (~0.05-0.1 ms per thread) that exceeds the actual rendering computation.
- **Takeaway for Game Engine Architecture**:
  - Multithreading in software rasterization is advantageous when triangle density and pixel fill rate are high (e.g. 50,000+ triangles or 4K resolution taking >16ms/frame).
  - For lightweight scenes, a single-threaded vectorized fast-path avoids thread dispatch latency entirely.
  - A persistent thread pool (e.g., work-stealing job system) with lockless task queues is required to eliminate per-frame OS thread spawning overhead.

### 3. SIMD Vectorization (SSE/AVX)
- Edge function evaluation:
  $$E(p) = (B_x - A_x)(P_y - A_y) - (B_y - A_y)(P_x - A_x)$$
- Evaluated 4 horizontal pixels simultaneously using 128-bit SIMD registers (`_mm_set_ps`, `_mm_mul_ps`, `_mm_sub_ps`).
- Verified exact numerical equivalence with scalar fallback via `tests/test_simd.cpp` (all 102 tests pass).

### 4. Cache Locality & Zero-Heap Buffers
- Framebuffer color data (`uint32_t`) and depth data (`float`) are laid out in contiguous row-major memory buffers:
  $$\text{Index}(x, y) = y \times \text{width} + x$$
- Horizontal scanline rasterization guarantees sequential, cache-friendly stride-1 reads and writes.
- Pre-allocated clipping vectors (`RenderScratchBuffer`) eliminate heap allocations from the per-triangle inner loop.
