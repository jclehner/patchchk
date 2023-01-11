#!/bin/sh

CMAKE_ARGS=
BUILD_DIR=build

if [ "$1" = "mingw" ]; then
	CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=i686-w64-mingw32.cmake"
	BUILD_DIR=build-win32
fi

rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"
cmake $CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release ..
make package
