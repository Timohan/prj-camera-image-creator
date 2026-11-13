#!/usr/bin/env python3
"""Generates objects<first>.dat ... objects<last>.dat into <cameraImageOutputDirectory>/camera_objects
(cameraImageOutputDirectory of configs/general.ini) or into the directory of --objectsDirectory:
usage genobjects.py <first> <last> [--objectsDirectory=<directory>]
Every file has:
- 0-60 people (Man, Woman or Child, each with the chance 1/3, 10 cm smaller ... 10 cm bigger than the normal
  height, a random shirt) and 0-30 cars (Car, Suv or SportCar, each with the chance 1/3, 5 % smaller ... 5 % bigger,
  a random paint), standing on the floor with a random heading, 1.5-30 m from 0/0/0. No other object touches a
  person or a car: people and cars are at least 1 cm from each other.
- the 36 cubes of objects0.dat 3 times (108 objects) placed randomly 1.5-30 m from 0/0/0, every one as a cube, a ball
  (the ellipsoid of the same size), a cylinder or a cone (of the same size, their axis is the y axis of the cube),
  each with the chance 1/4, with a random yaw, pitch and roll, at least 10 cm from the cars and 1 cm from the people.
- 1-3 random spot lights and a random ambient light.
The distances to the people and the cars are taken from their boxes (and the boxes of the cubes, balls, cylinders
and cones), so the real objects are at least this far.
If objects0.dat is missing, it is created first: 9 random cubes in each of the 4 corners (-x/-z, +x/-z, -x/+z, +x/+z),
1-5 random spot lights and random ambient light."""
import math, random, re, sys

import os

# repository root, this script is in the tools folder
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EYE = (0.0, 1000.0, 0.0)            # 0/1/0 in opengl units, mm
MIN_CAMERA_DISTANCE = 1000.0        # mm, from the cubes to cameraObjectPosition<number>
MIN_AMBIENT, MAX_AMBIENT = 0.1, 0.5
MIN_CUBE_DISTANCE, MAX_CUBE_DISTANCE = 1500.0, 30000.0
BASE_MAX_DISTANCE = 15000.0         # cubes of objects0.dat (their sizes and textures are used)
CUBE_GAP = 300.0                    # mm between the bounding spheres of cubes
MIN_VISIBLE = 0.3                   # part of the faces seen from the eye that must be seen
LIGHT_MIN_CUBES = 5
LIGHT_INNER, LIGHT_OUTER = 25.0, 35.0
LIGHT_HEIGHT = (3000.0, 8000.0)
LIGHT_AREA = 15000.0
NUDGE = 2.0                         # mm, sample points are this far outside of the face
SAMPLE_GRID = 7                     # points per side on every face
SINK_RATE = 0.25                    # cube can sink at most this part of its height under the floor
SHAPES = ['Cube', 'Ball', 'Cylinder', 'Cone']   # an object of objects<number>.dat is each of these with the same chance
BALL_SAMPLE_SEGMENTS = 24           # sample points around the ball, the cylinder and the cone
BALL_SAMPLE_RINGS = 12              # sample points from the bottom to the top of the ball and the side of the cylinder
                                    # and the cone
# objects0.dat when it is missing: cubes in every corner, cube side lengths in mm, textures of the images folder
BASE_CUBES_PER_CORNER = 9
BASE_CUBE_SIZE = (300.0, 1500.0)
BASE_TEXTURES = ['wood1.png', 'wood2.png', 'wood3.png', 'wood4.png', 'wood5.png', 'wood6.png', 'brick1.png', 'brick2.png']
BASE_LIGHTS = (1, 5)
BASE_SEED = 'objects0'              # objects0.dat is always the same
# people and cars: the count, the types with their normal box sizes (mm), the size changes and the textures
PEOPLE = (0, 60)
CARS = (0, 30)
BASE_COPIES = 3                     # every cube of objects0.dat is placed this many times (108 objects)
HUMAN_TYPES = [('Man', (560.0, 1800.0, 310.0)), ('Woman', (490.0, 1650.0, 300.0)), ('Child', (360.0, 1200.0, 215.0))]
HUMAN_HEIGHT_CHANGE = 100.0         # mm, the height is the normal height +- this, all sizes are scaled the same
SHIRTS = ['shirt_blue.png', 'shirt_red.png', 'shirt_white.png', 'shirt_green.png', 'shirt_gray.png']
CAR_TYPES = [('Car', (2000.0, 1450.0, 4600.0)), ('Suv', (2100.0, 1750.0, 4700.0)),
             ('SportCar', (2050.0, 1250.0, 4400.0))]
