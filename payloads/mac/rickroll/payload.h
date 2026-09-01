#pragma once
#include "ducky.h"

// Cmd+Space opens Spotlight. A full URL typed into Spotlight and confirmed
// with Enter opens in the target's default browser, so no terminal or shell
// is involved. MOD_GUI is the Cmd key on macOS.
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),
    KEY(MOD_GUI, KEY_SPACE),
    DELAY(600),
    STR("https://youtu.be/oHg5SJYRHA0?si=iosxfMrmUpipJ2y6\n"),
};
