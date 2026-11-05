# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

# Use optimized compiler settings for release builds unless the user selected a type.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to the Raspberry Pi Pico SDK directory.")
endif()

set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(pico_usb_mouse_firmware VERSION 1.0.0 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

# Pico-PIO-USB adds a second USB port in host mode using PIO.
# Clone https://github.com/sekigon-gonnoc/Pico-PIO-USB into:
#   <project>/lib/Pico-PIO-USB
set(PICO_PIO_USB_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB" CACHE PATH
    "Path to the Pico-PIO-USB repository")

if(NOT EXISTS "${PICO_PIO_USB_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "Pico-PIO-USB was not found at '${PICO_PIO_USB_PATH}'. "
        "Clone https://github.com/sekigon-gonnoc/Pico-PIO-USB into lib/Pico-PIO-USB.")
endif()

add_subdirectory("${PICO_PIO_USB_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/Pico-PIO-USB")

add_executable(pico_usb_mouse_firmware
    main.c
    # TinyUSB DCD/HCD backend for Pico-PIO-USB (native device + PIO host).
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

# Compile the WS2812 timing program for the RP2040-Zero onboard RGB LED.
pico_generate_pio_header(pico_usb_mouse_firmware
    ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
)

target_include_directories(pico_usb_mouse_firmware PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_usb_mouse_firmware PRIVATE
    pico_stdlib
    pico_multicore
    hardware_clocks
    hardware_pio
    tinyusb_device
    tinyusb_host
    pico_pio_usb
)

# Required by Pico-PIO-USB's TinyUSB backend.
target_compile_definitions(pico_usb_mouse_firmware PRIVATE
    PIO_USB_USE_TINYUSB
)

pico_enable_stdio_usb(pico_usb_mouse_firmware 0)
pico_enable_stdio_uart(pico_usb_mouse_firmware 0)
pico_add_extra_outputs(pico_usb_mouse_firmware)
