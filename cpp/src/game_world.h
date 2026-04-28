#ifndef TOPDOWN_GAME_WORLD_H
#define TOPDOWN_GAME_WORLD_H

#include "enemy.h"
#include "player_controller.h"

#include <cstdint>
#include <vector>

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace godot {

class GameWorld : public Node2D {
    GDCLASS(GameWorld, Node2D)

private:
    static constexpr int GRID_VISIBLE_WIDTH_DEFAULT = 10;
    static constexpr int GRID_VISIBLE_HEIGHT_DEFAULT = 10;
    static constexpr int GRID_VISIBLE_WIDTH_MAP = 40;
    static constexpr int GRID_VISIBLE_HEIGHT_MAP = 40;
    static constexpr int MAP_WIDTH_TILES = 80;
    static constexpr int MAP_HEIGHT_TILES = 80;
    static constexpr int TILE_SIZE = 64;
    static constexpr int PICKUP_PAIR_COUNT = 6;
    static constexpr int PICKUP_MIN_SPACING_TILES = 25;
    static constexpr int PICKUP_COMPASS_RANGE_TILES = 15;
    static constexpr int PICKUP_SPAWN_MARGIN_TILES = 3;
    static constexpr int PICKUP_PLAYER_BUFFER_TILES = 8;
    static constexpr int SHOOTER_ENEMY_COUNT = 15;
    static constexpr int CHASER_ENEMY_COUNT = 5;
    static constexpr int OIL_BARREL_COUNT = 7;
    static constexpr int QUADTREE_NODE_CAPACITY = 6;
    static constexpr int QUADTREE_MAX_DEPTH = 5;

    struct Bullet {
        Vector2 position;
        Vector2 velocity;
        float lifetime = 0.0f;
    };

    struct EnemyWave {
        Vector2 position;
        Vector2 velocity;
        float lifetime = 0.0f;
    };

    enum class PickupType {
        AMMO,
        HEALTH
    };

    struct Pickup {
        PickupType type;
        Vector2 position;
        int amount = 0;
        bool collected = false;
    };

    struct OilBarrel {
        Vector2 position;
        bool exploded = false;
    };

    struct FireHazard {
        Vector2 position;
        real_t radius = 0.0;
        real_t elapsed_time = 0.0;
        real_t shape_seed = 0.0;
        real_t damage_tick_timer = 0.0;
    };

    struct Structure {
        Rect2 bounds;
        Color fill_color;
        Color accent_color;
    };

    enum class SpatialItemKind : uint8_t {
        ENEMY,
        ENEMY_WAVE,
        PICKUP,
        OIL_BARREL
    };

    struct SpatialItem {
        SpatialItemKind kind;
        int index = -1;
        Vector2 position;
        real_t radius = 0.0;
    };

    struct QuadtreeNode {