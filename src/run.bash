#!/bin/bash

PROJECT_DIR="$(git rev-parse --show-toplevel)"

if [ ! $? -eq 0 ]; then
    echo "For whatever reason, project isn't being built as a git repository. Assuming current directory is the project dir."

    PROJECT_DIR=$(pwd)
fi

BUILD_DIR=$PROJECT_DIR/build
FILE=$PROJECT_DIR/example.ls

$BUILD_DIR/loaf $FILE
