# PICO_SDK_PATH from the environment wins; otherwise the AUR install location.
pico_sdk_path := env_var_or_default("PICO_SDK_PATH", "/usr/share/pico-sdk")
# "pico" = RP2040, "pico2" = RP2350. Switching requires `just rebuild`.
board := env_var_or_default("PICO_BOARD", "pico")
# which payload to compile in: <os>/<name> under payloads/.
payload := env_var_or_default("PAYLOAD", "linux/rickroll")

_default:
    @just --list

# configure and compile -> build/rp2040_ducky.uf2
build:
    cmake -B build -DPICO_SDK_PATH={{pico_sdk_path}} -DPICO_BOARD={{board}} -DPAYLOAD={{payload}}
    cmake --build build

# delete the build directory
clean:
    rm -rf build

# clean then build from scratch
rebuild: clean build

# build, then copy the .uf2 to a board held in BOOTSEL mode
flash: build
    #!/usr/bin/env bash
    set -euo pipefail
    drive="/run/media/$USER/RPI-RP2"
    if [ ! -d "$drive" ]; then
        echo "RPI-RP2 not mounted. Hold BOOTSEL, plug the board in, then rerun." >&2
        exit 1
    fi
    cp build/rp2040_ducky.uf2 "$drive"/
    echo "flashed -> $drive"

# open the selected payload in your editor
edit:
    ${EDITOR:-vi} payloads/{{payload}}/payload.h
