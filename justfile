# Thin wrappers over the duck CLI (cyclopts, in cli/, run via uv).
# Pass a payload and flags straight through, e.g.
#   just build mac/rickroll --board pico2
#   just flash linux/rickroll

duck := "uv run --project cli duck"

_default:
    @just --list --unsorted --list-heading $'rp2040-ducky\nusage: just <recipe> [payload] [--board pico|pico2]\nargs:  just <recipe> --help   (full arguments for a recipe)\n\n'

# configure and compile -> build/rp2040_ducky.uf2
[group('build')]
build *ARGS:
    {{duck}} build {{ARGS}}

# clean, then build
[group('build')]
rebuild *ARGS:
    {{duck}} rebuild {{ARGS}}

# delete the build directory
[group('build')]
clean:
    {{duck}} clean

# build, then copy the .uf2 to a board in BOOTSEL mode
[group('flash')]
flash *ARGS:
    {{duck}} flash {{ARGS}}

# list available payloads
[group('payloads')]
list:
    {{duck}} list

# open a payload in $EDITOR
[group('payloads')]
edit *ARGS:
    {{duck}} edit {{ARGS}}

# interactive picker: payload, board, action
[group('payloads')]
tui:
    {{duck}} tui
