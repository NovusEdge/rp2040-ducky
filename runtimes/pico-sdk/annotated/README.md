# Annotated C firmware — a guided read

These files teach the C firmware. Each file is a copy of the real source with
extra comments. The real firmware lives one directory up in `main.c`, `ducky.h`,
and `src/`. The build never compiles this folder.

The copies can drift. When you change the real source, these do not update. Read
them to learn the design, then trust the real files for the current code.

## Read in this order

1. `tusb_config.annotated.h` — the switches that pick what the USB stack builds.
2. `usb_descriptors.annotated.c` — how the board tells the host "I am a keyboard".
3. `ducky.annotated.h` — the data model. A payload is a list of steps.
4. `ducky.annotated.c` — the runner. It sends one key at a time.
5. `main.annotated.c` — the glue. It starts USB and runs the loop.

## The one idea to hold first

A USB keyboard never types on its own clock. The host asks the keyboard "any new
keys?" on a fixed schedule. The keyboard answers with a *report*: a small packet
that lists which keys are down right now. To type the letter `A`, the firmware
sends one report that says "A is down", then a later report that says "nothing is
down". The host turns that down-then-up pair into one keystroke. Every file here
serves that one loop.