CAR_SIZE_CHANGE = 0.05              # the sizes are the normal sizes scaled 1 +- this
PAINTS = ['car_red.png', 'car_blue.png', 'car_silver.png', 'car_white.png', 'car_black.png']
CAR_GAP = 100.0                     # mm, at least from a car to the cubes, balls, cylinders and cones
PERSON_GAP = 10.0                   # mm, at least from a person to them and between the people and the cars

HEADER = """# Position of cube (x/y/z/SizeX/SizeY/SizeZ/Yaw/Pitch/Roll/wood.png)
# in Cube, center position=x,y,z, size=sizeX,sizeY,SizeZ values mm, 2000=2.0f, Yaw/Pitch/Roll angle in degrees, and finally texture image as text string in images folder
# Ball has the same values, it is the ellipsoid inside the cube of the same values (sizeX/sizeY/sizeZ are its diameters)
# Cylinder has the same values, it is inside the cube of the same values, its axis is the y axis of the cube (sizeY
# is its height) and its ends are the ellipses of the diameters sizeX and sizeZ
# Cone has the same values, it is inside the cube of the same values, its base is the ellipse of the diameters sizeX
# and sizeZ at the bottom of the cube (-y) and its tip at the middle of the top of the cube (+y)
# Car (sedan), Suv and SportCar have the same values, the car fits into the cube of the same values (sizeX is its
# width with the side mirrors, sizeZ its length), its front is +z of the cube and its wheels are on the floor
# Man, Woman and Child have the same values, the standing person fills the cube of the same values (sizeX is the width
# from hand to hand, sizeY the height), the person looks toward +z of the cube and stands on the floor
"""
LIGHT_HEADER = """
# Spot light (x/y/z/directionX/directionY/directionZ/InnerAngle/OuterAngle/R/G/B)
# position values in mm, 3000=3.0f, direction is where the light points to (any length),
# InnerAngle/OuterAngle in degrees from the direction: full light inside InnerAngle, no light outside OuterAngle,
# R/G/B color of the light (1.0/1.0/1.0 = white), there can be many SpotLight lines (max 8 are used)
"""
AMBIENT_HEADER = """
# Ambient light R/G/B, it lights everything evenly (optional, default 0.25/0.25/0.25)
"""


def dot(a, b):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]


def sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])


class Box:
    """Cube as in Cube::init(): rotates roll first, then pitch and yaw."""
    def __init__(self, center, size, yaw, pitch, roll):
        self.c = center
        self.h = [v/2.0 for v in size]
        yw, p, r = map(math.radians, (yaw, pitch, roll))

        def apply(v):
            x, y, z = v
            rx = x*math.cos(r) + y*math.sin(r)
            ry = -x*math.sin(r) + y*math.cos(r)
            py = ry*math.cos(p) + z*math.sin(p)
            pz = -ry*math.sin(p) + z*math.cos(p)
            return (rx*math.cos(yw) + pz*math.sin(yw), py, -rx*math.sin(yw) + pz*math.cos(yw))
        self.axes = [apply((1, 0, 0)), apply((0, 1, 0)), apply((0, 0, 1))]
        self.radius = math.sqrt(sum(v*v for v in self.h))

    def to_local(self, p):
        d = sub(p, self.c)
        return tuple(dot(d, a) for a in self.axes)

    def to_world(self, l):
        return tuple(self.c[i] + l[0]*self.axes[0][i] + l[1]*self.axes[1][i] + l[2]*self.axes[2][i] for i in range(3))

    def lowest_offset(self):
        return min(self.axes[0][1]*sx*self.h[0] + self.axes[1][1]*sy*self.h[1] + self.axes[2][1]*sz*self.h[2]
                   for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1))

    def highest_offset(self):
        return max(self.axes[0][1]*sx*self.h[0] + self.axes[1][1]*sy*self.h[1] + self.axes[2][1]*sz*self.h[2]
                   for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1))

    def distance(self, p):
        l = self.to_local(p)
        return math.sqrt(sum(max(abs(l[i]) - self.h[i], 0.0)**2 for i in range(3)))

    def hit(self, a, b):
        """segment a-b goes through the box"""
        # bounding sphere first
        d = sub(b, a)
        length2 = dot(d, d)
        t = 0.0 if length2 == 0 else max(0.0, min(1.0, dot(sub(self.c, a), d)/length2))
        closest = (a[0] + d[0]*t, a[1] + d[1]*t, a[2] + d[2]*t)
        if dot(sub(closest, self.c), sub(closest, self.c)) > self.radius**2:
            return False
        la, lb = self.to_local(a), self.to_local(b)
        tmin, tmax = 0.0, 1.0
        for i in range(3):
            di = lb[i] - la[i]
            if abs(di) < 1e-12:
                if abs(la[i]) > self.h[i]:
                    return False
                continue
            t1, t2 = (-self.h[i] - la[i])/di, (self.h[i] - la[i])/di
            tmin = max(tmin, min(t1, t2))
            tmax = min(tmax, max(t1, t2))
            if tmin > tmax:
                return False
        return True

    def face_samples(self, viewer):
        """points just outside of the faces that face the viewer"""
        lv = self.to_local(viewer)
        points = []
        for a in range(3):
            for s in (-1, 1):
                if s*lv[a] <= self.h[a]:
                    continue
                b, c = [i for i in range(3) if i != a]
                for i in range(SAMPLE_GRID):
                    for j in range(SAMPLE_GRID):
                        l = [0.0, 0.0, 0.0]
                        l[a] = s*self.h[a]
                        l[b] = (-1 + 2*(i + .5)/SAMPLE_GRID)*self.h[b]
                        l[c] = (-1 + 2*(j + .5)/SAMPLE_GRID)*self.h[c]
                        w = self.to_world(l)
                        n = self.axes[a]
                        w = tuple(w[k] + s*n[k]*NUDGE for k in range(3))
                        if w[1] >= 0.0:
                            points.append(w)
        return points


