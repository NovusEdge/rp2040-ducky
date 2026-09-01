// WHAT THIS FILE IS
// This is the entry point. It starts the USB stack, forces a clean reconnect so
// the host runs enumeration fresh, then loops forever. The loop does two jobs on
// every pass: service USB, and run the payload one small step. This file also
// holds two TinyUSB callbacks that the build requires but this device never
// needs.

#include "bsp/board.h"
#include "hardware/resets.h"
#include "tusb.h"
#include "ducky.h"
// PAYLOAD_HEADER is set by CMake from the -DPAYLOAD flag. It expands to a path
// like "payloads/linux/rickroll/payload.h". That header defines the PAYLOAD
// array. So the payload is chosen at build time, compiled straight in.
#include PAYLOAD_HEADER

int main(void) {
  // THE RECONNECT DANCE
  // A warm reset (the RESET button, no power cycle) can leave the host still
  // believing the old USB device is present. Then the host skips enumeration
  // and the ducky never types. The three calls below force the host to see a
  // disconnect and then a fresh connect, so enumeration runs again.
  //
  // The long comment in the real file records a known limit: one cheap clone
  // enumerates only once per power-up and ignores this dance. Stock
  // CircuitPython re-enumerates on that same clone because it carries a chip
  // errata workaround. That is why the CircuitPython runtime exists next door.
  //
  // USB re-enumeration hardening for a warm RESET. First reset the USB block.
  // Then drop D+ long enough for the host to detect a clean disconnect.
  // A genuine Pico or WeAct board then re-enumerates on RESET.
  // This does not fix the clone used here. That clone enumerates one time per
  // bootrom handoff. It never re-enumerates on a RESET or a replug (TinyUSB
  // #1730). CircuitPython re-enumerates on RESET on the same board. The fix
  // is firmware-side and still unknown. See NOTES.md.

  // Reset the USB hardware block, then wait for it to come back. reset_block
  // holds the block in reset; unreset_block_wait releases it and blocks until
  // it is ready.
  reset_block(RESETS_RESET_USBCTRL_BITS);
  unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

  // board_init sets up the SDK board support (clocks, the millisecond timer).
  // tud_init(0) starts the TinyUSB device stack on port 0.
  board_init();
  tud_init(0);

  // Drop the connection, wait, then bring it back. Dropping D+ tells the host
  // "the device is gone". 300 ms is long enough for the host to notice. Then
  // tud_connect() presents the device again and enumeration begins.
  tud_disconnect();
  board_delay(300);
  tud_connect();

  // count is the number of steps in the compiled-in payload. sizeof(PAYLOAD)
  // divided by the size of one element gives the length of the array.
  const size_t count = sizeof(PAYLOAD) / sizeof(PAYLOAD[0]);
  // was_mounted remembers the mount state from the previous pass, so we can spot
  // the moment it changes.
  bool was_mounted = false;

  while (true) {
    // tud_task is the heartbeat of TinyUSB. It must run often. It answers host
    // requests, handles enumeration, and keeps the connection alive. Never block
    // the loop, or this stops running and USB dies.
    tud_task();

    // tud_mounted() is true once the host has finished enumeration and the
    // device is live. Detect the rising edge: not mounted last pass, mounted
    // now. On that edge, reset the runner so the payload plays from step one.
    bool mounted = tud_mounted();
    if (mounted && !was_mounted) ducky_reset();
    was_mounted = mounted;

    // While mounted, advance the payload one small step. ducky_run returns fast
    // and does at most one report, so the loop keeps spinning and tud_task keeps
    // running.
    if (mounted) ducky_run(PAYLOAD, count);
  }
}

// THE REQUIRED CALLBACKS
// TinyUSB will not link without these two functions. They handle the host
// reading from or writing to the HID device. This device is a keyboard only:
// the host never reads a report from it and never sends it one. So both do
// nothing. get_report returns 0 to mean "no data". set_report ignores its
// input. The (void) casts silence unused-argument warnings.
//
// TinyUSB requires these callbacks. The host never reads this keyboard and
// never writes to it. Both callbacks do nothing.
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void) instance; (void) report_id; (void) report_type;
  (void) buffer; (void) reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
  (void) instance; (void) report_id; (void) report_type;
  (void) buffer; (void) bufsize;
}
