#!/bin/bash
# This test should probably be run with the screen on the whole time for comparable results.
echo "Cleaning up.."
rm -vf tmp/*
echo "Starting.."
echo "---------- DEFAULT SCREENCAP METHOD ----------"
time ./screencap.sh
echo "---------- SCREENSHOT PNG METHOD ----------"
time ./screencap-png.sh
echo "---------- DEFAULT DROIDVNCSERVER METHOD ----------"
time ./droidvncserver.sh
echo "---------- DROIDVNCSERVER SIGNAL METHOD ---------- "
time ./droidvncserver-signal.sh
echo "All done."
