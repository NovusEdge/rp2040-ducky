#include "bsp/board.h"
#include "tusb.h"
#include "ducky.h"
#include PAYLOAD_HEADER

int main(void) {
    board_init();
    tud_init(0);

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

// Required by TinyUSB. The host never reads from or writes to this keyboard,
// so both are no-ops.
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
