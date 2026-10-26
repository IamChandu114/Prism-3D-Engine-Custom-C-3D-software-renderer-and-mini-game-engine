# Architectural Decision Records (ADR) - Prism

This document logs significant architectural and design choices made during the development of Prism.

---

## ADR 0001: Column-Major Memory Layout & Post-Multiplication
- **Status**: Accepted
- **Context**: Computer graphics conventions diverge between row-major (DirectX/HLSL) and column-major (OpenGL/GLSL/Mathematics textbooks).
- **Decision**: Prism uses column-major matrices with standard mathematical post-multiplication:
  $$\mathbf{v}' = M \cdot \mathbf{v}$$
  Memory is laid out sequentially by column: `data[col * 4 + row]`.
- **Consequences**:
  - Direct compatibility with standard 3D graphics literature and textbook derivations.
  - Consistent basis vectors access: columns 0, 1, 2 represent the transformed $X, Y, Z$ axes directly in memory.

---

## ADR 0002: Zero External Mathematics Libraries
- **Status**: Accepted
- **Context**: Game engine internships evaluate candidates on mathematical derivation and performance engineering fundamentals. Using GLM or Eigen obscures these mechanics.
- **Decision**: All linear algebra, vector math, quaternion operations, geometry primitives, and ray-intersection tests are written entirely from scratch in C++17.
- **Consequences**:
  - Full transparency: every formula, determinant, inverse, and slerp is visible, profiled, and unit tested.
  - Zero external transitive dependencies outside GoogleTest (for testing) and SDL2 (for window presentation).

---

## ADR 0003: Barycentric Edge-Function Rasterization with Top-Left Tie-Breaking
- **Status**: Accepted
- **Context**: Scanline rasterization (standard Bresenham-based span filling) is difficult to vectorize and prone to precision artifacts at shared edges.
- **Decision**: We implement barycentric rasterization using Juan Pineda's parallel edge function:
  $$E(p) = (B_x - A_x)(P_y - A_y) - (B_y - A_y)(P_x - A_x)$$
  Combined with a strict top-left fill rule.
- **Consequences**:
  - Zero cracked seams or double-rasterized pixels across adjacent triangles sharing an edge.
  - Natural vectorization via SIMD (evaluating 4 pixels simultaneously).

---

## ADR 0004: Sutherland-Hodgman Homogeneous 6-Plane Clipping
- **Status**: Accepted
- **Context**: Triangles crossing the camera near plane ($Z \le 0$) cause division by zero or coordinate inversion if clipped after perspective divide in NDC space.
- **Decision**: We clip triangles in 4D homogeneous clip space against all 6 frustum planes before perspective division:
  $$-w \le x \le w, \quad -w \le y \le w, \quad -w \le z \le w$$
- **Consequences**:
  - Polygons straddling the near plane are cleanly clipped into valid subtriangles.
  - Zero division-by-zero singularities during perspective divide.

---

## ADR 0005: SDL2 Strictly for Windowing & Buffer Presentation
- **Status**: Accepted
- **Context**: The engine must render without any hardware acceleration or graphics driver support.
- **Decision**: SDL2 is restricted strictly to creating the OS window, collecting keyboard/mouse input, and copying the finished software `uint32_t*` color buffer to an SDL streaming texture for display.
- **Consequences**:
  - Every single pixel, triangle, normal, and lighting reflection is calculated entirely by our CPU code.
  - Headless mode (`--headless N`) bypasses SDL2 entirely, enabling headless execution on headless CI runners.
