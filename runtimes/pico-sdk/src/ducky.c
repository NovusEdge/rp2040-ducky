#include "ducky.h"

#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

// Some hosts merge two reports sent in the same interval and drop a keystroke.
// So the runner sends a maximum of one report per interval.
#define POLL_INTERVAL_MS 10

// [ascii][0] = needs shift, [ascii][1] = HID keycode. '\n' maps to Enter.
static uint8_t const  conv_table[128][2]  = {HID_ASCII_TO_KEYCODE};

static uint32_t       last_ms             = 0;
static size_t         idx                 = 0;
static bool           key_down            = false;
static size_t         str_pos             = 0;
static bool           delay_started       = false;
static uint32_t       delay_start         = 0;

void ducky_reset(void) {
  idx = 0;
  str_pos = 0;
  key_down = false;
  delay_started = false;
}

bool ducky_done(size_t count) { return idx >= count; }

void ducky_run(const duck_step_t* steps, size_t count) {
  if (idx >= count) return;
  if (board_millis() - last_ms < POLL_INTERVAL_MS) return;
  if (!tud_hid_ready()) return;
  last_ms = board_millis();

  const duck_step_t* s = &steps[idx];

  switch (s->kind) {
    case STEP_DELAY:
      if (!delay_started) {
        delay_start = board_millis();
        delay_started = true;
        return;
      }
      if (board_millis() - delay_start >= s->ms) {
        delay_started = false;
        idx++;
      }
      return;

    case STEP_KEY:
      // Send the press on one call. Send the release on the next call. Then
      // advance.
      if (!key_down) {
        uint8_t kc[6] = {s->keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(0, s->modifier, kc);
        key_down = true;
      } else {
        tud_hid_keyboard_report(0, 0, NULL);
        key_down = false;
        idx++;
      }
      return;

    case STEP_STRING:
      if (str_pos >= strlen(s->str)) {
        str_pos = 0;
        idx++;
        return;
      }
      // Send a release between each character so the host registers repeats.
      if (!key_down) {
        uint8_t c = (uint8_t)s->str[str_pos];
        uint8_t modifier = conv_table[c][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
        uint8_t kc[6] = {conv_table[c][1], 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(0, modifier, kc);
        key_down = true;
      } else {
        tud_hid_keyboard_report(0, 0, NULL);
        key_down = false;
        str_pos++;
      }
      return;
  }
}
