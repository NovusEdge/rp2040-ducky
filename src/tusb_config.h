#pragma once

// HID-only build: no CDC, no MSC. The target enumerates a keyboard and
// nothing else, so no drive appears on it.

// The SDK defines PICO_RP2350 for a pico2/RP2350 target and PICO_RP2040 otherwise.
#if defined(PICO_RP2350) && PICO_RP2350
#define CFG_TUSB_MCU            OPT_MCU_RP2350
#else
#define CFG_TUSB_MCU            OPT_MCU_RP2040
#endif
#define CFG_TUSB_OS             OPT_OS_PICO

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_ENABLED         1

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif

#define CFG_TUD_HID             1
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_HID_EP_BUFSIZE  16
