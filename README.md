# 2 MIAMI 2 HEAT

`2 MIAMI 2 HEAT` is a Godot 4.6 top-down shooter built for `CST363` with a native C++ `GDExtension`. The project mixes a playable action game with a spatial-query demo so you can show how indexing changes search cost in a game world.

## What This Project Includes

- A main top-down shooter level powered by native C++ code
- A second playable efficiency level that compares `QuadTree` lookup against a brute-force scan
- Runtime toggles to compare spatial query behavior
- A short-range cone/flamethrower weapon that uses an `R-tree`-style enemy query path
- HUD metrics that expose candidate counts, checks, and timing

## Project Structure

- [C:\Users\colin\OneDrive\Documents\New project\project.godot](C:/Users/colin/OneDrive/Documents/New%20project/project.godot): Godot project entry point
- [C:\Users\colin\OneDrive\Documents\New project\scenes\main.tscn](C:/Users/colin/OneDrive/Documents/New%20project/scenes/main.tscn): main shooter level
- [C:\Users\colin\OneDrive\Documents\New project\scenes\efficiency_level.tscn](C:/Users/colin/OneDrive/Documents/New%20project/scenes/efficiency_level.tscn): second level for collision/query efficiency comparison
- [C:\Users\colin\OneDrive\Documents\New project\cpp\src\game_world.cpp](C:/Users/colin/OneDrive/Documents/New%20project/cpp/src/game_world.cpp): main world logic, HUD, spatial queries, pickups, barrels, fire, cone weapon, and level switching
- [C:\Users\colin\OneDrive\Documents\New project\cpp\src\player_controller.cpp](C:/Users/colin/OneDrive/Documents/New%20project/cpp/src/player_controller.cpp): player health, ammo, reloads, movement, and temporary power-up state
- [C:\Users\colin\OneDrive\Documents\New project\cpp\src\enemy.cpp](C:/Users/colin/OneDrive/Documents/New%20project/cpp/src/enemy.cpp): enemy movement, line-of-sight behavior, wandering, and pursuit logic
- [C:\Users\colin\OneDrive\Documents\New project\scripts\efficiency_level.gd](C:/Users/colin/OneDrive/Documents/New%20project/scripts/efficiency_level.gd): standalone efficiency comparison level logic
- [C:\Users\colin\OneDrive\Documents\New project\scripts\efficiency_quadtree.gd](C:/Users/colin/OneDrive/Documents/New%20project/scripts/efficiency_quadtree.gd): GDScript quadtree used by the efficiency level

## Main Level Features

- `80 x 80` tile map with walls and structures
- Obstacles block:
  - player movement
  - enemy movement
  - bullets
  - line of sight
  - cone attacks
- Two enemy types:
  - ranged red shooter enemies
  - blue chaser enemies
- Enemy sight behavior:
  - enemies wander when the player is not visible
  - enemies switch into chase or engagement behavior when they gain sight
- Oil barrels that explode into lingering fire hazards
- Fire hazards that damage enemies and the player over time
- Randomized pickups distributed across the map
- Temporary weapon and movement buffs
- Pause and zoomed map view

## Pickups

The main level currently includes four pickup types. Each type spawns at a random count between `5` and `20` when the scene starts.

- `Ammo`: refills reserve ammunition
- `Health`: restores player health
- `Speed`: gives a temporary movement speed boost
- `Cone`: swaps the player weapon to a temporary flamethrower-style cone attack

## Weapons And Combat

- Default weapon:
  - hitscan-like forward bullet fire implemented as moving projectiles
  - magazine plus reserve ammo system
  - manual reload with `R`
  - automatic reload trigger when the player clicks on an empty magazine
- Cone weapon:
  - temporary pickup-based alternate weapon
  - short-range area-of-effect cone attack
  - damages multiple enemies in front of the player
  - can ignite oil barrels
  - uses an `R-tree`-style candidate lookup path before exact distance, angle, and line-of-sight filtering

## Spatial Indexing Features

The project is designed to demonstrate the same high-level query pattern used by spatial databases and vector databases:

1. build or consult an index
2. retrieve a small candidate set cheaply
3. apply more expensive exact predicates only to those candidates

### Spatial Modes In The Main Level

