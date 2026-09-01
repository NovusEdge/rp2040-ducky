#pragma once
#include "ducky.h"

// Ctrl+Alt+T is the GNOME/Ubuntu default "open a terminal" shortcut. A desktop
// that unbinds it, or a non-GNOME session, needs a different opener. xdg-open
// hands the URL to whatever the target set as its default browser. Single
// quotes keep the shell from globbing the '?' in the URL.
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),
    KEY(MOD_CTRL | MOD_ALT, KEY_T),
    DELAY(1200),
    STR("xdg-open 'https://youtu.be/oHg5SJYRHA0?si=iosxfMrmUpipJ2y6'\n"),
};
