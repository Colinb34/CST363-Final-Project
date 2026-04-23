#ifndef TOPDOWN_PLAYER_CONTROLLER_H
#define TOPDOWN_PLAYER_CONTROLLER_H

#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {

class PlayerController : public CharacterBody2D {
    GDCLASS(PlayerController, CharacterBody2D)

private:
    double move_speed = 300.0;
    double reload_duration = 1.5;
    double reload_timer = 0.0;
    int max_health = 5;
    int current_health = 5;
    int magazine_size = 8;
    int bullets_in_magazine = 8;
    int reserve_ammo = 64;

    void finish_reload();

protected:
    static void _bind_methods();

public:
    PlayerController() = default;
    ~PlayerController() override = default;

    void _ready() override;
    void _physics_process(double delta) override;

    bool try_shoot(const Vector2 &target_global_position);
    void reload();
    bool add_reserve_ammo(int amount);
    bool heal(int amount);
    bool take_damage(int amount);

    int get_bullets_in_magazine() const;
    int get_current_health() const;
    int get_magazine_size() const;
    int get_max_health() const;
    int get_reserve_ammo() const;
    double get_reload_duration() const;
    double get_reload_progress() const;
    double get_move_speed() const;
    bool is_reloading() const;
};

}

#endif
