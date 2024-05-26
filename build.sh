#!/bin/sh

if [ ! -e pico-sdk ]; then
  git clone https://github.com/raspberrypi/pico-sdk
  cd pico-sdk
  git submodule update --init
  cd ..
fi
export PICO_SDK_PATH=`pwd`/pico-sdk

if [ ! -e pimoroni-pico ]; then
  git clone https://github.com/pimoroni/pimoroni-pico
  cd pimoroni-pico
  git submodule update --init
  cd ..
fi
export PIMORONI_PICO_PATH=`pwd`/pico-sdk

mkdir build
cd build
#cmake -DCMAKE_BUILD_TYPE=Debug .. -DPICO_BOARD=pimoroni_plasma2040
cmake .. -DPICO_BOARD=pimoroni_plasma2040
make
