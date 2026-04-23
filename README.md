# CST363-Final-Project

Simple 2D top-down shooter written in Godot 4 with a C++ GDExtension, built to demonstrate gameplay systems plus quadtree-based spatial partitioning.

## Included

- `project.godot` configured for a `640x640` viewport so the player sees a `10x10` tile area by default.
- `scenes/main.tscn` that boots a C++ `GameWorld`.
- `cpp/src/player_controller.*` with 8-round magazine logic, reserve ammo, `R` reload, health, and 8-direction movement.
- `cpp/src/game_world.*` with the `80x80` tile map, HUD, pickups, oil barrels, enemy waves, blue chasers, quadtree debug overlay, and game-over flow.
- `cpp/src/enemy.*` with shared enemy state and damage handling.
- `scripts/setup_dependencies.bat` to fetch `godot-cpp`.
- `scripts/build_windows_debug.bat` to build the Windows debug DLL once prerequisites are installed.

## Controls

- `W A S D`: move horizontally, vertically, and diagonally
- `Left Mouse`: shoot
- `R`: reload
- `M`: toggle visible area between `10x10` and `40x40`
- `U`: toggle quadtree overlay on and off

## Notes

- The map is `80 x 80` tiles.
- The default visible play space is `10 x 10` tiles, with a toggle to expand to `40 x 40`.
- The project includes quadtree visualization for spatial queries and collision broad-phase debugging.

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
