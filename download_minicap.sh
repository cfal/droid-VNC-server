#!/bin/bash
DIR="$(dirname "$(readlink "$0" || echo "$0")")"
mkdir -p $DIR/tmp && \
echo "Checking out minicap.." && \
git clone https://github.com/openstf/minicap $DIR/tmp/minicap && \
echo "Copying shared libraries.." && \
cp -vrf $DIR/tmp/minicap/jni/minicap-shared $DIR/jni/ && \
echo "Cleaning up.." && \
rm -rf $DIR/tmp && \
echo "Done."
