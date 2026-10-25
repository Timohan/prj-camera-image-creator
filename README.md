# prj-camera-image-creator
Camera image creator

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

# Taking pictures without the window
    prj-camera-image-creator --takePictures

Starts the program, saves the pictures and data files of all cameras exactly like pressing 's'
(into outputDirectory of configs/general.ini, from the objects file of objectsFile) and then exits with code 0.
If an unknown argument is given, the program prints the usage and exits with code 1.

# Selecting the objects file
    prj-camera-image-creator --objectsFile=objects1.dat

Uses the given file of the objects folder instead of objectsFile of configs/general.ini,
it can be used together with --takePictures, for example:

    prj-camera-image-creator --objectsFile=objects5.dat --takePictures

# Showing or hiding the cameras
    prj-camera-image-creator --camerasVisible=0

--camerasVisible=0 hides and --camerasVisible=1 shows the cameras at the start, instead of camerasVisible
of configs/general.ini, key 'c' still toggles them.

# Tools

## tools/takepictures.py
Takes the pictures of every .dat file of the objects folder (like `--takePictures --camerasVisible=0 --objectsFile=<file>`)
into outputDirectory of configs/general.ini. Build the program first with make.

    python3 tools/takepictures.py [--t<number of threads>]

--t<number> runs that many programs at the same time, for example `--t8`, default is 1.
Each program uses about 0.5 GB memory, on a 16 core machine 8 threads was about 7 times faster than 1 thread,
more than 8 threads was not faster.

Previous files of outputDirectory are first moved to /tmp/image-creator-<timestamp>.
If all pictures are created, the previous files are removed.
If even one objects file fails, the script stops, prints which file failed, removes the new files
and moves the previous files back to outputDirectory.

## tools/genobjects.py
Generates random objects files into the objects folder. Needs only Python 3.

    python3 tools/genobjects.py <first> <last>

For example `python3 tools/genobjects.py 1 99` writes objects/objects1.dat ... objects/objects99.dat
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
