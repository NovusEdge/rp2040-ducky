#pragma once
#include "ducky.h"

// Cmd+Space opens Spotlight. Type a full URL into Spotlight and press Enter.
// The target opens the URL in its default browser. This uses no terminal and
// no shell. MOD_GUI is the Cmd key on macOS.
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),
    KEY(MOD_GUI, KEY_SPACE),
    DELAY(600),
    STR("https://youtu.be/oHg5SJYRHA0?si=iosxfMrmUpipJ2y6\n"),
};
