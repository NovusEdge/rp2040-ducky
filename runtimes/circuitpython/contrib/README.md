# Optional: fixed-path automount for CIRCUITPY

`60-circuitpy.rules` is a udev rule that mounts the CircuitPython `CIRCUITPY`
drive at `/mnt/CIRCUITPY` every time the board enumerates. Linux + systemd only.
macOS and Windows already auto-mount removable drives, so this rule has no
equivalent there and none is needed.

You do not need this. The desktop already mounts `CIRCUITPY` under
`/run/media/<user>/`, and `duck cp-load` finds it there. Install this only if you
want a stable path that does not depend on a desktop session — a headless box, or
a script that runs before login. `duck cp-load` also searches `/mnt`, so it finds
the drive whether the rule is installed or not.

## Install

Set the owner ids first. vfat has no unix permissions, so the mount options
decide who can write the drive. Check yours:

```sh
id -u    # uid
id -g    # gid
```

If they are not 1000, edit `uid=1000,gid=1000` in the rule before installing.

```sh
sudo mkdir -p /mnt/CIRCUITPY
sudo cp 60-circuitpy.rules /etc/udev/rules.d/60-circuitpy.rules
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=block --action=add
```

Replug the board. It mounts at `/mnt/CIRCUITPY`.

## Uninstall

```sh
sudo rm /etc/udev/rules.d/60-circuitpy.rules
sudo udevadm control --reload-rules
sudo systemd-umount /mnt/CIRCUITPY 2>/dev/null || true
sudo rmdir /mnt/CIRCUITPY
```
