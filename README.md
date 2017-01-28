# Android VNC Viewer

This is an Android VNC server for Android 4.4 to 7.1 (lower Android versions may work, but untested). Originally forked from [droidVncServer](https://github.com/oNaiPs/droidVncServer), this project has been rewritten in C++ and currently only supports screen capture (ie. it cannot manipulate the device). It uses [libvncserver](https://libvnc.github.io) for VNC server capabilities, and the OpenSBF project's [minicap-shared](/opensbf/minicap/) native library as an interface for capturing frames.

Features:

- Fast screenshot
- Color degradation
- Image scaling
- Orientation change support
- Forced orientation support

This project cannot:

- Be started from the phone itself
- Control the phone

### Force screen rotation or orientation (`-r`, `-o`)

This can force the VNC viewer to display a screen rotated at 0, 90 or 270 degrees, or in landscape or portrait orientation.

If the device is in landscape mode, and portrait orientation is forced, the VNC server will transform the image to display the screen sideways.

As another example, forcing landscape orientation means that a device rotated at 90 or 270 degrees will be displayed upright by the VNC server. Forcing a 90 degree rotation means that a device rotated at 90 degrees will be displayed upright, while a device rotated at 270 degrees will instead be displayed upside down.

On some ZTE devices, the `-z` flag is necessary for the rotation or orientation to be set correctly.

### Orientation change support

When VNC screen rotation/orientation is not enforced, the VNC server will automatically resize its screen to mirror the device's current orientation. Detection of orientation changes is done using the OpenSBF project's [RotationWatcher.apk](/opensbf/RotationWatcher.apk).

### Android screenshot (`-S`, `-U`)

Starting the binary with the `-S` flag will take a screenshot instead of starting the VNC server. Depending on the filename, the saved screenshot will be encoded in JPEG or PNG format. JPEG encoding is done using the included `libjpeg-turbo` library, while PNG encoding is done using the `libpng` library.

When the VNC server is running on the phone, starting the VNC server binary with the `-U` flag will instead send a signal to the VNC server and cause it to save the current frame to `/data/local/tmp/screen.png`. This approach is much faster than running `screencap` on the Android device to take a screenshot.

On some Motorola devices, the `-X` flag is necessary to skip the first frame (which is always black), when a running VNC server is not found for fast screenshot to work.

### Screen scaling (`-s`)

By default, the VNC screen size will be the same as the device screen size (ie. 100%). This allows for scaling the VNC screen to a larger or smaller size.

### Color degradation (`-b`)

The `-b` flag allows you to specify the number of bytes per pixel (bpp). By default, bpp will be the same as on the device. The following color formats are used:

- bpp 1: RGB 332
- bpp 2: RGB 565
- bpp 4: RGBA 8888
- bpp 8: R16G16B16A16

## Build requirements

- [Android NDK](https://developer.android.com/ndk/index.html) must be installed. If using the `run.sh` script, `ndk-build` must be in your path.
- Android platform-tools must be installed and `adb` must be in your path to push files to the target device. See the [Android Developers](https://developer.android.com/index.html) website for details.

## Building and running

1. Run `download_minicap.sh` to download the prebuilt `minicap-shared` libraries from the [minicap](/openstf/minicap) repo.
2. `git clone https://github.com/openstf/RotationWatcher.apk`
3. Run `./gradlew assembleDebug` to build the RotationWatcher app.
4. Copy the built apk to the root of this repo as `rotate.apk`
5. Connect the target device with debugging enabled.
6. Run the `run.sh` script.

Note that the `run.sh` script only expects a single device connected to the computer. It will be necessary to modify the script and use the `-s` flag with `adb` commands to target a specific device if multiple devices are connected.