#include "player_controller.h"

#include <algorithm>

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void PlayerController::_bind_methods() {
    ClassDB::bind_method(D_METHOD("try_shoot", "target_global_position"), &PlayerController::try_shoot);
    ClassDB::bind_method(D_METHOD("reload"), &PlayerController::reload);
    ClassDB::bind_method(D_METHOD("add_reserve_ammo", "amount"), &PlayerController::add_reserve_ammo);
    ClassDB::bind_method(D_METHOD("heal", "amount"), &PlayerController::heal);
    ClassDB::bind_method(D_METHOD("take_damage", "amount"), &PlayerController::take_damage);
    ClassDB::bind_method(D_METHOD("get_bullets_in_magazine"), &PlayerController::get_bullets_in_magazine);
    ClassDB::bind_method(D_METHOD("get_current_health"), &PlayerController::get_current_health);
    ClassDB::bind_method(D_METHOD("get_magazine_size"), &PlayerController::get_magazine_size);
    ClassDB::bind_method(D_METHOD("get_max_health"), &PlayerController::get_max_health);
    ClassDB::bind_method(D_METHOD("get_reserve_ammo"), &PlayerController::get_reserve_ammo);
    ClassDB::bind_method(D_METHOD("get_reload_duration"), &PlayerController::get_reload_duration);
    ClassDB::bind_method(D_METHOD("get_reload_progress"), &PlayerController::get_reload_progress);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &PlayerController::get_move_speed);
    ClassDB::bind_method(D_METHOD("is_reloading"), &PlayerController::is_reloading);
}

void PlayerController::_ready() {
    set_collision_layer_value(1, true);
    set_collision_mask_value(1, true);
}

void PlayerController::_physics_process(double delta) {
    Input *input = Input::get_singleton();
    Vector2 direction;

    direction.x = input->get_action_strength("move_right") - input->get_action_strength("move_left");
    direction.y = input->get_action_strength("move_down") - input->get_action_strength("move_up");

    if (direction.length_squared() > 0.0) {
        direction = direction.normalized();
    }

    set_velocity(direction * move_speed);
    move_and_slide();

    if (reload_timer > 0.0) {
        reload_timer = std::max(0.0, reload_timer - delta);
        if (reload_timer == 0.0) {
            finish_reload();
        }
    }

    if (input->is_action_just_pressed("reload")) {
        reload();
    }
}

bool PlayerController::try_shoot(const Vector2 &) {
    if (is_reloading()) {
        return false;
    }

    if (bullets_in_magazine <= 0) {
        reload();
        return false;
    }

    bullets_in_magazine -= 1;
    return true;
}

void PlayerController::reload() {
    int missing_bullets = magazine_size - bullets_in_magazine;
    if (is_reloading() || missing_bullets <= 0 || reserve_ammo <= 0) {
        return;
    }

    reload_timer = reload_duration;
}

void PlayerController::finish_reload() {
    int missing_bullets = magazine_size - bullets_in_magazine;
    if (missing_bullets <= 0 || reserve_ammo <= 0) {
        return;
    }

    int bullets_to_load = std::min(missing_bullets, reserve_ammo);
    bullets_in_magazine += bullets_to_load;
    reserve_ammo -= bullets_to_load;
}

bool PlayerController::add_reserve_ammo(int amount) {
    if (amount <= 0) {
        return false;
    }

    reserve_ammo += amount;
    return true;
}

bool PlayerController::heal(int amount) {
    if (amount <= 0 || current_health >= max_health) {
        return false;
    }

    current_health = std::min(max_health, current_health + amount);
    return true;
}

bool PlayerController::take_damage(int amount) {
    if (amount <= 0 || current_health <= 0) {
        return false;
    }

    current_health = std::max(0, current_health - amount);
    return true;
}

int PlayerController::get_bullets_in_magazine() const {
    return bullets_in_magazine;
}

int PlayerController::get_current_health() const {
    return current_health;
}

int PlayerController::get_magazine_size() const {
    return magazine_size;
}

int PlayerController::get_max_health() const {
    return max_health;
}

int PlayerController::get_reserve_ammo() const {
    return reserve_ammo;
}

double PlayerController::get_reload_duration() const {
    return reload_duration;
}

double PlayerController::get_reload_progress() const {
    if (!is_reloading() || reload_duration <= 0.0) {
        return 0.0;
    }

    return 1.0 - (reload_timer / reload_duration);
}

double PlayerController::get_move_speed() const {
    return move_speed;
}

bool PlayerController::is_reloading() const {
    return reload_timer > 0.0;
}
