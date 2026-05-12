#include "game_world.h"

#include <algorithm>
#include <chrono>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

namespace {
using Clock = std::chrono::steady_clock;
}

void GameWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("restart_game"), &GameWorld::restart_game);
}

void GameWorld::_ready() {
    rng.instantiate();
    rng->randomize();
    ensure_input_actions();
    create_player();
    create_camera();
    create_hud();
    create_game_over_dialog();
    create_structures();
    create_enemies();
    create_oil_barrels();
    create_pickups();
    set_process(true);
    set_process_unhandled_input(true);
    queue_redraw();
}

void GameWorld::ensure_input_actions() {
    InputMap *input_map = InputMap::get_singleton();
    if (input_map == nullptr) {
        return;
    }

    struct KeyBinding {
        const char *action;
        Key keycode;
    };

const KeyBinding key_bindings[] = {
        {"move_up", Key::KEY_W},
        {"move_down", Key::KEY_S},
        {"move_left", Key::KEY_A},
        {"move_right", Key::KEY_D},
        {"reload", Key::KEY_R},
        {"toggle_pause", Key::KEY_P},
        {"toggle_map_view", Key::KEY_M},
        {"toggle_spatial_mode", Key::KEY_T},
        {"toggle_quadtree_overlay", Key::KEY_U},
        {"switch_level", Key::KEY_L},
    };

    for (const KeyBinding &binding : key_bindings) {
        if (!input_map->has_action(binding.action)) {
            input_map->add_action(binding.action, 0.5);
        }

        Ref<InputEventKey> event;
        event.instantiate();
        event->set_keycode(binding.keycode);
        event->set_physical_keycode(binding.keycode);
        if (!input_map->action_has_event(binding.action, event)) {
            input_map->action_add_event(binding.action, event);
        }
    }

    if (!input_map->has_action("shoot")) {
        input_map->add_action("shoot", 0.5);
    }

    Ref<InputEventMouseButton> mouse_event;
    mouse_event.instantiate();
    mouse_event->set_button_index(MouseButton::MOUSE_BUTTON_LEFT);
    if (!input_map->action_has_event("shoot", mouse_event)) {
        input_map->action_add_event("shoot", mouse_event);
    }
}

void GameWorld::create_player() {
    player = memnew(PlayerController);
    add_child(player);
    player->set_name("Player");
    player->set_position(Vector2(MAP_WIDTH_TILES * TILE_SIZE, MAP_HEIGHT_TILES * TILE_SIZE) * 0.5f);
}

void GameWorld::create_camera() {
    camera = memnew(Camera2D);
    player->add_child(camera);
    camera->set_name("Camera2D");
    camera->set_enabled(true);
    camera->set_position_smoothing_enabled(true);
    camera->set_position_smoothing_speed(8.0);
    apply_camera_view_mode();
}

void GameWorld::apply_camera_view_mode() {
    if (camera == nullptr) {
        return;
    }

    const real_t zoom_factor = expanded_map_view
        ? real_t(GRID_VISIBLE_WIDTH_DEFAULT) / real_t(GRID_VISIBLE_WIDTH_MAP)
        : 1.0f;
    camera->set_zoom(Vector2(zoom_factor, zoom_factor));
}

