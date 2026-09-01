cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rp2040_ducky C CXX ASM)

pico_sdk_init()

add_executable(rp2040_ducky
    main.c
    src/usb_descriptors.c
    src/ducky.c
)

# ducky.h and payloads/ sit in the root; tusb_config.h in src/.
target_include_directories(rp2040_ducky PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Selects which payload compiles in: -DPAYLOAD=<os>/<name>.
if(NOT DEFINED PAYLOAD)
    set(PAYLOAD "linux/rickroll")
endif()
target_compile_definitions(rp2040_ducky PRIVATE
    PAYLOAD_HEADER="payloads/${PAYLOAD}/payload.h"
)

target_link_libraries(rp2040_ducky
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(rp2040_ducky)
