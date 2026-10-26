# 3D Math Foundations - Prism Engine

This document is my personal mini-textbook and reference guide for the 3D mathematics powering Prism. Every matrix, transform, and geometric algorithm is derived from first principles with worked numerical examples so that I can recreate and explain them on a whiteboard in any graphics or game engine engineering interview.

---

## 1. Coordinate System Conventions & Vector Algebra

### 1.1 Spaces and Handedness
Prism uses a **Right-Handed Coordinate System** in View / Camera Space:
- $+X$ points **Right**
- $+Y$ points **Up**
- $-Z$ points **Forward** (into the scene)

Vectors are treated as **Column Vectors**:
$$v' = M \cdot v$$
Transformations compose from right to left: $(M_{\text{projection}} \cdot M_{\text{view}} \cdot M_{\text{model}}) \cdot v$.

Matrices are stored in memory in **Column-Major order**:
$$\text{data}[16] = [m_{00}, m_{10}, m_{20}, m_{30}, \; m_{01}, m_{11}, m_{21}, m_{31}, \; m_{02}, m_{12}, m_{22}, m_{32}, \; m_{03}, m_{13}, m_{23}, m_{33}]$$
Element at row $i$ and column $j$ is accessed via:
$$\text{index}(i, j) = j \times 4 + i$$

### 1.2 Dot and Cross Product
- **Dot Product**: $a \cdot b = a_x b_x + a_y b_y + a_z b_z = \|a\| \|b\| \cos\theta$.
  Measures directional alignment, projection length, and cosine of the angle.
- **Cross Product**: Produces a vector perpendicular to both $a$ and $b$ obeying the right-hand rule:
  $$a \times b = \begin{bmatrix} a_y b_z - a_z b_y \\ a_z b_x - a_x b_z \\ a_x b_y - a_y b_x \end{bmatrix}$$
- **Vector Reflection**: Given incident direction $v$ and unit surface normal $n$:
  $$v_{\text{reflect}} = v - 2(v \cdot n)n$$

---

## 2. Matrix Inversion: General vs Affine Fast Path

### 2.1 General $4 \times 4$ Matrix Inversion
General inversion utilizes Laplace expansion to compute the classical adjugate matrix divided by the determinant:
$$M^{-1} = \frac{1}{\det(M)} \text{adj}(M)$$
Computing $\det(M)$ and 16 cofactors takes $\sim 150$ floating-point operations.

### 2.2 Affine Fast-Path Inversion ($[R \mid T]$)
In game engines, camera and entity world transforms are affine:
$$M = \begin{bmatrix} R & T \\ 0^T & 1 \end{bmatrix}$$
where $R \in \mathbb{R}^{3 \times 3}$ and $T \in \mathbb{R}^3$.
If $M^{-1} = \begin{bmatrix} R' & T' \\ 0^T & 1 \end{bmatrix}$, then:
$$M \cdot M^{-1} = \begin{bmatrix} R R' & R T' + T \\ 0^T & 1 \end{bmatrix} = \begin{bmatrix} I & 0 \\ 0^T & 1 \end{bmatrix}$$
Therefore:
$$R' = R^{-1}, \quad T' = -R^{-1} T$$
For pure rotation matrices (orthonormal basis), $R^{-1} = R^T$, yielding:
$$M^{-1} = \begin{bmatrix} R^T & -R^T T \\ 0^T & 1 \end{bmatrix}$$
**Performance Gain**: Drops cost from 150+ FLOPS down to $\sim 30$ FLOPS, eliminating numerical drift.

#### Numeric Example:
Let $T = (3, 4, 5)^T$ and $R$ be a $90^\circ$ rotation around $Z$:
$$R = \begin{bmatrix} 0 & -1 & 0 \\ 1 & 0 & 0 \\ 0 & 0 & 1 \end{bmatrix}, \quad R^T = \begin{bmatrix} 0 & 1 & 0 \\ -1 & 0 & 0 \\ 0 & 0 & 1 \end{bmatrix}$$
$$-R^T T = -\begin{bmatrix} 0(3) + 1(4) + 0(5) \\ -1(3) + 0(4) + 0(5) \\ 0(3) + 0(4) + 1(5) \end{bmatrix} = \begin{bmatrix} -4 \\ 3 \\ -5 \end{bmatrix}$$
$$M^{-1} = \begin{bmatrix} 0 & 1 & 0 & -4 \\ -1 & 0 & 0 & 3 \\ 0 & 0 & 1 & -5 \\ 0 & 0 & 0 & 1 \end{bmatrix}$$

