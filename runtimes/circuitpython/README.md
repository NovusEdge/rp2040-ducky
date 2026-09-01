# CircuitPython runtime

A CircuitPython-based USB rubber ducky, vendored from
[dbisu/pico-ducky](https://github.com/dbisu/pico-ducky) (GPL-2.0, Dave
Bailey). `webapp.py`, `wsgiserver.py`, and the wifi-configuration flow were
omitted — this runtime only runs `payload.dd` from local storage.

Sits alongside the C firmware in `runtimes/pico-sdk/`. Same hardware, same
default payload (`linux/rickroll`), different tradeoff: no compiler, but
DuckyScript instead of C, and interpreted at runtime instead of baked into
the binary.

## Setup

1. Hold BOOT, tap RESET, release BOOT — the Pico mounts as `RPI-RP2`.
2. Copy `firmware/adafruit-circuitpython-raspberry_pi_pico-en_US-10.3.0.uf2`
   onto it. The board reboots as a `CIRCUITPY` drive. One-time step.
3. Copy `code.py`, `boot.py`, `duckyinpython.py`, `pins.py`, `payload.dd`,
   and `lib/` onto `CIRCUITPY`.

Built for board id `raspberry_pi_pico`. For a Pico W or Pico 2, grab the
matching `.uf2` from https://circuitpython.org/downloads and swap it in.

## Editing the payload

Edit `payload.dd` in place on the `CIRCUITPY` drive. Saving triggers CircuitPython's
auto-reload: the board resets and replays the new script. No reflash, unlike
the C runtime which needs a full rebuild per payload change.

Gotcha: the drive is unavailable while a payload is mid-run — the write
will hang or fail. Hard-reset (RESET button) to regain it. Windows may
cache the drive contents; eject before editing if changes don't take.

## Why stock CircuitPython over the bare C build

Stock CircuitPython carries the RP2040-E5 USB errata workaround, so the
board re-enumerates cleanly on RESET. The bare-TinyUSB C build hits
TinyUSB issue #1730 on the same errata and does not re-enumerate reliably
after a RESET on the same clone.

## Files

- `code.py`, `boot.py`, `duckyinpython.py`, `pins.py` — copied verbatim from
  pico-ducky. `pins.py` sets up an unused button/GPIO; harmless if unwired.
- `payload.dd` — active DuckyScript payload, mirrors the C firmware's
  `linux/rickroll` default.
- `lib/adafruit_hid/` — vendored from
  [Adafruit_CircuitPython_HID](https://github.com/adafruit/Adafruit_CircuitPython_HID)
  (MIT), release 6.1.10, source `.py`.
- `lib/adafruit_debouncer.py` — vendored from
  [Adafruit_CircuitPython_Debouncer](https://github.com/adafruit/Adafruit_CircuitPython_Debouncer)
  (MIT), release 2.0.15, source `.py`.
- `LICENSE` — GPL-2.0, from pico-ducky.
