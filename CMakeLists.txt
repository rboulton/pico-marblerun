cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
include(pimoroni_pico_import.cmake)

project(lights C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(lights
    main.cpp
)

include(common/pimoroni_i2c)
include(pimoroni-pico/libraries/plasma2040/plasma2040.cmake)
include(pimoroni-pico/drivers/rgbled/rgbled.cmake)
include(pimoroni-pico/drivers/button/button.cmake)
include(pimoroni-pico/drivers/analog/analog.cmake)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(lights
    pico_stdlib
    plasma2040
    rgbled
    button
    analog
)

# enable usb output
pico_enable_stdio_usb(lights 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(lights)
