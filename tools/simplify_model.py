"""Simplifies a 3D model (obj) to about the given count of triangles with Blender, keeping its shape:
usage blender -b --factory-startup -P tools/simplify_model.py -- <input.obj> <output.obj> <triangles>
      [--interiorWeight=<weight>] [--areaPower=<0..1>] [--subdivide=<levels>]
Every object of the model gets a share of the triangles by its surface area (the same triangle density everywhere,
at least MIN_TRIANGLES or all of its own triangles), so the very dense small parts (wheel bolts, brakes) lose the
most and the large body panels keep their shape. Every object is decimated (Blender Decimate modifier, collapse of
the edges that change the shape the least) to its share. The normals are smooth inside the surfaces and sharp at
the edges of over SMOOTH_ANGLE degrees. The output obj has the same object and material names (usemtl) as the
input and mtllib of the material file of the input (copy or link it next to the output), so its materials are the
same. Then prj-camera-image-creator --convertModel=<output.obj> --output=<model.own_obj> makes the .own_obj file
of the creator (README.md).
--interiorWeight=<weight>: the area of the interior materials (names with INTERIOR_NAMES: seats, dashboard, seat
belts, they are seen only through the tinted windows) counts this much (for example 0.25), the triangles go to the
outside of the car.
--areaPower=<0..1>: the share of an object is by area^power*triangles^(1-power): 1 (default) by the surface area
(the same triangle density everywhere, for a model that is much too detailed, like the 6.3 million triangles of the
Fortuner), 0 every object keeps the same part of its triangles (for a small reduction, the small detailed parts like
logos keep their share), 0.5 between them.
--subdivide=<levels>: a model of quads that is made to be smoothed (for example the 27832 triangles of the Mercedes
W222) is first subdivided (Catmull-Clark, every level 4 times the faces, the whole objects so that the parts of the
materials still meet), then decimated to the triangles."""
import math
import os
import sys

import bpy
import bmesh

MIN_TRIANGLES = 16
SMOOTH_ANGLE = 30.0
INTERIOR_NAMES = ('_int', 'interior', 'leather', 'seat', 'stitch', 'egg')


