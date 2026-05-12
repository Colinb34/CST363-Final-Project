extends Node2D

const EfficiencyQuadTreeScript = preload("res://scripts/efficiency_quadtree.gd")
const MAIN_LEVEL_PATH := "res://scenes/main.tscn"

const MODE_QUADTREE := 0
const MODE_BRUTE_FORCE := 1

const GRID_SIZE := 40
const CELL_SIZE := 16.0
const MAP_ORIGIN := Vector2(260.0, 24.0)
const MAP_SIZE := Vector2(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE)
const LEFT_PANEL_POSITION := Vector2(20.0, 24.0)
const LEFT_PANEL_SIZE := Vector2(220.0, 640.0)
const RIGHT_PANEL_POSITION := Vector2(920.0, 24.0)
const RIGHT_PANEL_SIZE := Vector2(260.0, 640.0)
const CPU_PANEL_POSITION := Vector2(260.0, 670.0)
const CPU_PANEL_SIZE := Vector2(640.0, 72.0)
const PLAYER_RADIUS := 6.0
const ENEMY_RADIUS := 5.0
const BULLET_RADIUS := 3.0
const PLAYER_SPEED := 118.0
const BURST_BULLET_COUNT := 10
const MAX_BULLETS := 10000

var map_rect := Rect2(MAP_ORIGIN, MAP_SIZE)
var player_pos := MAP_ORIGIN + Vector2(2.5, 2.5) * CELL_SIZE
var player_hits := 0
var player_flash := 0.0

var walls: Array = []
var enemies: Array = []
var bullets: Array = []

var clock_speed := 60.0
var enemy_fire_rate := 1.5
var bullet_speed := 150.0
var active_mode := MODE_QUADTREE
var show_quadtree := true
var simulation_paused := false
var accumulator := 0.0
var tick_count := 0

var last_tree_bounds: Array = []
var metrics := {
	"brute": {
		"checks": 0.0,
		"queries": 0.0,
		"candidates": 0.0,
		"hits": 0.0,
		"time_us": 0.0,
		"nodes": 0.0,
		"items": 0.0,
	},
	"quad": {
		"checks": 0.0,
		"queries": 0.0,
		"candidates": 0.0,
		"hits": 0.0,
		"time_us": 0.0,
		"nodes": 0.0,
		"items": 0.0,
	},
}
var cpu_stats := {
	"brute": {
		"current": 0.0,
		"average": 0.0,
		"samples": 0,
		"time_us": 0.0,
	},
	"quad": {
		"current": 0.0,
		"average": 0.0,
		"samples": 0,
		"time_us": 0.0,
	},
}

var clock_label: Label
var fire_rate_label: Label
var bullet_speed_label: Label
var stats_label: Label
var active_metrics_label: Label
var active_mode_label: Label
var cpu_tab_container: TabContainer
var quad_cpu_label: Label
var brute_cpu_label: Label
var pause_check: CheckBox
var mode_option: OptionButton


func _ready() -> void:
	_build_level()
	_build_ui()
	update_ui_text()


func _input(event: InputEvent) -> void:
	if event.is_action_pressed("switch_level"):
		get_tree().change_scene_to_file(MAIN_LEVEL_PATH)
		get_viewport().set_input_as_handled()
		return

	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_P:
		_set_paused(not simulation_paused)
		get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	if not simulation_paused:
		accumulator += delta
		var fixed_delta: float = 1.0 / maxf(clock_speed, 1.0)
		var steps := 0
		while accumulator >= fixed_delta and steps < 16:
			_simulate_tick(fixed_delta)
			accumulator -= fixed_delta
			steps += 1

		if steps == 16:
			accumulator = 0.0

		player_flash = max(player_flash - delta, 0.0)
	update_ui_text()
	queue_redraw()


func _simulate_tick(delta: float) -> void:
	tick_count += 1
	_move_player(delta)
	_update_enemies(delta)
	_update_bullets(delta)

	var active_result := _evaluate_collisions(active_mode)
	_record_metrics(active_result)

	if active_mode == MODE_BRUTE_FORCE:
		last_tree_bounds.clear()

	_apply_collision_result(active_result)


