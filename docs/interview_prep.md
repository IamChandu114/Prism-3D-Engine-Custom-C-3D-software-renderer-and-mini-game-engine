# Prism: 3D Graphics & Engine Software Engineering Interview Guide

This guide contains **25 essential interview questions (5 per project stage)** with rigorous mathematical derivations, architectural rationales, and model answers tailored for AAA game engine and graphics software engineering internship interviews.

---

# Table of Contents
1. [Stage 1: 3D Mathematics & Linear Algebra](#stage-1-3d-mathematics--linear-algebra)
2. [Stage 2: Framebuffers, Pipelines & Line Algorithms](#stage-2-framebuffers-pipelines--line-algorithms)
3. [Stage 3: Software Rasterization, Clipping & Shading](#stage-3-software-rasterization-clipping--shading)
4. [Stage 4: Collision Detection, Scene Graphs & Game Mechanics](#stage-4-collision-detection-scene-graphs--game-mechanics)
5. [Stage 5: Performance Engineering, SIMD & Concurrency](#stage-5-performance-engineering-simd--concurrency)

---

# Stage 1: 3D Mathematics & Linear Algebra

### Q1.1: Why does Prism use column-major matrix layout, and how does this affect memory traversal during vector multiplication?
**Model Answer:**
In column-major order, matrix elements are stored column by column in contiguous memory:
$$M = \begin{bmatrix} m_{00} & m_{01} & m_{02} & m_{03} \\ m_{10} & m_{11} & m_{12} & m_{13} \\ m_{20} & m_{21} & m_{22} & m_{23} \\ m_{30} & m_{31} & m_{32} & m_{33} \end{bmatrix} \implies \text{Array: } [m_{00}, m_{10}, m_{20}, m_{30}, m_{01}, m_{11}, \dots]$$
When multiplying a matrix by a column vector $\mathbf{v}' = M \mathbf{v}$:
$$\mathbf{v}' = v_0 \mathbf{c}_0 + v_1 \mathbf{c}_1 + v_2 \mathbf{c}_2 + v_3 \mathbf{c}_3$$
where $\mathbf{c}_i$ is the $i$-th column of $M$.
Because column elements are contiguous in memory, computing this linear combination accesses memory sequentially (stride-1), maximizing CPU L1 cache line utilization and mapping directly to SIMD operations (e.g. `_mm_mul_ps` on entire columns).

---

### Q1.2: Derive the difference between Euler angles and Quaternions. Why do Euler angles suffer from Gimbal Lock?
**Model Answer:**
Euler angles represent orientation as three sequential rotations around cardinal axes (e.g., $R_z(\psi) R_y(\theta) R_x(\phi)$).
**Gimbal Lock** occurs when the middle rotation aligns two of the three rotation axes. In a Pitch-Yaw-Roll system (order $Y \cdot X \cdot Z$), if pitch angle $\theta = \pm 90^\circ$, the rotation matrix collapses such that roll and yaw rotate around the same physical world axis:
$$\cos(\pm 90^\circ) = 0, \quad \sin(\pm 90^\circ) = \pm 1$$
This reduces the degrees of freedom from 3 to 2, causing the system to lose one rotational axis.
A **unit quaternion** $q = (s, \mathbf{v}) \in \mathbb{H}$ represents orientation as a point on the 4D unit hypersphere $S^3$. Because quaternions represent a single axis-angle rotation $\mathbf{v} = \mathbf{u} \sin(\theta/2), s = \cos(\theta/2)$, they have no coordinate singularities and never experience gimbal lock.

---

### Q1.3: Derive Spherical Linear Interpolation (Slerp) and explain why simple linear interpolation (Lerp) is insufficient for quaternions.
**Model Answer:**
Linear interpolation of quaternions followed by normalization is **Nlerp**:
$$\text{Nlerp}(q_1, q_2, t) = \frac{(1-t)q_1 + t q_2}{\|(1-t)q_1 + t q_2\|}$$
Nlerp traverses the chord between $q_1$ and $q_2$ on the unit sphere, which means angular velocity varies throughout the interpolation (faster in the middle, slower near endpoints).
**Slerp** moves along the great-circle arc with constant angular velocity:
Given $\cos \Omega = q_1 \cdot q_2$, the interpolation along the arc of angle $\Omega$ is:
$$\text{Slerp}(q_1, q_2, t) = \frac{\sin((1-t)\Omega)}{\sin \Omega} q_1 + \frac{\sin(t\Omega)}{\sin \Omega} q_2$$
**Shortest Path Rule**: Because $q$ and $-q$ represent the exact same 3D rotation, if $q_1 \cdot q_2 < 0$, we negate $q_2$ (and set $\cos \Omega = -q_1 \cdot q_2$) to ensure interpolation takes the shortest path ($\le 180^\circ$).

---

### Q1.4: Derive the Möller-Trumbore Ray-Triangle intersection algorithm.
**Model Answer:**
A point $P$ inside a triangle with vertices $V_0, V_1, V_2$ is parameterized by barycentric coordinates $(u, v)$:
$$P(u, v) = (1 - u - v)V_0 + u V_1 + v V_2 = V_0 + u(V_1 - V_0) + v(V_2 - V_0)$$
Let $E_1 = V_1 - V_0$ and $E_2 = V_2 - V_0$. A ray is $R(t) = O + t D$. Equating $R(t) = P(u, v)$:
$$O + t D = V_0 + u E_1 + v E_2 \implies [-D, E_1, E_2] \begin{bmatrix} t \\ u \\ v \end{bmatrix} = O - V_0$$
Let $T = O - V_0$. Applying Cramer's rule:
$$\det([-D, E_1, E_2]) = (D \times E_2) \cdot E_1 = P_{\text{vec}} \cdot E_1$$
$$\begin{bmatrix} t \\ u \\ v \end{bmatrix} = \frac{1}{P_{\text{vec}} \cdot E_1} \begin{bmatrix} (T \times E_1) \cdot E_2 \\ (D \times E_2) \cdot T \\ (T \times E_1) \cdot D \end{bmatrix} = \frac{1}{\det} \begin{bmatrix} Q_{\text{vec}} \cdot E_2 \\ P_{\text{vec}} \cdot T \\ Q_{\text{vec}} \cdot D \end{bmatrix}$$
where $P_{\text{vec}} = D \times E_2$ and $Q_{\text{vec}} = T \times E_1$.
The intersection is valid iff:
$$\det \ne 0, \quad u \in [0, 1], \quad v \in [0, 1], \quad u + v \le 1, \quad t > 0$$

---

### Q1.5: How does the Gribb-Hartmann method extract frustum clipping planes directly from the view-projection matrix?
**Model Answer:**
In homogeneous clip space, the canonical viewing volume is defined by:
$$-w \le x \le w, \quad -w \le y \le w, \quad -w \le z \le w$$
Since a clip space vertex is $\mathbf{v}_{\text{clip}} = M \mathbf{v}_{\text{world}}$, where $M = P \cdot V$, each component is the dot product of the corresponding row of $M$ with $\mathbf{v}_{\text{world}}$:
$$x = \text{row}_0 \cdot \mathbf{v}, \quad y = \text{row}_1 \cdot \mathbf{v}, \quad z = \text{row}_2 \cdot \mathbf{v}, \quad w = \text{row}_3 \cdot \mathbf{v}$$
The left plane boundary condition $-w \le x \iff x + w \ge 0$ becomes:
$$(\text{row}_3 + \text{row}_0) \cdot \mathbf{v} \ge 0$$
This is precisely a plane equation $\mathbf{n} \cdot \mathbf{p} + d = 0$ with plane coefficients:
$$\pi_{\text{left}} = \text{row}_3 + \text{row}_0, \quad \pi_{\text{right}} = \text{row}_3 - \text{row}_0$$
$$\pi_{\text{bottom}} = \text{row}_3 + \text{row}_1, \quad \pi_{\text{top}} = \text{row}_3 - \text{row}_1$$
$$\pi_{\text{near}} = \text{row}_3 + \text{row}_2, \quad \pi_{\text{far}} = \text{row}_3 - \text{row}_2$$
Normalizing each plane by $\|\mathbf{n}\|$ yields metric Euclidean distances.

---

# Stage 2: Framebuffers, Pipelines & Line Algorithms

### Q2.1: Derive Bresenham's Line Algorithm. Why does it use only integer arithmetic?
**Model Answer:**
Given a line from $(x_0, y_0)$ to $(x_1, y_1)$ with slope $0 \le m \le 1$ ($m = \frac{\Delta y}{\Delta x}$):
At step $x_k + 1$, the exact line equation is $y = m(x_k + 1) + c$.
The two candidate pixels are $(x_k + 1, y_k)$ and $(x_k + 1, y_k + 1)$.
The distance difference from the true line is:
$$d_1 = y - y_k, \quad d_2 = (y_k + 1) - y \implies d_1 - d_2 = 2m(x_k + 1) - 2y_k + 2c - 1$$
Multiplying by $\Delta x$ eliminates all floating-point divisions:
$$p_k = \Delta x (d_1 - d_2) = 2\Delta y \cdot x_k - 2\Delta x \cdot y_k + C$$
At step $k+1$:
$$p_{k+1} - p_k = 2\Delta y - 2\Delta x (y_{k+1} - y_k)$$
- If $p_k < 0$, select $y_{k+1} = y_k$, and $p_{k+1} = p_k + 2\Delta y$.
- If $p_k \ge 0$, select $y_{k+1} = y_k + 1$, and $p_{k+1} = p_k + 2\Delta y - 2\Delta x$.
Because $p_0 = 2\Delta y - \Delta x$, the entire algorithm operates exclusively with integer additions, subtractions, and bit shifts.

---

### Q2.2: How does Cohen-Sutherland line clipping use bitwise outcodes to accelerate clipping?
**Model Answer:**
Cohen-Sutherland partitions the 2D plane into 9 regions using a 4-bit outcode `[TOP, BOTTOM, RIGHT, LEFT]`:
- Trivial Accept: Both endpoints have code `0000` (`outcode0 | outcode1 == 0`). Line is entirely inside viewport.
- Trivial Reject: Both endpoints lie on the outside of the same boundary (`outcode0 & outcode1 != 0`). Line is entirely outside viewport.
- Partial: When neither holds, the line is clipped against the boundary corresponding to the set bit using linear interpolation:
  $$x = x_0 + (x_1 - x_0) \frac{y_{\text{edge}} - y_0}{y_1 - y_0}$$
This eliminates line segments without trigonometric or matrix calculations.

---

### Q2.3: Calculate the raw memory bandwidth required for a software renderer at 1080p and 4K at 60 FPS.
**Model Answer:**
For an RGBA32 color buffer (4 bytes/pixel) and 32-bit float depth buffer (4 bytes/pixel):
- Per-pixel read/write bandwidth in a typical scene with depth testing and color write:
  - 1 Depth Read (4B)
  - 1 Depth Write (4B)
  - 1 Color Write (4B)
  - Clear operations (8B per frame)
Total per-fragment bandwidth $\approx 12$ bytes/fragment. Assuming an average depth complexity (overdraw) of 2.0:
$$\text{Bandwidth}_{\text{frame}} = (\text{Width} \times \text{Height}) \times (8\text{B clear} + 2.0 \times 12\text{B}) = \text{Pixels} \times 32\text{ bytes}$$
- **1080p (1920x1080 = 2.07M pixels) @ 60 FPS**:
  $$2.07 \times 10^6 \times 32 \times 60 \approx 3.98 \text{ GB/s}$$
- **4K (3840x2160 = 8.29M pixels) @ 60 FPS**:
  $$8.29 \times 10^6 \times 32 \times 60 \approx 15.9 \text{ GB/s}$$
This demonstrates why CPU cache locality and early-Z depth testing are critical to avoid saturating host memory channels.

---

### Q2.4: What is the purpose of the homogeneous coordinate $w$, and why can't translation be represented in a $3 \times 3$ matrix?
**Model Answer:**
Matrix multiplication is a linear map ($f(\mathbf{u} + \mathbf{v}) = f(\mathbf{u}) + f(\mathbf{v})$ and $f(c\mathbf{u}) = c f(\mathbf{u})$), which means the origin must map to the origin: $M \cdot \mathbf{0} = \mathbf{0}$.
Translation $\mathbf{x}' = \mathbf{x} + \mathbf{t}$ is an **affine transformation**, not a linear transformation, because $\mathbf{0} + \mathbf{t} \ne \mathbf{0}$.
By embedding 3D Euclidean space into 4D projective space using homogeneous coordinates $\mathbf{x} = (x, y, z, 1)^T$, translation becomes a linear shear in 4D:
$$\begin{bmatrix} 1 & 0 & 0 & t_x \\ 0 & 1 & 0 & t_y \\ 0 & 0 & 1 & t_z \\ 0 & 0 & 0 & 1 \end{bmatrix} \begin{bmatrix} x \\ y \\ z \\ 1 \end{bmatrix} = \begin{bmatrix} x + t_x \\ y + t_y \\ z + t_z \\ 1 \end{bmatrix}$$
Furthermore, setting $w = -z_{\text{view}}$ in perspective projection enables the perspective divide by $w$ to scale geometry inversely with distance from the camera.

---

### Q2.5: Why is double buffering necessary, and how does SDL2 streaming texture presentation work?
**Model Answer:**
Without double buffering, rendering directly to visible display memory causes **screen tearing** and visible flickering, as the display scanout reads partially drawn frames mid-render.
Double buffering maintains two buffers:
1. **Back Buffer**: Software rasterizer writes into this contiguous memory block.
2. **Front Buffer**: Display controller presents this buffer to the screen.
In Prism, `Window::present` copies the back buffer into an SDL streaming texture via `SDL_LockTexture` and `SDL_RenderCopy`. During VSync, the front and back buffers are swapped atomically.

---

# Stage 3: Software Rasterization, Clipping & Shading

### Q3.1: Derive the 2D triangle edge function and explain the top-left fill rule.
**Model Answer:**
For an edge directed from $A=(A_x, A_y)$ to $B=(B_x, B_y)$, the implicit line equation is given by the 2D cross product:
$$E(P) = (\mathbf{B} - \mathbf{A}) \times (\mathbf{P} - \mathbf{A}) = (B_x - A_x)(P_y - A_y) - (B_y - A_y)(P_x - A_x)$$
- $E(P) > 0$: Point $P$ lies to the left of the edge.
- $E(P) < 0$: Point $P$ lies to the right of the edge.
- $E(P) = 0$: Point $P$ lies exactly on the edge.
**The Top-Left Fill Rule**:
When two adjacent triangles share an edge, points on the edge satisfy $E(P) = 0$ for both triangles. Without a tie-breaking rule, edge pixels would either be rasterized twice (causing blending artifacts) or zero times (causing cracked seams).
A pixel on an edge is shaded iff the edge is:
1. **Top Edge**: Exactly horizontal and pointing right ($B_y = A_y$ and $B_x > A_x$).
2. **Left Edge**: Going downwards ($B_y < A_y$).
This guarantees every shared pixel is shaded exactly once.

---

### Q3.2: Mathematically derive why perspective-correct interpolation requires interpolating $A/w$ rather than $A$.
**Model Answer:**
Let $z$ be camera-space depth. Screen-space coordinate $x_s$ is related to camera-space coordinate $x_e$ by:
$$x_s = \frac{x_e}{-z} \cdot d \implies x_e = -\frac{z x_s}{d}$$
Consider an attribute $A$ that varies linearly in 3D world/eye space:
$$A(x_e) = a x_e + b = -a \frac{z x_s}{d} + b$$
Notice $A$ is **not linear in $x_s$** because $z$ is in the numerator!
However, consider the reciprocal of depth $1/z$, which is linear in screen space:
$$\frac{1}{z} = c_1 x_s + c_0$$
Now divide $A$ by $z$:
$$\frac{A}{z} = a \frac{x_e}{z} + \frac{b}{z} = -a \frac{x_s}{d} + b (c_1 x_s + c_0)$$
Because the right-hand side is a degree-1 polynomial in $x_s$, **$A/z$ (and $A/w$) is strictly linear in screen space**.
Therefore, we linearly interpolate $\frac{A_i}{w_i}$ and $\frac{1}{w_i}$ across the screen triangle, and recover the perspective-correct attribute at each fragment:
$$A_{\text{correct}} = \frac{\sum_i \lambda_i \frac{A_i}{w_i}}{\sum_i \lambda_i \frac{1}{w_i}}$$

---

### Q3.3: Why must polygon clipping occur in homogeneous clip space rather than after perspective division in NDC?
**Model Answer:**
Perspective division computes $\mathbf{x}_{\text{ndc}} = \mathbf{x}_{\text{clip}} / w_{\text{clip}}$.
When a triangle crosses the camera plane ($z_{\text{camera}} \le 0$):
1. Vertices behind the camera have $w \le 0$. Dividing by zero causes an IEEE floating-point infinity or NaN singularity.
2. Vertices with $w < 0$ invert their NDC coordinates (points behind the camera flip through the origin and appear inverted in front of the camera).
In homogeneous clip space, the viewing frustum is bounded by 6 hyperplanes:
$$\pm x \le w, \quad \pm y \le w, \quad \pm z \le w$$
All 6 planes are linear in 4D clip space. Sutherland-Hodgman homogeneous clipping splits edges against $w \pm x = 0$ cleanly using linear interpolation without any divisions.

---

### Q3.4: Compare Blinn-Phong vs Phong reflection models. Why is Blinn-Phong preferred?
**Model Answer:**
- **Phong Model**: Calculates specular reflection using the reflected light vector $\mathbf{R} = 2(\mathbf{N} \cdot \mathbf{L})\mathbf{N} - \mathbf{L}$ and view vector $\mathbf{V}$:
  $$I_{\text{spec}} = (\mathbf{R} \cdot \mathbf{V})^\alpha$$
- **Blinn-Phong Model**: Calculates specular reflection using the halfway vector $\mathbf{H} = \frac{\mathbf{L} + \mathbf{V}}{\|\mathbf{L} + \mathbf{V}\|}$:
  $$I_{\text{spec}} = (\mathbf{N} \cdot \mathbf{H})^\beta$$
**Why Blinn-Phong is preferred:**
1. **Computational efficiency**: For directional lights and orthographic/distant viewers, $\mathbf{L}$ and $\mathbf{V}$ are constant across the mesh, meaning $\mathbf{H}$ can be precomputed once per light rather than computing $\mathbf{R}$ per vertex/pixel.
2. **Physical behavior at grazing angles**: When $\mathbf{R} \cdot \mathbf{V} < 0$ at steep angles, Phong drops abruptly to 0. Blinn-Phong produces smoother, more natural specular falloff.

---

### Q3.5: Derive Bilinear Texture Filtering.
**Model Answer:**
Given normalized UV coordinates $(u, v) \in [0, 1]^2$ and texture dimensions $(W, H)$:
1. Continuous texel coordinates: $x = u \cdot (W - 1), \quad y = v \cdot (H - 1)$.
2. Integer coordinates of 4 surrounding texels:
   $$x_0 = \lfloor x \rfloor, \quad x_1 = \min(x_0 + 1, W - 1)$$
   $$y_0 = \lfloor y \rfloor, \quad y_1 = \min(y_0 + 1, H - 1)$$
3. Fractional interpolation factors: $s = x - x_0, \quad t = y - y_0$.
4. Two horizontal linear interpolations:
   $$C_{\text{top}} = (1 - s) C(x_0, y_0) + s C(x_1, y_0)$$
   $$C_{\text{bottom}} = (1 - s) C(x_0, y_1) + s C(x_1, y_1)$$
5. Vertical linear interpolation:
   $$C_{\text{final}} = (1 - t) C_{\text{top}} + t C_{\text{bottom}}$$
This eliminates pixelation and blocky nearest-neighbor artifacts.

---

# Stage 4: Collision Detection, Scene Graphs & Game Mechanics

### Q4.1: Derive Sphere-AABB collision detection and contact resolution.
**Model Answer:**
Given a Sphere with center $\mathbf{C}$ and radius $r$, and an Axis-Aligned Bounding Box (AABB) $[\mathbf{B}_{\min}, \mathbf{B}_{\max}]$:
1. Find the point $\mathbf{P}$ on the AABB closest to $\mathbf{C}$ by clamping:
   $$\mathbf{P}_i = \text{clamp}(\mathbf{C}_i, \mathbf{B}_{\min, i}, \mathbf{B}_{\max, i}) \quad \forall i \in \{x, y, z\}$$
2. Compute distance vector $\mathbf{d} = \mathbf{C} - \mathbf{P}$.
3. Intersection occurs iff $\|\mathbf{d}\|^2 \le r^2$.
4. **Collision Resolution (Push-Out)**:
   - If $\|\mathbf{d}\| > 0$, the penetration depth is $\delta = r - \|\mathbf{d}\|$, and the contact normal is $\hat{\mathbf{n}} = \mathbf{d} / \|\mathbf{d}\|$.
   - The sphere position is corrected: $\mathbf{C}' = \mathbf{C} + \hat{\mathbf{n}} \cdot \delta$.

---

### Q4.2: How does raycasting against an interactive scene work, and what is its computational complexity?
**Model Answer:**
To interact with world objects, the camera fires a ray $\mathbf{R}(t) = \mathbf{O} + t \mathbf{D}$, where $\mathbf{O}$ is camera eye position and $\mathbf{D} = \mathbf{q}_{\text{orient}} \cdot (0, 0, -1)^T$.
Each entity's bounding volume is tested:
- Bounding Sphere test: solving the quadratic equation $(\mathbf{D} \cdot (\mathbf{O} - \mathbf{C}))^2 - (\|\mathbf{O} - \mathbf{C}\|^2 - r^2) \ge 0$.
- Bounding Box test: slab method testing intervals $[t_{\min}, t_{\max}]$.
For $N$ entities without spatial indexing, complexity is $O(N)$. With an AABB hierarchy or spatial grid, ray traversal drops to $O(\log N)$.

---

### Q4.3: Why is Euler integration common in games, and how does semi-implicit Euler improve stability?
**Model Answer:**
In explicit (forward) Euler:
$$\mathbf{x}_{n+1} = \mathbf{x}_n + \mathbf{v}_n \Delta t$$
$$\mathbf{v}_{n+1} = \mathbf{v}_n + \mathbf{a}_n \Delta t$$
Forward Euler adds artificial energy into oscillatory systems (e.g. springs and orbits), causing them to diverge and blow up.
In **semi-implicit (symplectic) Euler**:
$$\mathbf{v}_{n+1} = \mathbf{v}_n + \mathbf{a}_n \Delta t$$
$$\mathbf{x}_{n+1} = \mathbf{x}_n + \mathbf{v}_{n+1} \Delta t$$
Because the position update uses the *new* velocity $\mathbf{v}_{n+1}$, semi-implicit Euler preserves phase space volume (symplecticity), keeping harmonic systems and character velocity stable even with larger time steps $\Delta t$.

---

### Q4.4: How does a scene graph manage hierarchical transformations?
**Model Answer:**
A scene graph represents spatial relationships as a tree where each node has a local transform $M_{\text{local}} = T \cdot R \cdot S$.
The world matrix of node $i$ with parent $p$ is evaluated recursively:
$$M_{\text{world}, i} = M_{\text{world}, p} \cdot M_{\text{local}, i}$$
This ensures that when a parent moves or rotates (e.g., player holding a weapon), all child entities automatically inherit the transformation. A dirty-flag pattern avoids redundant matrix multiplications for stationary branches.

---

### Q4.5: How do you implement a first-person free-look camera without pitch flipping?
**Model Answer:**
Store pitch $\theta$ and yaw $\psi$ angles independently:
$$\theta = \text{clamp}(\theta + \Delta \theta, -89.0^\circ, +89.0^\circ)$$
$$\psi = \psi + \Delta \psi$$
Clamping pitch strictly between $[-89^\circ, +89^\circ]$ prevents the camera from flipping upside-down.
The final camera orientation quaternion is composed as:
$$q_{\text{camera}} = q_{\text{yaw}} \cdot q_{\text{pitch}} = \text{Quat}(\hat{\mathbf{y}}, \psi) \cdot \text{Quat}(\hat{\mathbf{x}}, \theta)$$
This guarantees smooth, singularity-free mouse look.

---

# Stage 5: Performance Engineering, SIMD & Concurrency

### Q5.1: State Amdahl's Law and explain why multithreading can slow down small software rasterizer workloads.
**Model Answer:**
Amdahl's law defines the maximum theoretical speedup $S$ of a program with parallel fraction $p$ using $N$ processors:
$$S(N) = \frac{1}{(1 - p) + \frac{p}{N}}$$
However, Amdahl's law assumes zero communication and dispatch overhead. In reality, launching threads incurs an operating system overhead $T_{\text{overhead}}$ (thread spawning, context switching, cache line invalidation):
$$T_{\text{parallel}} = (1 - p) T_s + \frac{p T_s}{N} + T_{\text{overhead}}$$
If the total serial execution time $T_s$ is very small (e.g., $0.10 \text{ ms}$ for a lightweight scene), $T_{\text{overhead}} \approx 0.05\text{--}0.2\text{ ms}$ dominates, resulting in an overall **slowdown** ($S < 1.0$).
Parallelization only yields speedups when the workload per task significantly exceeds thread dispatch latency.

---

### Q5.2: How does SIMD edge testing vectorize rasterization?
**Model Answer:**
The standard scalar edge function evaluates one pixel at a time:
$$E(x, y) = (B_x - A_x)(y - A_y) - (B_y - A_y)(x - A_x)$$
Using 128-bit SSE or 256-bit AVX, we evaluate 4 or 8 horizontal pixels simultaneously:
$$\mathbf{P}_x = [x, x+1, x+2, x+3]$$
$$\mathbf{E} = \mathbf{dx} \cdot \mathbf{dy}_{\text{offset}} - \mathbf{dy} \cdot (\mathbf{P}_x - \mathbf{A}_x)$$
Using `_mm_set_ps`, `_mm_mul_ps`, and `_mm_sub_ps`, all 4 edge equations are computed in 2 clock cycles.
A bitmask of inside pixels is extracted using `_mm_movemask_ps` for branchless pixel rejection.

---

### Q5.3: Explain false sharing and how to avoid it in a tile-based multithreaded rasterizer.
**Model Answer:**
Modern CPU caches operate on **cache lines** (typically 64 bytes). If two threads running on different cores write to different variables that reside on the same 64-byte cache line, the CPU cache coherency protocol (MESI) repeatedly invalidates and bounces the cache line between cores. This severe performance penalty is **false sharing**.
**Avoidance in Prism**:
Prism partitions the framebuffer into horizontal stripes of multiple scanlines (e.g., height $\ge 32$ pixels).
Since each horizontal row contains $640 \times 4\text{ bytes} = 2,560\text{ bytes}$ (40 distinct cache lines), worker threads write to memory addresses separated by tens of thousands of bytes, completely eliminating false sharing.

---

### Q5.4: What is the Structure of Arrays (SoA) vs Array of Structures (AoS) tradeoff in graphics pipelines?
**Model Answer:**
- **AoS (Array of Structures)**:
  ```cpp
  struct Vertex { Vec3 pos; Vec3 norm; Vec2 uv; };
  std::vector<Vertex> vertices;
  ```
- **SoA (Structure of Arrays)**:
  ```cpp
  struct VertexBuffer {
      std::vector<float> x, y, z;
      std::vector<float> nx, ny, nz;
      std::vector<float> u, v;
  };
  ```
**Tradeoff**:
In vertex transformation, only positions $(x, y, z)$ are multiplied by the model-view-projection matrix. In AoS, loading positions into cache lines also forces normals and UVs into cache, wasting ~60% of memory bandwidth.
SoA allows loading purely contiguous $x, y, z$ arrays into SIMD registers, achieving maximum vectorization throughput and cache efficiency.

---

### Q5.5: Why are zero-heap allocations critical in real-time game engine loops?
**Model Answer:**
Calls to `malloc` / `new` and `free` / `delete`:
1. Require navigating heap free-lists and acquire OS memory locks, causing non-deterministic frame time spikes (jitter).
2. Fragment physical memory over time, degrading cache locality.
3. May trigger kernel transitions or page faults.
In Prism, scratch memory (`RenderScratchBuffer`) and framebuffer memory are pre-allocated at startup. The rendering loop operates with **zero per-frame heap allocations**, guaranteeing steady 60+ FPS frame times.