---

## 3. Look-At (View Matrix) Derivation

### 3.1 First-Principles Derivation
We wish to map the world coordinate frame into the observer's local eye coordinate frame located at position $\mathbf{e}$ (eye), looking at target point $\mathbf{t}$, with reference up-guide $\mathbf{u}_{\text{guide}}$.

1. **Camera Forward Vector**:
   Vector pointing from eye to target:
   $$\mathbf{f} = \frac{\mathbf{t} - \mathbf{e}}{\|\mathbf{t} - \mathbf{e}\|}$$
2. **Camera View Axes**:
   In our right-handed coordinate system, looking forward means looking down the $-Z$ axis. Therefore, the camera space $+Z$ axis is:
   $$\mathbf{z}_c = -\mathbf{f}$$
3. **Camera Right Vector**:
   $$\mathbf{r} = \frac{\mathbf{f} \times \mathbf{u}_{\text{guide}}}{\|\mathbf{f} \times \mathbf{u}_{\text{guide}}\|}$$
4. **Camera True Up Vector**:
   $$\mathbf{u} = \mathbf{r} \times \mathbf{f}$$
5. **View Transform Construction**:
   The transformation from camera space to world space is given by placing the basis vectors as columns and translating by $\mathbf{e}$:
   $$M_{\text{cam}\to\text{world}} = \begin{bmatrix} \mathbf{r} & \mathbf{u} & -\mathbf{f} & \mathbf{e} \\ 0 & 0 & 0 & 1 \end{bmatrix}$$
   The view matrix is the inverse of $M_{\text{cam}\to\text{world}}$. Because the upper $3 \times 3$ matrix is orthonormal, its inverse is its transpose:
   $$V = M_{\text{cam}\to\text{world}}^{-1} = \begin{bmatrix} \mathbf{r}^T & -\mathbf{r} \cdot \mathbf{e} \\ \mathbf{u}^T & -\mathbf{u} \cdot \mathbf{e} \\ -\mathbf{f}^T & \mathbf{f} \cdot \mathbf{e} \\ 0 & 1 \end{bmatrix}$$
   Explicitly:
   $$V = \begin{bmatrix}
   r_x & r_y & r_z & -\mathbf{r} \cdot \mathbf{e} \\
   u_x & u_y & u_z & -\mathbf{u} \cdot \mathbf{e} \\
   -f_x & -f_y & -f_z & \mathbf{f} \cdot \mathbf{e} \\
   0 & 0 & 0 & 1
   \end{bmatrix}$$

#### Worked Numeric Example:
Let $\mathbf{e} = (0, 0, 5)^T$, $\mathbf{t} = (0, 0, 0)^T$, $\mathbf{u}_{\text{guide}} = (0, 1, 0)^T$.
- $\mathbf{t} - \mathbf{e} = (0, 0, -5)^T \implies \mathbf{f} = (0, 0, -1)^T$.
- $\mathbf{f} \times \mathbf{u}_{\text{guide}} = (0, 0, -1) \times (0, 1, 0) = (1, 0, 0)^T \implies \mathbf{r} = (1, 0, 0)^T$.
- $\mathbf{u} = \mathbf{r} \times \mathbf{f} = (1, 0, 0) \times (0, 0, -1) = (0, 1, 0)^T$.
- Translations:
  $-\mathbf{r} \cdot \mathbf{e} = 0$, $-\mathbf{u} \cdot \mathbf{e} = 0$, $\mathbf{f} \cdot \mathbf{e} = -5$.
