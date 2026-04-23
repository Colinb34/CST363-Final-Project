#ifndef TOPDOWN_ENEMY_H
#define TOPDOWN_ENEMY_H

#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {

class Enemy : public CharacterBody2D {
    GDCLASS(Enemy, CharacterBody2D)

public:
    enum class EnemyType {
        SHOOTER = 0,
        CHASER = 1
    };

private:
    EnemyType enemy_type = EnemyType::SHOOTER;
    double move_speed = 90.0;
    double shot_cooldown_duration = 3.0;
    double shot_cooldown_timer = 0.0;
    double freeze_duration = 3.0;
    double freeze_timer = 0.0;
    double chase_activation_range = 25.0 * 64.0;
    int max_hit_points = 3;
    int hit_points = 3;

protected:
    static void _bind_methods();

public:
    Enemy() = default;
    ~Enemy() override = default;

    void _physics_process(double delta) override;

    bool apply_damage(int amount);
    bool apply_environment_damage(int amount);
    bool can_fire() const;
    bool is_frozen() const;
    bool is_chaser() const;
    void reset_shot_cooldown();
    void reset_freeze_timer();
    void update_shot_cooldown(double delta);
    void update_freeze(double delta);

    void set_enemy_type(int p_enemy_type);
    int get_enemy_type() const;
    void set_move_speed(double p_speed);
    double get_move_speed() const;
    void set_freeze_duration(double p_freeze_duration);
    double get_freeze_duration() const;
    double get_freeze_progress() const;
    void set_chase_activation_range(double p_range);
    double get_chase_activation_range() const;

    void set_hit_points(int p_hit_points);
    int get_hit_points() const;
    int get_max_hit_points() const;
    int get_damage_taken() const;
    double get_shot_cooldown_duration() const;
    double get_shot_cooldown_progress() const;
};

}

#endif