void GameWorld::create_structures() {
    structures.clear();

    const Color wall_fill(0.16f, 0.20f, 0.25f);
    const Color wall_accent(0.34f, 0.41f, 0.48f);
    const real_t tile = real_t(TILE_SIZE);

    structures.push_back({Rect2(Vector2(10.0f * tile, 8.0f * tile), Vector2(10.0f * tile, 2.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(26.0f * tile, 10.0f * tile), Vector2(2.0f * tile, 15.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(18.0f * tile, 25.0f * tile), Vector2(12.0f * tile, 2.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(36.0f * tile, 18.0f * tile), Vector2(14.0f * tile, 2.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(46.0f * tile, 28.0f * tile), Vector2(2.0f * tile, 14.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(54.0f * tile, 44.0f * tile), Vector2(12.0f * tile, 2.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(24.0f * tile, 46.0f * tile), Vector2(2.0f * tile, 13.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(11.0f * tile, 55.0f * tile), Vector2(14.0f * tile, 2.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(60.0f * tile, 14.0f * tile), Vector2(9.0f * tile, 9.0f * tile)), wall_fill, wall_accent});
    structures.push_back({Rect2(Vector2(58.0f * tile, 58.0f * tile), Vector2(8.0f * tile, 8.0f * tile)), wall_fill, wall_accent});
}

void GameWorld::create_hud() {
    hud_layer = memnew(CanvasLayer);
    add_child(hud_layer);
    hud_layer->set_name("HudLayer");

    hud_label = memnew(Label);
    hud_layer->add_child(hud_label);
    hud_label->set_name("Hud");
    hud_label->set_position(Vector2(12.0f, 12.0f));
    update_hud();
}

void GameWorld::create_game_over_dialog() {
    game_over_dialog = memnew(AcceptDialog);
    add_child(game_over_dialog);
    game_over_dialog->set_name("GameOverDialog");
    game_over_dialog->set_title("Game Over");
    game_over_dialog->set_text("You ran out of health.");
    game_over_dialog->set_ok_button_text("Try Again");
    game_over_dialog->connect("confirmed", Callable(this, "restart_game"));
}

void GameWorld::set_gameplay_paused(bool paused) {
    if (paused == gameplay_paused) {
        return;
    }

    if (paused) {
        map_view_before_pause = expanded_map_view;
        expanded_map_view = true;
    } else {
        expanded_map_view = map_view_before_pause;
    }

    gameplay_paused = paused;
    apply_camera_view_mode();

    if (player != nullptr) {
        player->set_physics_process(!paused);
        if (paused) {
            player->set_velocity(Vector2());
        }
    }

    for (Enemy *enemy : enemies) {
        if (enemy == nullptr) {
            continue;
        }

        enemy->set_physics_process(!paused);
        if (paused) {
            enemy->set_velocity(Vector2());
        }
    }

    update_hud();
    queue_redraw();
}

void GameWorld::create_enemies() {
    enemies.clear();

    const real_t min_coord = real_t(PICKUP_SPAWN_MARGIN_TILES * TILE_SIZE);
    const real_t max_x = real_t((MAP_WIDTH_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);
    const real_t max_y = real_t((MAP_HEIGHT_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);

    const int total_enemies = SHOOTER_ENEMY_COUNT + CHASER_ENEMY_COUNT;
    for (int i = 0; i < total_enemies; ++i) {
        Vector2 position;
        bool placed = false;
        for (int attempt = 0; attempt < 200 && !placed; ++attempt) {
            position = Vector2(
                rng->randf_range(min_coord, max_x),
                rng->randf_range(min_coord, max_y)
            );

            if (!is_enemy_position_valid(position)) {
                continue;
            }

            placed = true;
        }

        if (!placed) {
            continue;
        }

        Enemy *enemy = memnew(Enemy);
        add_child(enemy);
        enemy->set_name("Enemy" + String::num_int64(i));
        enemy->set_position(position);
        if (i >= SHOOTER_ENEMY_COUNT) {
            enemy->set_enemy_type(1);
            enemy->set_move_speed(player != nullptr ? player->get_move_speed() * 1.3 : 390.0);
        }
        enemies.push_back(enemy);
    }
}

void GameWorld::create_pickups() {
    pickups.clear();

    struct PickupSpec {
        PickupType type;
        int amount;
        real_t duration;
        real_t magnitude;
    };

    const PickupSpec pickup_specs[] = {
        {PickupType::AMMO, 16, 0.0f, 0.0f},
        {PickupType::HEALTH, 2, 0.0f, 0.0f},
        {PickupType::SPEED, 0, 8.0f, 1.45f},
        {PickupType::CONE, 0, 10.0f, 0.0f},
    };

    for (const PickupSpec &spec : pickup_specs) {
        const int spawn_count = rng->randi_range(5, 20);
        for (int i = 0; i < spawn_count; ++i) {
            Pickup pickup;
            pickup.type = spec.type;
            pickup.amount = spec.amount;
            pickup.duration = spec.duration;
            pickup.magnitude = spec.magnitude;

            bool placed = false;
            for (int attempt = 0; attempt < 200 && !placed; ++attempt) {
                const real_t min_coord = real_t(PICKUP_SPAWN_MARGIN_TILES * TILE_SIZE);
                const real_t max_x = real_t((MAP_WIDTH_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);
                const real_t max_y = real_t((MAP_HEIGHT_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);

                Vector2 position(
                    rng->randf_range(min_coord, max_x),
                    rng->randf_range(min_coord, max_y)
                );

                if (!is_pickup_position_valid(position)) {
                    continue;
                }

                pickup.position = position;
                placed = true;
            }

            if (placed) {
                pickups.push_back(pickup);
            }
        }
    }
}

void GameWorld::create_oil_barrels() {
    oil_barrels.clear();

    const real_t min_coord = real_t(PICKUP_SPAWN_MARGIN_TILES * TILE_SIZE);
    const real_t max_x = real_t((MAP_WIDTH_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);
    const real_t max_y = real_t((MAP_HEIGHT_TILES - PICKUP_SPAWN_MARGIN_TILES) * TILE_SIZE);

    for (int i = 0; i < OIL_BARREL_COUNT; ++i) {
        OilBarrel barrel;
        bool placed = false;
        for (int attempt = 0; attempt < 200 && !placed; ++attempt) {
            const Vector2 position(
                rng->randf_range(min_coord, max_x),
                rng->randf_range(min_coord, max_y)
            );

            if (!is_barrel_position_valid(position)) {
                continue;
            }

            barrel.position = position;
            placed = true;
        }

        if (placed) {
            oil_barrels.push_back(barrel);
        }
    }
}

void GameWorld::_process(double delta) {
    if (gameplay_paused) {
        update_hud();
        queue_redraw();
        return;
    }

    clear_quadtree_debug();
    rebuild_spatial_index();
    check_game_over();
    if (game_over) {
        update_hud();
        queue_redraw();
        return;
    }

    clamp_player_to_map();
    player_cone_fire_cooldown_timer = std::max(0.0, player_cone_fire_cooldown_timer - delta);
    update_bullets(delta);
    update_enemies(delta);
    update_enemy_waves(delta);
    update_fire_hazards(delta);
    update_player_flame_bursts(delta);
    update_pickups();

    Input *input = Input::get_singleton();
    if (input != nullptr && player != nullptr && player->is_cone_weapon_active() && input->is_action_pressed("shoot") && player_cone_fire_cooldown_timer <= 0.0) {
        const Vector2 target = get_global_mouse_position();
        if (player->try_shoot(target)) {
            fire_player_weapon(target);
            player_cone_fire_cooldown_timer = 0.08;
        }
    }

    rebuild_spatial_index();
    check_game_over();
    finalize_spatial_stats();
    update_hud();
    queue_redraw();
}

void GameWorld::clamp_player_to_map() {
    if (player == nullptr) {
        return;
    }

    Vector2 pos = player->get_position();
    const real_t player_radius = 18.0f;
    pos.x = Math::clamp<real_t>(pos.x, player_radius, real_t(MAP_WIDTH_TILES * TILE_SIZE) - player_radius);
    pos.y = Math::clamp<real_t>(pos.y, player_radius, real_t(MAP_HEIGHT_TILES * TILE_SIZE) - player_radius);
    player->set_position(resolve_character_position(pos, player_radius));
}

void GameWorld::check_game_over() {
    if (game_over || player == nullptr || player->get_current_health() > 0) {
        return;
    }

    trigger_game_over();
}

void GameWorld::trigger_game_over() {
    game_over = true;
    if (game_over_dialog != nullptr) {
        game_over_dialog->popup_centered(Size2i(280, 120));
    }
}

void GameWorld::update_hud() {
    if (hud_label == nullptr || player == nullptr) {
        return;
    }

    String hud_text;
    hud_text += "Health: ";
    hud_text += build_health_pips();
    hud_text += "  ";
    hud_text += "Ammo: ";
    hud_text += String::num_int64(player->get_bullets_in_magazine());
    hud_text += "/";
    hud_text += String::num_int64(player->get_magazine_size());
    hud_text += "  Reserve: ";
    hud_text += String::num_int64(player->get_reserve_ammo());
    hud_text += "\nMove: WASD  Shoot: Left Mouse  Reload: R";
    hud_text += "\nMap: 80x80 tiles  View: ";
    hud_text += expanded_map_view ? "40x40" : "10x10";
    hud_text += " tiles  Toggle Map View: M";
    hud_text += "\nPause: P";
    hud_text += "  Spatial Mode: ";
    hud_text += use_quadtree_spatial_index ? "QuadTree (T)" : "Brute Force (T)";
    hud_text += "  Swap Level: L";
    hud_text += "\nWeapon: ";
    hud_text += player->is_cone_weapon_active() ? "Cone Arc" : "Standard";
    if (player->is_cone_weapon_active()) {
        hud_text += " ";
        hud_text += String::num(player->get_cone_weapon_time_remaining(), 1);
        hud_text += "s";
    }
    if (player->get_speed_boost_time_remaining() > 0.0) {
        hud_text += "  Speed Boost ";
        hud_text += String::num(player->get_speed_boost_time_remaining(), 1);
        hud_text += "s";
    }
    hud_text += "\nEnemies: ";
    hud_text += String::num_int64(static_cast<int64_t>(enemies.size()));
    hud_text += "  Barrels: ";
    hud_text += String::num_int64(static_cast<int64_t>(std::count_if(
        oil_barrels.begin(),
        oil_barrels.end(),
        [](const OilBarrel &barrel) { return !barrel.exploded; }
    )));
    int ammo_pickup_count = 0;
    int health_pickup_count = 0;
    int speed_pickup_count = 0;
    int cone_pickup_count = 0;
    for (const Pickup &pickup : pickups) {
        if (pickup.collected) {
            continue;
        }

        switch (pickup.type) {
            case PickupType::AMMO:
                ammo_pickup_count += 1;
                break;
            case PickupType::HEALTH:
                health_pickup_count += 1;
                break;
            case PickupType::SPEED:
                speed_pickup_count += 1;
                break;
            case PickupType::CONE:
                cone_pickup_count += 1;
                break;
        }
    }
    hud_text += "\nPickups: Ammo ";
    hud_text += String::num_int64(ammo_pickup_count);
    hud_text += "  Health ";
    hud_text += String::num_int64(health_pickup_count);
    hud_text += "  Speed ";
    hud_text += String::num_int64(speed_pickup_count);
    hud_text += "  Cone ";
    hud_text += String::num_int64(cone_pickup_count);
    hud_text += "\nQuadtree: ";
    hud_text += use_quadtree_spatial_index
        ? String::num_int64(static_cast<int64_t>(quadtree_debug_bounds.size()))
        : String("Bypassed");
    hud_text += "  Candidates: ";
    hud_text += String::num_int64(quadtree_last_candidate_count);
    hud_text += "  Overlay: ";
    hud_text += quadtree_overlay_visible ? "On (U)" : "Off (U)";
    hud_text += "\nSpatial Stats: Build ";
    hud_text += String::num(spatial_index_time_ms, 3);
    hud_text += " ms  Query ";
    hud_text += String::num(spatial_query_time_ms, 3);
    hud_text += " ms  Calls ";
    hud_text += String::num_int64(spatial_query_count);
    hud_text += "  Checks ";
    hud_text += String::num_int64(spatial_item_test_count);
    hud_text += "\nSpatial Avg: Build ";
    hud_text += String::num(smoothed_spatial_index_time_ms, 3);
    hud_text += " ms  Query ";
    hud_text += String::num(smoothed_spatial_query_time_ms, 3);
    hud_text += " ms  Checks ";
    hud_text += String::num_int64(smoothed_spatial_item_test_count);
    if (player->is_reloading()) {
        hud_text += "\nReloading...";
    }
    if (gameplay_paused) {
        hud_text += "\nPaused";
    }
    if (game_over) {
        hud_text += "\nGame Over";
    }
    hud_label->set_text(hud_text);
}

void GameWorld::update_bullets(double delta) {
    const float bullet_lifetime = 1.5f;
    std::vector<bool> enemy_destroyed(enemies.size(), false);
    std::vector<int> nearby_item_indices;

    for (Bullet &bullet : bullets) {
        bullet.position += bullet.velocity * float(delta);
        bullet.lifetime += float(delta);
    }

    for (Bullet &bullet : bullets) {
        if (bullet.lifetime > bullet_lifetime) {
            continue;
        }

        const Vector2 previous_position = bullet.position - (bullet.velocity * float(delta));
        if (segment_hits_structure(previous_position, bullet.position)) {
            bullet.lifetime = bullet_lifetime + 1.0f;
            continue;
        }

        const Rect2 bullet_query_area(bullet.position - Vector2(40.0f, 40.0f), Vector2(80.0f, 80.0f));
        query_spatial_items(bullet_query_area, nearby_item_indices, true);

        for (int item_index : nearby_item_indices) {
            const SpatialItem &item = spatial_items[item_index];

            if (item.kind == SpatialItemKind::ENEMY_WAVE) {
                if (item.index < 0 || item.index >= static_cast<int>(enemy_waves.size())) {
                    continue;
                }

                EnemyWave &wave = enemy_waves[item.index];
                if (wave.lifetime > bullet_lifetime) {
                    continue;
                }

                if (bullet.position.distance_to(wave.position) <= 16.0f) {
                    bullet.lifetime = bullet_lifetime + 1.0f;
                    wave.lifetime = bullet_lifetime + 1.0f;
                    break;
                }
            } else if (item.kind == SpatialItemKind::OIL_BARREL) {
                if (item.index < 0 || item.index >= static_cast<int>(oil_barrels.size())) {
                    continue;
                }

                OilBarrel &barrel = oil_barrels[item.index];
                if (barrel.exploded) {
                    continue;
                }

                if (bullet.position.distance_to(barrel.position) <= 24.0f) {
                    bullet.lifetime = bullet_lifetime + 1.0f;
                    barrel.exploded = true;

                    FireHazard fire;
                    fire.position = barrel.position;
                    fire.radius = rng->randf_range(2.5f, 3.5f) * real_t(TILE_SIZE);
                    fire.shape_seed = rng->randf_range(0.0f, 1000.0f);
                    fire_hazards.push_back(fire);
                    break;
                }
            } else if (item.kind == SpatialItemKind::ENEMY) {
                if (item.index < 0 || item.index >= static_cast<int>(enemies.size())) {
                    continue;
                }

                Enemy *enemy = enemies[item.index];
                if (enemy == nullptr) {
                    continue;
                }

                if (bullet.position.distance_to(enemy->get_position()) <= 26.0f) {
                    bullet.lifetime = bullet_lifetime + 1.0f;
                    if (enemy->apply_damage(1) && enemy->get_hit_points() <= 0) {
                        enemy_destroyed[item.index] = true;
                    }
                    break;
                }
            }
        }
    }

    bullets.erase(
        std::remove_if(
            bullets.begin(),
            bullets.end(),
            [bullet_lifetime, this](const Bullet &bullet) {
                Rect2 map_bounds(Vector2(), Vector2(MAP_WIDTH_TILES * TILE_SIZE, MAP_HEIGHT_TILES * TILE_SIZE));
                return bullet.lifetime > bullet_lifetime || !map_bounds.has_point(bullet.position);
            }
        ),
        bullets.end()
    );

    enemy_waves.erase(
        std::remove_if(
            enemy_waves.begin(),
            enemy_waves.end(),
            [bullet_lifetime, this](const EnemyWave &wave) {
                Rect2 map_bounds(Vector2(), Vector2(MAP_WIDTH_TILES * TILE_SIZE, MAP_HEIGHT_TILES * TILE_SIZE));
                return wave.lifetime > bullet_lifetime || !map_bounds.has_point(wave.position);
            }
        ),
        enemy_waves.end()
    );

    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; --i) {
        if (!enemy_destroyed[i]) {
            continue;
        }

        enemies[i]->queue_free();
        enemies.erase(enemies.begin() + i);
    }
}

void GameWorld::update_enemies(double delta) {
    if (player == nullptr) {
        return;
    }

    for (Enemy *enemy : enemies) {
        if (enemy == nullptr) {
            continue;
        }

        enemy->update_freeze(delta);
        enemy->update_shot_cooldown(delta);
        enemy->update_wander_timer(delta);

        const Vector2 to_player = player->get_position() - enemy->get_position();
        const real_t distance_to_player = to_player.length();
        const bool in_detection_range = distance_to_player <= enemy->get_chase_activation_range();
        const bool has_line_of_sight = in_detection_range && !segment_hits_structure(enemy->get_position(), player->get_position());
        enemy->update_sight_memory(delta, has_line_of_sight);

        if (enemy->get_wander_timer() <= 0.0 && !enemy->has_recent_sight()) {
            const real_t random_angle = rng->randf_range(0.0f, Math_TAU);
            enemy->set_wander_direction(Vector2(Math::cos(random_angle), Math::sin(random_angle)));
            enemy->set_wander_timer(rng->randf_range(0.8f, 2.2f));
        }

        if (enemy->is_chaser()) {
            if (enemy->is_frozen()) {
                enemy->set_velocity(Vector2());
                enemy->move_and_slide();
                continue;
            }

            if (enemy->has_recent_sight() && distance_to_player > 0.0f) {
                enemy->set_velocity(to_player.normalized() * enemy->get_move_speed());
            } else {
                enemy->set_velocity(enemy->get_wander_direction() * (enemy->get_move_speed() * 0.45));
            }
            enemy->move_and_slide();
            enemy->set_position(resolve_character_position(enemy->get_position(), 24.0f));

            if (enemy->get_position().distance_to(player->get_position()) <= 24.0f) {
                if (player->take_damage(1)) {
                    enemy->reset_freeze_timer();
                    enemy->set_velocity(Vector2());
                }
            }
            continue;
        }

        Vector2 shooter_velocity;
        if (enemy->has_recent_sight() && distance_to_player > 0.0f) {
            const real_t preferred_distance = 9.0f * TILE_SIZE;
            if (distance_to_player > preferred_distance) {
                shooter_velocity = to_player.normalized() * (enemy->get_move_speed() * 0.65);
            }
        } else {
            shooter_velocity = enemy->get_wander_direction() * (enemy->get_move_speed() * 0.4);
        }
        enemy->set_velocity(shooter_velocity);
        enemy->move_and_slide();
        enemy->set_position(resolve_character_position(enemy->get_position(), 24.0f));

        if (!has_line_of_sight || !enemy->can_fire()) {
            continue;
        }

        const Vector2 direction = (player->get_position() - enemy->get_position()).normalized();
        if (direction.length_squared() == 0.0) {
            continue;
        }

        EnemyWave wave;
        wave.position = enemy->get_position();
        wave.velocity = direction * (700.0f * 0.6f);
        if (!segment_hits_structure(enemy->get_position(), enemy->get_position() + (direction * 28.0f))) {
            enemy_waves.push_back(wave);
        }
        enemy->reset_shot_cooldown();
    }
}

void GameWorld::update_enemy_waves(double delta) {
    if (player == nullptr) {
        return;
    }

    const float wave_lifetime = 1.5f;
    std::vector<int> nearby_item_indices;
    const Rect2 player_query_area(player->get_position() - Vector2(48.0f, 48.0f), Vector2(96.0f, 96.0f));
    query_spatial_items(player_query_area, nearby_item_indices, true);

    for (int item_index : nearby_item_indices) {
        const SpatialItem &item = spatial_items[item_index];
        if (item.kind != SpatialItemKind::ENEMY_WAVE) {
            continue;
        }

        if (item.index < 0 || item.index >= static_cast<int>(enemy_waves.size())) {
            continue;
        }

        EnemyWave &wave = enemy_waves[item.index];
        if (wave.position.distance_to(player->get_position()) <= 20.0f) {
            player->take_damage(1);
            wave.lifetime = wave_lifetime + 1.0f;
        }
    }

    for (EnemyWave &wave : enemy_waves) {
        const Vector2 previous_position = wave.position;
        wave.position += wave.velocity * float(delta);
        wave.lifetime += float(delta);
        if (segment_hits_structure(previous_position, wave.position)) {
            wave.lifetime = wave_lifetime + 1.0f;
        }
    }
}

void GameWorld::update_fire_hazards(double delta) {
    if (player == nullptr) {
        return;
    }

    std::vector<bool> enemy_destroyed(enemies.size(), false);

    for (FireHazard &fire : fire_hazards) {
        fire.elapsed_time += real_t(delta);
        fire.damage_tick_timer += real_t(delta);

        if (fire.damage_tick_timer < 0.5f) {
            continue;
        }

        fire.damage_tick_timer = 0.0f;

        const PackedVector2Array player_fire_shape = build_fire_shape_points(
            fire.position,
            fire.radius,
            fire.elapsed_time,
            fire.shape_seed,
            1.0f
        );

        Geometry2D *geometry = Geometry2D::get_singleton();
        if (geometry != nullptr && geometry->is_point_in_polygon(player->get_position(), player_fire_shape)) {
            player->take_damage(1);
        }

        for (size_t i = 0; i < enemies.size(); ++i) {
            Enemy *enemy = enemies[i];
            if (enemy == nullptr || enemy->get_hit_points() <= 0) {
                continue;
            }

            if (enemy->get_position().distance_to(fire.position) <= fire.radius) {
                if (enemy->apply_environment_damage(1) && enemy->get_hit_points() <= 0) {
                    enemy_destroyed[i] = true;
                }
            }
        }
    }

    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; --i) {
        if (!enemy_destroyed[i]) {
            continue;
        }

        enemies[i]->queue_free();
        enemies.erase(enemies.begin() + i);
    }
}

void GameWorld::update_pickups() {
    if (player == nullptr) {
        return;
    }

    const real_t pickup_radius = 24.0f;
    std::vector<int> nearby_item_indices;
    const Rect2 pickup_query_area(player->get_position() - Vector2(40.0f, 40.0f), Vector2(80.0f, 80.0f));
    query_spatial_items(pickup_query_area, nearby_item_indices, true);

    for (int item_index : nearby_item_indices) {
        const SpatialItem &item = spatial_items[item_index];
        if (item.kind != SpatialItemKind::PICKUP || item.index < 0 || item.index >= static_cast<int>(pickups.size())) {
            continue;
        }

        Pickup &pickup = pickups[item.index];
        if (pickup.collected) {
            continue;
        }

        if (pickup.position.distance_to(player->get_position()) > pickup_radius) {
            continue;
        }

        bool consumed = false;
        if (pickup.type == PickupType::AMMO) {
            consumed = player->add_reserve_ammo(pickup.amount);
        } else if (pickup.type == PickupType::HEALTH) {
            consumed = player->heal(pickup.amount);
        } else if (pickup.type == PickupType::SPEED) {
            player->activate_speed_boost(pickup.duration, pickup.magnitude);
            consumed = true;
        } else if (pickup.type == PickupType::CONE) {
            player->activate_cone_weapon(pickup.duration);
            consumed = true;
        }

        if (consumed) {
            pickup.collected = true;
        }
    }
}

String GameWorld::build_health_pips() const {
    if (player == nullptr) {
        return "[. . . . .]";
    }

    String text = "[";
    for (int i = 0; i < player->get_max_health(); ++i) {
        if (i > 0) {
            text += " ";
        }
        text += i < player->get_current_health() ? "O" : ".";
    }
    text += "]";
    return text;
}

void GameWorld::clear_quadtree_debug() {
    quadtree_debug_bounds.clear();
    quadtree_debug_queries.clear();
    quadtree_debug_visited.clear();
    quadtree_debug_candidates.clear();
    quadtree_last_candidate_count = 0;
    spatial_query_count = 0;
    spatial_item_test_count = 0;
    spatial_index_time_ms = 0.0;
    spatial_query_time_ms = 0.0;
}

void GameWorld::collect_spatial_items() {
    spatial_items.clear();

    for (size_t i = 0; i < enemies.size(); ++i) {
        Enemy *enemy = enemies[i];
        if (enemy == nullptr || enemy->get_hit_points() <= 0) {
            continue;
        }

        SpatialItem item;
        item.kind = SpatialItemKind::ENEMY;
        item.index = static_cast<int>(i);
        item.position = enemy->get_position();
        item.radius = enemy->is_chaser() ? 28.0f : 24.0f;
        spatial_items.push_back(item);
    }

    for (size_t i = 0; i < enemy_waves.size(); ++i) {
        const EnemyWave &wave = enemy_waves[i];
        SpatialItem item;
        item.kind = SpatialItemKind::ENEMY_WAVE;
        item.index = static_cast<int>(i);
        item.position = wave.position;
        item.radius = 16.0f;
        spatial_items.push_back(item);
    }

    for (size_t i = 0; i < pickups.size(); ++i) {
        const Pickup &pickup = pickups[i];
        if (pickup.collected) {
            continue;
        }

        SpatialItem item;
        item.kind = SpatialItemKind::PICKUP;
        item.index = static_cast<int>(i);
        item.position = pickup.position;
        item.radius = 18.0f;
        spatial_items.push_back(item);
    }

    for (size_t i = 0; i < oil_barrels.size(); ++i) {
        const OilBarrel &barrel = oil_barrels[i];
        if (barrel.exploded) {
            continue;
        }

        SpatialItem item;
        item.kind = SpatialItemKind::OIL_BARREL;
        item.index = static_cast<int>(i);
        item.position = barrel.position;
        item.radius = 24.0f;
        spatial_items.push_back(item);
    }
}

void GameWorld::update_player_flame_bursts(double delta) {
    for (PlayerFlameBurst &burst : player_flame_bursts) {
        burst.elapsed_time += real_t(delta);
    }

    player_flame_bursts.erase(
        std::remove_if(
            player_flame_bursts.begin(),
            player_flame_bursts.end(),
            [](const PlayerFlameBurst &burst) {
                return burst.elapsed_time >= burst.lifetime;
            }
        ),
        player_flame_bursts.end()
    );
}

void GameWorld::collect_enemy_rtree_items() {
    enemy_rtree_items.clear();

    for (size_t i = 0; i < enemies.size(); ++i) {
        Enemy *enemy = enemies[i];
        if (enemy == nullptr || enemy->get_hit_points() <= 0) {
            continue;
        }

        const real_t radius = enemy->is_chaser() ? 28.0f : 24.0f;
        EnemyRTreeItem item;
        item.enemy_index = static_cast<int>(i);
        item.position = enemy->get_position();
        item.bounds = Rect2(item.position - Vector2(radius, radius), Vector2(radius * 2.0f, radius * 2.0f));
        enemy_rtree_items.push_back(item);
    }
}

void GameWorld::subdivide_quadtree_node(QuadtreeNode &node) {
    const Vector2 half_size = node.bounds.size * 0.5f;
    const Vector2 origin = node.bounds.position;

    node.children.clear();
    node.children.reserve(4);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            QuadtreeNode child;
            child.depth = node.depth + 1;
            child.bounds = Rect2(origin + Vector2(half_size.x * x, half_size.y * y), half_size);
            node.children.push_back(child);
        }
    }

    node.subdivided = true;
}

void GameWorld::insert_spatial_item(QuadtreeNode &node, int item_index) {
    const SpatialItem &item = spatial_items[item_index];
    const Rect2 item_bounds(item.position - Vector2(item.radius, item.radius), Vector2(item.radius * 2.0f, item.radius * 2.0f));

    if (node.subdivided) {
        for (QuadtreeNode &child : node.children) {
            if (child.bounds.encloses(item_bounds)) {
                insert_spatial_item(child, item_index);
                return;
            }
        }
    }

    node.item_indices.push_back(item_index);

    if (!node.subdivided && node.depth < QUADTREE_MAX_DEPTH && static_cast<int>(node.item_indices.size()) > QUADTREE_NODE_CAPACITY) {
        subdivide_quadtree_node(node);

        std::vector<int> remaining_items;
        remaining_items.reserve(node.item_indices.size());
        for (int existing_index : node.item_indices) {
            const SpatialItem &existing_item = spatial_items[existing_index];
            const Rect2 existing_bounds(
                existing_item.position - Vector2(existing_item.radius, existing_item.radius),
                Vector2(existing_item.radius * 2.0f, existing_item.radius * 2.0f)
            );

            bool moved_to_child = false;
            for (QuadtreeNode &child : node.children) {
                if (child.bounds.encloses(existing_bounds)) {
                    insert_spatial_item(child, existing_index);
                    moved_to_child = true;
                    break;
                }
            }

            if (!moved_to_child) {
                remaining_items.push_back(existing_index);
            }
        }

        node.item_indices = remaining_items;
    }
}

void GameWorld::rebuild_spatial_index() {
    const auto start_time = Clock::now();
    collect_spatial_items();
    rebuild_enemy_rtree();

    if (!use_quadtree_spatial_index) {
        quadtree_root = QuadtreeNode();
        quadtree_ready = false;
        spatial_index_time_ms += std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();
        return;
    }

    quadtree_root = QuadtreeNode();
    quadtree_root.bounds = Rect2(Vector2(), Vector2(real_t(MAP_WIDTH_TILES * TILE_SIZE), real_t(MAP_HEIGHT_TILES * TILE_SIZE)));
    quadtree_root.depth = 0;
    quadtree_ready = true;

    for (size_t i = 0; i < spatial_items.size(); ++i) {
        insert_spatial_item(quadtree_root, static_cast<int>(i));
    }

    std::vector<const QuadtreeNode *> pending_nodes;
    pending_nodes.push_back(&quadtree_root);
    while (!pending_nodes.empty()) {
        const QuadtreeNode *node = pending_nodes.back();
        pending_nodes.pop_back();
        quadtree_debug_bounds.push_back(node->bounds);

        for (const QuadtreeNode &child : node->children) {
            pending_nodes.push_back(&child);
        }
    }

    spatial_index_time_ms += std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();
}

void GameWorld::rebuild_enemy_rtree() {
    collect_enemy_rtree_items();
    enemy_rtree_root = EnemyRTreeNode();
    enemy_rtree_ready = !enemy_rtree_items.empty();
    if (!enemy_rtree_ready) {
        return;
    }

    std::vector<int> item_indices;
    item_indices.reserve(enemy_rtree_items.size());
    for (size_t i = 0; i < enemy_rtree_items.size(); ++i) {
        item_indices.push_back(static_cast<int>(i));
    }

    enemy_rtree_root = build_enemy_rtree_node(std::move(item_indices));
}

void GameWorld::query_spatial_items(const Rect2 &area, std::vector<int> &out_indices, bool record_debug) {
    const auto start_time = Clock::now();
    out_indices.clear();
    spatial_query_count += 1;

    if (record_debug) {
        quadtree_debug_queries.push_back(area);
    }

    if (!use_quadtree_spatial_index) {
        for (size_t i = 0; i < spatial_items.size(); ++i) {
            const SpatialItem &item = spatial_items[i];
            const Rect2 item_bounds(item.position - Vector2(item.radius, item.radius), Vector2(item.radius * 2.0f, item.radius * 2.0f));
            spatial_item_test_count += 1;
            if (area.intersects(item_bounds)) {
                out_indices.push_back(static_cast<int>(i));
            }
        }

        if (record_debug) {
            quadtree_last_candidate_count += static_cast<int>(out_indices.size());
            for (int item_index : out_indices) {
                quadtree_debug_candidates.push_back(spatial_items[item_index].position);
            }
        }
        spatial_query_time_ms += std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();
        return;
    }

    if (!quadtree_ready) {
        spatial_query_time_ms += std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();
        return;
    }

    query_quadtree_node(quadtree_root, area, out_indices, record_debug);
    if (record_debug) {
        quadtree_last_candidate_count += static_cast<int>(out_indices.size());
        for (int item_index : out_indices) {
            quadtree_debug_candidates.push_back(spatial_items[item_index].position);
        }
    }

    spatial_query_time_ms += std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();
}

void GameWorld::query_quadtree_node(const QuadtreeNode &node, const Rect2 &area, std::vector<int> &out_indices, bool record_debug) const {
    if (!node.bounds.intersects(area)) {
        return;
    }

    if (record_debug) {
        const_cast<GameWorld *>(this)->quadtree_debug_visited.push_back(node.bounds);
    }

    for (int item_index : node.item_indices) {
        const SpatialItem &item = spatial_items[item_index];
        const Rect2 item_bounds(item.position - Vector2(item.radius, item.radius), Vector2(item.radius * 2.0f, item.radius * 2.0f));
        const_cast<GameWorld *>(this)->spatial_item_test_count += 1;
        if (area.intersects(item_bounds)) {
            out_indices.push_back(item_index);
        }
    }

    for (const QuadtreeNode &child : node.children) {
        query_quadtree_node(child, area, out_indices, record_debug);
    }
}

GameWorld::EnemyRTreeNode GameWorld::build_enemy_rtree_node(std::vector<int> item_indices) const {
    EnemyRTreeNode node;
    if (item_indices.empty()) {
        return node;
    }

    Rect2 merged_bounds = enemy_rtree_items[item_indices.front()].bounds;
    for (int item_index : item_indices) {
        merged_bounds = merged_bounds.merge(enemy_rtree_items[item_index].bounds);
    }
    node.bounds = merged_bounds;

    if (item_indices.size() <= RTREE_NODE_CAPACITY) {
        node.leaf = true;
        node.item_indices = std::move(item_indices);
        return node;
    }

    node.leaf = false;
    const bool sort_by_x = merged_bounds.size.x >= merged_bounds.size.y;
    std::sort(
        item_indices.begin(),
        item_indices.end(),
        [this, sort_by_x](int lhs, int rhs) {
            const Vector2 left_center = enemy_rtree_items[lhs].bounds.get_center();
            const Vector2 right_center = enemy_rtree_items[rhs].bounds.get_center();
            return sort_by_x ? left_center.x < right_center.x : left_center.y < right_center.y;
        }
    );

    const size_t chunk_size = static_cast<size_t>(RTREE_NODE_CAPACITY);
    for (size_t start = 0; start < item_indices.size(); start += chunk_size) {
        const size_t end = std::min(item_indices.size(), start + chunk_size);
        std::vector<int> child_items(item_indices.begin() + start, item_indices.begin() + end);
        node.children.push_back(build_enemy_rtree_node(std::move(child_items)));
    }

    return node;
}

void GameWorld::query_enemy_rtree(const EnemyRTreeNode &node, const Rect2 &area, std::vector<int> &out_enemy_indices) const {
    if (!node.bounds.intersects(area)) {
        return;
    }

    if (node.leaf) {
        for (int item_index : node.item_indices) {
            if (item_index < 0 || item_index >= static_cast<int>(enemy_rtree_items.size())) {
                continue;
            }

            const EnemyRTreeItem &item = enemy_rtree_items[item_index];
            if (area.intersects(item.bounds)) {
                out_enemy_indices.push_back(item.enemy_index);
            }
        }
        return;
    }

    for (const EnemyRTreeNode &child : node.children) {
        query_enemy_rtree(child, area, out_enemy_indices);
    }
}

void GameWorld::finalize_spatial_stats() {
    constexpr double smoothing = 0.15;
    smoothed_spatial_index_time_ms = (smoothed_spatial_index_time_ms * (1.0 - smoothing)) + (spatial_index_time_ms * smoothing);
    smoothed_spatial_query_time_ms = (smoothed_spatial_query_time_ms * (1.0 - smoothing)) + (spatial_query_time_ms * smoothing);
    smoothed_spatial_item_test_count = static_cast<int>(
        Math::round((static_cast<double>(smoothed_spatial_item_test_count) * (1.0 - smoothing)) + (static_cast<double>(spatial_item_test_count) * smoothing))
    );
}

bool GameWorld::is_enemy_position_valid(const Vector2 &position) const {
    if (intersects_structure(Rect2(position - Vector2(28.0f, 28.0f), Vector2(56.0f, 56.0f)))) {
        return false;
    }

    if (player != nullptr) {
        const real_t player_buffer = real_t((PICKUP_PLAYER_BUFFER_TILES + 4) * TILE_SIZE);
        if (position.distance_to(player->get_position()) < player_buffer) {
            return false;
        }
    }

    for (const Enemy *enemy : enemies) {
        if (enemy != nullptr && enemy->get_position().distance_to(position) < real_t(4 * TILE_SIZE)) {
            return false;
        }
    }

    return true;
}

bool GameWorld::is_barrel_position_valid(const Vector2 &position) const {
    if (intersects_structure(Rect2(position - Vector2(24.0f, 24.0f), Vector2(48.0f, 48.0f)))) {
        return false;
    }

    if (player != nullptr) {
        const real_t player_buffer = real_t((PICKUP_PLAYER_BUFFER_TILES + 2) * TILE_SIZE);
        if (position.distance_to(player->get_position()) < player_buffer) {
            return false;
        }
    }

    for (const Enemy *enemy : enemies) {
        if (enemy != nullptr && enemy->get_position().distance_to(position) < real_t(3 * TILE_SIZE)) {
            return false;
        }
    }

    for (const OilBarrel &barrel : oil_barrels) {
        if (barrel.position.distance_to(position) < real_t(4 * TILE_SIZE)) {
            return false;
        }
    }

    for (const Pickup &pickup : pickups) {
        if (pickup.position.distance_to(position) < real_t(3 * TILE_SIZE)) {
            return false;
        }
    }

    return true;
}

bool GameWorld::is_pickup_position_valid(const Vector2 &position) const {
    if (intersects_structure(Rect2(position - Vector2(20.0f, 20.0f), Vector2(40.0f, 40.0f)))) {
        return false;
    }

    if (player != nullptr) {
        const real_t player_buffer = real_t(PICKUP_PLAYER_BUFFER_TILES * TILE_SIZE);
        if (position.distance_to(player->get_position()) < player_buffer) {
            return false;
        }
    }

    const real_t minimum_spacing = real_t(PICKUP_MIN_SPACING_TILES * TILE_SIZE);
    for (const Pickup &existing_pickup : pickups) {
        if (existing_pickup.position.distance_to(position) < minimum_spacing) {
            return false;
        }
    }

    for (const OilBarrel &barrel : oil_barrels) {
        if (!barrel.exploded && barrel.position.distance_to(position) < real_t(3 * TILE_SIZE)) {
            return false;
        }
    }

    return true;
}

bool GameWorld::intersects_structure(const Rect2 &rect) const {
    for (const Structure &structure : structures) {
        if (structure.bounds.intersects(rect)) {
            return true;
        }
    }

    return false;
}

bool GameWorld::segment_hits_structure(const Vector2 &from, const Vector2 &to) const {
    if (from == to) {
        return false;
    }

    Geometry2D *geometry = Geometry2D::get_singleton();
    if (geometry == nullptr) {
        return false;
    }

    for (const Structure &structure : structures) {
        if (structure.bounds.has_point(from) || structure.bounds.has_point(to)) {
            return true;
        }

        const Vector2 top_left = structure.bounds.position;
        const Vector2 top_right = structure.bounds.position + Vector2(structure.bounds.size.x, 0.0f);
        const Vector2 bottom_left = structure.bounds.position + Vector2(0.0f, structure.bounds.size.y);
        const Vector2 bottom_right = structure.bounds.position + structure.bounds.size;

        if (geometry->segment_intersects_segment(from, to, top_left, top_right).get_type() != Variant::NIL ||
            geometry->segment_intersects_segment(from, to, top_right, bottom_right).get_type() != Variant::NIL ||
            geometry->segment_intersects_segment(from, to, bottom_right, bottom_left).get_type() != Variant::NIL ||
            geometry->segment_intersects_segment(from, to, bottom_left, top_left).get_type() != Variant::NIL) {
            return true;
        }
    }

    return false;
}

Vector2 GameWorld::resolve_character_position(const Vector2 &position, real_t radius) const {
    Vector2 resolved = position;

    for (int iteration = 0; iteration < 4; ++iteration) {
        bool adjusted = false;
        for (const Structure &structure : structures) {
            const Rect2 expanded = structure.bounds.grow(radius);
            if (!expanded.has_point(resolved)) {
                continue;
            }

            const real_t push_left = Math::abs(resolved.x - expanded.position.x);
            const real_t push_right = Math::abs((expanded.position.x + expanded.size.x) - resolved.x);
            const real_t push_up = Math::abs(resolved.y - expanded.position.y);
            const real_t push_down = Math::abs((expanded.position.y + expanded.size.y) - resolved.y);

            const real_t min_push = std::min(std::min(push_left, push_right), std::min(push_up, push_down));
            if (min_push == push_left) {
                resolved.x = expanded.position.x;
            } else if (min_push == push_right) {
                resolved.x = expanded.position.x + expanded.size.x;
            } else if (min_push == push_up) {
                resolved.y = expanded.position.y;
            } else {
                resolved.y = expanded.position.y + expanded.size.y;
            }

            adjusted = true;
        }

        if (!adjusted) {
            break;
        }
    }

    return resolved;
}

const GameWorld::Pickup *GameWorld::find_nearest_pickup_in_range(real_t max_distance) const {
    if (player == nullptr) {
        return nullptr;
    }

    const Pickup *nearest_pickup = nullptr;
    real_t nearest_distance = max_distance;

    for (const Pickup &pickup : pickups) {
        if (pickup.collected) {
            continue;
        }

        const real_t distance = pickup.position.distance_to(player->get_position());
        if (distance <= nearest_distance) {
            nearest_distance = distance;
            nearest_pickup = &pickup;
        }
    }

    return nearest_pickup;
}

PackedVector2Array GameWorld::build_pentagon_points(const Vector2 &center, real_t radius, real_t rotation) const {
    PackedVector2Array points;
    for (int i = 0; i < 5; ++i) {
        const real_t angle = rotation + (Math_TAU * real_t(i) / 5.0);
        points.push_back(center + Vector2(Math::cos(angle), Math::sin(angle)) * radius);
    }
    return points;
}

PackedVector2Array GameWorld::build_diamond_points(const Vector2 &center, real_t width_radius, real_t height_radius) const {
    PackedVector2Array points;
    points.push_back(center + Vector2(0.0f, -height_radius));
    points.push_back(center + Vector2(width_radius, 0.0f));
    points.push_back(center + Vector2(0.0f, height_radius));
    points.push_back(center + Vector2(-width_radius, 0.0f));
    return points;
}

PackedVector2Array GameWorld::build_fire_shape_points(const Vector2 &center, real_t radius, real_t elapsed_time, real_t seed, real_t scale) const {
    PackedVector2Array points;
    const int point_count = 28;

    for (int i = 0; i < point_count; ++i) {
        const real_t angle = Math_TAU * real_t(i) / real_t(point_count);
        const real_t wobble_a = Math::sin((angle * 3.0f) + (elapsed_time * 1.8f) + seed);
        const real_t wobble_b = Math::sin((angle * 5.0f) - (elapsed_time * 1.1f) + (seed * 0.7f));
        const real_t wobble = (wobble_a * 0.08f) + (wobble_b * 0.06f);
        const real_t local_radius = radius * scale * (0.86f + wobble);
        points.push_back(center + Vector2(Math::cos(angle), Math::sin(angle)) * local_radius);
    }

    return points;
}

void GameWorld::fire_player_weapon(const Vector2 &target) {
    if (player == nullptr) {
        return;
    }

    if (player->is_cone_weapon_active()) {
        fire_player_cone_weapon(target);
        return;
    }

    Bullet bullet;
    bullet.position = player->get_position();
    bullet.velocity = (target - player->get_position()).normalized() * 700.0f;
    bullets.push_back(bullet);
}

void GameWorld::fire_player_cone_weapon(const Vector2 &target) {
    if (player == nullptr) {
        return;
    }

    const Vector2 origin = player->get_position();
    Vector2 direction = target - origin;
    if (direction.length_squared() == 0.0f) {
        return;
    }
    direction = direction.normalized();

    const real_t cone_range = 260.0f;
    const real_t cone_half_angle = 0.48f;
    const real_t min_dot = Math::cos(cone_half_angle);
    const Rect2 query_area(origin - Vector2(cone_range, cone_range), Vector2(cone_range * 2.0f, cone_range * 2.0f));

    PlayerFlameBurst burst;
    burst.origin = origin;
    burst.direction = direction;
    burst.range = cone_range;
    burst.half_angle = cone_half_angle;
    burst.lifetime = 0.16f;
    player_flame_bursts.push_back(burst);

    std::vector<int> candidate_enemy_indices;
    if (enemy_rtree_ready) {
        query_enemy_rtree(enemy_rtree_root, query_area, candidate_enemy_indices);
    } else {
        for (size_t i = 0; i < enemies.size(); ++i) {
            candidate_enemy_indices.push_back(static_cast<int>(i));
        }
    }

    std::vector<bool> enemy_destroyed(enemies.size(), false);
    for (int enemy_index : candidate_enemy_indices) {
        if (enemy_index < 0 || enemy_index >= static_cast<int>(enemies.size())) {
            continue;
        }

        Enemy *enemy = enemies[enemy_index];
        if (enemy == nullptr || enemy->get_hit_points() <= 0) {
            continue;
        }

        const Vector2 to_enemy = enemy->get_position() - origin;
        const real_t distance = to_enemy.length();
        if (distance <= 0.0f || distance > cone_range) {
            continue;
        }

        const real_t alignment = direction.dot(to_enemy / distance);
        if (alignment < min_dot || segment_hits_structure(origin, enemy->get_position())) {
            continue;
        }

        if (enemy->apply_environment_damage(2) && enemy->get_hit_points() <= 0) {
            enemy_destroyed[enemy_index] = true;
        }
    }

    for (OilBarrel &barrel : oil_barrels) {
        if (barrel.exploded) {
            continue;
        }

        const Vector2 to_barrel = barrel.position - origin;
        const real_t distance = to_barrel.length();
        if (distance <= 0.0f || distance > cone_range) {
            continue;
        }

        const real_t alignment = direction.dot(to_barrel / distance);
        if (alignment < min_dot || segment_hits_structure(origin, barrel.position)) {
            continue;
        }

        barrel.exploded = true;
        FireHazard fire;
        fire.position = barrel.position;
        fire.radius = rng->randf_range(2.5f, 3.5f) * real_t(TILE_SIZE);
        fire.shape_seed = rng->randf_range(0.0f, 1000.0f);
        fire_hazards.push_back(fire);
    }

    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; --i) {
        if (!enemy_destroyed[i]) {
            continue;
        }

        enemies[i]->queue_free();
        enemies.erase(enemies.begin() + i);
    }
}

void GameWorld::_draw() {
    const int map_pixel_width = MAP_WIDTH_TILES * TILE_SIZE;
    const int map_pixel_height = MAP_HEIGHT_TILES * TILE_SIZE;
    const Color background_color(0.09f, 0.11f, 0.14f);
    const Color grid_color(0.19f, 0.24f, 0.30f);
    const Color boundary_color(0.62f, 0.82f, 0.98f);
    const Color bullet_color(1.0f, 0.82f, 0.34f);
    const Color enemy_wave_color(1.0f, 0.45f, 0.78f);
    const Color ammo_pickup_color(0.36f, 0.79f, 1.0f);
    const Color health_pickup_color(1.0f, 0.37f, 0.37f);
    const Color speed_pickup_color(0.98f, 0.83f, 0.25f);
    const Color cone_pickup_color(0.73f, 0.48f, 1.0f);
    const Color compass_color(1.0f, 1.0f, 1.0f);
    const Color oil_barrel_color(0.72f, 0.12f, 0.08f);
    const Color quadtree_color(0.42f, 0.86f, 1.0f, 0.18f);
    const Color quadtree_visited_color(1.0f, 0.95f, 0.36f, 0.36f);
    const Color quadtree_query_color(0.65f, 1.0f, 0.65f, 0.42f);

    draw_rect(Rect2(Vector2(), Vector2(map_pixel_width, map_pixel_height)), background_color, true);

    for (int x = 0; x <= MAP_WIDTH_TILES; ++x) {
        float line_x = float(x * TILE_SIZE);
        draw_line(Vector2(line_x, 0.0f), Vector2(line_x, float(map_pixel_height)), grid_color, 1.0f);
    }

    for (int y = 0; y <= MAP_HEIGHT_TILES; ++y) {
        float line_y = float(y * TILE_SIZE);
        draw_line(Vector2(0.0f, line_y), Vector2(float(map_pixel_width), line_y), grid_color, 1.0f);
    }

    draw_rect(Rect2(Vector2(), Vector2(map_pixel_width, map_pixel_height)), boundary_color, false, 3.0f);

    for (const Structure &structure : structures) {
        draw_rect(structure.bounds, structure.fill_color, true);
        draw_rect(structure.bounds, structure.accent_color, false, 3.0f);

        const Vector2 stripe_origin = structure.bounds.position + Vector2(10.0f, 10.0f);
        const real_t stripe_width = std::max(0.0f, structure.bounds.size.x - 20.0f);
        for (real_t y = stripe_origin.y; y < structure.bounds.position.y + structure.bounds.size.y - 10.0f; y += 26.0f) {
            draw_line(
                Vector2(stripe_origin.x, y),
                Vector2(stripe_origin.x + stripe_width, y),
                Color(0.42f, 0.49f, 0.56f, 0.45f),
                2.0f
            );
        }
    }

    if (quadtree_overlay_visible) {
        for (const Rect2 &bounds : quadtree_debug_bounds) {
            draw_rect(bounds, quadtree_color, false, 1.0f);
        }

        for (const Rect2 &bounds : quadtree_debug_visited) {
            draw_rect(bounds, quadtree_visited_color, false, 2.0f);
        }

        for (const Rect2 &bounds : quadtree_debug_queries) {
            draw_rect(bounds, quadtree_query_color, false, 2.0f);
        }
    }

    if (player != nullptr) {
        draw_circle(player->get_position(), 18.0f, Color(0.42f, 0.93f, 0.57f));

        if (player->is_reloading()) {
            const Vector2 center = player->get_position();
            const real_t radius = 28.0f;
            const real_t start_angle = -Math_PI / 2.0;
            const real_t end_angle = start_angle + (Math_TAU * player->get_reload_progress());
            const int point_count = 48;
            draw_arc(center, radius, start_angle, start_angle + Math_TAU, point_count, Color(1.0f, 1.0f, 1.0f, 0.18f), 3.0f, true);
            draw_arc(center, radius, start_angle, end_angle, point_count, Color(1.0f, 1.0f, 1.0f), 5.0f, true);
        }

        if (player->is_cone_weapon_active()) {
            const Vector2 center = player->get_position();
            Vector2 aim_direction = get_global_mouse_position() - center;
            if (aim_direction.length_squared() > 0.0f) {
                aim_direction = aim_direction.normalized();
                const real_t cone_range = 260.0f;
                const real_t cone_half_angle = 0.48f;
                const real_t center_angle = aim_direction.angle();
                const real_t start_angle = center_angle - cone_half_angle;
                const real_t end_angle = center_angle + cone_half_angle;
                draw_arc(center, cone_range, start_angle, end_angle, 36, Color(0.8f, 0.62f, 1.0f, 0.8f), 4.0f, true);

                PackedVector2Array cone_fill;
                cone_fill.push_back(center);
                for (int i = 0; i <= 18; ++i) {
                    const real_t t = real_t(i) / 18.0f;
                    const real_t angle = start_angle + ((end_angle - start_angle) * t);
                    cone_fill.push_back(center + Vector2(Math::cos(angle), Math::sin(angle)) * cone_range);
                }
                draw_colored_polygon(cone_fill, Color(0.8f, 0.62f, 1.0f, 0.08f));
            }
        }

        const Pickup *nearest_pickup = find_nearest_pickup_in_range(real_t(PICKUP_COMPASS_RANGE_TILES * TILE_SIZE));
        if (nearest_pickup != nullptr) {
            const Vector2 direction = (nearest_pickup->position - player->get_position()).normalized();
            const Vector2 arrow_tip = player->get_position() + (direction * 46.0f);
            const Vector2 arrow_base = player->get_position() + (direction * 30.0f);
            const Vector2 perpendicular(-direction.y, direction.x);

            PackedVector2Array arrow_points;
            arrow_points.push_back(arrow_tip);
            arrow_points.push_back(arrow_base + perpendicular * 8.0f);
            arrow_points.push_back(arrow_base - perpendicular * 8.0f);

            draw_line(player->get_position() + direction * 20.0f, arrow_base, compass_color, 3.0f);
            draw_colored_polygon(arrow_points, compass_color);
        }
    }

    for (const Bullet &bullet : bullets) {
        draw_circle(bullet.position, 5.0f, bullet_color);
    }

    for (const PlayerFlameBurst &burst : player_flame_bursts) {
        const real_t fade = 1.0f - (burst.elapsed_time / burst.lifetime);
        const real_t center_angle = burst.direction.angle();
        const real_t start_angle = center_angle - burst.half_angle;
        const real_t end_angle = center_angle + burst.half_angle;

        PackedVector2Array outer_flame;
        PackedVector2Array inner_flame;
        outer_flame.push_back(burst.origin);
        inner_flame.push_back(burst.origin);
        for (int i = 0; i <= 18; ++i) {
            const real_t t = real_t(i) / 18.0f;
            const real_t angle = start_angle + ((end_angle - start_angle) * t);
            const real_t wave = 0.86f + (0.14f * Math::sin((t * 11.0f) + (burst.elapsed_time * 18.0f)));
            const real_t outer_range = burst.range * wave;
            const real_t inner_range = burst.range * 0.62f * wave;
            const Vector2 dir(Math::cos(angle), Math::sin(angle));
            outer_flame.push_back(burst.origin + dir * outer_range);
            inner_flame.push_back(burst.origin + dir * inner_range);
        }

        draw_colored_polygon(outer_flame, Color(1.0f, 0.36f, 0.08f, 0.20f * fade));
        draw_colored_polygon(inner_flame, Color(1.0f, 0.82f, 0.26f, 0.34f * fade));
        draw_arc(burst.origin, burst.range * (0.95f + (0.05f * fade)), start_angle, end_angle, 28, Color(1.0f, 0.76f, 0.2f, 0.55f * fade), 5.0f, true);
    }

    if (quadtree_overlay_visible) {
        for (const Vector2 &candidate : quadtree_debug_candidates) {
            draw_circle(candidate, 4.0f, Color(1.0f, 0.95f, 0.4f, 0.85f));
        }
    }

    for (const FireHazard &fire : fire_hazards) {
        const PackedVector2Array outer_fire = build_fire_shape_points(fire.position, fire.radius, fire.elapsed_time, fire.shape_seed, 1.0f);
        const PackedVector2Array mid_fire = build_fire_shape_points(fire.position, fire.radius, fire.elapsed_time, fire.shape_seed + 0.9f, 0.72f);
        const PackedVector2Array core_fire = build_fire_shape_points(fire.position, fire.radius, fire.elapsed_time, fire.shape_seed + 1.8f, 0.42f);

        draw_colored_polygon(outer_fire, Color(0.94f, 0.24f, 0.08f, 0.36f));
        draw_colored_polygon(mid_fire, Color(1.0f, 0.58f, 0.12f, 0.44f));
        draw_colored_polygon(core_fire, Color(1.0f, 0.84f, 0.26f, 0.52f));
        draw_arc(fire.position, fire.radius, 0.0f, Math_TAU, 48, Color(1.0f, 0.55f, 0.18f, 0.24f), 2.0f, true);
    }

    for (const OilBarrel &barrel : oil_barrels) {
        if (barrel.exploded) {
            continue;
        }

        draw_rect(Rect2(barrel.position - Vector2(14.0f, 18.0f), Vector2(28.0f, 36.0f)), oil_barrel_color, true);
        draw_rect(Rect2(barrel.position - Vector2(16.0f, 20.0f), Vector2(32.0f, 40.0f)), Color(0.2f, 0.05f, 0.04f), false, 3.0f);
        draw_line(barrel.position + Vector2(-12.0f, -8.0f), barrel.position + Vector2(12.0f, -8.0f), Color(0.98f, 0.82f, 0.25f), 2.0f);
        draw_line(barrel.position + Vector2(-12.0f, 8.0f), barrel.position + Vector2(12.0f, 8.0f), Color(0.98f, 0.82f, 0.25f), 2.0f);
    }

    for (const EnemyWave &wave : enemy_waves) {
        const real_t wave_angle = wave.velocity.angle();
        const real_t arc_start = wave_angle - (Math_PI * 0.65);
        const real_t arc_end = wave_angle + (Math_PI * 0.65);
        draw_arc(wave.position, 15.0f, arc_start, arc_end, 24, enemy_wave_color, 7.0f, true);
        draw_arc(wave.position, 9.0f, arc_start + 0.18, arc_end - 0.18, 24, Color(1.0f, 0.78f, 0.9f, 0.8f), 3.0f, true);
    }

    for (const Enemy *enemy : enemies) {
        if (enemy == nullptr) {
            continue;
        }

        const Vector2 center = enemy->get_position();
        if (enemy->is_chaser()) {
            const real_t damage_ratio = real_t(enemy->get_damage_taken()) / real_t(enemy->get_max_hit_points());
            Color diamond_color = Color(0.22f, 0.55f, 1.0f).lerp(Color(0.66f, 0.80f, 0.96f), damage_ratio);
            if (enemy->is_frozen()) {
                diamond_color = diamond_color.lerp(Color(0.82f, 0.93f, 1.0f), 0.35f);
            }
            PackedVector2Array diamond = build_diamond_points(center, 16.0f, 28.0f);
            draw_colored_polygon(diamond, diamond_color);
            draw_polyline(diamond, Color(0.05f, 0.18f, 0.42f), 3.0f, true);

            const int cracks = enemy->get_damage_taken();
            if (cracks >= 1) {
                draw_line(center + Vector2(-5.0f, -12.0f), center + Vector2(2.0f, 5.0f), Color(0.36f, 0.52f, 0.78f), 2.0f);
            }
            if (cracks >= 2) {
                draw_line(center + Vector2(6.0f, -7.0f), center + Vector2(-3.0f, 3.0f), Color(0.36f, 0.52f, 0.78f), 2.0f);
            }
            if (cracks >= 3) {
                draw_line(center + Vector2(-7.0f, 8.0f), center + Vector2(8.0f, 12.0f), Color(0.36f, 0.52f, 0.78f), 2.0f);
            }

            if (enemy->is_frozen()) {
                const real_t radius = 34.0f;
                const real_t start_angle = -Math_PI / 2.0;
                const real_t end_angle = start_angle + (Math_TAU * enemy->get_freeze_progress());
                draw_arc(center, radius, start_angle, start_angle + Math_TAU, 48, Color(1.0f, 1.0f, 1.0f, 0.18f), 3.0f, true);
                draw_arc(center, radius, start_angle, end_angle, 48, Color(0.82f, 0.93f, 1.0f), 5.0f, true);
            }
            continue;
        }

        const real_t damage_ratio = real_t(enemy->get_damage_taken()) / real_t(enemy->get_max_hit_points());
        const Color pentagon_color = Color(1.0f, 0.18f, 0.18f).lerp(Color(0.94f, 0.62f, 0.62f), damage_ratio);
        PackedVector2Array pentagon = build_pentagon_points(center, 22.0f);
        draw_colored_polygon(pentagon, pentagon_color);
        draw_polyline(pentagon, Color(0.4f, 0.05f, 0.05f), 3.0f, true);

        const int cracks = enemy->get_damage_taken();
        if (cracks >= 1) {
            draw_line(center + Vector2(-6.0f, -10.0f), center + Vector2(3.0f, 6.0f), Color(0.55f, 0.18f, 0.18f), 2.0f);
        }
        if (cracks >= 2) {
            draw_line(center + Vector2(7.0f, -8.0f), center + Vector2(-2.0f, 2.0f), Color(0.55f, 0.18f, 0.18f), 2.0f);
        }
        if (cracks >= 3) {
            draw_line(center + Vector2(-10.0f, 4.0f), center + Vector2(9.0f, 10.0f), Color(0.55f, 0.18f, 0.18f), 2.0f);
        }
    }

    for (const Pickup &pickup : pickups) {
        if (pickup.collected) {
            continue;
        }

        Color pickup_color = ammo_pickup_color;
        switch (pickup.type) {
            case PickupType::AMMO:
                pickup_color = ammo_pickup_color;
                break;
            case PickupType::HEALTH:
                pickup_color = health_pickup_color;
                break;
            case PickupType::SPEED:
                pickup_color = speed_pickup_color;
                break;
            case PickupType::CONE:
                pickup_color = cone_pickup_color;
                break;
        }
        draw_circle(pickup.position, 14.0f, pickup_color);
        draw_circle(pickup.position, 18.0f, Color(pickup_color.r, pickup_color.g, pickup_color.b, 0.18f));

        if (pickup.type == PickupType::AMMO) {
            draw_rect(Rect2(pickup.position - Vector2(8.0f, 4.0f), Vector2(16.0f, 8.0f)), Color(1.0f, 1.0f, 1.0f), true);
            draw_line(pickup.position + Vector2(0.0f, -7.0f), pickup.position + Vector2(0.0f, 7.0f), pickup_color, 2.0f);
        } else if (pickup.type == PickupType::HEALTH) {
            draw_rect(Rect2(pickup.position - Vector2(3.0f, 9.0f), Vector2(6.0f, 18.0f)), Color(1.0f, 1.0f, 1.0f), true);
            draw_rect(Rect2(pickup.position - Vector2(9.0f, 3.0f), Vector2(18.0f, 6.0f)), Color(1.0f, 1.0f, 1.0f), true);
        } else if (pickup.type == PickupType::SPEED) {
            draw_arc(pickup.position, 9.0f, -Math_PI * 0.35f, Math_PI * 0.9f, 20, Color(1.0f, 1.0f, 1.0f), 3.0f, true);
            PackedVector2Array bolt;
            bolt.push_back(pickup.position + Vector2(-2.0f, -10.0f));
            bolt.push_back(pickup.position + Vector2(5.0f, -3.0f));
            bolt.push_back(pickup.position + Vector2(0.0f, -2.0f));
            bolt.push_back(pickup.position + Vector2(6.0f, 9.0f));
            bolt.push_back(pickup.position + Vector2(-6.0f, 2.0f));
            bolt.push_back(pickup.position + Vector2(-1.0f, 1.0f));
            draw_colored_polygon(bolt, Color(1.0f, 1.0f, 1.0f));
        } else if (pickup.type == PickupType::CONE) {
            draw_arc(pickup.position, 11.0f, -0.5f, 0.5f, 20, Color(1.0f, 1.0f, 1.0f), 4.0f, true);
            draw_arc(pickup.position, 6.0f, -0.5f, 0.5f, 20, Color(1.0f, 1.0f, 1.0f), 3.0f, true);
            draw_line(pickup.position + Vector2(-8.0f, 0.0f), pickup.position + Vector2(-2.0f, 0.0f), Color(1.0f, 1.0f, 1.0f), 3.0f);
        }
    }
}

void GameWorld::_unhandled_input(const Ref<InputEvent> &event) {
    if (player == nullptr) {
        return;
    }

    if (event.is_valid() && event->is_action_pressed("toggle_pause")) {
        if (!game_over) {
            set_gameplay_paused(!gameplay_paused);
        }
        return;
    }

    if (event.is_valid() && event->is_action_pressed("toggle_map_view")) {
        expanded_map_view = !expanded_map_view;
        apply_camera_view_mode();
        update_hud();
        queue_redraw();
        return;
    }

    if (event.is_valid() && event->is_action_pressed("toggle_spatial_mode")) {
        use_quadtree_spatial_index = !use_quadtree_spatial_index;
        clear_quadtree_debug();
        rebuild_spatial_index();
        update_hud();
        queue_redraw();
        return;
    }

    if (event.is_valid() && event->is_action_pressed("switch_level")) {
        SceneTree *tree = get_tree();
        if (tree != nullptr) {
            tree->change_scene_to_file("res://scenes/efficiency_level.tscn");
        }
        return;
    }

    if (game_over || gameplay_paused) {
        return;
    }

    if (event.is_valid() && event->is_action_pressed("toggle_quadtree_overlay")) {
        quadtree_overlay_visible = !quadtree_overlay_visible;
        update_hud();
        queue_redraw();
        return;
    }

    Ref<InputEventMouseButton> mouse_button = event;
    if (mouse_button.is_valid() && mouse_button->is_pressed() && mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
        Vector2 target = get_global_mouse_position();
        if (player->try_shoot(target)) {
            fire_player_weapon(target);
            if (player->is_cone_weapon_active()) {
                player_cone_fire_cooldown_timer = 0.08;
            }
        }
    }
}

void GameWorld::restart_game() {
    SceneTree *tree = get_tree();
    if (tree != nullptr) {
        tree->reload_current_scene();
    }
}