class Ellipsoid(Box):
    """Ball as in Cube::init(): the ellipsoid of the semi-axes h inside the box, rotated like the box."""
    def __init__(self, center, size, yaw, pitch, roll):
        super().__init__(center, size, yaw, pitch, roll)
        self.radius = max(self.h)

    def lowest_offset(self):
        return -self.highest_offset()

    def highest_offset(self):
        return math.sqrt(sum((self.axes[i][1]*self.h[i])**2 for i in range(3)))

    def distance(self, p):
        """at most the distance to the ellipsoid, so the cameras are at least this far"""
        return max(0.0, math.dist(p, self.c) - self.radius)

    def hit(self, a, b):
        """segment a-b goes through the ellipsoid (the unit sphere in the scaled local coordinates)"""
        la = [v/self.h[i] for i, v in enumerate(self.to_local(a))]
        lb = [v/self.h[i] for i, v in enumerate(self.to_local(b))]
        d = [lb[i] - la[i] for i in range(3)]
        length2 = dot(d, d)
        t = 0.0 if length2 == 0 else max(0.0, min(1.0, -dot(la, d)/length2))
        closest = [la[i] + d[i]*t for i in range(3)]
        return dot(closest, closest) <= 1.0

    def face_samples(self, viewer):
        """points just outside of the surface that faces the viewer"""
        points = []
        for ring in range(BALL_SAMPLE_RINGS):
            up = math.pi*(ring + 0.5)/BALL_SAMPLE_RINGS
            for segment in range(BALL_SAMPLE_SEGMENTS):
                around = 2.0*math.pi*(segment + 0.5)/BALL_SAMPLE_SEGMENTS
                l = (self.h[0]*math.sin(up)*math.sin(around), -self.h[1]*math.cos(up),
                     self.h[2]*math.sin(up)*math.cos(around))
                g = [l[i]/(self.h[i]*self.h[i]) for i in range(3)]
                n = tuple(sum(g[j]*self.axes[j][k] for j in range(3)) for k in range(3))
                length = math.sqrt(dot(n, n))
                n = tuple(v/length for v in n)
                w = self.to_world(l)
                if dot(n, sub(viewer, w)) <= 0.0:
                    continue
                w = tuple(w[k] + n[k]*NUDGE for k in range(3))
                if w[1] >= 0.0:
                    points.append(w)
        return points


