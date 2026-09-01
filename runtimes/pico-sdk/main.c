#include "bsp/board.h"
#include "hardware/resets.h"
#include "tusb.h"
#include "ducky.h"
#include PAYLOAD_HEADER

// Milliseconds to wait after the payload finishes before a soft re-enumerate
// that replays it. 0 disables replay. A soft cycle toggles only the D+ pullup,
// so no chip reset occurs and the RP2040-E5 reset-path erratum is never hit.
#ifndef DUCKY_REPLAY_MS
#define DUCKY_REPLAY_MS 0
#endif

int main(void) {
    // USB re-enumeration on a warm RESET is unreliable on the clone used here.
    // The board enumerates once per bootrom handoff. A RESET can drop it off the
    // bus with no re-enumeration. Root cause is RP2040-E5: the device needs
    // ~800us of idle bus after a reset to leave RESET, which a busy host may
    // never provide (TinyUSB #1730). Both this firmware and CircuitPython have
    // re-enumerated on RESET on this board on some runs and failed on others.
    // The build now sets PICO_RP2040_USB_DEVICE_ENUMERATION_FIX, the SDK errata
    // workaround that forces that idle window; the sequence below still helps a
    // genuine Pico or WeAct. See NOTES.md.
    reset_block(RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

    board_init();
    tud_init(0);

    tud_disconnect();
    board_delay(300);
    tud_connect();

    const size_t count = sizeof(PAYLOAD) / sizeof(PAYLOAD[0]);
    bool was_mounted = false;
    bool done_latched = false;
    uint32_t done_at = 0;

    while (true) {
        tud_task();

        bool mounted = tud_mounted();
        if (mounted && !was_mounted) {
            ducky_reset();
            done_latched = false;
        }
        was_mounted = mounted;

        if (!mounted) continue;

        ducky_run(PAYLOAD, count);

#if DUCKY_REPLAY_MS > 0
        // Once the payload finishes, wait DUCKY_REPLAY_MS, then drop and raise
        // the connection. The host re-enumerates and the mount edge above
        // replays the payload. No reset button, no replug.
        if (ducky_done(count)) {
            if (!done_latched) {
                done_at = board_millis();
                done_latched = true;
            } else if (board_millis() - done_at >= DUCKY_REPLAY_MS) {
                tud_disconnect();
                board_delay(300);
                tud_connect();
                done_latched = false;
            }
        }
#endif
    }
}

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
