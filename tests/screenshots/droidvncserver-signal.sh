#!/bin/bash

./prepare.sh
adb shell LD_LIBRARY_PATH=/data/local/tmp exec /data/local/tmp/androidvncserver -U
adb pull /data/local/tmp/screen.png tmp/droidvncserver-signal.png

