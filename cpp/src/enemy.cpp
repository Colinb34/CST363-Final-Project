#include "enemy.h"

#include <algorithm>

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Enemy::_bind_methods() {
    ClassDB::bind_method(D_METHOD("apply_damage", "amount"), &Enemy::apply_damage);
    ClassDB::bind_method(D_METHOD("apply_environment_damage", "amount"), &Enemy::apply_environment_damage);
    ClassDB::bind_method(D_METHOD("can_fire"), &Enemy::can_fire);
    ClassDB::bind_method(D_METHOD("is_frozen"), &Enemy::is_frozen);
    ClassDB::bind_method(D_METHOD("is_chaser"), &Enemy::is_chaser);
    ClassDB::bind_method(D_METHOD("reset_shot_cooldown"), &Enemy::reset_shot_cooldown);
    ClassDB::bind_method(D_METHOD("reset_freeze_timer"), &Enemy::reset_freeze_timer);
    ClassDB::bind_method(D_METHOD("update_shot_cooldown", "delta"), &Enemy::update_shot_cooldown);
    ClassDB::bind_method(D_METHOD("update_freeze", "delta"), &Enemy::update_freeze);
    ClassDB::bind_method(D_METHOD("update_sight_memory", "delta", "has_line_of_sight"), &Enemy::update_sight_memory);
    ClassDB::bind_method(D_METHOD("has_recent_sight"), &Enemy::has_recent_sight);
    ClassDB::bind_method(D_METHOD("set_wander_direction", "direction"), &Enemy::set_wander_direction);
    ClassDB::bind_method(D_METHOD("get_wander_direction"), &Enemy::get_wander_direction);
    ClassDB::bind_method(D_METHOD("set_wander_timer", "duration"), &Enemy::set_wander_timer);
    ClassDB::bind_method(D_METHOD("get_wander_timer"), &Enemy::get_wander_timer);
    ClassDB::bind_method(D_METHOD("update_wander_timer", "delta"), &Enemy::update_wander_timer);
    ClassDB::bind_method(D_METHOD("set_enemy_type", "enemy_type"), &Enemy::set_enemy_type);
    ClassDB::bind_method(D_METHOD("get_enemy_type"), &Enemy::get_enemy_type);
    ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &Enemy::set_move_speed);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &Enemy::get_move_speed);
    ClassDB::bind_method(D_METHOD("set_freeze_duration", "freeze_duration"), &Enemy::set_freeze_duration);
    ClassDB::bind_method(D_METHOD("get_freeze_duration"), &Enemy::get_freeze_duration);
    ClassDB::bind_method(D_METHOD("get_freeze_progress"), &Enemy::get_freeze_progress);
    ClassDB::bind_method(D_METHOD("set_chase_activation_range", "chase_activation_range"), &Enemy::set_chase_activation_range);
    ClassDB::bind_method(D_METHOD("get_chase_activation_range"), &Enemy::get_chase_activation_range);
    ClassDB::bind_method(D_METHOD("set_hit_points", "hit_points"), &Enemy::set_hit_points);
    ClassDB::bind_method(D_METHOD("get_hit_points"), &Enemy::get_hit_points);
    ClassDB::bind_method(D_METHOD("get_max_hit_points"), &Enemy::get_max_hit_points);
    ClassDB::bind_method(D_METHOD("get_damage_taken"), &Enemy::get_damage_taken);
    ClassDB::bind_method(D_METHOD("get_shot_cooldown_duration"), &Enemy::get_shot_cooldown_duration);
    ClassDB::bind_method(D_METHOD("get_shot_cooldown_progress"), &Enemy::get_shot_cooldown_progress);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "enemy_type"), "set_enemy_type", "get_enemy_type");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed"), "set_move_speed", "get_move_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "freeze_duration"), "set_freeze_duration", "get_freeze_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chase_activation_range"), "set_chase_activation_range", "get_chase_activation_range");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "hit_points"), "set_hit_points", "get_hit_points");
}

