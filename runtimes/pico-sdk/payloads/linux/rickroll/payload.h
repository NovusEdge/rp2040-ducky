#pragma once
#include "ducky.h"

// Ctrl+Alt+T is the default "open a terminal" shortcut on GNOME and Ubuntu.
// A desktop that unbinds it needs a different opener. A non-GNOME session also
// needs a different opener. xdg-open sends the URL to the default browser on
// the target. The single quotes stop the shell from expanding the '?' in the URL.
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),
    KEY(MOD_CTRL | MOD_ALT, KEY_T),
    DELAY(1200),
    STR("xdg-open 'https://youtu.be/oHg5SJYRHA0?si=iosxfMrmUpipJ2y6'\n"),
};