func _build_level() -> void:
	var wall_specs := [
		{"cell": Vector2i(6, 5), "size": Vector2i(12, 2)},
		{"cell": Vector2i(23, 6), "size": Vector2i(2, 12)},
		{"cell": Vector2i(4, 18), "size": Vector2i(10, 2)},
		{"cell": Vector2i(17, 21), "size": Vector2i(2, 13)},
		{"cell": Vector2i(25, 24), "size": Vector2i(11, 2)},
		{"cell": Vector2i(31, 10), "size": Vector2i(2, 9)},
	]

	for spec in wall_specs:
		var cell: Vector2i = spec["cell"]
		var size: Vector2i = spec["size"]
		walls.append(Rect2(
			MAP_ORIGIN + Vector2(float(cell.x), float(cell.y)) * CELL_SIZE,
			Vector2(float(size.x), float(size.y)) * CELL_SIZE
		))

	var enemy_cells := [
		Vector2i(4, 7), Vector2i(11, 3), Vector2i(15, 8), Vector2i(21, 3),
		Vector2i(28, 4), Vector2i(36, 7), Vector2i(7, 13), Vector2i(28, 14),
		Vector2i(37, 16), Vector2i(16, 15), Vector2i(11, 23), Vector2i(23, 18),
		Vector2i(37, 24), Vector2i(6, 26), Vector2i(30, 28), Vector2i(12, 34),
		Vector2i(20, 33), Vector2i(26, 31), Vector2i(35, 34), Vector2i(4, 36),
	]

	for i in range(enemy_cells.size()):
		enemies.append({
			"id": i,
			"pos": _cell_center(enemy_cells[i]),
			"cooldown": 0.25 + float(i % 7) * 0.11,
		})


func _build_ui() -> void:
	var ui_layer := CanvasLayer.new()
	add_child(ui_layer)

	_build_cpu_usage_bar(ui_layer)

	var metrics_box := _build_panel(ui_layer, LEFT_PANEL_POSITION, LEFT_PANEL_SIZE)
	var metrics_title := Label.new()
	metrics_title.text = "Efficiency Metrics"
	metrics_title.add_theme_font_size_override("font_size", 20)
	metrics_box.add_child(metrics_title)

	stats_label = Label.new()
	stats_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	metrics_box.add_child(stats_label)

	var metrics_divider := HSeparator.new()
	metrics_box.add_child(metrics_divider)

	active_metrics_label = Label.new()
	active_metrics_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	metrics_box.add_child(active_metrics_label)

	var box := _build_panel(ui_layer, RIGHT_PANEL_POSITION, RIGHT_PANEL_SIZE)

	var title := Label.new()
	title.text = "Controls"
	title.add_theme_font_size_override("font_size", 20)
	box.add_child(title)

	active_mode_label = Label.new()
	box.add_child(active_mode_label)

	var level_hint := Label.new()
	level_hint.text = "Swap level: L"
	box.add_child(level_hint)

	mode_option = OptionButton.new()
	mode_option.add_item("Quadtree", MODE_QUADTREE)
	mode_option.add_item("Basic math scan", MODE_BRUTE_FORCE)
	mode_option.selected = 0
	mode_option.item_selected.connect(_on_mode_selected)
	box.add_child(mode_option)

	pause_check = CheckBox.new()
	pause_check.text = "Paused (P)"
	pause_check.button_pressed = simulation_paused
	pause_check.toggled.connect(_on_pause_toggled)
	box.add_child(pause_check)

	clock_label = Label.new()
	box.add_child(clock_label)

	var clock_slider := HSlider.new()
	clock_slider.min_value = 1.0
	clock_slider.max_value = 240.0
	clock_slider.step = 1.0
	clock_slider.value = clock_speed
	clock_slider.value_changed.connect(_on_clock_speed_changed)
	box.add_child(clock_slider)

	fire_rate_label = Label.new()
	box.add_child(fire_rate_label)

	var fire_slider := HSlider.new()
	fire_slider.min_value = 0.0
	fire_slider.max_value = 12.0
	fire_slider.step = 0.25
	fire_slider.value = enemy_fire_rate
	fire_slider.value_changed.connect(_on_fire_rate_changed)
	box.add_child(fire_slider)

	bullet_speed_label = Label.new()
	box.add_child(bullet_speed_label)

	var bullet_speed_slider := HSlider.new()
	bullet_speed_slider.min_value = 40.0
	bullet_speed_slider.max_value = 500.0
	bullet_speed_slider.step = 5.0
	bullet_speed_slider.value = bullet_speed
	bullet_speed_slider.value_changed.connect(_on_bullet_speed_changed)
	box.add_child(bullet_speed_slider)

	var show_tree_check := CheckBox.new()
	show_tree_check.text = "Show quadtree boxes"
	show_tree_check.button_pressed = show_quadtree
	show_tree_check.toggled.connect(_on_show_tree_toggled)
	box.add_child(show_tree_check)

	var clear_button := Button.new()
	clear_button.text = "Clear bullets"
	clear_button.pressed.connect(_on_clear_bullets_pressed)
	box.add_child(clear_button)