void Enemy::_physics_process(double) {}

bool Enemy::apply_damage(int amount) {
    if (amount <= 0 || hit_points <= 0 || is_frozen()) {
        return false;
    }

    hit_points = std::max(0, hit_points - amount);
    if (is_chaser() && hit_points > 0) {
        reset_freeze_timer();
    }
    return true;
}

bool Enemy::apply_environment_damage(int amount) {
    if (amount <= 0 || hit_points <= 0) {
        return false;
    }

    hit_points = std::max(0, hit_points - amount);
    return true;
}

bool Enemy::can_fire() const {
    return hit_points > 0 && !is_frozen() && !is_chaser() && shot_cooldown_timer <= 0.0;
}

bool Enemy::is_frozen() const {
    return freeze_timer > 0.0;
}

bool Enemy::is_chaser() const {
    return enemy_type == EnemyType::CHASER;
}

void Enemy::reset_shot_cooldown() {
    shot_cooldown_timer = shot_cooldown_duration;
}

void Enemy::reset_freeze_timer() {
    freeze_timer = freeze_duration;
}

void Enemy::update_shot_cooldown(double delta) {
    shot_cooldown_timer = std::max(0.0, shot_cooldown_timer - delta);
}

void Enemy::update_freeze(double delta) {
    freeze_timer = std::max(0.0, freeze_timer - delta);
}

void Enemy::update_sight_memory(double delta, bool has_line_of_sight) {
    if (has_line_of_sight) {
        sight_memory_timer = sight_memory_duration;
        return;
    }

    sight_memory_timer = std::max(0.0, sight_memory_timer - delta);
}

bool Enemy::has_recent_sight() const {
    return sight_memory_timer > 0.0;
}

void Enemy::set_wander_direction(const Vector2 &direction) {
    wander_direction = direction;
}

Vector2 Enemy::get_wander_direction() const {
    return wander_direction;
}

void Enemy::set_wander_timer(double duration) {
    wander_timer = std::max(0.0, duration);
}

double Enemy::get_wander_timer() const {
    return wander_timer;
}

void Enemy::update_wander_timer(double delta) {
    wander_timer = std::max(0.0, wander_timer - delta);
}

void Enemy::set_enemy_type(int p_enemy_type) {
    enemy_type = p_enemy_type == 1 ? EnemyType::CHASER : EnemyType::SHOOTER;
}

int Enemy::get_enemy_type() const {
    return enemy_type == EnemyType::CHASER ? 1 : 0;
}

void Enemy::set_move_speed(double p_speed) {
    move_speed = p_speed;
}

double Enemy::get_move_speed() const {
    return move_speed;
}

void Enemy::set_freeze_duration(double p_freeze_duration) {
    freeze_duration = std::max(0.0, p_freeze_duration);
}

double Enemy::get_freeze_duration() const {
    return freeze_duration;
}

double Enemy::get_freeze_progress() const {
    if (!is_frozen() || freeze_duration <= 0.0) {
        return 0.0;
    }

    return freeze_timer / freeze_duration;
}

void Enemy::set_chase_activation_range(double p_range) {
    chase_activation_range = std::max(0.0, p_range);
}

double Enemy::get_chase_activation_range() const {
    return chase_activation_range;
}

void Enemy::set_hit_points(int p_hit_points) {
    hit_points = std::clamp(p_hit_points, 0, max_hit_points);
}

int Enemy::get_hit_points() const {
    return hit_points;
}

int Enemy::get_max_hit_points() const {
    return max_hit_points;
}

int Enemy::get_damage_taken() const {
    return max_hit_points - hit_points;
}

double Enemy::get_shot_cooldown_duration() const {
    return shot_cooldown_duration;
}

double Enemy::get_shot_cooldown_progress() const {
    if (shot_cooldown_duration <= 0.0) {
        return 1.0;
    }

    return 1.0 - (shot_cooldown_timer / shot_cooldown_duration);
}
