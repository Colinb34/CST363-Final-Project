# QuadTree Shooter Starter

Small Godot 4 starter project for a top-down 2D shooter assignment that needs a quad tree.

## Included

- Player movement with `WASD`
- Mouse aiming and left-click shooting
- Two enemy types:
  - `chaser`: moves directly toward the player
  - `shooter`: tries to keep range and fires back when it has line of sight
- Obstacles and arena walls that block movement and bullets
- A custom `QuadTree` class used as a broad-phase lookup for bullet hit checks

## Import

1. Open Godot 4.
2. Choose **Import**.
3. Select the `QuadTreeShooterStarter` folder.
4. Open `project.godot`.

## Files to look at

- `scripts/quadtree.gd`: quad tree implementation
- `scripts/main.gd`: rebuilds the tree and uses it for hit candidate queries
- `scripts/player.gd`: player controls
- `scripts/enemy.gd`: both enemy behaviors

## Notes

This is intentionally simple so it's easy to present and extend for class. Good next steps would be:

- draw the quad tree partitions for debugging
- use the tree for enemy awareness or pickups too
- add health UI, score, waves, and sound
