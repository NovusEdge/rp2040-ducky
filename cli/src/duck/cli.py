import errno
import os
import shutil
import subprocess
import sys
import time
from enum import Enum
from pathlib import Path

import questionary
from cyclopts import App

app = App(
    name="duck",
    help="Build and flash the rp2040-ducky firmware and manage payloads.",
)


def _root() -> Path:
    for d in Path(__file__).resolve().parents:
        if (d / "runtimes" / "pico-sdk" / "CMakeLists.txt").is_file():
            return d / "runtimes" / "pico-sdk"
    raise SystemExit("cannot locate firmware (no runtimes/pico-sdk/CMakeLists.txt above this file)")


ROOT = _root()
PAYLOADS = ROOT / "payloads"
BUILD = ROOT / "build"
UF2 = BUILD / "rp2040_ducky.uf2"
DEFAULT_PAYLOAD = "linux/rickroll"
# BOOTSEL mounts as RPI-RP2 on RP2040 boards, RP2350 on newer ones.
DRIVE_LABELS = ("RPI-RP2", "RP2350")

CIRCUITPY = ROOT.parent.parent / "runtimes" / "circuitpython"
CIRCUITPY_FILES = ("code.py", "boot.py", "duckyinpython.py", "pins.py", "payload.dd")


class Board(str, Enum):
    pico = "pico"    # RP2040
    pico2 = "pico2"  # RP2350


def _discover() -> list[str]:
    return sorted(
        p.parent.relative_to(PAYLOADS).as_posix()
        for p in PAYLOADS.glob("*/*/payload.h")
    )


def _resolve(payload: str) -> str:
    if (PAYLOADS / payload / "payload.h").is_file():
        return payload
    available = "\n  ".join(_discover() or ["(none found)"])
    raise SystemExit(f"unknown payload '{payload}'. available:\n  {available}")


def _sdk() -> str:
    return os.environ.get("PICO_SDK_PATH", "/usr/share/pico-sdk")


def _build(payload: str, board: Board) -> None:
    _resolve(payload)
    subprocess.run(
        ["cmake", "-B", str(BUILD),
         f"-DPICO_SDK_PATH={_sdk()}",
         f"-DPICO_BOARD={board.value}",
         f"-DPAYLOAD={payload}"],
        cwd=ROOT, check=True,
    )
    subprocess.run(["cmake", "--build", str(BUILD)], cwd=ROOT, check=True)
    print(f"built {UF2.relative_to(ROOT)}  [{payload}, {board.value}]")


def _flash_to(drive: Path, uf2: Path = UF2) -> None:
    # The RPI-RP2 mount is not writable for a short time after it appears. The
    # board unmounts the moment a full UF2 lands. So retry a transient EACCES
    # or EROFS. Treat a disconnect after the write as success.
    dst = drive / uf2.name
    data = uf2.read_bytes()
    deadline = time.monotonic() + 10
    while True:
        try:
            with open(dst, "wb") as f:
                f.write(data)
                f.flush()
                try:
                    os.fsync(f.fileno())
                except OSError:
                    pass
            break
        except OSError as e:
            if e.errno in (errno.ENODEV, errno.EIO, errno.ENOENT):
                break
            if e.errno in (errno.EACCES, errno.EROFS) and time.monotonic() < deadline:
                time.sleep(0.3)
                continue
            raise
    print(f"flashed -> {drive}")


def _find_volume(labels: tuple[str, ...]) -> Path | None:
    # The board mounts under a different root per OS: a user media dir on Linux,
    # /Volumes on macOS, a drive letter on Windows.
    if sys.platform == "darwin":
        for label in labels:
            p = Path("/Volumes") / label
            if p.is_dir():
                return p
        return None
    if sys.platform == "win32":
        return _find_windows_volume(labels)
    user = os.environ.get("USER", "")
    # /mnt covers a fixed mountpoint from the optional udev rule in contrib/.
    bases = (Path("/run/media") / user, Path("/media") / user, Path("/media"), Path("/mnt"))
    for base in bases:
        for label in labels:
            if (base / label).is_dir():
                return base / label
    return None


def _find_windows_volume(labels: tuple[str, ...]) -> Path | None:
    import ctypes
    import string

    kernel32 = ctypes.windll.kernel32
    mask = kernel32.GetLogicalDrives()
    name = ctypes.create_unicode_buffer(261)  # MAX_PATH + 1
    for i, letter in enumerate(string.ascii_uppercase):
        if not (mask >> i) & 1:
            continue
        root = f"{letter}:\\"
        # GetVolumeInformationW writes the volume label into name. The other out
        # params are optional; pass NULL and a zero-length filesystem buffer.
        if kernel32.GetVolumeInformationW(
            ctypes.c_wchar_p(root), name, ctypes.sizeof(name),
            None, None, None, None, 0,
        ) and name.value in labels:
            return Path(root)
    return None


