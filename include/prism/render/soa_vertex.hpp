#pragma once

#include <cstddef>
#include <prism/math/mat4.hpp>
#include <prism/render/rasterizer.hpp>
#include <vector>

namespace prism::render {

// Structure of Arrays (SoA) layout for vertex attributes
struct SoAVertexBuffer {
    std::vector<float> px, py, pz; // positions
    std::vector<float> nx, ny, nz; // normals
    std::vector<float> u, v;       // UVs

    void clear() noexcept {
        px.clear();
        py.clear();
        pz.clear();
        nx.clear();
        ny.clear();
        nz.clear();
        u.clear();
        v.clear();
    }

    void reserve(size_t n) {
        px.reserve(n);
        py.reserve(n);
        pz.reserve(n);
        nx.reserve(n);
        ny.reserve(n);
        nz.reserve(n);
        u.reserve(n);
        v.reserve(n);
    }

    [[nodiscard]] size_t size() const noexcept { return px.size(); }

    void add_vertex(const Vertex& vert) {
        px.push_back(vert.position.x);
        py.push_back(vert.position.y);
        pz.push_back(vert.position.z);

        nx.push_back(vert.normal.x);
        ny.push_back(vert.normal.y);
        nz.push_back(vert.normal.z);

        u.push_back(vert.uv.x);
        v.push_back(vert.uv.y);
    }
};

// SoA output clip-space buffer
struct SoAClipBuffer {
    std::vector<float> cx, cy, cz, cw; // homogeneous clip positions
    std::vector<float> nx, ny, nz;     // world normals
    std::vector<float> u, v;           // UVs

    void resize(size_t n) {
        cx.resize(n);
        cy.resize(n);
        cz.resize(n);
        cw.resize(n);
        nx.resize(n);
        ny.resize(n);
        nz.resize(n);
        u.resize(n);
        v.resize(n);
    }

    [[nodiscard]] size_t size() const noexcept { return cx.size(); }
};

// Batched SoA vertex transformation
inline void transform_soa_vertices(const SoAVertexBuffer& in, const math::Mat4& mvp,
                                   const math::Mat4& model, SoAClipBuffer& out) noexcept {
    size_t count = in.size();
    out.resize(count);

    // Matrix elements (column-major)
    // MVP: row i is (m(i,0), m(i,1), m(i,2), m(i,3))
    for (size_t i = 0; i < count; ++i) {
        float x = in.px[i];
        float y = in.py[i];
        float z = in.pz[i];

        out.cx[i] = mvp(0, 0) * x + mvp(0, 1) * y + mvp(0, 2) * z + mvp(0, 3);
        out.cy[i] = mvp(1, 0) * x + mvp(1, 1) * y + mvp(1, 2) * z + mvp(1, 3);
        out.cz[i] = mvp(2, 0) * x + mvp(2, 1) * y + mvp(2, 2) * z + mvp(2, 3);
        out.cw[i] = mvp(3, 0) * x + mvp(3, 1) * y + mvp(3, 2) * z + mvp(3, 3);

        float nx = in.nx[i];
        float ny = in.ny[i];
        float nz = in.nz[i];
        // Normal transform by model matrix linear part
        float wnx = model(0, 0) * nx + model(0, 1) * ny + model(0, 2) * nz;
        float wny = model(1, 0) * nx + model(1, 1) * ny + model(1, 2) * nz;
        float wnz = model(2, 0) * nx + model(2, 1) * ny + model(2, 2) * nz;
        float inv_len = 1.0f / std::sqrt(wnx * wnx + wny * wny + wnz * wnz);
        out.nx[i] = wnx * inv_len;
        out.ny[i] = wny * inv_len;
        out.nz[i] = wnz * inv_len;

        out.u[i] = in.u[i];
        out.v[i] = in.v[i];
    }
}

} // namespace prism::render
