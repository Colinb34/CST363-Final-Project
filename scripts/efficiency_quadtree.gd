class_name EfficiencyQuadTree
extends RefCounted

const MAX_ITEMS := 6
const MAX_DEPTH := 6

var bounds: Rect2
var depth: int
var items: Array = []
var children: Array = []


func _init(p_bounds: Rect2, p_depth: int = 0) -> void:
	bounds = p_bounds
	depth = p_depth


func clear() -> void:
	items.clear()
	for child in children:
		child.clear()
	children.clear()


func insert(item: Dictionary) -> void:
	var item_bounds: Rect2 = item["aabb"]
	if not bounds.intersects(item_bounds, true):
		return

	if children.size() > 0:
		var child_index := _child_index_for(item_bounds)
		if child_index != -1:
			children[child_index].insert(item)
			return

	items.append(item)

	if items.size() > MAX_ITEMS and depth < MAX_DEPTH:
		if children.is_empty():
			_split()

		var index := items.size() - 1
		while index >= 0:
			var stored_bounds: Rect2 = items[index]["aabb"]
			var target_child := _child_index_for(stored_bounds)
			if target_child != -1:
				var moved_item: Dictionary = items[index]
				items.remove_at(index)
				children[target_child].insert(moved_item)
			index -= 1


func query(area: Rect2, results: Array) -> void:
	if not bounds.intersects(area, true):
		return

	for item in items:
		if item["aabb"].intersects(area, true):
			results.append(item)

	for child in children:
		child.query(area, results)


func count_nodes() -> int:
	var total := 1
	for child in children:
		total += child.count_nodes()
	return total


func count_items() -> int:
	var total := items.size()
	for child in children:
		total += child.count_items()
	return total


func collect_bounds(output: Array) -> void:
	output.append(bounds)
	for child in children:
		child.collect_bounds(output)


func _split() -> void:
	var half := bounds.size * 0.5
	var origin := bounds.position
	children = [
		EfficiencyQuadTree.new(Rect2(origin, half), depth + 1),
		EfficiencyQuadTree.new(Rect2(origin + Vector2(half.x, 0.0), half), depth + 1),
		EfficiencyQuadTree.new(Rect2(origin + Vector2(0.0, half.y), half), depth + 1),
		EfficiencyQuadTree.new(Rect2(origin + half, half), depth + 1),
	]


func _child_index_for(area: Rect2) -> int:
	var vertical_mid := bounds.position.x + bounds.size.x * 0.5
	var horizontal_mid := bounds.position.y + bounds.size.y * 0.5

	var fits_left := area.position.x >= bounds.position.x and area.end.x <= vertical_mid
	var fits_right := area.position.x >= vertical_mid and area.end.x <= bounds.end.x
	var fits_top := area.position.y >= bounds.position.y and area.end.y <= horizontal_mid
	var fits_bottom := area.position.y >= horizontal_mid and area.end.y <= bounds.end.y

	if fits_left:
		if fits_top:
			return 0
		if fits_bottom:
			return 2
	elif fits_right:
		if fits_top:
			return 1
		if fits_bottom:
			return 3

	return -1
