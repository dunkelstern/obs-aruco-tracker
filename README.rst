=================
obs-aruco-tracker
=================

Track ArUco markers and output movement commands to re-center the marker.

------
v0.1.0
------

MODULES
=======
ArUco Tracker Filter
--------------------
   Add to a video source to generate camera gimbal direction output used to re-center the marker.

REQUIREMENTS
============

* *obs-studio*
* *openCV 4*

INSTALLATION
============

For ArchLinux:
--------------

Use the included PKGBUILD


For installation from source:
=============================

General:
--------
* :code:`git clone https://github.com/dunkelstern/obs-aruco-tracker`
* :code:`cd obs-aruco-tracker`
* :code:`mkdir build`
* :code:`cd build`
* :code:`cmake .. -DSYSTEM_INSTALL=0` for local installation or :code:`cmake . -DSYSTEM_INSTALL=1` for system installation
* :code:`make`
* :code:`make install`
