#!/bin/bash

dir=/data/local/tmp

set -exo pipefail

adb uninstall rotation.watcher
adb shell rm -vf $dir/androidvncserver $dir/rotate.apk $dir/minicap.so

set -o pipefail

echo "Done."

