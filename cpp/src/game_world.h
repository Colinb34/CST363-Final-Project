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
        Rect2 bounds;
        int depth = 0;
        bool subdivided = false;
        std::vector<int> item_indices;
        std::vector<QuadtreeNode> children;
    };

    PlayerController *player = nullptr;
    Camera2D *camera = nullptr;
    CanvasLayer *hud_layer = nullptr;
    Label *hud_label = nullptr;
    AcceptDialog *game_over_dialog = nullptr;
    Ref<RandomNumberGenerator> rng;
    bool game_over = false;
    bool expanded_map_view = false;
    bool quadtree_overlay_visible = true;
    bool quadtree_ready = false;
    int quadtree_last_candidate_count = 0;
    std::vector<Enemy *> enemies;
    std::vector<Bullet> bullets;
    std::vector<EnemyWave> enemy_waves;
    std::vector<Pickup> pickups;
    std::vector<OilBarrel> oil_barrels;
    std::vector<FireHazard> fire_hazards;
    std::vector<SpatialItem> spatial_items;
    std::vector<Rect2> quadtree_debug_bounds;
    std::vector<Rect2> quadtree_debug_queries;
    std::vector<Rect2> quadtree_debug_visited;
    std::vector<Vector2> quadtree_debug_candidates;
    QuadtreeNode quadtree_root;

    void create_player();
    void create_camera();
    void create_hud();
    void create_game_over_dialog();
    void apply_camera_view_mode();
    void create_enemies();
    void create_oil_barrels();
    void create_pickups();
    void clamp_player_to_map();
    void check_game_over();
    void trigger_game_over();
    void update_hud();
    void update_bullets(double delta);
    void update_enemies(double delta);
    void update_enemy_waves(double delta);
    void update_fire_hazards(double delta);
    void update_pickups();
    String build_health_pips() const;
    void rebuild_spatial_index();
    void clear_quadtree_debug();
    void collect_spatial_items();
    void insert_spatial_item(QuadtreeNode &node, int item_index);
    void subdivide_quadtree_node(QuadtreeNode &node);
    void query_spatial_items(const Rect2 &area, std::vector<int> &out_indices, bool record_debug = false);
    void query_quadtree_node(const QuadtreeNode &node, const Rect2 &area, std::vector<int> &out_indices, bool record_debug) const;
    bool is_enemy_position_valid(const Vector2 &position) const;
    bool is_barrel_position_valid(const Vector2 &position) const;
    bool is_pickup_position_valid(const Vector2 &position) const;
    const Pickup *find_nearest_pickup_in_range(real_t max_distance) const;
    PackedVector2Array build_pentagon_points(const Vector2 &center, real_t radius, real_t rotation = -Math_PI / 2.0) const;
    PackedVector2Array build_diamond_points(const Vector2 &center, real_t width_radius, real_t height_radius) const;
    PackedVector2Array build_fire_shape_points(const Vector2 &center, real_t radius, real_t elapsed_time, real_t seed, real_t scale = 1.0) const;

protected:
    static void _bind_methods();

public:
    GameWorld() = default;
    ~GameWorld() override = default;

    void restart_game();
    void _ready() override;
    void _process(double delta) override;
    void _draw() override;
    void _unhandled_input(const Ref<InputEvent> &event) override;
};

}

#endif
