# Debugging log

## USB re-enumeration on the dev board (open)

Symptom: the firmware types its payload once, then a RESET or unplug/replug
never fires it again.

Root cause (evidence-backed): this specific RP2040 clone enumerates our TinyUSB
firmware once per bootrom handoff and re-enumerates on a RUN-pin RESET only
intermittently. The board's USB hardware is otherwise functional:

- CircuitPython 10.3.0 on the same board re-enumerated on RESET in ~1.2 s
  (kernel: remove at 142559.096, add at 142560.320).
- A later CircuitPython run on the same board did NOT re-enumerate: kernel logged
  `usb 3-7: USB disconnect, device number 75` at 298549 with no following add.
  The USB-presence watcher saw the board absent across the whole window.
- The bootrom (RP2 Boot) re-enumerates on every reset/replug.
- Our C firmware: removed on RESET at 142716.437, never returned.

The two CircuitPython runs disagree, so re-enumeration on RESET is intermittent
here, not a clean pass or fail. That fits RP2040-E5: the USB device controller
needs ~800 us of idle bus (J-state) after a bus reset before it leaves RESET. On
a busy host that idle window may or may not occur, so the same board recovers on
some resets and hangs on others. Matches TinyUSB #2478 (tud_mounted stays true
after disconnect on RP2040) and #1730 (some clones never re-appear as a USB
device after disconnect; only a bootrom cycle restores USB; still open upstream).

Fixes tried, none worked on this board:
1. Re-arm the runner on the `tud_mounted()` edge — the edge never fires,
   because `tud_mounted()` stays true after disconnect.
2. Forced `tud_disconnect()` / delay / `tud_connect()` at boot.
3. `reset_block(RESETS_RESET_USBCTRL_BITS)` + a 300 ms disconnect at boot.

These three toggle pullups at the TinyUSB level. None forces the 800 us idle
J-state that RP2040-E5 actually requires, so none addresses the root cause.

Tried: `PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1` (set in CMakeLists.txt, kept).
This is the SDK's documented RP2040-E5 workaround. `dcd_rp2040.c` calls
`rp2040_usb_device_enumeration_fix()` on each bus reset, which seizes GPIO15 to
force ~1 ms of J-state before handing the pins back to the PHY. The current code
never enabled it before.

Result (2026-09-02): it did not cure this clone. With the fix compiled in, the
board enumerated on flash (device 079, cafe:4004) and typed its marker. A RESET
then dropped it (ABSENT at 02:37:19). It stayed off the bus across several more
RESET presses. Recovery needed a BOOTSEL cycle (device 080, RP2 Boot at 02:38:54).
This matches TinyUSB #1730: on a clone whose D+ pullup or GPIO15/16 wiring differs
from a genuine Pico, forcing J-state on the pads does not reach the host bus, so
the pad-level fix cannot help. The fix stays enabled because it is correct for a
conforming board and costs nothing here.

A user-pressed RESET drives the hardware reset line, so a firmware-initiated
`watchdog_reboot()` cannot intercept it. No firmware path makes a warm RESET
re-enumerate on this specific clone. BOOTSEL/replug through the bootrom is the
recovery for a reflash.

## Resolved for replay: soft re-enumeration (2026-09-02)

Reading CircuitPython's USB path showed it calls `tud_init()` once, with no soft
teardown/re-init to copy. The erratum lives on the reset path, so the fix is to
never reset: `tud_disconnect()` / 300 ms / `tud_connect()` toggles only the D+
pullup and re-enumerates without a chip reset.

`DUCKY_REPLAY_MS` (CMake, default 0) enables this. After the payload ends the
firmware waits that long, soft-cycles USB, and the mount edge replays. Verified
on the clone at DUCKY_REPLAY_MS=5000: the board cycled C-FIRMWARE -> ABSENT ->
C-FIRMWARE and retyped the marker every ~6 s for many cycles with no RESET and no
replug. This is the mechanism the hardware RESET could not achieve.

Caveat: replay types into whatever window has focus on the host, on every cycle.

Untried, if pursued later: DMM check of the D+ idle level and pullup (~1.5k to
3V3) to confirm the reset-path hardware cause.

If the enumeration fix does not close it, the reliable fallback is a
`watchdog_reboot()` cold reboot through the bootrom (the path a BOOTSEL replug
uses, which always re-enumerates on this board), rather than a warm RESET.
AIRCR SYSRESETREQ is unreliable on RP2040 and does not reset the USB block.

Decision (2026-09-01): accept fire-once for the C firmware for now; solve later
if tractable. Evaluate the CircuitPython / pico-ducky route next.

Decision (2026-09-02): CircuitPython did not prove a reliable re-enum escape on
this clone either (see the failed run above), so the fix stays firmware-side.
Enabled the SDK enumeration fix as the next experiment.

## References

- RP2040-E5 erratum and the J-state workaround: RP2040 datasheet errata;
  https://forums.raspberrypi.com/viewtopic.php?t=331479
- SDK fix: `pico-sdk/src/rp2_common/pico_fix/rp2040_usb_device_enumeration/`,
  called from `lib/tinyusb/.../dcd_rp2040.c` when the compile flag is set.
- TinyUSB #1730 (clone fails to re-appear after disconnect, still open):
  https://github.com/hathach/tinyusb/issues/1730
