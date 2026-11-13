#include "car_mesh.h"
#include "../math/math3d.h"

// Every car is scaled into its box. Heights are fractions of the box height (0 is the ground, 1 the top of the box),
// positions along the car are fractions of the length (-0.5 rear, 0.5 front) and widths fractions of the box width
// (the body is 2*bodyHalfWidth wide, the side mirrors reach the sides of the box).

// cross-sections of the body along the car and the segments of the rounded parts
#define CAR_STATIONS        300
#define CAR_ARC_SEGMENTS    5
#define WHEEL_SEGMENTS      48
#define WHEEL_SPOKES        5
// window edges: the side windows are from this fraction of the cabin height above the belt to the upper one
#define WINDOW_BOTTOM       0.06f
#define WINDOW_TOP          0.90f

using namespace mesh;

namespace {

// a box on the car: x/y are the middle (fractions of the width and the height), halfX/halfY its half sizes
struct Part
{
    float x, y, halfX, halfY;
};

struct Range
{
    float from, to;
};

struct CarStyle
{
    // top of the side of the body (bumper top, hood, the window sill and the trunk) and the top of the cabin
    // (rear window, roof and windshield, the cabin is where it is above the belt), from the rear to the front
    const Key *belt;
    int beltCount;
    const Key *roof;
    int roofCount;
    float bodyHalfWidth;
    float planRounding;     // the corners seen from above are rounded over this fraction of the length at both ends
    float planNarrowing;    // the ends are this much narrower than the middle
    float clearance;        // ground clearance
    float bumperRise;       // the bottom of the bumpers rises this much at the ends
    float crown;            // the hood, the roof and the trunk are this much higher in the middle than at their sides
    float cabinBase;        // half width of the bottom of the side windows, fraction of the half width of the body
    float cabinTop;         // half width of the top of the side windows
    float wheelRadius;      // fraction of the height, at most wheelRadiusMaxLength of the length
    float wheelRadiusMaxLength;
    float wheelWidth;       // fraction of the box width
    float rimRadius;        // fraction of the wheel radius
    float frontAxle;
    float rearAxle;
    float archScale;        // the wheel arch is this much larger than the wheel
    Range windshield;       // the windshield and the rear window along the car
    Range rearWindow;
    Range windowFront;      // front edge of the side windows at their bottom and their top
    Range windowRear;       // rear edge of the side windows at their bottom and their top
    Range pillars[2];       // dark pillars between the side windows
    int pillarCount;
    Part headlight;         // on the front end, both sides (x is the middle of the right one)
    Part taillight;         // on the rear end, both sides
    Part grille;            // on the front end, in the middle
    float frontPlate;       // height of the license plates, 0 = none
    float rearPlate;
    Part rearBumper;        // dark lower part of the rear bumper, in the middle
    float mirrorZ;          // side mirrors along the car and their height
    float mirrorY;
    float mirrorSize;       // half size of the mirror along the width, fraction of the width
    float handles[2];       // door handles along the car
    int handleCount;
    float handleY;
    bool roofRails;
    bool sideSkirts;
    bool sideIntakes;
    bool rearWing;
};

const Key SEDAN_BELT[] = {
    { -0.500f, 0.560f }, { -0.490f, 0.605f }, { -0.470f, 0.630f }, { -0.400f, 0.645f }, { -0.300f, 0.635f },
    { 0.000f, 0.620f }, { 0.150f, 0.600f }, { 0.300f, 0.565f }, { 0.440f, 0.530f }, { 0.480f, 0.510f },
    { 0.500f, 0.470f } };
const Key SEDAN_ROOF[] = {
    { -0.500f, 0.300f }, { -0.420f, 0.550f }, { -0.375f, 0.660f }, { -0.330f, 0.800f }, { -0.280f, 0.930f },
    { -0.220f, 0.985f }, { -0.100f, 1.000f }, { 0.000f, 0.990f }, { 0.040f, 0.950f }, { 0.090f, 0.830f },
    { 0.140f, 0.670f }, { 0.170f, 0.550f }, { 0.500f, 0.300f } };
const Key SUV_BELT[] = {
    { -0.500f, 0.580f }, { -0.490f, 0.610f }, { -0.470f, 0.620f }, { -0.300f, 0.615f }, { 0.000f, 0.600f },
    { 0.200f, 0.580f }, { 0.350f, 0.560f }, { 0.450f, 0.540f }, { 0.485f, 0.510f }, { 0.500f, 0.470f } };
const Key SUV_ROOF[] = {
    { -0.500f, 0.400f }, { -0.497f, 0.620f }, { -0.488f, 0.800f }, { -0.470f, 0.910f }, { -0.440f, 0.950f },
    { -0.300f, 0.960f }, { 0.000f, 0.960f }, { 0.060f, 0.935f }, { 0.120f, 0.840f }, { 0.180f, 0.700f },
    { 0.225f, 0.560f }, { 0.500f, 0.300f } };
const Key SPORT_BELT[] = {
    { -0.500f, 0.520f }, { -0.490f, 0.560f }, { -0.470f, 0.580f }, { -0.400f, 0.590f }, { -0.200f, 0.570f },
    { 0.000f, 0.530f }, { 0.100f, 0.500f }, { 0.250f, 0.450f }, { 0.400f, 0.400f }, { 0.470f, 0.360f },
    { 0.490f, 0.330f }, { 0.500f, 0.290f } };
const Key SPORT_ROOF[] = {
    { -0.500f, 0.300f }, { -0.460f, 0.550f }, { -0.420f, 0.620f }, { -0.360f, 0.720f }, { -0.280f, 0.850f },
    { -0.200f, 0.960f }, { -0.120f, 1.000f }, { -0.060f, 0.980f }, { 0.000f, 0.890f }, { 0.060f, 0.740f },
    { 0.110f, 0.550f }, { 0.500f, 0.300f } };

const CarStyle SEDAN = {
    SEDAN_BELT, sizeof(SEDAN_BELT)/sizeof(Key), SEDAN_ROOF, sizeof(SEDAN_ROOF)/sizeof(Key),
    0.45f, 0.07f, 0.22f, 0.10f, 0.10f, 0.02f, 0.86f, 0.72f,
    0.215f, 0.07f, 0.115f, 0.645f, 0.315f, -0.29f, 1.12f,
    { 0.008f, 0.15f }, { -0.37f, -0.24f }, { 0.125f, 0.015f }, { -0.35f, -0.25f },
    { { -0.085f, -0.06f }, { 0.0f, 0.0f } }, 1,
    { 0.265f, 0.425f, 0.075f, 0.026f }, { 0.265f, 0.505f, 0.08f, 0.03f }, { 0.0f, 0.325f, 0.16f, 0.045f },
    0.24f, 0.33f, { 0.0f, 0.225f, 0.30f, 0.022f },
    0.125f, 0.66f, 0.075f, { 0.03f, -0.17f }, 2, 0.56f,
    false, false, false, false };
const CarStyle SUV = {
    SUV_BELT, sizeof(SUV_BELT)/sizeof(Key), SUV_ROOF, sizeof(SUV_ROOF)/sizeof(Key),
    0.45f, 0.06f, 0.15f, 0.13f, 0.07f, 0.015f, 0.89f, 0.80f,
    0.21f, 0.08f, 0.125f, 0.62f, 0.33f, -0.31f, 1.14f,
    { 0.065f, 0.21f }, { -0.497f, -0.462f }, { 0.20f, 0.075f }, { -0.462f, -0.44f },
    { { -0.05f, -0.025f }, { -0.315f, -0.285f } }, 2,
    { 0.27f, 0.43f, 0.075f, 0.03f }, { 0.31f, 0.53f, 0.045f, 0.055f }, { 0.0f, 0.36f, 0.16f, 0.04f },
    0.215f, 0.33f, { 0.0f, 0.215f, 0.33f, 0.03f },
    0.19f, 0.64f, 0.07f, { 0.06f, -0.17f }, 2, 0.55f,
    true, true, false, false };
const CarStyle SPORT = {
    SPORT_BELT, sizeof(SPORT_BELT)/sizeof(Key), SPORT_ROOF, sizeof(SPORT_ROOF)/sizeof(Key),
    0.46f, 0.08f, 0.25f, 0.075f, 0.07f, 0.025f, 0.80f, 0.62f,
    0.27f, 0.075f, 0.13f, 0.74f, 0.33f, -0.30f, 1.08f,
    { -0.075f, 0.105f }, { -0.40f, -0.215f }, { 0.095f, -0.035f }, { -0.30f, -0.17f },
    { { 0.0f, 0.0f }, { 0.0f, 0.0f } }, 0,
    { 0.29f, 0.255f, 0.075f, 0.018f }, { 0.27f, 0.47f, 0.13f, 0.02f }, { 0.0f, 0.18f, 0.22f, 0.04f },
    0.0f, 0.33f, { 0.0f, 0.19f, 0.30f, 0.03f },
    0.075f, 0.57f, 0.06f, { -0.07f, 0.0f }, 1, 0.49f,
    false, false, true, true };

const float PAINT[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const float GLASS[4] = { 0.05f, 0.07f, 0.09f, 1.0f };
const float BLACK[4] = { 0.03f, 0.03f, 0.03f, 1.0f };
const float TRIM[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
const float TIRE[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
const float RIM[4] = { 0.75f, 0.76f, 0.78f, 1.0f };
const float BRAKE[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
const float HEADLIGHT[4] = { 0.95f, 0.95f, 0.90f, 1.0f };
const float TAILLIGHT[4] = { 0.75f, 0.04f, 0.03f, 1.0f };
const float PLATE[4] = { 0.90f, 0.90f, 0.86f, 1.0f };

// one point of a cross-section of the body (right half, x >= 0) in the units of the box
struct SectionPoint
{
    float x, y;
};

// indices of the right half of a cross-section: the lower body is up to SECTION_CABIN_BASE, the side windows from
// there to SECTION_ROOF_EDGE and the top (hood, roof, trunk, windshield and rear window) from there to the center
const int SECTION_CABIN_BASE = 2*CAR_ARC_SEGMENTS + 3;
const int SECTION_ROOF_EDGE = SECTION_CABIN_BASE + 3;
const int SECTION_POINTS = SECTION_ROOF_EDGE + 3;

struct Station
{
    float zFraction;    // -0.5 rear .. 0.5 front
    float z;
    float halfWidth;
    float bottom, belt, cabin;  // heights in the units of the box from the ground
    SectionPoint points[SECTION_POINTS];
};

} // namespace

void buildCarMesh(CarType type, float sizeX, float sizeY, float sizeZ, std::vector<MeshVertex> &paint,
                  std::vector<MeshVertex> &detail)
{
    const CarStyle &style = type == CarType::Suv ? SUV : (type == CarType::Sport ? SPORT : SEDAN);
    Builder builder(paint, detail);
    const float width = sizeX;
    const float height = sizeY;
    const float length = sizeZ;
    const float ground = -height/2.0f;
    const float wheelRadius = std::min(style.wheelRadius*height, style.wheelRadiusMaxLength*length);
    const float wheelWidth = style.wheelWidth*width;
    const float archRadius = wheelRadius*style.archScale;
    const float axles[2] = { style.frontAxle*length, style.rearAxle*length };
    const float crown = style.crown*height;

    // cross-sections of the body
    std::vector<Station> stations(CAR_STATIONS + 1);
    for (int i=0;i<=CAR_STATIONS;i++) {
        Station &station = stations[i];
        const float zf = -0.5f + static_cast<float>(i)/CAR_STATIONS;
        station.zFraction = zf;
        station.z = zf*length;
        // the corners seen from above and the bumpers are rounded at the ends
        const float end = std::max(0.0f, (fabsf(zf) - (0.5f - style.planRounding))/style.planRounding);
        const float rounding = 1.0f - sqrtf(std::max(0.0f, 1.0f - end*end));
        const float halfWidth = style.bodyHalfWidth*width*(1.0f - style.planNarrowing*rounding);
        float bottom = (style.clearance + style.bumperRise*end*end)*height;
        for (const float axle : axles) {
            const float dz = station.z - axle;
            if (fabsf(dz) < archRadius) {
                bottom = std::max(bottom, wheelRadius + sqrtf(archRadius*archRadius - dz*dz));
            }
        }
        const float belt = std::max(curve(style.belt, style.beltCount, zf)*height, bottom + 0.04f*height);
        // the middle of the roof (with its crown) is at the roof curve, so the car stays in its box
        const float cabin = std::max(0.0f, curve(style.roof, style.roofCount, zf)*height - crown - belt);
        station.halfWidth = halfWidth;
        station.bottom = bottom;
        station.belt = belt;
        station.cabin = cabin;

        const float lowerRadius = std::min(0.05f*height, 0.3f*(belt - bottom));
        const float shoulderRadius = std::min(0.06f*height, 0.3f*(belt - bottom));
        const float cabinBase = std::min(halfWidth - shoulderRadius, style.cabinBase*halfWidth);
        const float cabinTop = style.cabinTop*halfWidth;
        SectionPoint *p = station.points;
        int k = 0;
        p[k++] = { 0.0f, bottom };
        for (int a=0;a<=CAR_ARC_SEGMENTS;a++) {
            const float angle = -PI_VALUE/2.0f + static_cast<float>(a)*PI_VALUE/2.0f/CAR_ARC_SEGMENTS;
            p[k++] = { halfWidth - lowerRadius + lowerRadius*cosf(angle), bottom + lowerRadius + lowerRadius*sinf(angle) };
        }
        for (int a=0;a<=CAR_ARC_SEGMENTS;a++) {
            const float angle = static_cast<float>(a)*PI_VALUE/2.0f/CAR_ARC_SEGMENTS;
            p[k++] = { halfWidth - shoulderRadius + shoulderRadius*cosf(angle),
                       belt - shoulderRadius + shoulderRadius*sinf(angle) };
        }
        // the cabin base, the side window (a little bulged) and the edge of the roof
        p[k++] = { cabinBase, belt };
        p[k++] = { (cabinBase + cabinTop)/2.0f + 0.01f*halfWidth*(cabin > 0.0f ? 1.0f : 0.0f), belt + 0.45f*cabin };
        p[k++] = { cabinTop, belt + 0.88f*cabin };
        p[k++] = { 0.93f*cabinTop, belt + cabin + 0.3f*crown };
        // the hood, the roof or the trunk to the middle
        p[k++] = { 0.55f*cabinTop, belt + cabin + 0.8f*crown };
        p[k++] = { 0.0f, belt + cabin + crown };
        // k == SECTION_POINTS
    }

    // the full ring of a cross-section: the right half from the bottom to the top, the left half back down
    const int ring = 2*SECTION_POINTS - 2;
    auto halfIndex = [ring](int j) { return j < SECTION_POINTS ? j : ring - j; };
    std::vector<Vec> positions(static_cast<size_t>((CAR_STATIONS + 1)*ring));
    std::vector<Vec> normals(positions.size());
    for (int i=0;i<=CAR_STATIONS;i++) {
        for (int j=0;j<ring;j++) {
            const SectionPoint &p = stations[i].points[halfIndex(j)];
            positions[i*ring + j] = { j < SECTION_POINTS ? p.x : -p.x, ground + p.y, stations[i].z };
        }
    }
    // smooth normals of the body from the neighbors along the car and around the section
    auto surfaceNormal = [&](int i, int j) {
        const Vec along = positions[std::min(i + 1, CAR_STATIONS)*ring + j] - positions[std::max(i - 1, 0)*ring + j];
        const Vec around = positions[i*ring + (j + 1)%ring] - positions[i*ring + (j + ring - 1)%ring];
        return normalized(cross(along, around));
    };
    // the grid has the same orientation everywhere, the side of the middle of the car points outward (+x)
    const float outward = surfaceNormal(CAR_STATIONS/2, CAR_ARC_SEGMENTS + 2).x < 0.0f ? -1.0f : 1.0f;
    for (int i=0;i<=CAR_STATIONS;i++) {
        for (int j=0;j<ring;j++) {
            normals[i*ring + j] = surfaceNormal(i, j)*outward;
        }
    }
    auto inside = [](const Range &range, float z) { return z > range.from && z < range.to; };
    // color of a quad of the body from its middle: side windows (with the pillars), windshield, rear window or paint
    auto quadColor = [&](int i, int halfLow, int halfHigh, float y) -> const float * {
        const float zf = (stations[i].zFraction + stations[i + 1].zFraction)/2.0f;
        const float belt = (stations[i].belt + stations[i + 1].belt)/2.0f;
        const float cabin = (stations[i].cabin + stations[i + 1].cabin)/2.0f;
        if (cabin < 0.03f*height) {
            return PAINT;
        }
        if (halfLow >= SECTION_CABIN_BASE && halfHigh <= SECTION_ROOF_EDGE) {
            const float up = (y - ground - belt)/cabin;
            const float rate = (up - WINDOW_BOTTOM)/(WINDOW_TOP - WINDOW_BOTTOM);
            const float front = style.windowFront.from + rate*(style.windowFront.to - style.windowFront.from);
            const float rear = style.windowRear.from + rate*(style.windowRear.to - style.windowRear.from);
            if (up < WINDOW_BOTTOM || up > WINDOW_TOP || zf > front || zf < rear) {
                return PAINT;
            }
            for (int k=0;k<style.pillarCount;k++) {
                if (inside(style.pillars[k], zf)) {
                    return BLACK;
                }
            }
            return GLASS;
        }
        if (halfLow >= SECTION_ROOF_EDGE && (inside(style.windshield, zf) || inside(style.rearWindow, zf))) {
            return GLASS;
        }
        return PAINT;
    };
    for (int i=0;i<CAR_STATIONS;i++) {
        for (int j=0;j<ring;j++) {
            const int j1 = (j + 1)%ring;
            const int index[4] = { i*ring + j, i*ring + j1, (i + 1)*ring + j1, (i + 1)*ring + j };
            Vec p[4], n[4];
            float y = 0.0f;
            for (int k=0;k<4;k++) {
                p[k] = positions[index[k]];
                n[k] = normals[index[k]];
                y += p[k].y/4.0f;
            }
            const int h0 = halfIndex(j);
            const int h1 = halfIndex(j1);
            const float *color = quadColor(i, std::min(h0, h1), std::max(h0, h1), y);
            const float s0 = static_cast<float>(j)/ring;
            const float s1 = static_cast<float>(j + 1)/ring;
            const float t0 = static_cast<float>(i)/CAR_STATIONS;
            const float t1 = static_cast<float>(i + 1)/CAR_STATIONS;
            const float st[4][2] = { { s0, t0 }, { s1, t0 }, { s1, t1 }, { s0, t1 } };
            builder.quad(color == PAINT, p, n, st, color);
        }
    }
    // the flat ends of the body (the bumpers), fans from the middle of the section
    for (const int i : { 0, CAR_STATIONS }) {
        const Station &station = stations[i];
        const Vec n = { 0.0f, 0.0f, i == 0 ? -1.0f : 1.0f };
        Vec center = { 0.0f, 0.0f, station.z };
        for (int j=0;j<ring;j++) {
            center.y += positions[i*ring + j].y/ring;
        }
        const float halfWidth = style.bodyHalfWidth*width;
        for (int j=0;j<ring;j++) {
            const Vec p[3] = { center, positions[i*ring + j], positions[i*ring + (j + 1)%ring] };
            for (const Vec &q : p) {
                builder.vertex(true, q, n, 0.5f + q.x/(2.0f*halfWidth), (q.y - ground)/height, PAINT);
            }
        }
    }

    const Vec ex = { 1.0f, 0.0f, 0.0f };
    const Vec ey = { 0.0f, 1.0f, 0.0f };
    const Vec ez = { 0.0f, 0.0f, 1.0f };
    const float front = length/2.0f;
    const float rear = -length/2.0f;
    const float out = 0.004f*length;
    // the station of a position along the car
    auto stationAt = [&](float zf) -> const Station & {
        const int i = static_cast<int>(roundf((zf + 0.5f)*CAR_STATIONS));
        return stations[std::min(std::max(i, 0), CAR_STATIONS)];
    };
    for (const float side : { -1.0f, 1.0f }) {
        // wheels: the tire, the rim, the dark disc behind the spokes, the spokes and the hub turned around the axle,
        // a profile point is the radius and the distance out from the middle of the wheel (+ away from the car)
        struct ProfilePoint
        {
            float radius, out;
        };
        const float R = wheelRadius;
        const float w = wheelWidth;
        const float rim = style.rimRadius*R;
        const ProfilePoint tire[] = {
            { rim, 0.42f*w }, { rim + 0.45f*(R - rim), 0.50f*w }, { 0.95f*R, 0.47f*w }, { R, 0.38f*w },
            { R, -0.38f*w }, { 0.95f*R, -0.47f*w }, { rim + 0.45f*(R - rim), -0.50f*w }, { rim, -0.42f*w },
            { 0.0f, -0.42f*w } };
        const ProfilePoint rimProfile[] = { { 0.87f*rim, 0.20f*w }, { 0.93f*rim, 0.36f*w }, { rim, 0.42f*w },
                                            { 1.02f*rim, 0.40f*w } };
        const ProfilePoint brake[] = { { 0.0f, 0.20f*w }, { 0.87f*rim, 0.20f*w } };
        const ProfilePoint hub[] = { { 0.0f, 0.38f*w }, { 0.25f*rim, 0.38f*w }, { 0.28f*rim, 0.30f*w },
                                     { 0.28f*rim, 0.20f*w } };
        const float wheelX = side*(style.bodyHalfWidth*width - w/2.0f - 0.005f*width);
        for (const float axle : axles) {
            const Vec center = { wheelX, ground + R, axle };
            // the normal of a profile segment from p0 to p1 is (radius -dOut, out +dRadius), so the profiles go
            // from the inside to the outside of the wheel on the visible side
            auto lathe = [&](const ProfilePoint *profile, int count, const float color[4]) {
                for (int k=0;k + 1<count;k++) {
                    const ProfilePoint &p0 = profile[k];
                    const ProfilePoint &p1 = profile[k + 1];
                    const Vec n2 = normalized(Vec{ -(p1.out - p0.out), p1.radius - p0.radius, 0.0f });
                    auto point = [&](const ProfilePoint &p, float a) {
                        return center + Vec{ side*p.out, p.radius*cosf(a), p.radius*sinf(a) };
                    };
                    auto normal = [&](float a) { return Vec{ side*n2.y, n2.x*cosf(a), n2.x*sinf(a) }; };
                    for (int s=0;s<WHEEL_SEGMENTS;s++) {
                        const float a0 = static_cast<float>(s)*2.0f*PI_VALUE/WHEEL_SEGMENTS;
                        const float a1 = static_cast<float>(s + 1)*2.0f*PI_VALUE/WHEEL_SEGMENTS;
                        const Vec p[4] = { point(p0, a0), point(p0, a1), point(p1, a1), point(p1, a0) };
                        const Vec n[4] = { normal(a0), normal(a1), normal(a1), normal(a0) };
                        const float st[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
                        builder.quad(false, p, n, st, color);
                    }
                }
            };
            lathe(tire, sizeof(tire)/sizeof(tire[0]), TIRE);
            lathe(rimProfile, sizeof(rimProfile)/sizeof(rimProfile[0]), RIM);
            lathe(brake, sizeof(brake)/sizeof(brake[0]), BRAKE);
            lathe(hub, sizeof(hub)/sizeof(hub[0]), RIM);
            for (int s=0;s<WHEEL_SPOKES;s++) {
                const float a = static_cast<float>(s)*2.0f*PI_VALUE/WHEEL_SPOKES;
                const Vec radial = { 0.0f, cosf(a), sinf(a) };
                const Vec tangent = { 0.0f, -sinf(a), cosf(a) };
                const Vec spoke = center + radial*(0.575f*rim) + ex*(side*0.28f*w);
                builder.box(false, spoke, radial, tangent, ex, Vec{ 0.33f*rim, 0.075f*rim, 0.08f*w }, RIM);
            }
        }

        // lights on the flat ends, they come out of the ends a little
        const float lightDepth = 0.03f*length;
        const Part &head = style.headlight;
        const Part &tail = style.taillight;
        builder.box(false, Vec{ side*head.x*width, ground + head.y*height, front - lightDepth/2.0f + out },
                    ex, ey, ez, Vec{ head.halfX*width, head.halfY*height, lightDepth/2.0f }, HEADLIGHT);
        builder.box(false, Vec{ side*tail.x*width, ground + tail.y*height, rear + lightDepth/2.0f - out },
                    ex, ey, ez, Vec{ tail.halfX*width, tail.halfY*height, lightDepth/2.0f }, TAILLIGHT);
        // side mirror (paint) at the bottom of the windshield, its outer end at the side of the box, door handles
        const float mirrorX = 0.495f*width - style.mirrorSize*width;
        builder.box(true, Vec{ side*mirrorX, ground + style.mirrorY*height, style.mirrorZ*length }, ex, ey, ez,
                    Vec{ style.mirrorSize*width, 0.035f*height, 0.025f*length }, PAINT);
        for (int k=0;k<style.handleCount;k++) {
            const float handleX = stationAt(style.handles[k]).halfWidth;
            builder.box(false, Vec{ side*handleX, ground + style.handleY*height, style.handles[k]*length }, ex, ey, ez,
                        Vec{ 0.006f*width, 0.01f*height, 0.025f*length }, BLACK);
        }
        if (style.roofRails) {
            // a rail on 3 feet on both sides of the roof, its top stays inside the box
            const float railX = side*0.29f*width;
            const float railY = 0.985f*height;
            builder.box(false, Vec{ railX, ground + railY, -0.20f*length }, ex, ey, ez,
                        Vec{ 0.012f*width, 0.01f*height, 0.21f*length }, TRIM);
            for (const float zf : { -0.40f, -0.20f, 0.0f }) {
                const float roof = curve(style.roof, style.roofCount, zf)*height - 2.0f*crown;
                builder.box(false, Vec{ railX, ground + (roof + railY)/2.0f, zf*length }, ex, ey, ez,
                            Vec{ 0.012f*width, (railY - roof)/2.0f, 0.012f*length }, TRIM);
            }
        }
        if (style.sideSkirts) {
            // dark plastic along the bottom of the sides between the wheel arches
            const float from = axles[1] + archRadius;
            const float to = axles[0] - archRadius;
            const float skirtX = stationAt(0.0f).halfWidth;
            builder.box(false, Vec{ side*skirtX, ground + (style.clearance + 0.05f)*height, (from + to)/2.0f }, ex, ey,
                        ez, Vec{ 0.008f*width, 0.05f*height, (to - from)/2.0f }, TRIM);
        }
        if (style.sideIntakes) {
            // dark air intakes in front of the rear wheels
            const float zf = (axles[1] + archRadius)/length + 0.06f;
            const float intakeX = stationAt(zf).halfWidth;
            builder.box(false, Vec{ side*intakeX, ground + 0.33f*height, zf*length }, ex, ey, ez,
                        Vec{ 0.006f*width, 0.06f*height, 0.05f*length }, BLACK);
        }
    }
    const float depth = 0.02f*length;
    const Part &grille = style.grille;
    builder.box(false, Vec{ 0.0f, ground + grille.y*height, front - depth/2.0f + out }, ex, ey, ez,
                Vec{ grille.halfX*width, grille.halfY*height, depth/2.0f }, BLACK);
    if (style.frontPlate > 0.0f) {
        builder.box(false, Vec{ 0.0f, ground + style.frontPlate*height, front - depth/2.0f + 1.5f*out }, ex, ey, ez,
                    Vec{ 0.145f*width, 0.038f*height, depth/2.0f }, PLATE);
    }
    builder.box(false, Vec{ 0.0f, ground + style.rearPlate*height, rear + depth/2.0f - 1.5f*out }, ex, ey, ez,
                Vec{ 0.145f*width, 0.038f*height, depth/2.0f }, PLATE);
    const Part &bumper = style.rearBumper;
    builder.box(false, Vec{ 0.0f, ground + bumper.y*height, rear + depth/2.0f - out }, ex, ey, ez,
                Vec{ bumper.halfX*width, bumper.halfY*height, depth/2.0f }, BLACK);
    if (style.rearWing) {
        // wing on 2 stands at the end of the trunk
        const float zf = -0.465f;
        const float deck = stationAt(zf).belt + crown;
        const float wingY = deck + 0.10f*height;
        builder.box(false, Vec{ 0.0f, ground + wingY, zf*length }, ex, ey, ez,
                    Vec{ 0.40f*width, 0.012f*height, 0.035f*length }, BLACK);
        for (const float side : { -1.0f, 1.0f }) {
            builder.box(false, Vec{ side*0.30f*width, ground + (deck + wingY)/2.0f - 0.01f*height, zf*length }, ex, ey,
                        ez, Vec{ 0.008f*width, (wingY - deck)/2.0f + 0.01f*height, 0.02f*length }, BLACK);
        }
    }
}
