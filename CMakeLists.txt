# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_BUILD_TYPE Debug)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD seeed_xiao_rp2040 CACHE STRING "Board type")
set(TINYUSB_OPT_OS OPT_OS_FREERTOS CACHE STRING "TinyUSB OS")

#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(PICO_DEOPTIMIZED_DEBUG 1)
#endif()

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Include FreeRTOS Kernel
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ext/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040)

project(xiao_sensor_servo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(xiao_sensor_servo)

pico_set_program_name(xiao_sensor_servo "Xiao Sensor Servo")
pico_set_program_version(xiao_sensor_servo "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(xiao_sensor_servo 1)
pico_enable_stdio_usb(xiao_sensor_servo 0)

# Add sources
target_sources(xiao_sensor_servo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/debug.c
    ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hid/hid_device.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hid/lamp_array.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lighting/neopixel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lighting/status_led.c
    ${CMAKE_CURRENT_LIST_DIR}/src/uart/cdc_device.c
    ${CMAKE_CURRENT_LIST_DIR}/src/uart/pio_uart.c
)

# Generate PIO header
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/uart/uart_rx.pio)
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/uart/uart_tx.pio)
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/lighting/ws2812.pio)

# Add the standard include files to the build
target_include_directories(xiao_sensor_servo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(xiao_sensor_servo 
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    tinyusb_device
    tinyusb_board
    pico_stdlib
    pico_unique_id
    hardware_irq
    hardware_dma
    hardware_pio
    hardware_watchdog
)

pico_add_extra_outputs(xiao_sensor_servo)

