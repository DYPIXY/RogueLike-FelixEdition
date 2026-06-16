#!/bin/bash
cmake -S. -Bbuild
cd build
make -j$(nproc)
cd ..
./build/m3
