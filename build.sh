#!/bin/sh
if [ -d build ]; then
    rm build -rf
fi
mkdir build
cd build
cmake ..
make -j5
cp rinha_cpp /app/
if [ "$1" = "deploy" ]; then
    echo "Deploying..."
    ./rinha_cpp
fi