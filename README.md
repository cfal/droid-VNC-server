Fork of droid-VNC-server that uses openstf's minicap shared library for screen capture, with some additional features.
WIP - Currently only works as a VNC viewer

## Original docs

The droid-VNC-server projects consists in three main modules parts: the daemon, wrapper libs and the GUI.

## Daemon

Provides the vnc server functionality, injects input/touch events, clipboard management, etc
Available in jni/ folder

## Wrapper libs

Compiled against the AOSP so everyone can build the daemon/GUI without having to fetch +2GB files.
Currently there are 2 wrappers, gralloc and flinger.

Available in nativeMethods/ folder, and precompiled libs in nativeMethods/lib/

## GUI

GUI handles user-friendly control of the daemon. Connects to the daemon using local IPC.
Should use `vnc-build.sh` and `vnc-daemon.sh` instead though.

## Compiling

Use the `vnc-build.sh` script to build the daemon and wrapper libs and to deploy to a connected device.

Options:

    - `-w`: Compile the wrapper libs
    - `-s`: Skip deploy to device
    - `-a SDK`: Specify which SDK number to use

Pushes the androidvncserver and wrapper lib to /data/local sets the proper permissions.

## Starting the daemon

Use the `vnc-daemon.sh` script to control the daemon.

Options:

    - `-s VAL`: Set the scale. Defaults to 100.
    - `-m VAL': Choose the screen capture method, gralloc or flinger (default).

Arguments:

    - `start`
    - `stop`
    - `restart`

## Building the GUI App

It shouldn't be necessary to use the GUI app to control the daemon, but if
you must, here are the steps:

### Compile C daemon

In project folder:

  1. `$ ndk-build`
  1. `$ ./updateExecsAndLibs.sh`

### Compile Wrapper libs

Setup:

    1. `$ cd <aosp_folder>`
    1. `$ . build/envsetup.sh`
    1. `$ lunch`
    1. `$ ln -s <droid-vnc-folder>/nativeMethods/ external/`

To build:

    1. `$ cd external/nativeMethods`
    1. `$ mm .`
    1. `$ cd <droid-vnc-folder>`
    1. `$ ./updateExecsAndLibs.sh`

### Compile GUI

Use ant to build and deploy.

`$ ant debug install`
