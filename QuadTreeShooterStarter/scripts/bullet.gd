extends Node2D

const COLLISION_RADIUS := 6.0

var velocity := Vector2.ZERO
var speed := 720.0
var damage := 1
var lifetime := 2.0
var owner_group := ""
var target_group := ""
var world: Node = null

@onready var visual: Polygon2D = $Visual

func setup(start_position: Vector2, direction: Vector2, bullet_speed: float, hit_damage: int, source_group: String, hit_group: String, world_ref: Node, tint: Color) -> void:
	global_position = start_position
	velocity = direction.normalized()
	speed = bullet_speed
	damage = hit_damage
	owner_group = source_group
	target_group = hit_group
	world = world_ref
	visual.color = tint
	rotation = velocity.angle()

func _physics_process(delta: float) -> void:
	if world == null:
		queue_free()
		return

	var next_position := global_position + velocity * speed * delta
	if _hit_wall(next_position):
		queue_free()
		return

	var target: Node = world.find_target_hit(global_position, next_position, target_group, COLLISION_RADIUS)
	if target != null:
		if target.has_method("take_damage"):
			target.take_damage(damage)
		queue_free()
		return

	global_position = next_position
	rotation = velocity.angle()
	lifetime -= delta
	if lifetime <= 0.0:
		queue_free()

func _hit_wall(next_position: Vector2) -> bool:
	var space_state := get_world_2d().direct_space_state
	var query := PhysicsRayQueryParameters2D.create(global_position, next_position)
	query.collision_mask = 2
	query.exclude = []
	return not space_state.intersect_ray(query).is_empty()
