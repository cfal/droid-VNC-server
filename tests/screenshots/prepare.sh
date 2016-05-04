#!/bin/bash
SCREEN_IS_ON=$(adb shell dumpsys input_method | egrep 'm(ScreenOn|Interactive)=true')
if [[ $SCREEN_IS_ON == "" ]]; then
    echo "Turning screen on.."
    adb shell input keyevent KEYCODE_POWER
else
    echo "Screen is already on"
fi

mkdir -p tmp
