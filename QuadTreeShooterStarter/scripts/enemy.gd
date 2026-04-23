extends CharacterBody2D

@export var enemy_type := "chaser"
@export var move_speed := 120.0
@export var max_health := 3
@export var shoot_cooldown := 1.0
@export var preferred_range := 260.0

var health := max_health
var shoot_timer := 0.0
var player: Node2D = null
var world: Node = null

func _ready() -> void:
	health = max_health
	add_to_group("enemy")

func _physics_process(delta: float) -> void:
	if player == null or not is_instance_valid(player):
		return

	var to_player := player.global_position - global_position
	var distance := to_player.length()
	var direction := to_player.normalized()

	if enemy_type == "chaser":
		velocity = direction * move_speed
	else:
		if distance > preferred_range + 40.0:
			velocity = direction * move_speed
		elif distance < preferred_range - 40.0:
			velocity = -direction * move_speed
		else:
			velocity = Vector2.ZERO

	move_and_slide()
	look_at(player.global_position)

	shoot_timer = maxf(shoot_timer - delta, 0.0)
	if enemy_type == "shooter" and shoot_timer <= 0.0 and distance <= preferred_range + 120.0 and _has_line_of_sight():
		shoot_timer = shoot_cooldown
		world.spawn_bullet(global_position + direction * 18.0, direction, 520.0, 1, "enemy_bullets", "player", Color(1, 0.556863, 0.556863, 1))

func take_damage(amount: int) -> void:
	health -= amount
	modulate = Color(1, 0.65, 0.65)
	if health <= 0:
		queue_free()

func _process(delta: float) -> void:
	modulate = modulate.lerp(Color.WHITE, delta * 10.0)

func get_hit_bounds() -> Rect2:
	return Rect2(global_position - Vector2(14, 14), Vector2(28, 28))

func _has_line_of_sight() -> bool:
	if player == null:
		return false
	var query := PhysicsRayQueryParameters2D.create(global_position, player.global_position)
	query.collision_mask = 2
	var hit := get_world_2d().direct_space_state.intersect_ray(query)
	return hit.is_empty()
