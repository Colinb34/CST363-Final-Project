# 2 MIAMI 2 HEAT

Godot 4 top-down shooter for `CST363` built with a C++ `GDExtension`. The current version focuses on spatial indexing, enemy perception, pickups, and vector-database-style nearest-candidate querying inside a playable shooter.

## Overview

- `project.godot` starts the game at `1280x720`
- `scenes/main.tscn` boots the native `GameWorld`
- `cpp/src/game_world.*` contains the map, HUD, pickups, spatial queries, cone weapon logic, pause/map view, and drawing
- `cpp/src/player_controller.*` contains movement, ammo, reload, health, temporary buffs, and weapon state
- `cpp/src/enemy.*` contains enemy movement, sight-memory, wandering, and combat state

## Current Features

- `80 x 80` tile map with obstacles that block movement, bullets, vision, and cone attacks
- Two enemy types:
  - red ranged shooters
  - blue chasers
- Enemy sight-line behavior:
  - enemies wander when they do not see the player
  - enemies chase or engage when they gain line of sight
- Oil barrels that explode into fire hazards
- Four pickup types:
  - ammo
  - health
  - speed boost
  - temporary cone/flamethrower weapon
- Two spatial query modes:
  - `QuadTree`
  - `Brute Force`
- Additional enemy `R-tree` style bounding-box query path used for the cone weapon candidate lookup

## Controls

- `W A S D`: move
- `Left Mouse`: fire weapon
- `R`: reload
- `P`: pause and zoom the map out
- `M`: toggle map zoom
- `T`: switch between `QuadTree` and `Brute Force` spatial query modes
- `U`: toggle quadtree debug overlay

## Spatial Indexing Demo

This project is designed to show concepts similar to vector databases:

- game objects are stored by position in space
- a spatial index narrows down likely nearby candidates
- exact checks happen after candidate retrieval
- different query structures can be used for different tasks

In the current build:

- the `QuadTree` handles broad-phase lookup for bullets, pickups, and nearby world interactions
- the `Brute Force` mode lets you compare behavior against no spatial partitioning
- the cone weapon uses an `R-tree` style hierarchy for enemy candidate retrieval inside an area query

The HUD shows:

- active spatial mode
- quadtree node/candidate information
- per-frame and smoothed spatial timing statistics
- pickup counts
- active weapon and timed buff state

## Build Requirements

Install these before building:

- Godot `4.6+`
- Python
- SCons
- C++ build tools for your platform

## Windows Build

1. Run `scripts\setup_dependencies.bat`
2. Run `scripts\build_windows_debug.bat`
3. Open the project in Godot from `project.godot`

## Notes

- The cone weapon is a temporary pickup effect, not the default weapon
- Pickup counts are randomized at startup, with each pickup type spawning between `5` and `20` times
- The project uses native C++ gameplay code, so logic changes require rebuilding the extension DLL
