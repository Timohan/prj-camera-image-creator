# prj-camera-image-creator
Camera image creator

# prj-tg-ui-lib requirement
The window, the labels and the keyboard and mouse input are from prj-tg-ui-lib
(https://github.com/Timohan/prj-tg-ui-lib.git), it needs prj-ttf-reader (https://github.com/Timohan/prj-ttf-reader)
and the packages libglew-dev, libpng-dev, libglfw3-dev, make and build-essential:

    sudo apt install libglew-dev libpng-dev libglfw3-dev make build-essential
    git clone https://github.com/Timohan/prj-ttf-reader.git
    (build and install prj-ttf-reader as told in its README)
    git clone https://github.com/Timohan/prj-tg-ui-lib.git
    cd prj-tg-ui-lib/lib
    make
    sudo make install

test: pkg-config --modversion prj-tg-ui-lib

# OpenCV requirement
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git

git checkout 4.9.0 on both

cd opencv
mkdir build && cd build

cmake -D CMAKE_BUILD_TYPE=RELEASE \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D WITH_QT=ON \
      -D WITH_OPENGL=ON \
      -D OPENCV_GENERATE_PKGCONFIG=ON \
      -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
      ..

make
sudo make install
sudo ldconfig

test: pkg-config --modversion opencv4

# Directories
cameraImageOutputDirectory of configs/general.ini (/mnt/cameras) has two directories:
- `<cameraImageOutputDirectory>/camera_output` (output directory): the saved pictures and data files,
- `<cameraImageOutputDirectory>/camera_objects` (objects directory): the objects files (cubes and lights), objects0.dat
  has the cubes that tools/genobjects.py places.

The program and the tools create a directory (and its parents) when they save files there and it does not exist,
if it cannot be created, they print `Could not create directory <directory>`.

# Creating pictures (quick start)
1. Build the program:

       make

2. Select the cameras in configs/general.ini with cameraObjectsFile (the camera files are in configs/):
   - fisheye pictures: `cameraObjectsFile=cameraobjects_elp.ini` (6 ELP 180 degree fisheye cameras, fisheye lens
     model, the area outside of the lens circle is black),
   - non-fisheye pictures: `cameraObjectsFile=cameraobjects_ids.ini` (6 IDS cameras with a 4 mm lens, pinhole lens
     model) or `cameraObjectsFile=cameraobjects.ini` (6 cameras with a 35 mm lens, no distortion).

   The lens of every camera is cameraObjectDistortionModel<number> (fisheye or pinhole) of that file, so a file can
   also mix them. imageOutputScale of configs/general.ini sets the resolution of the saved pictures
   (0.1 is 1/10 of the camera resolution).
3. Create the objects files (cubes and lights) with tools/genobjects.py and select one with objectsFile of
   configs/general.ini. The objects files do not depend on the lens: genobjects.py places the cubes around the
   camera positions of configs/cameraobjects.ini, which are the same in all three camera files. Keep the fisheye
   and the non-fisheye pictures in their own cameraImageOutputDirectory, so they do not mix in one camera_output.
   Example when /mnt/cameras is empty:

   Non-fisheye, set in configs/general.ini:

       cameraImageOutputDirectory=/mnt/cameras
       cameraObjectsFile=cameraobjects_ids.ini
       objectsFile=objects1.dat

   and create the objects files:

       python3 tools/genobjects.py 1 10

   It prints `objects0.dat (it was missing)` and `objects1.dat` ... `objects10.dat`, and creates
   /mnt/cameras/camera_objects/objects0.dat (the cubes that the other files place randomly) and objects1.dat ...
   objects10.dat.

   Fisheye, set in configs/general.ini:

       cameraImageOutputDirectory=/mnt/cameras/fisheye
       cameraObjectsFile=cameraobjects_elp.ini
       objectsFile=objects1.dat

   and run the same command:

       python3 tools/genobjects.py 1 10

   It creates the same files to /mnt/cameras/fisheye/camera_objects: the same number always gives the same file,
   so objects1.dat is the same scene for both lenses and their pictures can be compared. The directories are
   created when they are missing.
4. Open the creator:

       ./prj-camera-image-creator

   It opens the objects file of objectsFile (objects1.dat above) from the camera_objects directory of
   cameraImageOutputDirectory, with the cameras of cameraObjectsFile, so with the fisheye settings of step 3 it opens
   /mnt/cameras/fisheye/camera_objects/objects1.dat with the fisheye cameras, and with the non-fisheye settings
   /mnt/cameras/camera_objects/objects1.dat with the IDS cameras.

   To open another created objects file, give it on the command line, for example objects5.dat of the same
   directory:

       ./prj-camera-image-creator --objectsFile=objects5.dat

   or change objectsFile in configs/general.ini. Close the creator and open it again for every objects file: one
   window shows one objects file. To change between fisheye and non-fisheye, change cameraImageOutputDirectory and
   cameraObjectsFile in configs/general.ini as in step 3 and open the creator again.

   The window shows the scene from the base camera. Keys 1-6 show the view of that camera,
   key 0 goes back to the base camera, the arrow keys turn the view, the mouse wheel moves it and key 'c' shows or hides
   the camera objects.
5. Press a key to save the pictures of all cameras:
   - 's' saves the picture (png) and the data file (.dat, the correct distance of every pixel) of every camera,
     these are the training images of prj-camera-image-learner,
   - 'a' saves only the pictures (png), without the data files.

   The files go to `<cameraImageOutputDirectory>/camera_output`, named
   `<objects file name>_<timestamp>_<camera number>.png` (and `.dat`), for example
   `objects1_20261108-092717_1.png` ... `objects1_20261108-092717_6.png`. The saved pictures never have the camera
   objects.

The 6 pictures of one save are one scene: prj-camera-image-viewer shows it with `--scene=objects1_20261108-092717`.
Many scenes at once: tools/genobjects.py and tools/takepictures.py (see Tools below), or
`prj-camera-image-creator --takePictures` (next section) for one scene without pressing keys.

# Example objects file
examples/objects_shapes.dat has all 4 object types (Cube, Ball, Cylinder and Cone), 10 objects around the cameras,
upright and turned (a cylinder and a cone lie on their side, a cone is upside down on its tip), with comments on
every line and on the values. Copy it to the objects directory and open it:

    cp examples/objects_shapes.dat /mnt/cameras/camera_objects/
    ./prj-camera-image-creator --objectsFile=objects_shapes.dat

examples/objects_cars.dat has the 3 car types, a sedan (Car), an SUV (Suv) and a sports car (SportCar), 7 cars
around the cameras:

    cp examples/objects_cars.dat /mnt/cameras/camera_objects/
    ./prj-camera-image-creator --objectsFile=objects_cars.dat

`Car=`, `Suv=` and `SportCar=` have the same values x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture as the other
objects. The car (src/scene/car_mesh.cpp) fits into the cube of the values: sizeX is its width with the side mirrors,
sizeY its height and sizeZ its length, its front is +z of the cube and its wheels touch the bottom of the cube
(y = sizeY/2 puts it on the floor). Realistic sizes:

| Type | Car | Values | Example |
|---|---|---|---|
| `Car` | sedan, 4 doors, hood, cabin and trunk, 4.6 m x 1.8 m x 1.45 m | 2000/1450/4600 | `Car=0/725/6500/2000/1450/4600/-90/0/0/car_red.png` |
| `Suv` | tall 2-box body, steep tailgate, high ground clearance, large wheels, roof rails, dark side skirts, 4.7 m x 1.9 m x 1.75 m | 2100/1750/4700 | `Suv=-4300/875/5000/2100/1750/4700/-90/0/0/car_silver.png` |
| `SportCar` | low 2-door coupe, long hood, fastback, large low-profile wheels, side intakes, rear wing, 4.4 m x 1.9 m x 1.25 m | 2050/1250/4400 | `SportCar=4300/625/5000/2050/1250/4400/-90/0/0/car_blue.png` |

The body has wheel arches, a hood, a cabin and a trunk or a tailgate. The texture is its paint (images/car_red.png,
car_blue.png, car_silver.png, car_white.png and car_black.png), the windows, tires, rims, lights, grille, license
plates and door handles have their own colors. A car is one surface number in the data files. The objects file lists
the cars after the cones: the cars, the SUVs and then the sports cars. tools/genobjects.py places 0-30 random cars.

examples/objects_people.dat has standing men (Man), women (Woman) and children (Child) around the cameras and a car
next to them:

    cp examples/objects_people.dat /mnt/cameras/camera_objects/
    ./prj-camera-image-creator --objectsFile=objects_people.dat

`Man=`, `Woman=` and `Child=` have the same values x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture as the other
objects. The standing person (src/scene/human_mesh.cpp, the arms down at the sides) fills the cube of the values:
sizeX is the width from hand to hand, sizeY the height and sizeZ the depth (from the back to the toes), the person
looks toward +z of the cube and stands on its bottom (y = sizeY/2 puts the person on the floor). Scale all 3 sizes the
same for another height (a man of 1.70 m is 529/1700/293): size = normal size * height / normal height.
examples/objects_people_sizes.dat has men, women and children of 5 heights each (10 cm smaller, 5 cm smaller, the
normal height, 5 cm bigger and 10 cm bigger) in 3 rows in front of camera 1:

    cp examples/objects_people_sizes.dat /mnt/cameras/camera_objects/
    ./prj-camera-image-creator --objectsFile=objects_people_sizes.dat

| Type | Person | Values | Example |
|---|---|---|---|
| `Man` | 1.80 m, T-shirt, jeans with a belt, short hair | 560/1800/310 | `Man=0/900/3000/560/1800/310/180/0/0/shirt_blue.png` |
| `Woman` | 1.65 m, T-shirt, knee-length skirt, long hair to the shoulders | 490/1650/300 | `Woman=3250/825/-6000/490/1650/300/0/0/0/shirt_red.png` |
| `Child` | 1.20 m (about 7 years), T-shirt, shorts, sneakers, short hair | 360/1200/215 | `Child=2650/600/-5900/360/1200/215/0/0/0/shirt_green.png` |

The example of the man stands 3 m in front of camera 1 and looks at it. The texture is the shirt (images/shirt_blue.png,
shirt_red.png, shirt_white.png, shirt_green.png and shirt_gray.png), the people have hair, eyes, eyebrows, a nose, a
mouth, ears, hands and shoes. A person is one surface number in the data files, the objects file lists the men, the
women and then the children after the sports cars. tools/genobjects.py places 0-60 random people.

# Taking pictures
    prj-camera-image-creator --takePictures

Starts the program, saves the pictures and data files of all cameras exactly like pressing 's'
(into the output directory, from the objects file of objectsFile) and then exits with code 0.
If an unknown argument is given, the program prints the usage and exits with code 1.

    prj-camera-image-creator --takePictures --silent

Does the same without showing the window, --silent can be used only together with --takePictures.

# Saving only the pictures
Key 's' saves the picture (png) and the data file (.dat) of every camera into the output directory, key 'a' saves
only the pictures in the same way (same file names, resolution and lens), without the data files.
The saved pictures and data files never have the camera objects, even if they are shown in the window (key 'c'),
like the training images of prj-camera-image-learner.

# Selecting the objects file
    prj-camera-image-creator --objectsFile=objects1.dat

Uses the given file of the objects directory (`<cameraImageOutputDirectory>/camera_objects`) instead of
objectsFile of configs/general.ini,
it can be used together with --takePictures, for example:

    prj-camera-image-creator --objectsFile=objects5.dat --takePictures

# Showing or hiding the cameras
    prj-camera-image-creator --camerasVisible=0

--camerasVisible=0 hides and --camerasVisible=1 shows the cameras at the start, instead of camerasVisible
of configs/general.ini, key 'c' still toggles them. The label next to the "Hidden areas" label shows "Cameras: shown"
or "Cameras: hidden".

# Showing what the cameras cannot see
Key 'h' shows every surface (floor, cubes, camera objects) and the sky that none of the cameras can see as black,
pressing 'h' again shows them normally. The label next to the "Click a pixel" label shows "Hidden areas: black" or
"Hidden areas: shown". A camera sees a pixel if its lens puts it inside the saved image
(resolution of imageOutputScale, distortion and field of view of cameraObjectsFile) and no nearer surface is
in front of it (the camera objects never block the view, because they are not in the saved pictures).
Saved pictures are not changed by 'h'.

# Distance of a pixel
Left mouse click on a pixel shows its distance in the label at the top left corner (white text on a blue background,
"Click a pixel" before the first click), like in prj-camera-image-viewer:

    <x>/<y> Distance from camera: <distance> mm

x/y is the window pixel from the top left corner and the distance is from the position of the current view (base camera
or camera object) to the surface at that pixel, rendered like the distances of the saved data files. In the camera
object view the text starts with "Camera <number>". "Sky" means no surface at that pixel, "N/A" is outside of the
image of the camera object view.

# Tools

## tools/takepictures.py
Takes the pictures of every .dat file of the objects directory (like `--takePictures --silent --camerasVisible=0 --objectsFile=<file>`)
into the output directory, the windows are not shown. Build the program first with make.

    python3 tools/takepictures.py [--t<number of threads>]

--t<number> runs that many programs at the same time, for example `--t8`, default is 1.
Each program uses about 0.5 GB memory, on a 16 core machine 8 threads was about 7 times faster than 1 thread,
more than 8 threads was not faster.

Previous files of the output directory are first moved to /tmp/image-creator-<timestamp>.
If all pictures are created, the previous files are removed.
If even one objects file fails, the script stops, prints which file failed, removes the new files
and moves the previous files back to the output directory.

## tools/genobjects.py
Generates random objects files into the objects directory. Needs only Python 3.

    python3 tools/genobjects.py <first> <last> [--objectsDirectory=<directory>]

For example `python3 tools/genobjects.py 1 99` writes objects1.dat ... objects99.dat of the objects directory
(existing files with the same names are overwritten), `--objectsDirectory=<directory>` writes them into another
directory instead (objects0.dat is read from there, or created there when it is missing).

Every file has:
- the cubes (sizes and textures) of objects0.dat 3 times (108 objects, BASE_COPIES of the script), every one randomly a
  cube (`Cube=`), a ball (`Ball=`), a cylinder (`Cylinder=`) or a cone (`Cone=`), each with the chance 1/4 (SHAPES of
  the script), placed randomly around 0/0/0 at 1.5-30 m distance with random yaw, pitch and roll, every object touches the floor and can be partly under
  it, they do not overlap,
- all four have the same values x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture: x/y/z is the center,
  sizeX/sizeY/sizeZ the width, height and depth, yaw/pitch/roll turn it like a cube and the texture is one of the
  wood and brick images of objects0.dat (images folder). A ball is the ellipsoid that fits into the cube of the same
  values (the sizes are its diameters), a cylinder fits into it with its axis along the y axis of the cube (sizeY is
  its height, sizeX and sizeZ the diameters of its elliptic ends) and a cone fits into it with its base (the
  ellipse of the diameters sizeX and sizeZ) at the bottom of the cube (-y) and its tip at the middle of the top
  (+y), for example
  `Ball=7290.0/270.7/1310.0/1440.0/720.0/580.0/78.0/152.0/10.5/wood3.png`,
  `Cylinder=-2610.0/271.5/-410.0/520.0/790.0/560.0/174.4/354.8/84.5/wood5.png` and
  `Cone=-1350.0/418.0/1530.0/440.0/710.0/740.0/241.3/33.0/41.4/wood2.png`,
- 0-30 cars, each randomly a sedan (`Car=`), an SUV (`Suv=`) or a sports car (`SportCar=`) with the chance 1/3,
  5 % smaller ... 5 % bigger than its normal size, a random paint (car_*.png), and 0-60 people, each randomly a man
  (`Man=`), a woman (`Woman=`) or a child (`Child=`) with the chance 1/3, 10 cm smaller ... 10 cm bigger than the
  normal height (all sizes scaled the same), a random shirt (shirt_*.png). They stand still on the floor (pitch and
  roll 0) with a random heading (yaw), randomly around 0/0/0 at 1.5-30 m distance, for example
  `Car=2140.0/707.2/-12130.0/1951.0/1414.5/4487.3/273.9/0.0/0.0/car_white.png` and
  `Woman=1540.0/778.0/26570.0/462.1/1556.1/282.9/233.8/0.0/0.0/shirt_blue.png`,
- nothing touches a car or a person: the cubes, balls, cylinders and cones are at least 10 cm from the cars and at
  least 1 cm from the people, the people and the cars are at least 1 cm from each other (the distances are between
  the boxes of the objects, so the real objects are at least this far),
- every object is at least 1000 mm from the cameraObjectPosition<number> positions of configs/cameraobjects.ini,
- every cube, ball, cylinder and cone is visible from the position 0/1000/0 mm (at least 30% of its surface seen from
  there past the other cubes, balls, cylinders and cones),
- 1-3 white spot lights with random positions and directions, each lights at least 5 cubes, balls, cylinders or
  cones (the cars and the people cast shadows too),
- ambient light 0.1-0.5.

The result depends only on the file number, so the same number always gives the same file. Every object is one
surface number in the data files: a cube has 6 flat faces, the normals of a ball change smoothly over it, a cylinder
has a smooth side and 2 flat ends, a cone a smooth side and a flat base, a car or a person is one surface too.
Use a file with `objectsFile=objects1.dat` in configs/general.ini.

If objects0.dat is missing from the objects directory, it is created first (and printed as
`objects0.dat (it was missing)`), it has:
- 9 random cubes in each of the 4 corners (-x/-z, +x/-z, -x/+z, +x/+z), 36 cubes, at 1.5-15 m distance from 0/0/0,
  every side 300-1500 mm, random texture (wood1-6.png, brick1-2.png), random yaw, pitch and roll,
  every cube touches the floor and can be partly under it, cubes do not overlap and they are at least 1000 mm from
  the cameraObjectPosition<number> positions,
- 1-5 white spot lights with random positions and directions, each lights at least 5 cubes,
- ambient light 0.1-0.5.

The created objects0.dat is always the same. An existing objects0.dat is never changed.