class Cylinder(Box):
    """Cylinder as in Cube::init(): inside the box, its axis is the y axis of the box, its ends are the ellipses of
    the semi-axes h[0] and h[2]"""
    def __init__(self, center, size, yaw, pitch, roll):
        super().__init__(center, size, yaw, pitch, roll)
        self.radius = math.sqrt(max(self.h[0], self.h[2])**2 + self.h[1]**2)

    def lowest_offset(self):
        return -self.highest_offset()

    def highest_offset(self):
        return abs(self.axes[1][1])*self.h[1] + math.sqrt((self.axes[0][1]*self.h[0])**2 + (self.axes[2][1]*self.h[2])**2)

    def distance(self, p):
        """at most the distance to the cylinder, so the cameras are at least this far"""
        return max(0.0, math.dist(p, self.c) - self.radius)

    def hit(self, a, b):
        """segment a-b goes through the cylinder (the unit circle in the scaled local x/z, |y| <= h[1])"""
        la, lb = self.to_local(a), self.to_local(b)
        tmin, tmax = 0.0, 1.0
        dy = lb[1] - la[1]
        if abs(dy) < 1e-12:
            if abs(la[1]) > self.h[1]:
                return False
        else:
            t1, t2 = (-self.h[1] - la[1])/dy, (self.h[1] - la[1])/dy
            tmin, tmax = max(tmin, min(t1, t2)), min(tmax, max(t1, t2))
            if tmin > tmax:
                return False
        # nearest point to the axis on the part of the segment inside the ends
        px, pz = la[0]/self.h[0], la[2]/self.h[2]
        dx, dz = (lb[0] - la[0])/self.h[0], (lb[2] - la[2])/self.h[2]
        length2 = dx*dx + dz*dz
        t = tmin if length2 == 0 else max(tmin, min(tmax, -(px*dx + pz*dz)/length2))
        return (px + dx*t)**2 + (pz + dz*t)**2 <= 1.0

    def face_samples(self, viewer):
        """points just outside of the side and the ends that face the viewer"""
        candidates = []
        for ring in range(BALL_SAMPLE_RINGS):
            y = (-1 + 2*(ring + .5)/BALL_SAMPLE_RINGS)*self.h[1]
            for segment in range(BALL_SAMPLE_SEGMENTS):
                around = 2.0*math.pi*(segment + 0.5)/BALL_SAMPLE_SEGMENTS
                candidates.append(((self.h[0]*math.sin(around), y, self.h[2]*math.cos(around)),
                                   (math.sin(around)/self.h[0], 0.0, math.cos(around)/self.h[2])))
        for s in (-1, 1):
            for i in range(SAMPLE_GRID):
                for j in range(SAMPLE_GRID):
                    u, v = -1 + 2*(i + .5)/SAMPLE_GRID, -1 + 2*(j + .5)/SAMPLE_GRID
                    if u*u + v*v <= 1.0:
                        candidates.append(((u*self.h[0], s*self.h[1], v*self.h[2]), (0.0, s, 0.0)))
        return self.facing_points(candidates, viewer)

    def facing_points(self, candidates, viewer):
        """the candidates (local point, local gradient) that face the viewer, just outside of the surface"""
        points = []
        for l, g in candidates:
            n = tuple(sum(g[j]*self.axes[j][k] for j in range(3)) for k in range(3))
            length = math.sqrt(dot(n, n))
            n = tuple(v/length for v in n)
            w = self.to_world(l)
            if dot(n, sub(viewer, w)) <= 0.0:
                continue
            w = tuple(w[k] + n[k]*NUDGE for k in range(3))
            if w[1] >= 0.0:
                points.append(w)
        return points


