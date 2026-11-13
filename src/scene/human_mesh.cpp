#include "human_mesh.h"
#include "../math/math3d.h"
#include <functional>

using namespace mesh;

// A person is built in meters (the ground is y 0, the person looks toward +z) and then scaled into the box.
// The torso is lofted superellipses, the arms, the legs, the neck and the skirt are tubes along smooth paths, the
// head, the hair, the hands, the shoes, the nose, the ears, the eyes and the bust are ellipsoids.
#define RING_SEGMENTS       32
#define TORSO_ROWS          64
#define LIMB_ROWS           40
#define ELLIPSOID_RINGS     24
// superellipse exponent of the torso (2 is an ellipse, larger is more square)
#define TORSO_EXPONENT      2.4f

namespace {

const float SHIRT[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const float SKIN[4] = { 0.86f, 0.67f, 0.55f, 1.0f };
const float HAIR[4] = { 0.18f, 0.12f, 0.08f, 1.0f };
const float BLOND_HAIR[4] = { 0.62f, 0.47f, 0.26f, 1.0f };
const float EYE[4] = { 0.08f, 0.06f, 0.05f, 1.0f };
const float MOUTH[4] = { 0.55f, 0.30f, 0.28f, 1.0f };
const float LIPS[4] = { 0.65f, 0.25f, 0.27f, 1.0f };
const float JEANS[4] = { 0.13f, 0.18f, 0.30f, 1.0f };
const float SKIRT[4] = { 0.12f, 0.12f, 0.14f, 1.0f };
const float SHORTS[4] = { 0.45f, 0.38f, 0.25f, 1.0f };
const float BELT[4] = { 0.20f, 0.12f, 0.07f, 1.0f };
const float BUCKLE[4] = { 0.70f, 0.70f, 0.72f, 1.0f };
const float SHOE[4] = { 0.06f, 0.05f, 0.05f, 1.0f };
const float FLAT_SHOE[4] = { 0.30f, 0.07f, 0.07f, 1.0f };
const float SNEAKER[4] = { 0.85f, 0.85f, 0.85f, 1.0f };

using ColorFunction = std::function<const float *(Vec)>;

enum class Lower
{
    Jeans,  // long jeans with a belt
    Skirt,  // skirt from the waist to the knees, bare legs
    Shorts, // shorts to above the knees, bare legs
};

struct LimbPoint
{
    float rate;     // 0 at the upper joint, 1 at the middle joint, 2 at the lower joint
    float radiusX;
    float radiusZ;
};

struct HumanStyle
{
    // torso from the crotch to the neck: half width, half depth and the middle of the depth along the height
    const Key *torsoWidth;
    const Key *torsoDepth;
    const Key *torsoMiddle;
    int torsoCount;
    float shirtBottom;      // shirt above, the belt (jeans) down to beltBottom
    float beltBottom;
    float sleeveEnd;        // short sleeves down to this height
    Vec shoulder, elbow, wrist;     // right arm (x > 0), the left one is mirrored
    LimbPoint arm[5];
    Vec deltoid, deltoidRadius;     // shoulder muscle
    Vec hand, handRadius;           // from the wrist
    Vec thumb, thumbRadius;
    Vec hip, knee, ankle;
    LimbPoint leg[7];
    Vec shoe, shoeRadius;
    Vec neck[3];
    float neckRadiusX, neckRadiusZ;
    Vec head, headRadius;
    float hairVolume;       // the hair is this much out of the head
    float hairBack;         // the hairline is lower at the sides and the back by this much (long hair)
    bool longHair;          // hair down to the shoulders at the back
    const float *hairColor;
    const float *mouthColor;
    Lower lower;
    float shortsEnd;        // shorts down to this height
    float skirtTop, skirtHips, skirtHem;    // heights of the waist, the hips and the hem of the skirt
    float skirtRadius[3][2];        // half width and half depth at the waist, the hips and the hem
    const float *shoeColor;
    bool bust;
};

const Key MAN_WIDTH[] = {
    { 0.80f, 0.100f }, { 0.86f, 0.165f }, { 0.93f, 0.178f }, { 1.00f, 0.165f }, { 1.06f, 0.152f }, { 1.15f, 0.158f },
    { 1.25f, 0.172f }, { 1.34f, 0.182f }, { 1.42f, 0.188f }, { 1.47f, 0.165f }, { 1.51f, 0.105f }, { 1.55f, 0.060f } };
const Key MAN_DEPTH[] = {
    { 0.80f, 0.060f }, { 0.86f, 0.095f }, { 0.93f, 0.108f }, { 1.00f, 0.104f }, { 1.06f, 0.100f }, { 1.15f, 0.105f },
    { 1.25f, 0.115f }, { 1.34f, 0.118f }, { 1.42f, 0.106f }, { 1.47f, 0.085f }, { 1.51f, 0.065f }, { 1.55f, 0.055f } };
const Key MAN_MIDDLE[] = {
    { 0.80f, -0.015f }, { 0.86f, -0.022f }, { 0.93f, -0.020f }, { 1.00f, -0.008f }, { 1.06f, 0.000f },
    { 1.15f, 0.005f }, { 1.25f, 0.012f }, { 1.34f, 0.012f }, { 1.42f, 0.005f }, { 1.47f, 0.000f },
    { 1.51f, -0.005f }, { 1.55f, -0.005f } };
const Key WOMAN_WIDTH[] = {
    { 0.733f, 0.090f }, { 0.788f, 0.160f }, { 0.853f, 0.178f }, { 0.917f, 0.160f }, { 0.972f, 0.125f },
    { 1.054f, 0.130f }, { 1.146f, 0.145f }, { 1.228f, 0.152f }, { 1.302f, 0.158f }, { 1.348f, 0.140f },
    { 1.384f, 0.090f }, { 1.421f, 0.050f } };
const Key WOMAN_DEPTH[] = {
    { 0.733f, 0.055f }, { 0.788f, 0.090f }, { 0.853f, 0.105f }, { 0.917f, 0.095f }, { 0.972f, 0.085f },
    { 1.054f, 0.088f }, { 1.146f, 0.102f }, { 1.228f, 0.104f }, { 1.302f, 0.090f }, { 1.348f, 0.072f },
    { 1.384f, 0.055f }, { 1.421f, 0.048f } };
const Key WOMAN_MIDDLE[] = {
    { 0.733f, -0.015f }, { 0.788f, -0.025f }, { 0.853f, -0.025f }, { 0.917f, -0.010f }, { 0.972f, 0.000f },
    { 1.054f, 0.005f }, { 1.146f, 0.014f }, { 1.228f, 0.012f }, { 1.302f, 0.004f }, { 1.348f, 0.000f },
    { 1.384f, -0.004f }, { 1.421f, -0.004f } };
const Key CHILD_WIDTH[] = {
    { 0.52f, 0.060f }, { 0.56f, 0.112f }, { 0.60f, 0.122f }, { 0.66f, 0.115f }, { 0.72f, 0.112f }, { 0.80f, 0.118f },
    { 0.86f, 0.123f }, { 0.90f, 0.108f }, { 0.93f, 0.070f }, { 0.95f, 0.040f } };
const Key CHILD_DEPTH[] = {
    { 0.52f, 0.040f }, { 0.56f, 0.060f }, { 0.60f, 0.070f }, { 0.66f, 0.078f }, { 0.72f, 0.078f }, { 0.80f, 0.080f },
    { 0.86f, 0.074f }, { 0.90f, 0.060f }, { 0.93f, 0.045f }, { 0.95f, 0.040f } };
const Key CHILD_MIDDLE[] = {
    { 0.52f, -0.016f }, { 0.56f, -0.020f }, { 0.60f, -0.016f }, { 0.66f, 0.000f }, { 0.72f, 0.004f },
    { 0.80f, 0.006f }, { 0.86f, 0.003f }, { 0.90f, 0.000f }, { 0.93f, -0.004f }, { 0.95f, -0.004f } };

const HumanStyle MAN = {
    MAN_WIDTH, MAN_DEPTH, MAN_MIDDLE, sizeof(MAN_WIDTH)/sizeof(Key), 1.02f, 0.985f, 1.24f,
    { 0.185f, 1.43f, -0.005f }, { 0.232f, 1.13f, -0.02f }, { 0.255f, 0.875f, 0.015f },
    { { 0.0f, 0.050f, 0.054f }, { 0.4f, 0.046f, 0.050f }, { 1.0f, 0.037f, 0.040f }, { 1.3f, 0.041f, 0.043f },
      { 2.0f, 0.027f, 0.022f } },
    { 0.172f, 1.41f, -0.003f }, { 0.052f, 0.060f, 0.055f },
    { 0.004f, -0.083f, 0.004f }, { 0.022f, 0.088f, 0.047f }, { 0.249f, 0.825f, 0.052f }, { 0.015f, 0.036f, 0.016f },
    { 0.092f, 0.93f, -0.005f }, { 0.103f, 0.50f, 0.012f }, { 0.112f, 0.09f, -0.025f },
    { { 0.0f, 0.090f, 0.095f }, { 0.25f, 0.085f, 0.090f }, { 0.75f, 0.066f, 0.070f }, { 1.0f, 0.053f, 0.056f },
      { 1.3f, 0.058f, 0.063f }, { 1.7f, 0.045f, 0.047f }, { 2.0f, 0.043f, 0.043f } },
    { 0.115f, 0.05f, 0.035f }, { 0.052f, 0.058f, 0.135f },
    { { 0.0f, 1.47f, -0.012f }, { 0.0f, 1.55f, -0.008f }, { 0.0f, 1.63f, 0.0f } }, 0.055f, 0.057f,
    { 0.0f, 1.684f, 0.012f }, { 0.080f, 0.117f, 0.100f }, 0.035f, 0.0f, false, HAIR, MOUTH,
    Lower::Jeans, 0.0f, 0.0f, 0.0f, 0.0f, { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } }, SHOE, false };
const HumanStyle WOMAN = {
    WOMAN_WIDTH, WOMAN_DEPTH, WOMAN_MIDDLE, sizeof(WOMAN_WIDTH)/sizeof(Key), 0.975f, 0.975f, 1.13f,
    { 0.157f, 1.31f, -0.005f }, { 0.200f, 1.035f, -0.018f }, { 0.222f, 0.80f, 0.014f },
    { { 0.0f, 0.042f, 0.045f }, { 0.4f, 0.039f, 0.042f }, { 1.0f, 0.031f, 0.034f }, { 1.3f, 0.034f, 0.036f },
      { 2.0f, 0.023f, 0.019f } },
    { 0.148f, 1.295f, -0.003f }, { 0.044f, 0.051f, 0.047f },
    { 0.004f, -0.075f, 0.004f }, { 0.019f, 0.078f, 0.041f }, { 0.217f, 0.755f, 0.046f }, { 0.013f, 0.032f, 0.014f },
    { 0.088f, 0.853f, -0.008f }, { 0.092f, 0.458f, 0.012f }, { 0.098f, 0.082f, -0.022f },
    { { 0.0f, 0.086f, 0.090f }, { 0.25f, 0.078f, 0.083f }, { 0.75f, 0.057f, 0.061f }, { 1.0f, 0.045f, 0.048f },
      { 1.3f, 0.049f, 0.054f }, { 1.7f, 0.036f, 0.039f }, { 2.0f, 0.029f, 0.031f } },
    { 0.100f, 0.04f, 0.030f }, { 0.042f, 0.046f, 0.115f },
    { { 0.0f, 1.35f, -0.012f }, { 0.0f, 1.42f, -0.008f }, { 0.0f, 1.49f, 0.0f } }, 0.045f, 0.047f,
    { 0.0f, 1.545f, 0.010f }, { 0.073f, 0.107f, 0.092f }, 0.06f, 0.30f, true, BLOND_HAIR, LIPS,
    Lower::Skirt, 0.0f, 0.975f, 0.865f, 0.50f, { { 0.140f, 0.098f }, { 0.200f, 0.138f }, { 0.218f, 0.152f } }, FLAT_SHOE,
    true };
const HumanStyle CHILD = {
    CHILD_WIDTH, CHILD_DEPTH, CHILD_MIDDLE, sizeof(CHILD_WIDTH)/sizeof(Key), 0.66f, 0.66f, 0.80f,
    { 0.118f, 0.875f, -0.004f }, { 0.143f, 0.69f, -0.012f }, { 0.158f, 0.53f, 0.010f },
    { { 0.0f, 0.034f, 0.036f }, { 0.4f, 0.031f, 0.033f }, { 1.0f, 0.026f, 0.028f }, { 1.3f, 0.028f, 0.029f },
      { 2.0f, 0.019f, 0.017f } },
    { 0.110f, 0.862f, -0.002f }, { 0.035f, 0.040f, 0.037f },
    { 0.003f, -0.055f, 0.003f }, { 0.015f, 0.058f, 0.032f }, { 0.154f, 0.497f, 0.034f }, { 0.010f, 0.024f, 0.011f },
    { 0.060f, 0.60f, -0.005f }, { 0.065f, 0.32f, 0.008f }, { 0.070f, 0.06f, -0.015f },
    { { 0.0f, 0.060f, 0.063f }, { 0.25f, 0.056f, 0.060f }, { 0.75f, 0.044f, 0.047f }, { 1.0f, 0.036f, 0.038f },
      { 1.3f, 0.039f, 0.042f }, { 1.7f, 0.030f, 0.032f }, { 2.0f, 0.028f, 0.028f } },
    { 0.073f, 0.035f, 0.025f }, { 0.036f, 0.040f, 0.090f },
    { { 0.0f, 0.92f, -0.008f }, { 0.0f, 0.965f, -0.005f }, { 0.0f, 1.01f, 0.0f } }, 0.036f, 0.038f,
    { 0.0f, 1.095f, 0.008f }, { 0.074f, 0.100f, 0.088f }, 0.04f, 0.05f, false, HAIR, MOUTH,
    Lower::Shorts, 0.39f, 0.0f, 0.0f, 0.0f, { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } }, SNEAKER, false };

