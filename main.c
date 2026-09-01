#include "bsp/board.h"
#include "hardware/resets.h"
#include "tusb.h"
#include "ducky.h"
#include PAYLOAD_HEADER

int main(void) {
    // USB re-enumeration hardening for a warm RESET. First reset the USB block.
    // Then drop D+ long enough for the host to detect a clean disconnect.
    // A genuine Pico or WeAct board then re-enumerates on RESET.
    // This does not fix the clone used here. That clone enumerates one time per
    // bootrom handoff. It never re-enumerates on a RESET or a replug (TinyUSB
    // #1730). CircuitPython re-enumerates on RESET on the same board. The fix
    // is firmware-side and still unknown. See NOTES.md.
    reset_block(RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

    board_init();
    tud_init(0);

    tud_disconnect();
    board_delay(300);
    tud_connect();

    const size_t count = sizeof(PAYLOAD) / sizeof(PAYLOAD[0]);
    bool was_mounted = false;

    while (true) {
        tud_task();

        bool mounted = tud_mounted();
        if (mounted && !was_mounted) ducky_reset();
        was_mounted = mounted;

        if (mounted) ducky_run(PAYLOAD, count);
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
