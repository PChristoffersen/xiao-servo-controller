# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
   set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_SDK_PATH ${USERHOME}/.pico-sdk/sdk/2.0.0)
set(PICO_TOOLCHAIN_PATH ${USERHOME}/.pico-sdk/toolchain/13_2_Rel1)
set(pioasm_HINT ${USERHOME}/.pico-sdk/tools/2.0.0/pioasm)
if(EXISTS ${pioasm_HINT})
    set(pioasm_DIR ${pioasm_HINT})
endif()
set(picotool_HINT ${USERHOME}/.pico-sdk/picotool/2.0.0/picotool)
if(EXISTS ${picotool_HINT})
    set(picotool_DIR ${picotool_HINT})
endif()
if(PICO_TOOLCHAIN_PATH MATCHES "RISCV")
    set(PICO_PLATFORM rp2350-riscv CACHE STRING "Pico Platform")
    if(PICO_TOOLCHAIN_PATH MATCHES "COREV")
        set(PICO_COMPILER pico_riscv_gcc_zcb_zcmp)
    endif()
endif()
# ====================================================================================
set(PICO_BOARD seeed_xiao_rp2040 CACHE STRING "Board type")
set(TINYUSB_OPT_OS OPT_OS_FREERTOS CACHE STRING "TinyUSB OS")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Include FreeRTOS Kernel
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ext/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040)

project(xiao_sensor_servo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(xiao_sensor_servo)

pico_set_program_name(xiao_sensor_servo "xiao_sensor_servo")
pico_set_program_version(xiao_sensor_servo "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(xiao_sensor_servo 1)
pico_enable_stdio_usb(xiao_sensor_servo 0)

# Add sources
target_sources(xiao_sensor_servo PRIVATE
    src/main.c
)

# Generate PIO header
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/uart/uart_rx.pio)
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/uart/uart_tx.pio)
pico_generate_pio_header(xiao_sensor_servo ${CMAKE_CURRENT_LIST_DIR}/src/lighting/ws2812.pio)

# Add the standard library to the build
target_link_libraries(xiao_sensor_servo
    pico_stdlib
)

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
    hardware_pwm
    hardware_pio
)

pico_add_extra_outputs(xiao_sensor_servo)

