## obs-aruco-tracker

Track ArUco markers and output movement commands to re-center the marker.

### v1.0.0

![Screenshot](docs/obs.png)

![Youtube video with explanation](http://img.youtube.com/vi/Wq0YTLhK8NY/0.jpg)](http://www.youtube.com/watch?v=Wq0YTLhK8NY "Camera Gimbal")

### MODULES

#### ArUco Tracker Filter

Add to a video source to generate camera gimbal direction output used to re-center the marker.

Generate and Print markers from here: [ArUco Marker generator](http://chev.me/arucogen/), use the 4x4 dictionary.

Find an example Marker in the `docs/` subdir:

![ArUco 4x4 ID 15](docs/4x4_1000-15.svg)

#### REQUIREMENTS

* *obs-studio*
* *openCV 4*

#### INSTALLATION

##### For ArchLinux:

Use the included PKGBUILD


##### For installation from source (Linux):

Required: OpenCV and OBS-Studio + dev packages

```bash
git clone https://github.com/dunkelstern/obs-aruco-tracker
cd obs-aruco-tracker
mkdir build
cd build
cmake .. -DSYSTEM_INSTALL=0
make
make install
```

for local installation or `cmake .. -DSYSTEM_INSTALL=1` for system installation

##### Installation from Source (OSX):

Plugin has not been portet to OSX yet, if you want to try, start with the Linux version.

##### Installation from Source (Windows):

You will need a working build of obs:

Required: Visual Studio 2017, CMake

1. Checkout the source from https://github.com/obsproject/obs-studio
2. Create build directory
3. Download dependencies from https://obsproject.com/downloads/dependencies2017.zip
4. Create Visual Studio solution with CMake:
    ```powershell
    cd build
    cmake .. -DDepsPath=/path/to/deps -DDISABLE_UI=1 -G "VisualStudio 15 2017 64bit"
    ```
5. Build OBS

You will need a working build of openCV:

Required: Visual Studio 2017 or 2019, CMake

1. Get VcPkg https://github.com/microsoft/vcpkg (follow the quick start guide)
2. Compile openCV: `vcpkg install opencv[contrib]`
3. Export openCV `vcpkg export opencv --zip`
4. Unpack the zip somewhere where you can find it

Now we can build the plugin:

Required: Visual Studio 2019, CMake

```powershell
cd obs-aruco-tracker
mkdir build
cd build
cmake .. \
    -DSYSTEM_INSTALL=0 \
    -DCMAKE_TOOLCHAIN_FILE="$HOME\Code\obs-aruco-tracker\build\opencv\scripts\buildsystems\vcpkg.cmake" \
    -DLIBOBS_LIB="$HOME\Code\obs-studio\build\libobs\Debug\obs.lib" \
    -DLIBOBS_INCLUDE_DIR="$HOME\Code\obs-studio\libobs"
    -DW32_PTHREADS_LIB="$HOME\Code\obs-studio\build\deps\w32-pthreads\Debug\w32-pthreads.lib"
    -G "Visual Studio 16" -A x64
```

As you can see I build my stuff in `$HOME\Code\*`, I unpacked the `VcPkg` zip directly into the build dir and renamed it to `opencv`.

#### Arduino sketch

The Arduino sketch is used for controlling a camera gimbal (nicknamed "Klein Glotzi").

You will need the Library [PWMServo](https://github.com/PaulStoffregen/PWMServo) for the sketch to work.
Before flashing the Sketch make sure that the calibration values are good for your
Servos. (See the Sketch for more information)


#### "Klein Glotzi" Camera gimbal

![Klein Glotzi](docs/glotzi.png)

- The plans for the Gimbal can be seen here: [Klein Glotzi on OnShape CAD](https://cad.onshape.com/documents/6b43bf9158c2330bf8f3274f/w/bf932df38055a7275eddfee5/e/89a3396a8a0f5c6514141639)
- The STL Files for 3D-Printing the gimbal are in the STL Subfolder.
- In addition to an Arduino Uno (or Nano) you will need some wires and two Tower SG90 Micro Servos.
- The Gimbal mount is built for a MS Lifecam, so you probably want one of those too.
