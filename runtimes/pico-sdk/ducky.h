#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tusb.h"

typedef enum { STEP_STRING, STEP_KEY, STEP_DELAY } step_kind_t;

typedef struct {
    step_kind_t kind;
    const char *str;    // STEP_STRING
    uint8_t modifier;   // STEP_KEY
    uint8_t keycode;    // STEP_KEY
    uint32_t ms;        // STEP_DELAY
} duck_step_t;

#define STR(s)    {.kind = STEP_STRING, .str = (s)}
#define KEY(m, k) {.kind = STEP_KEY, .modifier = (m), .keycode = (k)}
#define DELAY(t)  {.kind = STEP_DELAY, .ms = (t)}

#define MOD_CTRL  KEYBOARD_MODIFIER_LEFTCTRL
#define MOD_SHIFT KEYBOARD_MODIFIER_LEFTSHIFT
#define MOD_ALT   KEYBOARD_MODIFIER_LEFTALT
#define MOD_GUI   KEYBOARD_MODIFIER_LEFTGUI

#define KEY_ENTER HID_KEY_ENTER
#define KEY_TAB   HID_KEY_TAB
#define KEY_SPACE HID_KEY_SPACE
#define KEY_R     HID_KEY_R
#define KEY_T     HID_KEY_T

// Sends one USB report per call and walks the step list one time. Call this
// each main-loop iteration while the device is mounted. It does nothing after
// the last step.
void ducky_run(const duck_step_t *steps, size_t count);

// Sets the runner back to the first step. Call this on each new mount. A
// replug or a suspend/resume then replays the payload without a reflash.
void ducky_reset(void);

// True once the runner has walked past the last step. Pass the same count given
// to ducky_run. Used to time a soft re-enumerate for replay.
bool ducky_done(size_t count);
