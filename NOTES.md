# Debugging log

## USB re-enumeration on the dev board (open)

Symptom: the firmware types its payload once, then a RESET or unplug/replug
never fires it again.

Root cause (evidence-backed): this specific RP2040 clone enumerates our TinyUSB
firmware once per bootrom handoff and never re-enumerates on a RUN-pin RESET or
a replug. The board's USB hardware is fine:

- CircuitPython 10.3.0 on the same board re-enumerated on RESET in ~1.2 s
  (kernel: remove at 142559.096, add at 142560.320).
- The bootrom (RP2 Boot) re-enumerates on every reset/replug.
- Our firmware: removed on RESET at 142716.437, never returned.

Matches TinyUSB issues #2478 (tud_mounted stays true after disconnect on
RP2040) and #1730 (some clones fail to show up as a USB device after
disconnect; only a bootrom cycle restores USB).

Fixes tried, none worked on this board:
1. Re-arm the runner on the `tud_mounted()` edge — the edge never fires,
   because `tud_mounted()` stays true after disconnect.
2. Forced `tud_disconnect()` / delay / `tud_connect()` at boot.
3. `reset_block(RESETS_RESET_USBCTRL_BITS)` + a 300 ms disconnect at boot.

CircuitPython works on this exact board, so the fix is firmware-side and still
unfound. Next step to try: read CircuitPython's RP2040 USB init/reset path and
match it exactly, rather than guessing.

Decision (2026-09-01): accept fire-once for the C firmware for now; solve later
if tractable. Evaluate the CircuitPython / pico-ducky route next, which is
proven to re-enumerate reliably on this board (cost: a mass-storage drive
appears, so the HID-only stealth is lost).

The re-enumeration hardening left in `main.c` helps a genuine Pico/WeAct and is
inert on the clone.