/*!
 * \brief addGrid
 * adds the quads of rows of closed rings (rows x around positions), with the given normals or smooth normals from the
 * neighbors (pointing away from the middle of the rings), the color of a quad is from its middle
 */
void addGrid(Builder &builder, const std::vector<Vec> &positions, const std::vector<Vec> *givenNormals, int rows,
             int around, const ColorFunction &color)
{
    std::vector<Vec> normals;
    if (givenNormals) {
        normals = *givenNormals;
    } else {
        normals.resize(positions.size());
        for (int i=0;i<rows;i++) {
            for (int j=0;j<around;j++) {
                const Vec along = positions[std::min(i + 1, rows - 1)*around + j] - positions[std::max(i - 1, 0)*around + j];
                const Vec side = positions[i*around + (j + 1)%around] - positions[i*around + (j + around - 1)%around];
                normals[i*around + j] = normalized(cross(along, side));
            }
        }
        // the grid has the same orientation everywhere, the middle row decides the outward side
        const int m = rows/2;
        Vec middle = { 0.0f, 0.0f, 0.0f };
        for (int j=0;j<around;j++) {
            middle = middle + positions[m*around + j]*(1.0f/around);
        }
        if (dot(normals[m*around], positions[m*around] - middle) < 0.0f) {
            for (Vec &n : normals) {
                n = n*-1.0f;
            }
        }
    }
    for (int i=0;i + 1<rows;i++) {
        for (int j=0;j<around;j++) {
            const int j1 = (j + 1)%around;
            const int index[4] = { i*around + j, i*around + j1, (i + 1)*around + j1, (i + 1)*around + j };
            Vec p[4], n[4];
            Vec middle = { 0.0f, 0.0f, 0.0f };
            for (int k=0;k<4;k++) {
                p[k] = positions[index[k]];
                n[k] = normals[index[k]];
                middle = middle + p[k]*0.25f;
            }
            const float s0 = static_cast<float>(j)/around;
            const float s1 = static_cast<float>(j + 1)/around;
            const float t0 = static_cast<float>(i)/(rows - 1);
            const float t1 = static_cast<float>(i + 1)/(rows - 1);
            const float st[4][2] = { { s0, t0 }, { s1, t0 }, { s1, t1 }, { s0, t1 } };
            const float *c = color(middle);
            builder.quad(c == SHIRT, p, n, st, c);
        }
    }
}

