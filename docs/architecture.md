# Architecture Overview - Prism

Prism is a modular C++17 3D software rendering and game engine built from fundamental principles without third-party graphics or math libraries.

## System Architecture

```mermaid
graph TD
    subgraph Core ["Core Engine Layer"]
        App[Application / Window (SDL2)]
        Input[Input System]
        Timer[Fixed-Timestep Clock (60Hz)]
    end

    subgraph Math ["Prism Math (No GLM/Eigen)"]
        Vec[Vec2 / Vec3 / Vec4]
        Mat[Mat3 / Mat4 (Column-Major)]
        Quat[Quaternion]
        Geom[Planes / AABB / Sphere / Ray / Frustum]
        Inter[Moller-Trumbore / Slab / Sphere Intersections]
    end

    subgraph Pipeline ["Software Graphics Pipeline"]
        VertexShader[Vertex Processing & Transform]
        Clipper[Homogeneous Sutherland-Hodgman Clip]
        NDC[Perspective Division (1/w)]
        Rasterizer[Barycentric Tile/Edge Rasterizer]
        PixelShader[Depth Test & Shading (Blinn-Phong, Texture)]
        Framebuffer[CPU Framebuffer (RGBA32 + Float Depth)]
    end

    subgraph Game ["Game & Scene Layer"]
        Entity[Entity Component / List]
        Physics[Swept AABB Collision & Sliding]
        AI[Patrol & Line-of-Sight Raycasting]
    end

    App --> Pipeline
    Math --> Pipeline
    Math --> Game
    Timer --> Game
    Input --> Game
    Game --> Pipeline
```

## Directory Structure
- `include/prism/math/`: Core vector, matrix, quaternion, and geometric intersection headers.
- `include/prism/render/`: Software rasterizer, framebuffer, camera, lights, materials.
- `include/prism/game/`: Entity system, collision, camera controller, character movement.
- `include/prism/core/`: Application window wrapper, clock, logger.
- `src/`: Non-header-only engine implementations.
- `apps/`: Standalone applications (`demo_maze`, `bench`).
- `tests/`: Automated unit and regression test suite using GoogleTest.
- `docs/`: Mathematical derivations, ADRs, performance logs, architecture documentation.
