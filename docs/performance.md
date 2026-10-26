# Performance Benchmarks and Profiling Log - Prism

This document tracks empirical measurements for Prism across development stages. All figures recorded here must be measured directly from benchmark runs.

## Benchmark Methodology
- Platform: Linux (WSL2 / Ubuntu 24.04 x86_64)
- CPU: Host processor
- Baseline Scene: Deterministic camera path through fixed triangle loads (10k, 100k, 500k triangles).
- Metrics: Average FPS, 95th Percentile Frame Time (ms), Peak Memory Working Set (MB).

## Optimization Progression
| Stage / Optimization | Avg FPS (10k) | p95 Frame Time (ms) | Avg FPS (100k) | Speedup vs Baseline | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| Baseline (Scalar, Unoptimized) | TBD | TBD | TBD | 1.0x | Stage 5 Baseline |
| a) Frustum Culling | TBD | TBD | TBD | TBD | TBD |
| b) BVH Spatial Partitioning | TBD | TBD | TBD | TBD | TBD |
| c) SoA Vertex Layout | TBD | TBD | TBD | TBD | TBD |
| d) SIMD (AVX/SSE) Math & Edge Setup | TBD | TBD | TBD | TBD | TBD |
| e) Multithreaded Tile Rasterizer | TBD | TBD | TBD | TBD | TBD |
| f) Pre-allocated Zero-Heap Frame Buffer | TBD | TBD | TBD | TBD | TBD |

## Profiler Hotspots (perf / Tracy)
*To be populated during Stage 5 profiling.*
