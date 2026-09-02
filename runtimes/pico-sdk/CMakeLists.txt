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
    # RP2040-E5 workaround. dcd_rp2040 then forces ~1ms of idle J-state after a
    # bus reset, which the errata needs to leave RESET on a busy host. Targets
    # the clone's fail-to-re-enumerate-on-RESET symptom. Uses GPIO15 briefly.
    PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

# Soft-replay: -DDUCKY_REPLAY_MS=<ms> makes the firmware re-enumerate itself that
# long after the payload ends, replaying it with no reset or replug. Unset = off.
if(DEFINED DUCKY_REPLAY_MS)
    target_compile_definitions(rp2040_ducky PRIVATE DUCKY_REPLAY_MS=${DUCKY_REPLAY_MS})
endif()

target_link_libraries(rp2040_ducky
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(rp2040_ducky)
