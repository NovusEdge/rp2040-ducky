#pragma once

// WHAT THIS FILE IS
// TinyUSB is the USB software stack. It is a big library. It can act as many
// kinds of USB device: a keyboard, a serial port, a disk, a MIDI instrument.
// You do not want all of that. This file is the set of switches that tells
// TinyUSB what to build. TinyUSB reads these #define values at compile time and
// leaves out every part you set to 0.

// HID-only build with no CDC and no MSC. The target enumerates a keyboard
// only. No drive appears on the target.
//
// TEACHING NOTE
// HID means "Human Interface Device". A keyboard is one. CDC means a serial
// port. MSC means a mass-storage disk (the CIRCUITPY drive is MSC). This ducky
// wants to be a keyboard and nothing else. A disk or a serial port would be a
// second thing the host sees, and you do not want the target to notice a new
// drive appear.

// The SDK defines PICO_RP2350 for a pico2/RP2350 target and PICO_RP2040
// otherwise.
//
// TEACHING NOTE
// One firmware source builds for two chips. The RP2040 is the original Pico
// chip. The RP2350 is the newer Pico 2 chip. Their USB hardware differs a
// little, so TinyUSB needs to know which one it runs on. The #if picks the
// right value on its own from a flag the SDK sets.
#if defined(PICO_RP2350) && PICO_RP2350
#define CFG_TUSB_MCU OPT_MCU_RP2350
#else
#define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

// CFG_TUSB_OS says which operating system TinyUSB runs under. OPT_OS_PICO means
// "no full OS, just the Pico SDK". TinyUSB then uses SDK timers and no threads.
#define CFG_TUSB_OS OPT_OS_PICO

// RHPORT0 is the one USB port on the board (the physical micro-USB socket).
// OPT_MODE_DEVICE means the board acts as a device, not a host. A keyboard is a
// device; the PC is the host. FULL_SPEED means 12 Mbit/s, the normal speed for
// a keyboard.
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// Turn the device side of the stack on. TUD means "TinyUSB Device".
#define CFG_TUD_ENABLED 1

// Endpoint 0 is the control channel every USB device must have. The host uses
// it to ask the device who it is. 64 bytes is the standard max packet size for
// a full-speed device.
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

// The class switches. HID is on. Everything else is off. Each 0 drops code and
// removes one thing the host would otherwise see.
#define CFG_TUD_HID 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

// The size of the buffer for one HID report. A keyboard report is 8 bytes: one
// byte of modifiers (Shift, Ctrl...), one reserved byte, and six key slots. 16
// leaves room to spare.
#define CFG_TUD_HID_EP_BUFSIZE 16
