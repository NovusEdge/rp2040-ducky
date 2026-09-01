#pragma once
#include "ducky.h"

// Diagnostic payload. It types a marker one time per mount into the focused
// window. Focus a text editor. Then unplug and replug to check whether the
// firmware fires again.
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),
    STR("DUCKY_RUN\n"),
};
