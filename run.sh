#!/bin/bash

set -e

BUILD_DIR="./build"
USE_ORB=${1:-OFF}

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -DUSE_ORB=$USE_ORB

cmake --build .

./bin/VerletSimulator