func _build_cpu_usage_bar(parent: Node) -> void:
	var panel := PanelContainer.new()
	panel.position = CPU_PANEL_POSITION
	panel.size = CPU_PANEL_SIZE
	panel.custom_minimum_size = panel.size
	parent.add_child(panel)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 10)
	margin.add_theme_constant_override("margin_right", 10)
	margin.add_theme_constant_override("margin_top", 2)
	margin.add_theme_constant_override("margin_bottom", 2)
	panel.add_child(margin)

	cpu_tab_container = TabContainer.new()
	cpu_tab_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cpu_tab_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	margin.add_child(cpu_tab_container)

	quad_cpu_label = Label.new()
	quad_cpu_label.name = "Quadtree"
	quad_cpu_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	quad_cpu_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	cpu_tab_container.add_child(quad_cpu_label)

	brute_cpu_label = Label.new()
	brute_cpu_label.name = "Basic Scan"
	brute_cpu_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	brute_cpu_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	cpu_tab_container.add_child(brute_cpu_label)


func _build_panel(parent: Node, panel_position: Vector2, panel_size: Vector2) -> VBoxContainer:
	var panel := PanelContainer.new()
	panel.position = panel_position
	panel.size = panel_size
	panel.custom_minimum_size = panel.size
	parent.add_child(panel)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 12)
	margin.add_theme_constant_override("margin_right", 12)
	margin.add_theme_constant_override("margin_top", 12)
	margin.add_theme_constant_override("margin_bottom", 12)
	panel.add_child(margin)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	margin.add_child(scroll)

	var box := VBoxContainer.new()
	box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	box.add_theme_constant_override("separation", 8)
	scroll.add_child(box)
	return box


func _move_player(delta: float) -> void:
	var direction := Vector2.ZERO
	if Input.is_key_pressed(KEY_W):
		direction.y -= 1.0
	if Input.is_key_pressed(KEY_S):
		direction.y += 1.0
	if Input.is_key_pressed(KEY_A):
		direction.x -= 1.0
	if Input.is_key_pressed(KEY_D):
		direction.x += 1.0

	if direction == Vector2.ZERO:
		return

	direction = direction.normalized()
	var motion := direction * PLAYER_SPEED * delta
	var next_x := player_pos + Vector2(motion.x, 0.0)
	if _player_position_is_clear(next_x):
		player_pos.x = next_x.x

	var next_y := player_pos + Vector2(0.0, motion.y)
	if _player_position_is_clear(next_y):
		player_pos.y = next_y.y


func _update_enemies(delta: float) -> void:
	if enemy_fire_rate <= 0.0:
		return

	var interval := 1.0 / enemy_fire_rate
	for i in range(enemies.size()):
		var enemy: Dictionary = enemies[i]
		enemy["cooldown"] = float(enemy["cooldown"]) - delta
		while enemy["cooldown"] <= 0.0:
			_spawn_bullet_burst(enemy)
			enemy["cooldown"] = float(enemy["cooldown"]) + interval
		enemies[i] = enemy


func _update_bullets(delta: float) -> void:
	var kept := []
	for bullet in bullets:
		bullet["pos"] = bullet["pos"] + bullet["vel"] * delta
		bullet["life"] = float(bullet["life"]) - delta
		if bullet["life"] > 0.0 and map_rect.grow(8.0).has_point(bullet["pos"]):
			kept.append(bullet)
	bullets = kept


