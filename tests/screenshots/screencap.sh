#!/bin/bash

./prepare.sh
adb shell exec screencap /sdcard/screencap.data
adb pull /sdcard/screencap.data tmp/
# TODO: conversion to png
