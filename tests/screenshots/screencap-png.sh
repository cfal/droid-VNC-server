#!/bin/bash

./prepare.sh
adb shell exec screencap /sdcard/screencap.png
adb pull /sdcard/screencap.png tmp/