$$V = \begin{bmatrix} 1 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \\ 0 & 0 & 1 & -5 \\ 0 & 0 & 0 & 1 \end{bmatrix}$$
Transforming a world target point $(0, 0, 0, 1)^T$:
$$V \cdot (0, 0, 0, 1)^T = (0, 0, -5, 1)^T$$
The target lies 5 units down the camera's $-Z$ axis, exactly matching physical reality.

---

## 4. Perspective Projection Matrix Derivation

### 4.1 First-Principles Geometric Derivation
Given a camera at the origin looking down $-Z$, let the viewing frustum be defined by:
- Vertical field of view $\text{fov}_y$
- Aspect ratio $a = \frac{\text{width}}{\text{height}}$
- Near clipping plane distance $n > 0$ (located at $z = -n$)
- Far clipping plane distance $f > 0$ (located at $z = -f$)

#### 1. X and Y Projection via Similar Triangles:
Consider a point $(x, y, z)$ in view space where $z < 0$. Projecting onto the near plane $z = -n$:
$$\frac{x_p}{-n} = \frac{x}{z} \implies x_p = -n \frac{x}{z} = n \frac{x}{-z}$$
$$\frac{y_p}{-n} = \frac{y}{z} \implies y_p = -n \frac{y}{z} = n \frac{y}{-z}$$
At the near plane, the half-height $h_n$ is:
$$h_n = n \tan\left(\frac{\text{fov}_y}{2}\right)$$
And half-width $w_n = a \cdot h_n = a \, n \tan\left(\frac{\text{fov}_y}{2}\right)$.

We want to normalize $x_p \in [-w_n, w_n]$ and $y_p \in [-h_n, h_n]$ to NDC $[-1, 1]$:
$$x_{\text{ndc}} = \frac{x_p}{w_n} = \frac{n \frac{x}{-z}}{a \, n \tan(\text{fov}_y/2)} = \frac{x}{-z \cdot a \tan(\text{fov}_y/2)}$$
$$y_{\text{ndc}} = \frac{y_p}{h_n} = \frac{n \frac{y}{-z}}{n \tan(\text{fov}_y/2)} = \frac{y}{-z \tan(\text{fov}_y/2)}$$

#### 2. Homogeneous Representation:
In homogeneous coordinates, $[x_c, y_c, z_c, w_c]^T$ maps to NDC via perspective division:
$$x_{\text{ndc}} = \frac{x_c}{w_c}, \quad y_{\text{ndc}} = \frac{y_c}{w_c}, \quad z_{\text{ndc}} = \frac{z_c}{w_c}$$
Comparing with our geometric equations, setting $w_c = -z$:
$$x_c = \frac{x}{a \tan(\text{fov}_y/2)}, \quad y_c = \frac{y}{\tan(\text{fov}_y/2)}$$

#### 3. Depth Mapping ($z \to z_{\text{ndc}}$):
We must map $z \in [-n, -f]$ non-linearly to NDC $z_{\text{ndc}} \in [-1, 1]$.
Linear form for clip $z_c$:
$$z_c = A z + B$$
Thus:
$$z_{\text{ndc}} = \frac{A z + B}{-z}$$
Boundary conditions:
1. At near plane $z = -n$: $z_{\text{ndc}} = -1$:
   $$\frac{-A n + B}{n} = -1 \implies -A n + B = -n$$
2. At far plane $z = -f$: $z_{\text{ndc}} = +1$:
   $$\frac{-A f + B}{f} = +1 \implies -A f + B = f$$

Subtracting condition 1 from condition 2:
$$(-A f + B) - (-A n + B) = f - (-n) \implies -A(f - n) = f + n \implies A = -\frac{f + n}{f - n}$$
Substituting $A$ back into condition 1:
$$B = -n + A n = -n - \frac{n(f + n)}{f - n} = \frac{-n(f - n) - n(f + n)}{f - n} = -\frac{2fn}{f - n}$$

#### 4. The Complete Matrix:
$$P = \begin{bmatrix}
\frac{1}{a \tan(\text{fov}_y/2)} & 0 & 0 & 0 \\
0 & \frac{1}{\tan(\text{fov}_y/2)} & 0 & 0 \\
0 & 0 & -\frac{f + n}{f - n} & -\frac{2fn}{f - n} \\
0 & 0 & -1 & 0
\end{bmatrix}$$