// closes the ring of a grid with a flat fan from its middle
void addCap(Builder &builder, const std::vector<Vec> &positions, int row, int around, Vec normal, const float color[4])
{
    Vec middle = { 0.0f, 0.0f, 0.0f };
    for (int j=0;j<around;j++) {
        middle = middle + positions[row*around + j]*(1.0f/around);
    }
    for (int j=0;j<around;j++) {
        for (const Vec &p : { middle, positions[row*around + j], positions[row*around + (j + 1)%around] }) {
            builder.vertex(color == SHIRT, p, normal, 0.5f, 0.5f, color);
        }
    }
}

/*!
 * \brief addEllipsoid
 * ellipsoid of the center and the radii, deform can change a point of it (from its direction on the unit sphere),
 * the points below floor are moved up to it (a flat bottom)
 */
void addEllipsoid(Builder &builder, Vec center, Vec radius, const ColorFunction &color,
                  const std::function<Vec(Vec)> &deform = nullptr, float floor = -1.0e9f)
{
    const int rows = ELLIPSOID_RINGS + 1;
    std::vector<Vec> positions, normals;
    for (int i=0;i<rows;i++) {
        const float up = static_cast<float>(i)*PI_VALUE/ELLIPSOID_RINGS;
        for (int j=0;j<RING_SEGMENTS;j++) {
            const float a = static_cast<float>(j)*2.0f*PI_VALUE/RING_SEGMENTS;
            const Vec unit = { sinf(up)*sinf(a), -cosf(up), sinf(up)*cosf(a) };
            Vec local = { radius.x*unit.x, radius.y*unit.y, radius.z*unit.z };
            if (deform) {
                local = deform(unit);
            }
            Vec p = center + local;
            Vec n = normalized(Vec{ unit.x/radius.x, unit.y/radius.y, unit.z/radius.z });
            if (p.y < floor) {
                p.y = floor;
                n = { 0.0f, -1.0f, 0.0f };
            }
            positions.push_back(p);
            normals.push_back(n);
        }
    }
    addGrid(builder, positions, &normals, rows, RING_SEGMENTS, color);
}

