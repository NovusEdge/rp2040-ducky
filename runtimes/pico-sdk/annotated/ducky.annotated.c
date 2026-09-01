// WHAT THIS FILE IS
// This file is the runner. It turns a payload (the step array from ducky.h) into
// USB key reports over time. The hard part is that it must never block. The main
// loop has to keep calling tud_task() to service USB, so the runner cannot sit
// in a sleep or a busy wait. Instead it keeps its progress in a few static
// variables and does one tiny step per call. This is a state machine driven by
// the main loop.

#include "ducky.h"

#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

// WHY A POLL INTERVAL
// The host polls the keyboard every 10 ms (set in the descriptor). If the
// firmware sends two reports inside one 10 ms window, some hosts keep only the
// last and drop the keystroke between them. So the runner sends at most one
// report per interval. It checks a timer at the top of every call and returns
// early when too little time has passed.
//
// Some hosts merge two reports sent in the same interval and drop a keystroke.
// So the runner sends a maximum of one report per interval.
#define POLL_INTERVAL_MS 10

// THE ASCII-TO-KEYCODE TABLE
// A payload string holds ASCII bytes. A USB report needs keycodes, not ASCII.
// This table maps each ASCII value to two things: does it need Shift, and which
// keycode. The macro HID_ASCII_TO_KEYCODE from TinyUSB fills all 128 rows.
// Example: 'A' needs Shift and the A keycode; 'a' needs the same keycode with no
// Shift. '\n' maps to Enter.
//
// [ascii][0] = needs shift, [ascii][1] = HID keycode. '\n' maps to Enter.
static uint8_t const  conv_table[128][2]  = {HID_ASCII_TO_KEYCODE};

// THE STATE
// These static variables are the runner's memory between calls. static here
// means the value survives from one call to the next and stays private to this
// file.
static uint32_t       last_ms             = 0;      // when the last report went out
static size_t         idx                 = 0;      // which step we are on
static bool           key_down            = false;  // is a key currently pressed?
static size_t         str_pos             = 0;      // which char of a string we are on
static bool           delay_started       = false;  // has a DELAY step begun timing?
static uint32_t       delay_start         = 0;      // when the current DELAY began

// Reset all state to the start. The main loop calls this on each new mount, so a
// replug replays the whole payload.
void ducky_reset(void) {
  idx = 0;
  str_pos = 0;
  key_down = false;
  delay_started = false;
}

// The runner. The main loop calls this on every pass while the device is
// mounted. Read the three early returns first; they are the guards that keep the
// function fast and safe.
void ducky_run(const duck_step_t* steps, size_t count) {
  // Guard 1: the payload is finished. idx walked past the last step. Do nothing
  // forever after this.
  if (idx >= count) return;
  // Guard 2: rate limit. Not enough time has passed since the last report. Come
  // back later. board_millis() is a millisecond clock from the SDK.
  if (board_millis() - last_ms < POLL_INTERVAL_MS) return;
  // Guard 3: the USB endpoint is busy with the previous report. Wait until it is
  // free, or the report is lost.
  if (!tud_hid_ready()) return;
  // We are about to act this interval. Stamp the clock now.
  last_ms = board_millis();

  // Look at the current step.
  const duck_step_t* s = &steps[idx];

  switch (s->kind) {
    case STEP_DELAY:
      // A DELAY does not send a report. It waits. The first time we reach a
      // DELAY step, we record the start time and return. On later calls we check
      // if enough time has passed. This is how you "sleep" without blocking.
      if (!delay_started) {
        delay_start = board_millis();
        delay_started = true;
        return;
      }
      if (board_millis() - delay_start >= s->ms) {
        delay_started = false;  // arm the next DELAY step
        idx++;                  // move to the next step
      }
      return;

    case STEP_KEY:
      // A key press is two reports, not one. Report A says "this key is down".
      // Report B says "nothing is down". The host turns the down-then-up pair
      // into one keystroke. key_down remembers which half we owe next.
      //
      // Send the press on one call. Send the release on the next call. Then
      // advance.
      if (!key_down) {
        // The press. A keyboard report holds up to six key slots. We use one.
        // The modifier byte carries Shift/Ctrl/Alt/Gui. Report id 0 is the only
        // report this device has.
        uint8_t kc[6] = {s->keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(0, s->modifier, kc);
        key_down = true;
      } else {
        // The release. All-zero report: no modifiers, no keys. NULL is TinyUSB
        // shorthand for six empty slots.
        tud_hid_keyboard_report(0, 0, NULL);
        key_down = false;
        idx++;  // this key is done; go to the next step
      }
      return;

    case STEP_STRING:
      // A string is a loop over its characters. str_pos is the cursor. When it
      // reaches the end, the string is done and we advance to the next step.
      if (str_pos >= strlen(s->str)) {
        str_pos = 0;
        idx++;
        return;
      }
      // Each character is a press then a release, the same two-report pattern as
      // STEP_KEY. The release between characters matters: without it, the host
      // sees the same key held down and will not register a repeat of the same
      // letter (like the two l's in "hello").
      //
      // Send a release between each character so the host registers repeats.
      if (!key_down) {
        // Press. Look up the character in the table. Column 0 says whether to
        // hold Shift. Column 1 is the keycode.
        uint8_t c = (uint8_t)s->str[str_pos];
        uint8_t modifier = conv_table[c][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
        uint8_t kc[6] = {conv_table[c][1], 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(0, modifier, kc);
        key_down = true;
      } else {
        // Release, then move the cursor to the next character.
        tud_hid_keyboard_report(0, 0, NULL);
        key_down = false;
        str_pos++;
      }
      return;
  }
}
