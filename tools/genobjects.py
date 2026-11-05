#!/usr/bin/env python3
"""Generates objects<first>.dat ... objects<last>.dat into <cameraImageOutputDirectory>/camera_objects
(cameraImageOutputDirectory of configs/general.ini):
usage genobjects.py <first> <last>
Cubes of objects0.dat are placed randomly, 1-3 random spot lights, random ambient light."""
import math, random, re, sys

import os

# repository root, this script is in the tools folder
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EYE = (0.0, 1000.0, 0.0)            # 0/1/0 in opengl units, mm
MIN_CAMERA_DISTANCE = 1000.0        # mm, from the cubes to cameraObjectPosition<number>
MIN_AMBIENT, MAX_AMBIENT = 0.1, 0.5
MIN_CUBE_DISTANCE, MAX_CUBE_DISTANCE = 1500.0, 15000.0
CUBE_GAP = 300.0                    # mm between the bounding spheres of cubes
MIN_VISIBLE = 0.3                   # part of the faces seen from the eye that must be seen
LIGHT_MIN_CUBES = 5
LIGHT_INNER, LIGHT_OUTER = 25.0, 35.0
LIGHT_HEIGHT = (3000.0, 8000.0)
LIGHT_AREA = 15000.0
NUDGE = 2.0                         # mm, sample points are this far outside of the face
SAMPLE_GRID = 7                     # points per side on every face
SINK_RATE = 0.25                    # cube can sink at most this part of its height under the floor

HEADER = """# Position of cube (x/y/z/SizeX/SizeY/SizeZ/Yaw/Pitch/Roll/wood.png)
# in Cube, center position=x,y,z, size=sizeX,sizeY,SizeZ values mm, 2000=2.0f, Yaw/Pitch/Roll angle in degrees, and finally texture image as text string in images folder
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


def blocked(a, b, boxes):
    return any(box.hit(a, b) for box in boxes)


def light_touches(pos, direction, boxes, need=LIGHT_MIN_CUBES):
    """cubes that have a lit point inside the inner cone, not in shadow of other cubes"""
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
            if dot(v, direction)/math.sqrt(dot(v, v)) >= cos_inner and not blocked(pos, p, boxes):
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


def place_cubes(rng, base, cameras):
    """random place, yaw, pitch and roll for every cube, all seen from the eye"""
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
            box0 = Box((0.0, 0.0, 0.0), size, yaw, pitch, roll)
            # lowest corner is at the floor or sinks partly under it, so the cube always touches the floor
            extent = box0.highest_offset() - box0.lowest_offset()
            sink = rng.uniform(0.0, SINK_RATE)*extent
            y = math.floor((-box0.lowest_offset() - sink)*10.0)/10.0
            box = Box((x, y, z), size, yaw, pitch, roll)
            if any(box.distance(c) < MIN_CAMERA_DISTANCE for c in cameras):
                continue
            if any(math.dist(box.c, o[0].c) < box.radius + o[0].radius + CUBE_GAP for o in placed):
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
            placed.append((box, (x, y, z, size, yaw, pitch, roll, texture), visible, len(samples)))
            result[index] = placed[-1]
            break
        else:
            return None
    # placed list was rebuilt, take the final boxes by the index
    boxes = {id(o[0]): o for o in placed}
    return [boxes[id(r[0])][1] for r in result], [o[0] for o in placed]


def random_light(rng, boxes):
    for _ in range(20000):
        pos = (round(rng.uniform(-LIGHT_AREA, LIGHT_AREA), -1), round(rng.uniform(*LIGHT_HEIGHT), -1),
               round(rng.uniform(-LIGHT_AREA, LIGHT_AREA), -1))
        v = [rng.gauss(0, 1) for _ in range(3)]
        n = math.sqrt(dot(v, v))
        direction = tuple(round(c/n, 4) for c in v)
        n = math.sqrt(dot(direction, direction))
        direction = tuple(c/n for c in direction)
        if light_touches(pos, direction, boxes) >= LIGHT_MIN_CUBES:
            return pos, tuple(round(c, 4) for c in direction)
    return None


def generate(number, cameras, base):
    rng = random.Random(number)
    while True:
        placed = place_cubes(rng, base, cameras)
        if not placed:
            continue
        params, boxes = placed
        lights = []
        for _ in range(rng.randint(1, 3)):
            light = random_light(rng, boxes)
            if not light:
                break
            lights.append(light)
        else:
            break
    ambient = round(rng.uniform(MIN_AMBIENT, MAX_AMBIENT), 2)
    text = '# objects%d.dat, random cubes and spot lights\n' % number + HEADER
    for x, y, z, size, yaw, pitch, roll, texture in params:
        text += 'Cube=%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%.1f/%s\n' % (x, y, z, *size, yaw, pitch, roll, texture)
    text += LIGHT_HEADER
    for pos, d in lights:
        text += 'SpotLight=%.1f/%.1f/%.1f/%.4f/%.4f/%.4f/%.1f/%.1f/1.0/1.0/1.0\n' % (*pos, *d, LIGHT_INNER, LIGHT_OUTER)
    text += AMBIENT_HEADER + 'AmbientLight=%.2f/%.2f/%.2f\n' % (ambient, ambient, ambient)
    return text


if __name__ == '__main__':
    objects_directory = read_objects_directory()
    create_directory(objects_directory)
    cameras = read_camera_positions()
    base = read_base_cubes(objects_directory)
    first, last = int(sys.argv[1]), int(sys.argv[2])
    for n in range(first, last + 1):
        open(os.path.join(objects_directory, 'objects%d.dat' % n), 'w').write(generate(n, cameras, base))
        print('objects%d.dat' % n, flush=True)