struct TubePoint
{
    Vec center;
    float radiusX;  // across the tube along x (or along the side of the path)
    float radiusZ;  // across the tube toward the front
};

/*!
 * \brief addTube
 * tube with elliptic cross-sections along the smooth path through the points (and their radii), closed at both ends
 */
void addTube(Builder &builder, const std::vector<TubePoint> &points, const ColorFunction &color)
{
    // the path and the radii are smooth curves of the length along the straight lines between the points
    const int count = static_cast<int>(points.size());
    std::vector<float> lengths(count, 0.0f);
    for (int k=1;k<count;k++) {
        const Vec d = points[k].center - points[k - 1].center;
        lengths[k] = lengths[k - 1] + sqrtf(dot(d, d));
    }
    std::vector<Key> keys[5];
    for (int k=0;k<count;k++) {
        const float values[5] = { points[k].center.x, points[k].center.y, points[k].center.z, points[k].radiusX,
                                  points[k].radiusZ };
        for (int v=0;v<5;v++) {
            keys[v].push_back({ lengths[k], values[v] });
        }
    }
    auto at = [&](int v, float l) { return curve(keys[v].data(), count, l); };
    std::vector<Vec> positions;
    for (int i=0;i<LIMB_ROWS;i++) {
        const float l = lengths.back()*static_cast<float>(i)/(LIMB_ROWS - 1);
        const float dl = lengths.back()/(LIMB_ROWS - 1)*0.5f;
        const Vec center = { at(0, l), at(1, l), at(2, l) };
        const Vec tangent = normalized(Vec{ at(0, l + dl) - at(0, l - dl), at(1, l + dl) - at(1, l - dl),
                                            at(2, l + dl) - at(2, l - dl) });
        // u is across the path along x, v toward the front (+z) for a path along y
        Vec u = cross(Vec{ 0.0f, 0.0f, 1.0f }, tangent);
        u = dot(u, u) > 1.0e-6f ? normalized(u) : Vec{ 1.0f, 0.0f, 0.0f };
        const Vec v = normalized(cross(tangent, u));
        for (int j=0;j<RING_SEGMENTS;j++) {
            const float a = static_cast<float>(j)*2.0f*PI_VALUE/RING_SEGMENTS;
            positions.push_back(center + u*(at(3, l)*sinf(a)) + v*(at(4, l)*cosf(a)));
        }
    }
    addGrid(builder, positions, nullptr, LIMB_ROWS, RING_SEGMENTS, color);
    const Vec first = normalized(points[0].center - points[1].center);
    const Vec last = normalized(points[count - 1].center - points[count - 2].center);
    addCap(builder, positions, 0, RING_SEGMENTS, first, color(points[0].center));
    addCap(builder, positions, LIMB_ROWS - 1, RING_SEGMENTS, last, color(points[count - 1].center));
}