func _spawn_bullet_burst(enemy: Dictionary) -> void:
	if bullets.size() >= MAX_BULLETS:
		return

	var origin: Vector2 = enemy["pos"]
	var angle_offset := float(tick_count % BURST_BULLET_COUNT) * 0.07
	for i in range(BURST_BULLET_COUNT):
		if bullets.size() >= MAX_BULLETS:
			return

		var angle := TAU * float(i) / float(BURST_BULLET_COUNT) + angle_offset
		var direction := Vector2.from_angle(angle)
		bullets.append({
			"pos": origin + direction * (ENEMY_RADIUS + BULLET_RADIUS + 2.0),
			"vel": direction * bullet_speed,
			"radius": BULLET_RADIUS,
			"owner": int(enemy["id"]),
			"life": 8.0,
		})


func _evaluate_collisions(mode: int) -> Dictionary:
	var started := Time.get_ticks_usec()
	var removals := {}
	var hits := 0
	var player_hits_in_result := 0
	var checks := 0
	var queries := 0
	var candidates_seen := 0
	var nodes := 0
	var items := 0

	var collision_objects := _build_collision_objects()
	var tree = null
	if mode == MODE_QUADTREE:
		tree = EfficiencyQuadTreeScript.new(map_rect)
		for item in collision_objects:
			tree.insert(item)
		nodes = tree.count_nodes()
		items = tree.count_items()
		last_tree_bounds.clear()
		tree.collect_bounds(last_tree_bounds)

	for i in range(bullets.size()):
		var bullet: Dictionary = bullets[i]
		var query_area := _circle_aabb(bullet["pos"], bullet["radius"])
		var candidates := []
		queries += 1

		if mode == MODE_QUADTREE:
			tree.query(query_area, candidates)
			candidates_seen += candidates.size()
		else:
			candidates = collision_objects
			candidates_seen += collision_objects.size()

		for item in candidates:
			if item["kind"] == "enemy" and int(item["id"]) == int(bullet["owner"]):
				continue

			checks += 1
			if _bullet_overlaps_item(bullet, item):
				removals[i] = true
				hits += 1
				if item["kind"] == "player":
					player_hits_in_result += 1
				break

	var elapsed := Time.get_ticks_usec() - started
	return {
		"mode": mode,
		"remove": removals,
		"player_hits": player_hits_in_result,
		"checks": checks,
		"queries": queries,
		"candidates": candidates_seen,
		"hits": hits,
		"time_us": elapsed,
		"nodes": nodes,
		"items": items,
	}


func _apply_collision_result(result: Dictionary) -> void:
	var removals: Dictionary = result["remove"]
	if removals.is_empty():
		return

	var kept := []
	for i in range(bullets.size()):
		if not removals.has(i):
			kept.append(bullets[i])
	bullets = kept
	player_hits += int(result["player_hits"])
	if int(result["player_hits"]) > 0:
		player_flash = 0.18


func _build_collision_objects() -> Array:
	var objects := []
	for i in range(walls.size()):
		objects.append({
			"kind": "wall",
			"id": i,
			"rect": walls[i],
			"aabb": walls[i],
		})

	objects.append({
		"kind": "player",
		"id": -1,
		"pos": player_pos,
		"radius": PLAYER_RADIUS,
		"aabb": _circle_aabb(player_pos, PLAYER_RADIUS),
	})

	for enemy in enemies:
		objects.append({
			"kind": "enemy",
			"id": int(enemy["id"]),
			"pos": enemy["pos"],
			"radius": ENEMY_RADIUS,
			"aabb": _circle_aabb(enemy["pos"], ENEMY_RADIUS),
		})

	return objects


func _bullet_overlaps_item(bullet: Dictionary, item: Dictionary) -> bool:
	var pos: Vector2 = bullet["pos"]
	var radius: float = bullet["radius"]

	match item["kind"]:
		"wall":
			return _circle_overlaps_rect(pos, radius, item["rect"])
		"player", "enemy":
			return _circle_overlaps_circle(pos, radius, item["pos"], item["radius"])
	return false


func _player_position_is_clear(pos: Vector2) -> bool:
	if not map_rect.grow(-PLAYER_RADIUS).has_point(pos):
		return false

	for wall in walls:
		if _circle_overlaps_rect(pos, PLAYER_RADIUS, wall):
			return false

	return true


func _circle_overlaps_rect(center: Vector2, radius: float, rect: Rect2) -> bool:
	var nearest := Vector2(
		clampf(center.x, rect.position.x, rect.end.x),
		clampf(center.y, rect.position.y, rect.end.y)
	)
	return center.distance_squared_to(nearest) <= radius * radius


