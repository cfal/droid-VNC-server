#!/bin/bash

./prepare.sh
adb shell LD_LIBRARY_PATH=/data/local/tmp exec /data/local/tmp/androidvncserver -S /sdcard/droidvncserver.png
adb pull /sdcard/droidvncserver.png tmp/
