#pragma once

// WHAT THIS FILE IS
// This header is the data model for a payload. A payload is what the ducky
// types. Here a payload is a plain C array of steps. Each step is one of three
// kinds: type a string, press one key combo, or wait. The runner in ducky.c
// walks this array. This file also gives you short macros so a payload reads
// almost like a script.

#include <stddef.h>
#include <stdint.h>
#include "tusb.h"

// The three kinds of step. An enum is a named integer. The runner switches on
// this value to decide what a step does.
typedef enum { STEP_STRING, STEP_KEY, STEP_DELAY } step_kind_t;

// One step. A step carries the fields for every kind, but only the fields for
// its own kind hold real values. This wastes a few bytes per step and keeps the
// type simple. A STEP_STRING uses str. A STEP_KEY uses modifier and keycode. A
// STEP_DELAY uses ms.
typedef struct {
    step_kind_t kind;
    const char *str;    // STEP_STRING: the text to type
    uint8_t modifier;   // STEP_KEY: Shift/Ctrl/Alt/Gui bits, or 0
    uint8_t keycode;    // STEP_KEY: which key, as a USB HID keycode
    uint32_t ms;        // STEP_DELAY: how long to wait, in milliseconds
} duck_step_t;

// THE PAYLOAD MACROS
// C lets you fill a struct by field name. These macros hide that syntax so a
// payload file reads like a list of actions. A payload uses them like:
//   STR("hello"), KEY(MOD_GUI, KEY_R), DELAY(500)
// The .kind = ... syntax is a designated initializer: it sets one named field
// and leaves the rest zero.
#define STR(s)    {.kind = STEP_STRING, .str = (s)}
#define KEY(m, k) {.kind = STEP_KEY, .modifier = (m), .keycode = (k)}
#define DELAY(t)  {.kind = STEP_DELAY, .ms = (t)}

// Friendly names for the modifier bits. A modifier is a key you hold while you
// press another, like Shift or Ctrl. The values come from TinyUSB. GUI is the
// Windows key or the Mac Command key.
#define MOD_CTRL  KEYBOARD_MODIFIER_LEFTCTRL
#define MOD_SHIFT KEYBOARD_MODIFIER_LEFTSHIFT
#define MOD_ALT   KEYBOARD_MODIFIER_LEFTALT
#define MOD_GUI   KEYBOARD_MODIFIER_LEFTGUI

// Friendly names for a few keycodes. A keycode is the USB number for a physical
// key, not an ASCII letter. HID_KEY_R is the R key, whatever letter it prints.
// A payload adds more of these as it needs them.
#define KEY_ENTER HID_KEY_ENTER
#define KEY_TAB   HID_KEY_TAB
#define KEY_SPACE HID_KEY_SPACE
#define KEY_R     HID_KEY_R
#define KEY_T     HID_KEY_T

// THE TWO FUNCTIONS THE RUNNER EXPORTS
// ducky_run does a small piece of work and returns fast. The main loop calls it
// again and again. It never blocks. On each call it may send one report, or
// check a timer, or do nothing. It advances through the steps over many calls.
//
// Sends one USB report per call and walks the step list one time. Call this
// each main-loop iteration while the device is mounted. It does nothing after
// the last step.
void ducky_run(const duck_step_t *steps, size_t count);

// ducky_reset moves the runner back to the first step. The main loop calls it
// each time the host mounts the device. A replug then replays the payload with
// no reflash.
//
// Sets the runner back to the first step. Call this on each new mount. A
// replug or a suspend/resume then replays the payload without a reflash.
void ducky_reset(void);