#### Worked Numeric Example:
Let $\text{fov}_y = 90^\circ$ ($\tan(45^\circ) = 1$), $a = 1.0$, $n = 1.0$, $f = 100.0$.
- $A = -\frac{100 + 1}{100 - 1} = -\frac{101}{99} \approx -1.020202$
- $B = -\frac{2(100)(1)}{99} = -\frac{200}{99} \approx -2.020202$
$$P = \begin{bmatrix} 1 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \\ 0 & 0 & -\frac{101}{99} & -\frac{200}{99} \\ 0 & 0 & -1 & 0 \end{bmatrix}$$

Test point at near plane $(0, 0, -1, 1)^T$:
$$P \cdot \begin{bmatrix} 0 \\ 0 \\ -1 \\ 1 \end{bmatrix} = \begin{bmatrix} 0 \\ 0 \\ \frac{101}{99} - \frac{200}{99} \\ 1 \end{bmatrix} = \begin{bmatrix} 0 \\ 0 \\ -1 \\ 1 \end{bmatrix} \implies z_{\text{ndc}} = \frac{-1}{1} = -1.0$$
Test point at far plane $(0, 0, -100, 1)^T$:
$$P \cdot \begin{bmatrix} 0 \\ 0 \\ -100 \\ 1 \end{bmatrix} = \begin{bmatrix} 0 \\ 0 \\ \frac{10100}{99} - \frac{200}{99} \\ 100 \end{bmatrix} = \begin{bmatrix} 0 \\ 0 \\ 100 \\ 100 \end{bmatrix} \implies z_{\text{ndc}} = \frac{100}{100} = +1.0$$
Perfect boundary satisfaction.

---

## 5. Quaternions: Euler Angles, Gimbal Lock, and SLERP

### 5.1 Euler Angles vs Quaternions: Gimbal Lock
A 3D rotation represented by Euler angles decomposes rotation into three sequential planar rotations, e.g., $R(\psi, \theta, \phi) = R_z(\phi) R_x(\theta) R_y(\psi)$.
When the intermediate pitch angle $\theta = \pm 90^\circ$ ($\frac{\pi}{2}$ rad), the first and third axes align:
$$\cos(90^\circ) = 0, \quad \sin(90^\circ) = 1$$
The resulting rotation matrix simplifies such that $(\psi + \phi)$ act about the exact same physical degree of freedom. One entire rotational axis is lost (Gimbal Lock).

**Unit Quaternions** represent rotation as an element of the hypersphere $S^3 \subset \mathbb{H}$. The 4D unit sphere covers $SO(3)$ twice without singularities or coordinate degeneracies.

### 5.2 Spherical Linear Interpolation (SLERP) Derivation
Given two unit quaternions $q_1, q_2 \in S^3$, we wish to interpolate along the great-circle arc at constant angular velocity:
$$q(t) = c_1(t) q_1 + c_2(t) q_2$$
Let $\theta$ be the angle between $q_1$ and $q_2$:
$$\cos\theta = q_1 \cdot q_2 = w_1 w_2 + x_1 x_2 + y_1 y_2 + z_1 z_2$$

Because $q$ and $-q$ represent the same physical 3D orientation, if $q_1 \cdot q_2 < 0$, we negate $q_2 \to -q_2$ and $\cos\theta \to -\cos\theta$ to take the acute, shortest-path rotation arc.

At fraction $t \in [0, 1]$, the angle rotated from $q_1$ is $t\theta$, and the angle remaining to $q_2$ is $(1 - t)\theta$.
Projecting $q(t)$ onto the orthogonal basis formed by $q_1$ and $q_1^\perp$:
$$q(t) = \frac{\sin((1 - t)\theta)}{\sin\theta} q_1 + \frac{\sin(t\theta)}{\sin\theta} q_2$$