class Cone(Cylinder):
    """Cone as in Cube::init(): inside the box, its base is the ellipse of the semi-axes h[0] and h[2] at local
    y = -h[1] and its tip at local y = h[1]"""
    def lowest_offset(self):
        rim = math.sqrt((self.axes[0][1]*self.h[0])**2 + (self.axes[2][1]*self.h[2])**2)
        return min(self.axes[1][1]*self.h[1], -self.axes[1][1]*self.h[1] - rim)

    def highest_offset(self):
        rim = math.sqrt((self.axes[0][1]*self.h[0])**2 + (self.axes[2][1]*self.h[2])**2)
        return max(self.axes[1][1]*self.h[1], -self.axes[1][1]*self.h[1] + rim)

    def inside_value(self, l):
        """negative or 0 inside of the cone (between the base and the tip): (x/h0)^2 + (z/h2)^2 - ((h1 - y)/2h1)^2"""
        return (l[0]/self.h[0])**2 + (l[2]/self.h[2])**2 - ((self.h[1] - l[1])/(2.0*self.h[1]))**2

    def hit(self, a, b):
        """segment a-b goes through the cone: the smallest inside_value() on the part of the segment between the base
        and the tip is at an end of the part or at the vertex of the quadratic"""
        la, lb = self.to_local(a), self.to_local(b)
        tmin, tmax = 0.0, 1.0
        dy = lb[1] - la[1]
        if abs(dy) < 1e-12:
            if abs(la[1]) > self.h[1]:
                return False
        else:
            t1, t2 = (-self.h[1] - la[1])/dy, (self.h[1] - la[1])/dy
            tmin, tmax = max(tmin, min(t1, t2)), min(tmax, max(t1, t2))
            if tmin > tmax:
                return False
        point = lambda t: tuple(la[i] + (lb[i] - la[i])*t for i in range(3))
        values = [self.inside_value(point(tmin)), self.inside_value(point(tmax))]
        # inside_value along the segment is q2*t^2 + q1*t + q0, the vertex is at -q1/(2*q2)
        f0, fm, f1 = self.inside_value(point(0.0)), self.inside_value(point(0.5)), self.inside_value(point(1.0))
        q2 = 2.0*(f1 - 2.0*fm + f0)
        q1 = f1 - f0 - q2
        if q2 > 0.0:
            t = -q1/(2.0*q2)
            if tmin < t < tmax:
                values.append(self.inside_value(point(t)))
        return min(values) <= 0.0

    def face_samples(self, viewer):
        """points just outside of the side and the base that face the viewer"""
        height = 2.0*self.h[1]
        candidates = []
        for ring in range(BALL_SAMPLE_RINGS):
            u = (ring + .5)/BALL_SAMPLE_RINGS          # 0 at the base, 1 at the tip
            y = -self.h[1] + u*height
            for segment in range(BALL_SAMPLE_SEGMENTS):
                around = 2.0*math.pi*(segment + 0.5)/BALL_SAMPLE_SEGMENTS
                candidates.append((((1 - u)*self.h[0]*math.sin(around), y, (1 - u)*self.h[2]*math.cos(around)),
                                   (math.sin(around)/self.h[0], 1.0/height, math.cos(around)/self.h[2])))
        for i in range(SAMPLE_GRID):
            for j in range(SAMPLE_GRID):
                u, v = -1 + 2*(i + .5)/SAMPLE_GRID, -1 + 2*(j + .5)/SAMPLE_GRID
                if u*u + v*v <= 1.0:
                    candidates.append(((u*self.h[0], -self.h[1], v*self.h[2]), (0.0, -1.0, 0.0)))
        return self.facing_points(candidates, viewer)


def cross(a, b):
    return (a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0])


def boxes_near(a, b, gap):
    """the box of a (its cube) and the box of b grown by gap on every side overlap (separating axis test): False
    means that the boxes are at least gap apart (the grown box contains every point nearer than gap to b)"""
    hb = [v + gap for v in b.h]
    t = sub(b.c, a.c)
    axes = list(a.axes) + list(b.axes) + [cross(u, v) for u in a.axes for v in b.axes]
    for axis in axes:
        if dot(axis, axis) < 1e-9:
            continue
        ra = sum(a.h[i]*abs(dot(a.axes[i], axis)) for i in range(3))
        rb = sum(hb[i]*abs(dot(b.axes[i], axis)) for i in range(3))
        if abs(dot(t, axis)) > ra + rb:
            return False
    return True


def blocked(a, b, boxes):
    return any(box.hit(a, b) for box in boxes)


def light_touches(pos, direction, boxes, need=LIGHT_MIN_CUBES, blockers=None):
    """cubes that have a lit point inside the inner cone, not in shadow of other cubes (or of the blockers)"""
    blockers = boxes if blockers is None else blockers
    cos_inner = math.cos(math.radians(LIGHT_INNER))
    count = 0
    for box in boxes:
        d = sub(box.c, pos)
        dist = math.sqrt(dot(d, d))
        if dist <= box.radius:
            continue
        angle = math.degrees(math.acos(max(-1.0, min(1.0, dot(d, direction)/dist))))
        if angle - math.degrees(math.asin(box.radius/dist)) > LIGHT_INNER:
            continue
        for p in box.face_samples(pos):
            v = sub(p, pos)
            if dot(v, direction)/math.sqrt(dot(v, v)) >= cos_inner and not blocked(pos, p, blockers):
                count += 1
                break
        if count >= need:
            break
    return count


