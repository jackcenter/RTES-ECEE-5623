#!/bin/bash

mkdir -p build
cd build || exit 1

cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .