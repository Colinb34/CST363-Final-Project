extends Node2D

const WORLD_SIZE := Vector2(1280, 720)

@export var bullet_scene: PackedScene = preload("res://scenes/bullet.tscn")

var quadtree: QuadTree
var player: CharacterBody2D

@onready var arena: Node2D = $Arena
@onready var projectiles: Node2D = $Arena/Projectiles
@onready var enemies_root: Node2D = $Arena/Enemies

func _ready() -> void:
	player = $Arena/Player
	player.world = self
	quadtree = QuadTree.new(Rect2(Vector2.ZERO, WORLD_SIZE))

	for enemy in enemies_root.get_children():
		enemy.player = player
		enemy.world = self

func _physics_process(_delta: float) -> void:
	_rebuild_quadtree()
	_cleanup_invalid_enemy_refs()

func spawn_bullet(start_position: Vector2, direction: Vector2, bullet_speed: float, damage: int, owner_group: String, target_group: String, tint: Color) -> void:
	var bullet := bullet_scene.instantiate()
	projectiles.add_child(bullet)
	bullet.setup(start_position, direction, bullet_speed, damage, owner_group, target_group, self, tint)

func find_target_hit(start_point: Vector2, end_point: Vector2, target_group: String, collision_radius: float) -> Node:
	var min_point := Vector2(minf(start_point.x, end_point.x), minf(start_point.y, end_point.y)) - Vector2.ONE * collision_radius
	var max_point := Vector2(maxf(start_point.x, end_point.x), maxf(start_point.y, end_point.y)) + Vector2.ONE * collision_radius
	var search_area := Rect2(min_point, max_point - min_point)
	var candidates: Array = []
	quadtree.query(search_area, candidates)

	var best_target: Node = null
	var best_distance := INF
	for candidate in candidates:
		if not is_instance_valid(candidate):
			continue
		if not candidate.is_in_group(target_group):
			continue

		var closest: Vector2 = Geometry2D.get_closest_point_to_segment(candidate.global_position, start_point, end_point)
		var distance_to_line: float = candidate.global_position.distance_to(closest)
		if distance_to_line > collision_radius + 14.0:
			continue

		var travel_distance: float = start_point.distance_to(closest)
		if travel_distance < best_distance:
			best_distance = travel_distance
			best_target = candidate

	return best_target

func _rebuild_quadtree() -> void:
	quadtree.clear()
	if is_instance_valid(player):
		quadtree.insert(player, player.get_hit_bounds())
	for enemy in enemies_root.get_children():
		if is_instance_valid(enemy):
			quadtree.insert(enemy, enemy.get_hit_bounds())

func _cleanup_invalid_enemy_refs() -> void:
	for child in enemies_root.get_children():
		if child == null:
			continue
		if child.player == null:
			child.player = player
		if child.world == null:
			child.world = self
