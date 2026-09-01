# rp2040-ducky

A HID-only USB keystroke injector for a generic RP2040 dev board, built on the
Pico SDK and TinyUSB. The board enumerates as a keyboard and nothing else — no
mass-storage drive appears on the target — then runs one compiled-in payload:
typed text, key combos, and delays. It runs on every mount, so a replug or a
RESET tap replays it without a reflash.

For learning and testing on machines you own or are authorized to test.

## What each file is

`main.c` sets up USB and, on each fresh mount, rewinds and runs the payload.
`src/ducky.c` is the step runner: it walks a `duck_step_t[]` array, sending one
USB report per interval for `STR` (typed text), `KEY` (a modifier+key press),
and `DELAY` (a wait). `ducky.h` defines those step macros and the key aliases.
`src/usb_descriptors.c` declares the single HID keyboard interface;
`src/tusb_config.h` disables CDC, MSC, and the rest, which keeps the target
from seeing a drive. Payloads live under `payloads/<os>/<name>/payload.h`.

## Toolchain (Arch)

You need the ARM bare-metal compiler and CMake. `cmake` is already present;
install the rest:

```
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils
```

Then get the Pico SDK. Clone it with submodules so TinyUSB comes along, and
point `PICO_SDK_PATH` at it:

```
git clone --branch master --recurse-submodules https://github.com/raspberrypi/pico-sdk ~/pico-sdk
export PICO_SDK_PATH=~/pico-sdk
```

Put that `export` in your shell profile, or pass `-DPICO_SDK_PATH=~/pico-sdk`
to the first `cmake` call. Without the submodules the build fails at link time
on `tinyusb_device` — that library lives in the SDK's `lib/tinyusb` submodule.

## Build

```
cmake -B build
cmake --build build
```

The flashable file is `build/rp2040_ducky.uf2`.

## Flash

Hold the BOOTSEL button on the board while plugging it into USB. A drive named
`RPI-RP2` appears. Copy the `.uf2` onto it; the board resets and starts running.

```
cp build/rp2040_ducky.uf2 /run/media/$USER/RPI-RP2/
```

If your clone already ran CircuitPython or another firmware, a `CIRCUITPY`
drive may show up instead of `RPI-RP2`. Unplug, hold BOOTSEL for a few seconds,
and plug in again to force the ROM bootloader. A bad `.uf2` cannot brick the
board — the bootloader is in ROM, so you can always reflash.

## Payloads

Each payload is a directory `payloads/<os>/<name>/` holding a `payload.h` that
defines one `duck_step_t PAYLOAD[]` array. Pick which one compiles in with the
`payload` flag; it defaults to `linux/rickroll`:

```
just payload=mac/rickroll build
just payload=linux/rickroll flash
```

Switching the payload reconfigures CMake, so pass the flag to `build` (or
`rebuild`). `just payload=<os>/<name> edit` opens one in `$EDITOR`.

A payload is a list of steps:

```c
static const duck_step_t PAYLOAD[] = {
    DELAY(1500),                       // wait ms — lead with one so the host settles
    KEY(MOD_CTRL | MOD_ALT, KEY_T),    // one modifier+key press
    STR("xdg-open 'https://...'\n"),   // typed text; '\n' sends Enter
};
```

`STR` resolves shift and symbols from the TinyUSB ASCII table. Lead every
payload with a `DELAY` of about 1.5 s: a host drops keystrokes until it loads
its keyboard driver. Raise it if the first characters go missing.

The step model covers text, single combos, and waits. Full DuckyScript
(`REPEAT`, `STRINGLN`, loops) is not implemented.