def read_camera_positions():
    positions = []
    for line in open(REPO + '/configs/cameraobjects.ini'):
        m = re.match(r'cameraObjectPosition(\d+)=([^/]+)/([^/]+)/([^/\s]+)', line)
        if m:
            positions.append(tuple(float(m.group(i)) for i in (2, 3, 4)))
    return positions


def read_objects_directory():
    """<cameraImageOutputDirectory of general.ini>/camera_objects, relative path is relative to the repository like
    for the program"""
    key = 'cameraImageOutputDirectory='
    for line in open(REPO + '/configs/general.ini'):
        line = line.strip()
        if line.startswith(key):
            directory = line[len(key):].strip()
            if directory:
                return os.path.abspath(os.path.join(REPO, os.path.expanduser(directory), 'camera_objects'))
    sys.exit('Error: cameraImageOutputDirectory is missing in configs/general.ini')


def create_directory(directory):
    """creates the directory and its parents if they do not exist, exits with an error if it cannot be created"""
    try:
        os.makedirs(directory, exist_ok=True)
    except OSError:
        pass
    if not os.path.isdir(directory):
        sys.exit('Could not create directory %s' % directory)


def place_corner_cube(rng, size, corner, placed, cameras):
    """random place, yaw, pitch and roll of a cube in the corner (sign of x and z), touching the floor, not too near
    to the cameras and the placed cubes, None if no place is found"""
    for _ in range(5000):
        yaw, pitch, roll = [round(rng.uniform(0, 360), 1) for _ in range(3)]
        angle = rng.uniform(0.0, math.pi/2.0)
        distance = rng.uniform(MIN_CUBE_DISTANCE, BASE_MAX_DISTANCE)
        x = round(corner[0]*distance*math.sin(angle), -1)
        z = round(corner[1]*distance*math.cos(angle), -1)
        box0 = Box((0.0, 0.0, 0.0), size, yaw, pitch, roll)
        extent = box0.highest_offset() - box0.lowest_offset()
        sink = rng.uniform(0.0, SINK_RATE)*extent
        y = math.floor((-box0.lowest_offset() - sink)*10.0)/10.0
        box = Box((x, y, z), size, yaw, pitch, roll)
        if any(box.distance(c) < MIN_CAMERA_DISTANCE for c in cameras):
            continue
        if any(math.dist(box.c, o.c) < box.radius + o.radius + CUBE_GAP for o in placed):
            continue
        return box, (x, y, z, yaw, pitch, roll)
    return None


def generate_base(cameras):
    """text of objects0.dat: BASE_CUBES_PER_CORNER random cubes in every corner, 1-5 spot lights, ambient light"""
    rng = random.Random(BASE_SEED)
    corners = [((-1, 1), '-x/+y/+z'), ((1, 1), '+x/+y/+z'), ((-1, -1), '-x/+y/-z'), ((1, -1), '+x/+y/-z')]
    while True:
        boxes, sections = [], []
        for corner, name in corners:
            lines = []
            for _ in range(BASE_CUBES_PER_CORNER):
                size = tuple(round(rng.uniform(*BASE_CUBE_SIZE), -1) for _ in range(3))
                texture = rng.choice(BASE_TEXTURES)
                placed = place_corner_cube(rng, size, corner, boxes, cameras)
                if not placed:
                    break
                box, (x, y, z, yaw, pitch, roll) = placed
                boxes.append(box)
                lines.append('Cube=%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%s\n'
                             % (x, y, z, *size, yaw, pitch, roll, texture))
            else:
                sections.append('\n# random cubes in %s area within %d m distance\n' % (name, BASE_MAX_DISTANCE/1000.0)
                                + ''.join(lines))
                continue
            break
        else:
            lights = []
            for _ in range(rng.randint(*BASE_LIGHTS)):
                light = random_light(rng, boxes)
                if not light:
                    break
                lights.append(light)
            else:
                break
    ambient = round(rng.uniform(MIN_AMBIENT, MAX_AMBIENT), 2)
    text = '# objects0.dat, random cubes in the 4 corners, the cubes of the other objects files are taken from here\n'
    text += HEADER + ''.join(sections) + LIGHT_HEADER
    for pos, d in lights:
        text += 'SpotLight=%.1f/%.1f/%.1f/%.4f/%.4f/%.4f/%.1f/%.1f/1.0/1.0/1.0\n' % (*pos, *d, LIGHT_INNER, LIGHT_OUTER)
    text += AMBIENT_HEADER + 'AmbientLight=%.2f/%.2f/%.2f\n' % (ambient, ambient, ambient)
    return text


