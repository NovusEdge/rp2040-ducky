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
        if (d / "CMakeLists.txt").is_file():
            return d
    raise SystemExit("cannot locate project root (no CMakeLists.txt above this file)")


ROOT = _root()
PAYLOADS = ROOT / "payloads"
BUILD = ROOT / "build"
UF2 = BUILD / "rp2040_ducky.uf2"
DEFAULT_PAYLOAD = "linux/rickroll"
# BOOTSEL mounts as RPI-RP2 on RP2040 boards, RP2350 on newer ones.
DRIVE_LABELS = ("RPI-RP2", "RP2350")


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


def _flash_to(drive: Path) -> None:
    # The RPI-RP2 mount is not writable for a short time after it appears. The
    # board unmounts the moment a full UF2 lands. So retry a transient EACCES
    # or EROFS. Treat a disconnect after the write as success.
    dst = drive / UF2.name
    data = UF2.read_bytes()
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


def _find_drive() -> Path | None:
    user = os.environ.get("USER", "")
    for base in (Path("/run/media") / user, Path("/media") / user, Path("/media")):
        for label in DRIVE_LABELS:
            if (base / label).is_dir():
                return base / label
    return None


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