#### Numeric Example:
Let $q_1 = (1, 0, 0, 0)$ (identity) and $q_2 = (0, 0, 0, 1)$ ($180^\circ$ around $Z$).
$\cos\theta = 0 \implies \theta = \frac{\pi}{2}$, $\sin\theta = 1$.
At $t = 0.5$:
$$\sin(0.5 \times \pi / 2) = \sin(\pi / 4) = \frac{\sqrt{2}}{2} \approx 0.707106$$
$$q(0.5) = 0.707106(1, 0, 0, 0) + 0.707106(0, 0, 0, 1) = (0.707106, 0, 0, 0.707106)$$
This represents a $90^\circ$ rotation around $Z$, which is exact.

---

## 6. Möller–Trumbore Ray-Triangle Intersection

### 6.1 Derivation
A ray is parameterized as:
$$R(t) = O + t D, \quad t \ge 0$$
A point on a triangle with vertices $V_0, V_1, V_2$ is defined by barycentric coordinates $(u, v)$:
$$T(u, v) = (1 - u - v)V_0 + u V_1 + v V_2 = V_0 + u(V_1 - V_0) + v(V_2 - V_0)$$
Let edge vectors be $E_1 = V_1 - V_0$ and $E_2 = V_2 - V_0$.
Equating ray and triangle points:
$$O + t D = V_0 + u E_1 + v E_2 \implies [-D, E_1, E_2] \begin{bmatrix} t \\ u \\ v \end{bmatrix} = O - V_0$$
Let $T = O - V_0$. This is a linear system $M x = T$. By Cramer's Rule:
$$\det(M) = |-D, E_1, E_2| = (D \times E_2) \cdot E_1$$
Let $P = D \times E_2$. Then $\det(M) = P \cdot E_1$.
If $|\det| < \epsilon$, the ray is parallel to the triangle.

Using vector scalar triple product identities:
$$\begin{bmatrix} t \\ u \\ v \end{bmatrix} = \frac{1}{(D \times E_2) \cdot E_1} \begin{bmatrix} (T \times E_1) \cdot E_2 \\ (D \times E_2) \cdot T \\ (T \times E_1) \cdot D \end{bmatrix}$$
Let $Q = T \times E_1$. Then:
$$u = \frac{T \cdot P}{\det}, \quad v = \frac{D \cdot Q}{\det}, \quad t = \frac{E_2 \cdot Q}{\det}$$
Valid hit conditions:
1. $u \ge 0$ and $u \le 1$
2. $v \ge 0$ and $u + v \le 1$
3. $t > \epsilon$ (hit in front of ray origin)

#### Worked Numeric Example:
Let triangle vertices be:
$V_0 = (-1, -1, 0)$, $V_1 = (1, -1, 0)$, $V_2 = (0, 1, 0)$.
$E_1 = (2, 0, 0)$, $E_2 = (1, 2, 0)$.
Ray from $O = (0, 0, 5)$ pointing $D = (0, 0, -1)$.
- $P = D \times E_2 = (0, 0, -1) \times (1, 2, 0) = (2, -1, 0)$.
- $\det = P \cdot E_1 = (2)(2) + (-1)(0) + (0)(0) = 4$.
- $T = O - V_0 = (0, 0, 5) - (-1, -1, 0) = (1, 1, 5)$.
- $u = \frac{T \cdot P}{\det} = \frac{(1)(2) + (1)(-1) + (5)(0)}{4} = \frac{1}{4} = 0.25$.
- $Q = T \times E_1 = (1, 1, 5) \times (2, 0, 0) = (0, 10, -2)$.
- $v = \frac{D \cdot Q}{\det} = \frac{(0)(0) + (0)(10) + (-1)(-2)}{4} = \frac{2}{4} = 0.5$.
- $u + v = 0.25 + 0.5 = 0.75 \le 1$ (inside triangle!).
- $t = \frac{E_2 \cdot Q}{\det} = \frac{(1)(0) + (2)(10) + (0)(-2)}{4} = \frac{20}{4} = 5.0$.
Hit Point: $O + t D = (0, 0, 5) + 5(0, 0, -1) = (0, 0, 0)^T$. Exactly on the triangle!