def create_base(objects_directory, cameras):
    """creates objects0.dat if it is missing"""
    filename = os.path.join(objects_directory, 'objects0.dat')
    if os.path.isfile(filename):
        return
    open(filename, 'w').write(generate_base(cameras))
    print('objects0.dat (it was missing)', flush=True)


def read_base_cubes(objects_directory):
    """cubes of objects0.dat of the objects directory"""
    filename = os.path.join(objects_directory, 'objects0.dat')
    if not os.path.isfile(filename):
        sys.exit('Error: %s is missing, the cubes of the generated files are taken from it' % filename)
    cubes = []
    for line in open(filename):
        if line.startswith('Cube='):
            v = line[5:].strip().split('/')
            cubes.append((tuple(float(x) for x in v[3:6]), v[9]))
    return cubes


def place_people_and_cars(rng, cameras):
    """0-30 cars and 0-60 people standing on the floor with a random heading, at least PERSON_GAP from each other
    (between their boxes), returns the lines of the objects file and the (box, gap to the cubes) of every one"""
    lines, placed = [], []
    wanted = [('car', i) for i in range(rng.randint(*CARS))] + [('person', i) for i in range(rng.randint(*PEOPLE))]
    for kind, _ in wanted:
        if kind == 'car':
            name, normal = rng.choice(CAR_TYPES)
            scale = rng.uniform(1.0 - CAR_SIZE_CHANGE, 1.0 + CAR_SIZE_CHANGE)
            texture, gap = rng.choice(PAINTS), CAR_GAP
        else:
            name, normal = rng.choice(HUMAN_TYPES)
            scale = (normal[1] + rng.uniform(-HUMAN_HEIGHT_CHANGE, HUMAN_HEIGHT_CHANGE))/normal[1]
            texture, gap = rng.choice(SHIRTS), PERSON_GAP
        size = tuple(round(v*scale, 1) for v in normal)
        for _ in range(5000):
            yaw = round(rng.uniform(0, 360), 1)
            angle = rng.uniform(0, 2*math.pi)
            distance = rng.uniform(MIN_CUBE_DISTANCE, MAX_CUBE_DISTANCE)
            x = round(distance*math.sin(angle), -1)
            z = round(distance*math.cos(angle), -1)
            # standing on the floor: the bottom of the box is at y 0
            box = Box((x, size[1]/2.0, z), size, yaw, 0.0, 0.0)
            if any(box.distance(c) < MIN_CAMERA_DISTANCE for c in cameras):
                continue
            if any(boxes_near(box, o, PERSON_GAP) for o, _ in placed):
                continue
            placed.append((box, gap))
            lines.append('%s=%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/0.0/0.0/%s\n'
                         % (name, x, size[1]/2.0, z, *size, yaw, texture))
            break
    return lines, placed


def place_cubes(rng, base, cameras, shapes, others=()):
    """random place, yaw, pitch and roll for every cube, all seen from the eye, shapes[index] is the shape of the
    cube (SHAPES), the box of every cube is at least the gap from the boxes of others (box, gap)"""
    placed = []     # (box, params, visible sample points, sample count)
    order = sorted(range(len(base)), key=lambda i: -max(base[i][0]))   # big cubes first
    result = [None]*len(base)
    for index in order:
        size, texture = base[index]
        for _ in range(5000):
            yaw, pitch, roll = [round(rng.uniform(0, 360), 1) for _ in range(3)]
            angle = rng.uniform(0, 2*math.pi)
            distance = rng.uniform(MIN_CUBE_DISTANCE, MAX_CUBE_DISTANCE)
            x = round(distance*math.sin(angle), -1)
            z = round(distance*math.cos(angle), -1)
            shape = {'Cube': Box, 'Ball': Ellipsoid, 'Cylinder': Cylinder, 'Cone': Cone}[shapes[index]]
            box0 = shape((0.0, 0.0, 0.0), size, yaw, pitch, roll)
            # lowest point is at the floor or sinks partly under it, so the cube or ball always touches the floor
            extent = box0.highest_offset() - box0.lowest_offset()
            sink = rng.uniform(0.0, SINK_RATE)*extent
            y = math.floor((-box0.lowest_offset() - sink)*10.0)/10.0
            box = shape((x, y, z), size, yaw, pitch, roll)
            if any(box.distance(c) < MIN_CAMERA_DISTANCE for c in cameras):
                continue
            if any(math.dist(box.c, o[0].c) < box.radius + o[0].radius + CUBE_GAP for o in placed):
                continue
            if any(boxes_near(box, o, gap) for o, gap in others):
                continue
            samples = box.face_samples(EYE)
            visible = [p for p in samples if not blocked(EYE, p, [o[0] for o in placed])]
            if not samples or len(visible)/len(samples) < MIN_VISIBLE:
                continue
            # the new cube must not hide the cubes that are already placed
            filtered = []
            ok = True
            for o in placed:
                left = [p for p in o[2] if not box.hit(EYE, p)]
                if len(left)/o[3] < MIN_VISIBLE:
                    ok = False
                    break
                filtered.append(left)
            if not ok:
                continue
            placed = [(o[0], o[1], filtered[i], o[3]) for i, o in enumerate(placed)]
            placed.append((box, (shapes[index], x, y, z, size, yaw, pitch, roll, texture), visible, len(samples)))
            result[index] = placed[-1]
            break
        else:
            return None
    # placed list was rebuilt, take the final boxes by the index
    boxes = {id(o[0]): o for o in placed}
    return [boxes[id(r[0])][1] for r in result], [o[0] for o in placed]


