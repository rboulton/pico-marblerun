#!/bin/sh
# Checkout the pico-sdk in a peer directory to this project.
# Make sure it submodules are initted

mkdir build
cd build
cmake .. -DPICO_SDK_PATH=../../pico-sdk -DPICO_BOARD=pimoroni_plasma2040