func _circle_overlaps_circle(a_pos: Vector2, a_radius: float, b_pos: Vector2, b_radius: float) -> bool:
	var total := a_radius + b_radius
	return a_pos.distance_squared_to(b_pos) <= total * total


func _circle_aabb(center: Vector2, radius: float) -> Rect2:
	return Rect2(center - Vector2(radius, radius), Vector2(radius * 2.0, radius * 2.0))


func _cell_center(cell: Vector2i) -> Vector2:
	return MAP_ORIGIN + Vector2(float(cell.x) + 0.5, float(cell.y) + 0.5) * CELL_SIZE


func _record_metrics(result: Dictionary) -> void:
	var key := "quad" if int(result["mode"]) == MODE_QUADTREE else "brute"
	var blend := 0.2
	for metric_name in ["checks", "queries", "candidates", "hits", "time_us", "nodes", "items"]:
		metrics[key][metric_name] = lerpf(
			float(metrics[key][metric_name]),
			float(result[metric_name]),
			blend
		)
	_record_cpu_sample(key, float(result["time_us"]))


func _record_cpu_sample(metric_key: String, time_us: float) -> void:
	var current_cpu := _collision_cpu_percent_for_time(time_us)
	var samples := int(cpu_stats[metric_key]["samples"]) + 1
	var previous_average := float(cpu_stats[metric_key]["average"])
	cpu_stats[metric_key]["samples"] = samples
	cpu_stats[metric_key]["current"] = current_cpu
	cpu_stats[metric_key]["average"] = previous_average + ((current_cpu - previous_average) / float(samples))
	cpu_stats[metric_key]["time_us"] = time_us


func _reset_cpu_stats(metric_key: String) -> void:
	cpu_stats[metric_key]["current"] = 0.0
	cpu_stats[metric_key]["average"] = 0.0
	cpu_stats[metric_key]["samples"] = 0
	cpu_stats[metric_key]["time_us"] = 0.0


func update_ui_text() -> void:
	if clock_label == null:
		return

	var active_name := "Quadtree" if active_mode == MODE_QUADTREE else "Basic math scan"
	active_mode_label.text = "Active: %s%s" % [active_name, " (paused)" if simulation_paused else ""]
	if pause_check != null and pause_check.button_pressed != simulation_paused:
		pause_check.set_pressed_no_signal(simulation_paused)
	clock_label.text = "Forced clock: %d ticks/sec" % int(clock_speed)
	fire_rate_label.text = "Enemy burst rate: %.2f/sec each" % enemy_fire_rate
	bullet_speed_label.text = "Bullet speed: %d px/sec" % int(bullet_speed)
	stats_label.text = "Paused: %s\nBullets: %d / %d\nBurst size: %d\nEnemies: %d\nWalls: %d\nPlayer hits: %d\nTicks: %d\nLevel swap: L" % [
		"yes" if simulation_paused else "no",
		bullets.size(),
		MAX_BULLETS,
		BURST_BULLET_COUNT,
		enemies.size(),
		walls.size(),
		player_hits,
		tick_count,
	]
	var metric_key := "quad" if active_mode == MODE_QUADTREE else "brute"
	_update_cpu_tabs(metric_key)
	if active_mode == MODE_QUADTREE:
		active_metrics_label.text = "Quadtree metrics\nExact checks: %d\nCandidates: %d\nHits: %d\nNodes: %d\nItems: %d\nTime: %.1f us" % [
			int(metrics[metric_key]["checks"]),
			int(metrics[metric_key]["candidates"]),
			int(metrics[metric_key]["hits"]),
			int(metrics[metric_key]["nodes"]),
			int(metrics[metric_key]["items"]),
			metrics[metric_key]["time_us"],
		]
	else:
		active_metrics_label.text = "Basic scan metrics\nExact checks: %d\nCandidates: %d\nHits: %d\nTime: %.1f us" % [
			int(metrics[metric_key]["checks"]),
			int(metrics[metric_key]["candidates"]),
			int(metrics[metric_key]["hits"]),
			metrics[metric_key]["time_us"],
		]


func _update_cpu_tabs(active_metric_key: String) -> void:
	if cpu_tab_container == null:
		return

	cpu_tab_container.current_tab = 0 if active_metric_key == "quad" else 1
	quad_cpu_label.text = _cpu_tab_text("Quadtree collision", "quad")
	brute_cpu_label.text = _cpu_tab_text("Basic scan collision", "brute")


