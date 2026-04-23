# TopDownCppShooter

This package contains a Godot 4 top-down shooter project backed by a C++ GDExtension.

## Included

- `project.godot` configured for a 640x640 viewport so the player sees a 10x10 tile area.
- `scenes/main.tscn` that boots a C++ `GameWorld`.
- `cpp/src/player_controller.*` with 8-round magazine logic, reserve ammo, `R` reload, and 8-direction movement.
- `cpp/src/game_world.*` with a 200x200 tile map, player spawn, camera, bullets, and HUD.
- `cpp/src/enemy.*` placeholder enemy class ready to extend later.
- `scripts/setup_dependencies.bat` to fetch `godot-cpp`.
- `scripts/build_windows_debug.bat` to build the Windows debug DLL once prerequisites are installed.

## Controls

- `W A S D`: move horizontally, vertically, and diagonally
- `Left Mouse`: shoot
- `R`: reload from reserve ammo

## Notes

- The map is `80 x 80` tiles.
- The visible play space is tuned to a `10 x 10` tile view using a `640 x 640` window and `64px` tiles.
- The initial ammo setup is `8 / 8` in the magazine and `64` in reserve.

## Build Requirements

Install these before building:

- Godot 4.2+ editor
- Python
- SCons
- A C++ compiler toolchain for Godot on your platform

## Windows Build

1. Run `scripts\setup_dependencies.bat`
2. Run `scripts\build_windows_debug.bat`
3. Open the project folder in Godot

If you want release binaries or another platform, use the same `SConstruct` file with the matching `platform=` and `target=` values.
