#!/bin/bash

# Quick function which exits if the last command ran fails. Intended to be used
# after compiling something.
function compileCheckError() {
    if [ ! $? -eq 0 ]; then
        echo "Compilation failed, exiting."

        exit 1
    fi
}

PROJECT_DIR="$(git rev-parse --show-toplevel)"

if [ ! $? -eq 0 ]; then
    echo "For whatever reason, project isn't being built as a git repository. Assuming current directory is the project dir."

    PROJECT_DIR=$(pwd)
fi

BUILD_DIR=$PROJECT_DIR/build
SRC_DIR=$PROJECT_DIR/src
VENDOR_DIR=$PROJECT_DIR/vendor

USLIB_FLAGS="-I$VENDOR_DIR/uslib"

ls $SRC_DIR

GCC="gcc"
GPP="g++ -Wall -Werror -std=c++11 -g"

START_TIME=$(date +%s)

mkdir -p $BUILD_DIR
pushd $BUILD_DIR

echo "Building program..."
$GPP -o loaf -I$SRC_DIR $SRC_DIR/main.cpp $USLIB_FLAGS

echo "Done!"

END_TIME=$(date +%s)
TOTAL_TIME=$(expr $END_TIME - $START_TIME)

echo "Took to build project: $TOTAL_TIME"

popd
