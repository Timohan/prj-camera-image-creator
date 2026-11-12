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
cameraImageOutputDirectory of configs/general.ini (/mnt/camera) has two directories:
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
   Example when /mnt/camera is empty:

   Non-fisheye, set in configs/general.ini:

       cameraImageOutputDirectory=/mnt/camera
       cameraObjectsFile=cameraobjects_ids.ini
       objectsFile=objects1.dat

   and create the objects files:

       python3 tools/genobjects.py 1 10

   It prints `objects0.dat (it was missing)` and `objects1.dat` ... `objects10.dat`, and creates
   /mnt/camera/camera_objects/objects0.dat (the cubes that the other files place randomly) and objects1.dat ...
   objects10.dat.

   Fisheye, set in configs/general.ini:

       cameraImageOutputDirectory=/mnt/camera/fisheye
       cameraObjectsFile=cameraobjects_elp.ini
       objectsFile=objects1.dat

   and run the same command:

       python3 tools/genobjects.py 1 10

   It creates the same files to /mnt/camera/fisheye/camera_objects: the same number always gives the same file,
   so objects1.dat is the same scene for both lenses and their pictures can be compared. The directories are
   created when they are missing.
4. Open the creator:

       ./prj-camera-image-creator

   It opens the objects file of objectsFile (objects1.dat above) from the camera_objects directory of
   cameraImageOutputDirectory, with the cameras of cameraObjectsFile, so with the fisheye settings of step 3 it opens
   /mnt/camera/fisheye/camera_objects/objects1.dat with the fisheye cameras, and with the non-fisheye settings
   /mnt/camera/camera_objects/objects1.dat with the IDS cameras.

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

    python3 tools/genobjects.py <first> <last>

For example `python3 tools/genobjects.py 1 99` writes objects1.dat ... objects99.dat of the objects directory
(existing files with the same names are overwritten).

Every file has:
- the cubes (sizes and textures) of objects0.dat, placed randomly around 0/0/0 at 1.5-15 m distance with random yaw, pitch and roll,
  every cube touches the floor and can be partly under it, cubes do not overlap,
- every cube is at least 1000 mm from the cameraObjectPosition<number> positions of configs/cameraobjects.ini,
- every cube is visible from the position 0/1000/0 mm (at least 30% of its faces seen from there),
- 1-3 white spot lights with random positions and directions, each lights at least 5 cubes,
- ambient light 0.1-0.5.

The result depends only on the file number, so the same number always gives the same file.
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
