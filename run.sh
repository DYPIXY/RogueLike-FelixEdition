#!/bin/bash
cmake -S. -B/tmp/build
cd /tmp/build
make -j$(nproc)
cd -
/tmp/build/dest