Vec lerp(Vec a, Vec b, float t)
{
    return a + (b - a)*t;
}

// a limb from 3 joints: rate 0..1 is between the upper and the middle joint, 1..2 between the middle and the lower one
std::vector<TubePoint> limb(Vec upper, Vec middle, Vec lower, const LimbPoint *points, int count, float side)
{
    std::vector<TubePoint> list;
    for (int k=0;k<count;k++) {
        const LimbPoint &p = points[k];
        Vec c = p.rate <= 1.0f ? lerp(upper, middle, p.rate) : lerp(middle, lower, p.rate - 1.0f);
        c.x *= side;
        list.push_back({ c, p.radiusX, p.radiusZ });
    }
    return list;
}

// a point relative to the head: x/y/z in its radii from its center
Vec onHead(const HumanStyle &style, float x, float y, float z)
{
    return style.head + Vec{ x*style.headRadius.x, y*style.headRadius.y, z*style.headRadius.z };
}

Vec headSize(const HumanStyle &style, float x, float y, float z)
{
    return { x*style.headRadius.x, y*style.headRadius.y, z*style.headRadius.z };
}

} // namespace

void buildHumanMesh(HumanType type, float sizeX, float sizeY, float sizeZ, std::vector<MeshVertex> &paint,
                    std::vector<MeshVertex> &detail)
{
    const HumanStyle &style = type == HumanType::Woman ? WOMAN : (type == HumanType::Child ? CHILD : MAN);
    std::vector<MeshVertex> listPaint, listDetail;
    Builder builder(listPaint, listDetail);
    auto solid = [](const float *color) { return [color](Vec) { return color; }; };
    const Vec ex = { 1.0f, 0.0f, 0.0f };
    const Vec ey = { 0.0f, 1.0f, 0.0f };
    const Vec ez = { 0.0f, 0.0f, 1.0f };
    const float *lowerColor = style.lower == Lower::Jeans ? JEANS : (style.lower == Lower::Skirt ? SKIRT : SHORTS);

    // torso: shirt, belt and jeans, skirt or shorts
    {
        const int count = style.torsoCount;
        const float bottom = style.torsoWidth[0].z;
        const float top = style.torsoWidth[count - 1].z;
        std::vector<Vec> positions;
        for (int i=0;i<TORSO_ROWS;i++) {
            const float y = bottom + (top - bottom)*static_cast<float>(i)/(TORSO_ROWS - 1);
            const float halfWidth = curve(style.torsoWidth, count, y);
            const float halfDepth = curve(style.torsoDepth, count, y);
            const float middle = curve(style.torsoMiddle, count, y);
            for (int j=0;j<RING_SEGMENTS;j++) {
                const float a = static_cast<float>(j)*2.0f*PI_VALUE/RING_SEGMENTS;
                auto power = [](float c) {
                    return (c < 0.0f ? -1.0f : 1.0f)*powf(fabsf(c), 2.0f/TORSO_EXPONENT);
                };
                positions.push_back({ halfWidth*power(sinf(a)), y, middle + halfDepth*power(cosf(a)) });
            }
        }
        const ColorFunction clothes = [&style, lowerColor](Vec p) {
            return p.y > style.shirtBottom ? SHIRT : (p.y > style.beltBottom ? BELT : lowerColor);
        };
        addGrid(builder, positions, nullptr, TORSO_ROWS, RING_SEGMENTS, clothes);
        addCap(builder, positions, 0, RING_SEGMENTS, Vec{ 0.0f, -1.0f, 0.0f }, lowerColor);
        addCap(builder, positions, TORSO_ROWS - 1, RING_SEGMENTS, Vec{ 0.0f, 1.0f, 0.0f }, SKIN);
        const float beltMiddle = (style.shirtBottom + style.beltBottom)/2.0f;
        const float front = curve(style.torsoMiddle, count, beltMiddle) + curve(style.torsoDepth, count, beltMiddle);
        if (style.lower == Lower::Jeans) {
            // belt buckle at the front
            builder.box(false, Vec{ 0.0f, beltMiddle, front }, ex, ey, ez, Vec{ 0.024f, 0.014f, 0.006f }, BUCKLE);
        }
        if (style.bust) {
            // the bust is below the shoulders
            const float y = style.shoulder.y - 0.13f;
            const float z = curve(style.torsoMiddle, count, y) + curve(style.torsoDepth, count, y) - 0.018f;
            for (const float side : { -1.0f, 1.0f }) {
                addEllipsoid(builder, Vec{ side*0.042f, y, z }, Vec{ 0.060f, 0.056f, 0.036f }, solid(SHIRT));
            }
        }
    }
    if (style.lower == Lower::Skirt) {
        // skirt from the waist over the hips to the hem above the knees
        addTube(builder, { { Vec{ 0.0f, style.skirtTop, 0.0f }, style.skirtRadius[0][0], style.skirtRadius[0][1] },
                           { Vec{ 0.0f, style.skirtHips, -0.018f }, style.skirtRadius[1][0], style.skirtRadius[1][1] },
                           { Vec{ 0.0f, style.skirtHem, -0.008f }, style.skirtRadius[2][0], style.skirtRadius[2][1] } },
                solid(SKIRT));
    }
    const ColorFunction sleeve = [&style](Vec p) { return p.y > style.sleeveEnd ? SHIRT : SKIN; };
    const ColorFunction legColor = [&style](Vec p) {
        if (style.lower == Lower::Jeans) {
            return JEANS;
        }
        return style.lower == Lower::Shorts && p.y > style.shortsEnd ? SHORTS : SKIN;
    };
    for (const float side : { -1.0f, 1.0f }) {
        auto mirror = [side](Vec v) { return Vec{ side*v.x, v.y, v.z }; };
        // arm: from the shoulder down to the elbow (a little back) and the wrist (a little forward), the shoulder
        // muscle covers its top
        addTube(builder, limb(style.shoulder, style.elbow, style.wrist, style.arm, 5, side), sleeve);
        addEllipsoid(builder, mirror(style.deltoid), style.deltoidRadius, solid(SHIRT));
        // hand and thumb
        addEllipsoid(builder, mirror(style.wrist + style.hand), style.handRadius, solid(SKIN));
        addEllipsoid(builder, mirror(style.thumb), style.thumbRadius, solid(SKIN));
        // leg: from the hip down to the knee (a little forward) and the ankle
        addTube(builder, limb(style.hip, style.knee, style.ankle, style.leg, 7, side), legColor);
        // shoe with a flat sole on the ground
        addEllipsoid(builder, mirror(style.shoe), style.shoeRadius, solid(style.shoeColor), nullptr, 0.0f);
    }
    // neck and head: the jaw is narrower, the hair is a little out of the head above the hairline (higher at the
    // front, long hair is lower at the sides and the back)
    addTube(builder, { { style.neck[0], style.neckRadiusX*1.02f, style.neckRadiusZ*1.02f },
                       { style.neck[1], style.neckRadiusX, style.neckRadiusZ },
                       { style.neck[2], style.neckRadiusX, style.neckRadiusZ } }, solid(SKIN));
    const Vec head = style.head;
    const Vec headRadius = style.headRadius;
    auto hairline = [&style](Vec unit) {
        return unit.y - (0.45f - (0.35f + style.hairBack)*(1.0f - unit.z));
    };
    auto headPoint = [&](Vec unit) {
        const float jaw = 1.0f - 0.18f*powf(std::max(0.0f, -unit.y), 1.5f);
        const float h = std::min(std::max(hairline(unit)/0.06f + 0.5f, 0.0f), 1.0f);
        const float hair = 1.0f + style.hairVolume*h*h*(3.0f - 2.0f*h);
        return Vec{ headRadius.x*unit.x*jaw*hair, headRadius.y*unit.y*hair, headRadius.z*unit.z*hair };
    };
    const ColorFunction headColor = [&](Vec p) {
        const Vec unit = normalized(Vec{ (p.x - head.x)/headRadius.x, (p.y - head.y)/headRadius.y,
                                         (p.z - head.z)/headRadius.z });
        return hairline(unit) > 0.0f ? style.hairColor : SKIN;
    };
    addEllipsoid(builder, head, headRadius, headColor, headPoint);
    if (style.longHair) {
        // long hair down the back of the head to the shoulders
        addEllipsoid(builder, onHead(style, 0.0f, -0.55f, -0.42f), headSize(style, 1.0f, 0.95f, 0.62f),
                     solid(style.hairColor));
    }
    // face: nose, eyes, eyebrows, mouth and ears
    addEllipsoid(builder, onHead(style, 0.0f, -0.16f, 0.96f), headSize(style, 0.16f, 0.23f, 0.20f), solid(SKIN));
    for (const float side : { -1.0f, 1.0f }) {
        addEllipsoid(builder, onHead(style, side*0.39f, 0.094f, 0.90f), headSize(style, 0.14f, 0.06f, 0.07f),
                     solid(EYE));
        builder.box(false, onHead(style, side*0.41f, 0.256f, 0.88f), ex, ey, ez,
                    headSize(style, 0.20f, 0.03f, 0.05f), style.hairColor);
        addEllipsoid(builder, onHead(style, side*0.99f, -0.034f, -0.12f), headSize(style, 0.16f, 0.265f, 0.21f),
                     solid(SKIN));
    }
    builder.box(false, onHead(style, 0.0f, -0.53f, 0.85f), ex, ey, ez, headSize(style, 0.225f, 0.026f, 0.04f),
                style.mouthColor);

    // scales the person (in meters) into the box: every axis to the size of the box, the center of the box is 0/0/0
    float low[3] = { 1.0e9f, 1.0e9f, 1.0e9f };
    float high[3] = { -1.0e9f, -1.0e9f, -1.0e9f };
    for (const std::vector<MeshVertex> *list : { &listPaint, &listDetail }) {
        for (const MeshVertex &v : *list) {
            for (int k=0;k<3;k++) {
                low[k] = std::min(low[k], v.position[k]);
                high[k] = std::max(high[k], v.position[k]);
            }
        }
    }
    const float size[3] = { sizeX, sizeY, sizeZ };
    float scale[3];
    for (int k=0;k<3;k++) {
        scale[k] = size[k]/(high[k] - low[k]);
    }
    auto fit = [&](const std::vector<MeshVertex> &from, std::vector<MeshVertex> &to) {
        for (MeshVertex v : from) {
            for (int k=0;k<3;k++) {
                v.position[k] = (v.position[k] - (low[k] + high[k])/2.0f)*scale[k];
            }
            const Vec n = normalized(Vec{ v.normal[0]/scale[0], v.normal[1]/scale[1], v.normal[2]/scale[2] });
            v.normal[0] = n.x;
            v.normal[1] = n.y;
            v.normal[2] = n.z;
            to.push_back(v);
        }
    };
    fit(listPaint, paint);
    fit(listDetail, detail);
}