def _find_drive() -> Path | None:
    return _find_volume(DRIVE_LABELS)


def _find_circuitpy() -> Path | None:
    return _find_volume(("CIRCUITPY",))


@app.command(name="list")
def list_() -> None:
    """List available payloads."""
    print("\n".join(_discover()))


@app.command
def build(payload: str = DEFAULT_PAYLOAD, *, board: Board = Board.pico) -> None:
    """Configure and compile the firmware."""
    _build(payload, board)


@app.command
def rebuild(payload: str = DEFAULT_PAYLOAD, *, board: Board = Board.pico) -> None:
    """Delete the build directory, then build."""
    shutil.rmtree(BUILD, ignore_errors=True)
    _build(payload, board)


@app.command
def flash(
    payload: str = DEFAULT_PAYLOAD,
    *,
    board: Board = Board.pico,
    wait: bool = True,
) -> None:
    """Build, then copy the .uf2 to a board in BOOTSEL mode."""
    _build(payload, board)
    drive = _find_drive()
    if drive is None and wait:
        print("waiting for BOOTSEL drive — hold BOOT, tap RESET, release BOOT")
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline and drive is None:
            time.sleep(0.5)
            drive = _find_drive()
    if drive is None:
        raise SystemExit("no BOOTSEL drive found. Put the board in BOOTSEL and retry.")
    _flash_to(drive)


@app.command
def clean() -> None:
    """Delete the build directory."""
    shutil.rmtree(BUILD, ignore_errors=True)
    print("removed build/")


@app.command
def edit(payload: str = DEFAULT_PAYLOAD) -> None:
    """Open a payload in $EDITOR."""
    path = PAYLOADS / _resolve(payload) / "payload.h"
    subprocess.run([os.environ.get("EDITOR", "vi"), str(path)])


@app.command(name="cp-setup")
def cp_setup(*, wait: bool = True) -> None:
    """Copy the CircuitPython .uf2 to a board in BOOTSEL mode."""
    uf2s = list((CIRCUITPY / "firmware").glob("*.uf2"))
    if len(uf2s) != 1:
        raise SystemExit(
            f"expected exactly one .uf2 under {CIRCUITPY / 'firmware'}, found {len(uf2s)}"
        )
    drive = _find_drive()
    if drive is None and wait:
        print("waiting for BOOTSEL drive — hold BOOT, tap RESET, release BOOT")
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline and drive is None:
            time.sleep(0.5)
            drive = _find_drive()
    if drive is None:
        raise SystemExit("no BOOTSEL drive found. Put the board in BOOTSEL and retry.")
    _flash_to(drive, uf2s[0])


@app.command(name="cp-load")
def cp_load(*, wait: bool = True) -> None:
    """Copy the CircuitPython runtime and payload to a mounted CIRCUITPY drive."""
    drive = _find_circuitpy()
    if drive is None and wait:
        # The board re-enumerates after cp-setup or a replug, and the mount can
        # lag the CLI. Poll so a load right after setup does not miss the drive.
        print("waiting for CIRCUITPY drive to mount")
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline and drive is None:
            time.sleep(0.5)
            drive = _find_circuitpy()
    if drive is None:
        raise SystemExit(
            "no CIRCUITPY drive found. Flash CircuitPython first via `duck cp-setup`, then replug the board."
        )
    for name in CIRCUITPY_FILES:
        shutil.copy2(CIRCUITPY / name, drive / name)
    shutil.copytree(CIRCUITPY / "lib", drive / "lib", dirs_exist_ok=True)
    print(f"loaded -> {drive}")


@app.command
def tui() -> None:
    """Pick a payload, board, and action interactively."""
    if not sys.stdin.isatty():
        raise SystemExit("tui needs an interactive terminal")
    payloads = _discover()
    if not payloads:
        raise SystemExit("no payloads found under payloads/")

    payload = questionary.select(
        "payload", choices=payloads,
        default=DEFAULT_PAYLOAD if DEFAULT_PAYLOAD in payloads else payloads[0],
    ).ask()
    if payload is None:
        return
    action = questionary.select("action", choices=["build", "flash", "edit"]).ask()
    if action is None:
        return

    if action == "edit":
        edit(payload)
        return

    board = questionary.select("board", choices=[b.value for b in Board]).ask()
    if board is None:
        return

    if action == "build":
        _build(payload, Board(board))
    else:
        flash(payload, board=Board(board))


def main() -> None:
    app()