func _cpu_tab_text(title: String, metric_key: String) -> String:
	var current_cpu := 0.0 if simulation_paused and active_mode == _mode_for_metric_key(metric_key) else float(cpu_stats[metric_key]["current"])
	return "%s CPU   Current: %.2f%%   Average: %.2f%%   Samples: %d   %.1f us/tick" % [
		title,
		current_cpu,
		float(cpu_stats[metric_key]["average"]),
		int(cpu_stats[metric_key]["samples"]),
		float(cpu_stats[metric_key]["time_us"]),
	]


func _mode_for_metric_key(metric_key: String) -> int:
	return MODE_QUADTREE if metric_key == "quad" else MODE_BRUTE_FORCE


func _collision_cpu_percent_for_time(time_us: float) -> float:
	if simulation_paused:
		return 0.0

	var collision_us_per_second := time_us * maxf(clock_speed, 1.0)
	return collision_us_per_second / 10000.0


func _draw() -> void:
	draw_rect(map_rect, Color(0.055, 0.075, 0.08), true)
	draw_rect(map_rect, Color(0.72, 0.78, 0.78), false, 2.0)

	for x in range(GRID_SIZE + 1):
		var start := MAP_ORIGIN + Vector2(float(x) * CELL_SIZE, 0.0)
		draw_line(start, start + Vector2(0.0, MAP_SIZE.y), Color(0.16, 0.20, 0.20), 1.0)
	for y in range(GRID_SIZE + 1):
		var start := MAP_ORIGIN + Vector2(0.0, float(y) * CELL_SIZE)
		draw_line(start, start + Vector2(MAP_SIZE.x, 0.0), Color(0.16, 0.20, 0.20), 1.0)

	for wall in walls:
		draw_rect(wall, Color(0.47, 0.50, 0.53), true)
		draw_rect(wall, Color(0.82, 0.85, 0.86), false, 1.0)

	if show_quadtree and not last_tree_bounds.is_empty():
		for bounds in last_tree_bounds:
			if bounds != map_rect:
				draw_rect(bounds, Color(0.1, 0.72, 0.9, 0.35), false, 1.0)

	for enemy in enemies:
		draw_circle(enemy["pos"], ENEMY_RADIUS + 2.0, Color(0.30, 0.07, 0.06))
		draw_circle(enemy["pos"], ENEMY_RADIUS, Color(0.92, 0.18, 0.12))

	for bullet in bullets:
		draw_circle(bullet["pos"], BULLET_RADIUS, Color(1.0, 0.76, 0.19))

	var player_color := Color(0.2, 1.0, 0.25) if player_flash == 0.0 else Color(0.8, 1.0, 0.8)
	draw_circle(player_pos, PLAYER_RADIUS + 2.0, Color(0.02, 0.14, 0.04))
	draw_circle(player_pos, PLAYER_RADIUS, player_color)


func _on_mode_selected(index: int) -> void:
	var new_mode := mode_option.get_item_id(index)
	if new_mode != active_mode:
		active_mode = new_mode
		_reset_cpu_stats("quad" if active_mode == MODE_QUADTREE else "brute")
	else:
		active_mode = new_mode
	_update_cpu_tabs("quad" if active_mode == MODE_QUADTREE else "brute")
	update_ui_text()


func _set_paused(enabled: bool) -> void:
	if simulation_paused == enabled:
		if pause_check != null and pause_check.button_pressed != enabled:
			pause_check.set_pressed_no_signal(enabled)
		return

	simulation_paused = enabled
	if simulation_paused:
		accumulator = 0.0
	if pause_check != null:
		pause_check.set_pressed_no_signal(simulation_paused)
	update_ui_text()
	queue_redraw()


func _on_pause_toggled(enabled: bool) -> void:
	_set_paused(enabled)


func _on_clock_speed_changed(value: float) -> void:
	clock_speed = value


func _on_fire_rate_changed(value: float) -> void:
	enemy_fire_rate = value


func _on_bullet_speed_changed(value: float) -> void:
	bullet_speed = value


func _on_show_tree_toggled(enabled: bool) -> void:
	show_quadtree = enabled


func _on_clear_bullets_pressed() -> void:
	bullets.clear()
