#ifndef MESH_TOOLS_H
#define MESH_TOOLS_H

#include <vector>
#include <math.h>
#include <algorithm>

// tools of the generated meshes (cars and people): vertices, vectors, smooth curves and a triangle builder

// a vertex of a generated mesh in the coordinates of its box (the center of the box is 0/0/0)
struct MeshVertex
{
    float position[3];
    float normal[3];
    float st[2];
    float color[4];
};

namespace mesh {

struct Key
{
    float z;
    float value;
};

/*!
 * \brief curve
 * smooth curve through the keys (cubic Hermite with the slopes of the neighbors), the first and the last value
 * outside the keys
 */
inline float curve(const Key *keys, int count, float z)
{
    if (z <= keys[0].z) {
        return keys[0].value;
    }
    if (z >= keys[count - 1].z) {
        return keys[count - 1].value;
    }
    int i = 0;
    while (z > keys[i + 1].z) {
        i++;
    }
    auto slope = [keys, count](int k) {
        const int a = std::max(k - 1, 0);
        const int b = std::min(k + 1, count - 1);
        return (keys[b].value - keys[a].value)/(keys[b].z - keys[a].z);
    };
    const float h = keys[i + 1].z - keys[i].z;
    const float t = (z - keys[i].z)/h;
    const float m0 = slope(i)*h;
    const float m1 = slope(i + 1)*h;
    const float t2 = t*t;
    const float t3 = t2*t;
    return (2.0f*t3 - 3.0f*t2 + 1.0f)*keys[i].value + (t3 - 2.0f*t2 + t)*m0 + (-2.0f*t3 + 3.0f*t2)*keys[i + 1].value
            + (t3 - t2)*m1;
}

struct Vec
{
    float x, y, z;
};

inline Vec operator+(Vec a, Vec b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec operator-(Vec a, Vec b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec operator*(Vec a, float f) { return { a.x*f, a.y*f, a.z*f }; }
inline float dot(Vec a, Vec b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec cross(Vec a, Vec b) { return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
inline Vec normalized(Vec a)
{
    const float len = sqrtf(dot(a, a));
    return len > 0.0f ? a*(1.0f/len) : Vec{ 0.0f, 1.0f, 0.0f };
}

// collects the triangles: the paint triangles get the texture of the object, the detail triangles their own colors
class Builder
{
public:
    Builder(std::vector<MeshVertex> &paint, std::vector<MeshVertex> &detail) : m_paint(paint), m_detail(detail) {}

    void vertex(bool isPaint, Vec p, Vec n, float s, float t, const float color[4])
    {
        (isPaint ? m_paint : m_detail).push_back(
            { { p.x, p.y, p.z }, { n.x, n.y, n.z }, { s, t }, { color[0], color[1], color[2], color[3] } });
    }
    // quad of the corners 0-1-2-3 around it as 2 triangles with the normals and the texture coordinates
    void quad(bool isPaint, const Vec p[4], const Vec n[4], const float st[4][2], const float color[4])
    {
        for (const int i : { 0, 1, 2, 0, 2, 3 }) {
            vertex(isPaint, p[i], n[i], st[i][0], st[i][1], color);
        }
    }
    // box of the half sizes along the axes ex/ey/ez (unit vectors at right angles)
    void box(bool isPaint, Vec center, Vec ex, Vec ey, Vec ez, Vec half, const float color[4])
    {
        const Vec x = ex*half.x;
        const Vec y = ey*half.y;
        const Vec z = ez*half.z;
        const float st[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
        for (const float side : { -1.0f, 1.0f }) {
            // middle of the face, its 2 half edges and its normal
            const Vec faces[3][4] = {
                { center + x*side, y, z, ex*side }, { center + y*side, x, z, ey*side },
                { center + z*side, x, y, ez*side } };
            for (const auto &f : faces) {
                const Vec p[4] = { f[0] - f[1] - f[2], f[0] + f[1] - f[2], f[0] + f[1] + f[2], f[0] - f[1] + f[2] };
                const Vec n[4] = { f[3], f[3], f[3], f[3] };
                quad(isPaint, p, n, st, color);
            }
        }
    }

private:
    std::vector<MeshVertex> &m_paint;
    std::vector<MeshVertex> &m_detail;
};

} // namespace mesh

#endif // MESH_TOOLS_H
