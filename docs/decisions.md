# Architecture Decision Records (ADR) - Prism

## ADR 0001: Coordinate System, Handedness, and Matrix Convention
- **Date**: 2026-10-26
- **Status**: Accepted
- **Context**: 3D math libraries must consistently specify vector multiplication conventions (row-vector vs column-vector), memory layout (row-major vs column-major), and spatial handedness.
- **Decision**:
  1. **Column-Vector convention**: Transformations are applied as $v' = M \cdot v$. Composite transformations evaluate right-to-left: $(M_3 \cdot M_2 \cdot M_1) \cdot v$.
  2. **Column-Major memory layout**: Contiguous storage where elements in memory represent columns: `data[col * 4 + row]`. This directly maps to standard cache-friendly SIMD dot-products on column basis and graphics APIs.
  3. **Right-Handed View Space**: +X is Right, +Y is Up, -Z is Forward (looking down negative Z).
  4. **NDC Coordinate Space**: Normalized Device Coordinates span $[-1, 1]$ across $X$, $Y$, and $Z$ ($Z \in [-1, 1]$ standard right-handed convention).

## ADR 0002: Header-First Math Library with constexpr
- **Date**: 2026-10-26
- **Status**: Accepted
- **Context**: In software rendering, vector and matrix operations are in the innermost hot loops. Function call overhead must be zero.
- **Decision**: The math library will be header-only with inline and `constexpr` where feasible, ensuring the compiler can aggressively inline, vectorize, and eliminate temporaries.
