extends CharacterBody2D

const PLAYER_GROUP := "player"

@export var move_speed := 240.0
@export var fire_cooldown := 0.18
@export var bullet_speed := 820.0
@export var bullet_damage := 1
@export var max_health := 8

var health := max_health
var fire_timer := 0.0
var world: Node = null

func _ready() -> void:
	health = max_health
	add_to_group(PLAYER_GROUP)

func _physics_process(delta: float) -> void:
	var input_vector := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	velocity = input_vector * move_speed
	move_and_slide()

	look_at(get_global_mouse_position())

	fire_timer = maxf(fire_timer - delta, 0.0)
	if Input.is_action_pressed("shoot") and fire_timer <= 0.0 and world != null:
		fire_timer = fire_cooldown
		var bullet_direction := (get_global_mouse_position() - global_position).normalized()
		world.spawn_bullet(global_position + bullet_direction * 20.0, bullet_direction, bullet_speed, bullet_damage, "player_bullets", "enemy", Color(0.541176, 0.929412, 1, 1))

func take_damage(amount: int) -> void:
	health -= amount
	modulate = Color(1, 0.6, 0.6)
	if health <= 0:
		get_tree().reload_current_scene()

func _process(delta: float) -> void:
	modulate = modulate.lerp(Color.WHITE, delta * 10.0)

func get_hit_bounds() -> Rect2:
	return Rect2(global_position - Vector2(14, 14), Vector2(28, 28))