- `QuadTree` mode:
  - uses the native quadtree path for broad-phase world queries
  - narrows candidate sets before exact checks
- `Brute Force` mode:
  - bypasses the quadtree entirely
  - linearly scans all indexed spatial items

These modes make it easy to compare “indexed retrieval first” against “check everything.”

### What The Main Level Uses The QuadTree For

- Bullet candidate lookup
- Nearby object checks
- Pickup-range and interaction-style broad-phase queries
- Debug overlay visualization of quadtree partitions

### What The Main Level Uses The R-Tree-Style Hierarchy For

- Cone/flamethrower enemy candidate retrieval
- Bounding-box grouping for area-style overlap queries before exact cone filtering

## Efficiency Level

The second level is a focused visualization/demo scene that compares quadtree collision retrieval against a basic math scan.

It includes:

- a compact test arena
- moving player
- static walls
- enemy bullet bursts
- selectable collision mode
- visible quadtree boxes
- per-mode candidate, check, and timing metrics

This level is intended as the cleaner “database systems” demo, while the main level shows the same ideas inside actual gameplay.

## Controls

### Main Level

- `W A S D`: move
- `Left Mouse`: shoot
- `R`: reload
- `P`: pause and zoom the map out
- `M`: toggle map zoom
- `T`: switch between `QuadTree` and `Brute Force` spatial mode
- `U`: toggle quadtree debug overlay
- `L`: switch to the efficiency level

### Efficiency Level

- `W A S D`: move
- `P`: pause simulation
- `L`: return to the main level
- right-side dropdown: switch between `Quadtree` and `Basic math scan`
- right-side sliders: adjust forced clock speed, enemy burst rate, and bullet speed
- right-side checkbox: toggle quadtree box rendering
- right-side button: clear bullets

## HUD And Debug Information

The main level HUD exposes:

- player health
- magazine ammo and reserve ammo
- active weapon
- active timed buffs
- map zoom state
- enemy and barrel counts
- pickup counts by type
- current spatial mode
- quadtree overlay status
- candidate counts
- build/query timing in milliseconds
- smoothed timing/check averages

The efficiency level UI exposes:

- active collision mode
- bullet count
- player hit count
- enemy and wall counts
- quadtree node and item counts
- candidate counts
- exact collision checks
- microsecond timing
- simple CPU-style usage estimate

## Database Systems Connection

This project does not use a traditional DBMS such as `MySQL`, `PostgreSQL`, or `MongoDB`. Instead, it uses in-memory spatial access methods.

### In This Project

- The native main level uses a `QuadTree` and an `R-tree`-style hierarchy
- The efficiency level uses a GDScript `QuadTree` and a brute-force scan for comparison

### Why That Matters

These structures demonstrate the same principle used by spatial and vector databases:

- index first
- retrieve likely nearby candidates cheaply
- refine using exact predicates

In the game, those exact predicates include:

- circle-vs-circle overlap
- circle-vs-rectangle overlap
- distance checks
- angle checks
- wall / line-of-sight blocking checks

So while this project is not a database, it is a practical demonstration of database-style indexing and candidate refinement inside a game.

## Build Requirements

To run the native main level from source, install:

- Godot `4.6.x`
- Python
- SCons
- Visual Studio C++ build tools on Windows

## Windows Setup And Build

1. Open a terminal in [C:\Users\colin\OneDrive\Documents\New project](C:/Users/colin/OneDrive/Documents/New%20project)
2. Run:

```powershell
scripts\setup_dependencies.bat
```

3. Build the extension:

```powershell
scripts\build_windows_debug.bat
```

4. Open [C:\Users\colin\OneDrive\Documents\New project\project.godot](C:/Users/colin/OneDrive/Documents/New%20project/project.godot) in Godot

The compiled Windows debug library is expected at:

- [C:\Users\colin\OneDrive\Documents\New project\bin\libtopdown_shooter.windows.template_debug.x86_64.dll](C:/Users/colin/OneDrive/Documents/New%20project/bin/libtopdown_shooter.windows.template_debug.x86_64.dll)

## Notes

- The main level is implemented mostly in native C++, so gameplay code changes require rebuilding the extension
- The efficiency level is implemented in GDScript and can be iterated on faster
- The project starts on the main shooter level by default
- Use `L` in either scene to move between the two levels