def random_light(rng, boxes, blockers=None):
    for _ in range(20000):
        pos = (round(rng.uniform(-LIGHT_AREA, LIGHT_AREA), -1), round(rng.uniform(*LIGHT_HEIGHT), -1),
               round(rng.uniform(-LIGHT_AREA, LIGHT_AREA), -1))
        v = [rng.gauss(0, 1) for _ in range(3)]
        n = math.sqrt(dot(v, v))
        direction = tuple(round(c/n, 4) for c in v)
        n = math.sqrt(dot(direction, direction))
        direction = tuple(c/n for c in direction)
        if light_touches(pos, direction, boxes, blockers=blockers) >= LIGHT_MIN_CUBES:
            return pos, tuple(round(c, 4) for c in direction)
    return None


def generate(number, cameras, base):
    rng = random.Random(number)
    # own random numbers for the shapes, so the shapes depend only on the file number
    shape_rng = random.Random('shapes%d' % number)
    shapes = [shape_rng.choice(SHAPES) for _ in base]
    while True:
        people_lines, others = place_people_and_cars(rng, cameras)
        placed = place_cubes(rng, base, cameras, shapes, others)
        if not placed:
            continue
        params, boxes = placed
        lights = []
        for _ in range(rng.randint(1, 3)):
            light = random_light(rng, boxes, boxes + [o for o, _ in others])
            if not light:
                break
            lights.append(light)
        else:
            break
    ambient = round(rng.uniform(MIN_AMBIENT, MAX_AMBIENT), 2)
    text = ('# objects%d.dat, random cubes, balls, cylinders, cones, cars, people and spot lights\n' % number
            + HEADER)
    for shape, x, y, z, size, yaw, pitch, roll, texture in params:
        text += '%s=%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%s\n' % (shape, x, y, z, *size, yaw, pitch, roll, texture)
    if people_lines:
        text += '\n# cars and people standing on the floor\n' + ''.join(people_lines)
    text += LIGHT_HEADER
    for pos, d in lights:
        text += 'SpotLight=%.1f/%.1f/%.1f/%.4f/%.4f/%.4f/%.1f/%.1f/1.0/1.0/1.0\n' % (*pos, *d, LIGHT_INNER, LIGHT_OUTER)
    text += AMBIENT_HEADER + 'AmbientLight=%.2f/%.2f/%.2f\n' % (ambient, ambient, ambient)
    return text


if __name__ == '__main__':
    arguments = [a for a in sys.argv[1:] if not a.startswith('--objectsDirectory=')]
    directories = [a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith('--objectsDirectory=')]
    if len(arguments) != 2 or (directories and not directories[-1]):
        sys.exit('usage: genobjects.py <first> <last> [--objectsDirectory=<directory>]')
    if directories:
        objects_directory = os.path.abspath(os.path.expanduser(directories[-1]))
    else:
        objects_directory = read_objects_directory()
    create_directory(objects_directory)
    cameras = read_camera_positions()
    create_base(objects_directory, cameras)
    base = read_base_cubes(objects_directory)*BASE_COPIES
    first, last = int(arguments[0]), int(arguments[1])
    for n in range(first, last + 1):
        open(os.path.join(objects_directory, 'objects%d.dat' % n), 'w').write(generate(n, cameras, base))
        print('objects%d.dat' % n, flush=True)