def main():
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    interior_weight = 1.0
    area_power = 1.0
    subdivide = 0
    for arg in args:
        if arg.startswith('--interiorWeight='):
            interior_weight = float(arg.split('=', 1)[1])
        elif arg.startswith('--areaPower='):
            area_power = float(arg.split('=', 1)[1])
        elif arg.startswith('--subdivide='):
            subdivide = int(arg.split('=', 1)[1])
    args = [a for a in args if not a.startswith('--')]
    if len(args) != 3:
        sys.exit('usage: blender -b --factory-startup -P simplify_model.py -- <input.obj> <output.obj> <triangles> '
                 '[--interiorWeight=<weight>] [--areaPower=<0..1>]')
    source, output, target = args[0], args[1], int(args[2])
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    bpy.ops.wm.obj_import(filepath=source, use_split_objects=True, use_split_groups=True)
    # a model of quads that is made to be smoothed (subdivision surface) is subdivided first, the whole objects, so the
    # parts of the materials still meet (the normals of the file are cleared, they are made again at the end)
    if subdivide > 0:
        for obj in [o for o in bpy.context.scene.objects if o.type == 'MESH']:
            bpy.context.view_layer.objects.active = obj
            with bpy.context.temp_override(object=obj, active_object=obj):
                if obj.data.has_custom_normals:
                    bpy.ops.mesh.customdata_custom_splitnormals_clear()
                modifier = obj.modifiers.new('subdivide', 'SUBSURF')
                modifier.subdivision_type = 'CATMULL_CLARK'
                modifier.levels = subdivide
                modifier.render_levels = subdivide
                bpy.ops.object.modifier_apply(modifier=modifier.name)
    # every material of an object is its own object, so every material gets its share (a small material of an object
    # is not collapsed away by the larger ones)
    for obj in [o for o in bpy.context.scene.objects if o.type == 'MESH']:
        if len(obj.data.materials) > 1:
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.mesh.separate(type='MATERIAL')
            bpy.ops.object.mode_set(mode='OBJECT')
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    # triangles and surface area of every object
    info = []
    for obj in meshes:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        area = sum(f.calc_area() for f in bm.faces)
        names = (obj.name + ' ' + ' '.join(m.name for m in obj.data.materials if m)).lower()
        if any(n in names for n in INTERIOR_NAMES):
            area *= interior_weight
        # the weight of the share: the area (area_power 1) or the triangles of the object (0, every object keeps the
        # same part of its triangles) or between them
        weight = (area**area_power)*(len(bm.faces)**(1.0 - area_power)) if len(bm.faces) else 0.0
        if any(n in names for n in INTERIOR_NAMES) and area_power < 1.0:
            weight *= interior_weight**(1.0 - area_power)
        info.append([obj, len(bm.faces), weight])
        bm.free()
    total = sum(i[1] for i in info)
    print('objects %d, triangles %d' % (len(info), total))
    # shares by the area: the objects that have fewer triangles than their share keep them all and the rest of the
    # triangles go to the others
    share = {}
    left = list(info)
    budget = target
    while left:
        area = sum(i[2] for i in left) or 1.0
        capped = [i for i in left if i[1] <= max(MIN_TRIANGLES, budget*i[2]/area)]
        if not capped:
            for i in left:
                share[i[0].name] = max(MIN_TRIANGLES, int(budget*i[2]/area))
            break
        for i in capped:
            share[i[0].name] = i[1]
            budget -= i[1]
        left = [i for i in left if i not in capped]
    for obj, count, area in info:
        bpy.context.view_layer.objects.active = obj
        mesh = obj.data
        # the normals of the file are cleared, the smooth and sharp normals are made again after the decimation
        if mesh.has_custom_normals:
            with bpy.context.temp_override(object=obj, active_object=obj):
                bpy.ops.mesh.customdata_custom_splitnormals_clear()
        ratio = min(1.0, share[obj.name]/count) if count else 1.0
        if ratio < 1.0:
            modifier = obj.modifiers.new('decimate', 'DECIMATE')
            modifier.decimate_type = 'COLLAPSE'
            modifier.ratio = ratio
            modifier.use_collapse_triangulate = True
            with bpy.context.temp_override(object=obj, active_object=obj):
                bpy.ops.object.modifier_apply(modifier=modifier.name)
        for polygon in mesh.polygons:
            polygon.use_smooth = True
        mesh.use_auto_smooth = True
        mesh.auto_smooth_angle = math.radians(SMOOTH_ANGLE)
    result = 0
    for obj in meshes:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        result += len(bm.faces)
        bm.free()
    print('simplified to %d triangles' % result)
    bpy.ops.wm.obj_export(filepath=output, export_selected_objects=False, export_triangulated_mesh=True,
                          export_normals=True, export_uv=True, export_materials=True, apply_modifiers=True,
                          forward_axis='NEGATIVE_Z', up_axis='Y')
    # the materials of the input: mtllib of the input file, and the object, group and material names of the input
    # (Blender writes the spaces of the names as _, the names of the output get the names of the input back, the
    # objects are written as groups g)
    mtllib = None
    names = {}
    with open(source, errors='replace') as f:
        for line in f:
            if line.startswith('mtllib ') and mtllib is None:
                mtllib = line
            elif line.startswith(('o ', 'g ', 'usemtl ')):
                name = line.split(None, 1)[1].strip() if len(line.split(None, 1)) > 1 else ''
                names[name.replace(' ', '_')] = name

    def original(name):
        if name in names:
            return names[name]
        # a copy of an object: <name>.001
        base, dot, number = name.rpartition('.')
        if dot and number.isdigit() and base in names:
            return names[base] + '.' + number
        return name

    # the material file that Blender wrote is replaced by the material file of the input (Blender changes the colors
    # and the transparency of the materials, the names stay)
    written = os.path.splitext(output)[0] + '.mtl'
    with open(output) as f:
        lines = []
        for line in f:
            if line.startswith('mtllib '):
                continue
            for key in ('o ', 'usemtl '):
                if line.startswith(key):
                    line = key + original(line[len(key):].rstrip('\n')) + '\n'
            # the objects are written as groups: assimp reads the name of a group to its end, the name of an object
            # only to its first space ("body color" of the R8)
            if line.startswith('o '):
                line = 'g ' + line[2:]
            lines.append(line)
    with open(output, 'w') as f:
        if mtllib:
            f.write(mtllib)
        f.writelines(lines)
    if os.path.exists(written) and (not mtllib or os.path.basename(written) != mtllib.split(None, 1)[1].strip()):
        os.remove(written)

main()
