class_name QuadTree
extends RefCounted

const MAX_OBJECTS := 4
const MAX_DEPTH := 5

var bounds: Rect2
var depth: int
var objects: Array = []
var children: Array[QuadTree] = []

func _init(tree_bounds: Rect2, tree_depth: int = 0) -> void:
	bounds = tree_bounds
	depth = tree_depth

func clear() -> void:
	objects.clear()
	for child in children:
		child.clear()
	children.clear()

func insert(item: Variant, item_bounds: Rect2) -> void:
	if not bounds.intersects(item_bounds):
		return

	if children.is_empty():
		objects.append({"item": item, "bounds": item_bounds})
		if objects.size() > MAX_OBJECTS and depth < MAX_DEPTH:
			_split()
			var to_redistribute := objects.duplicate()
			objects.clear()
			for entry in to_redistribute:
				_insert_into_children_or_self(entry)
		return

	_insert_into_children_or_self({"item": item, "bounds": item_bounds})

func query(area: Rect2, results: Array) -> void:
	if not bounds.intersects(area):
		return

	for entry in objects:
		if entry.bounds.intersects(area):
			results.append(entry.item)

	for child in children:
		child.query(area, results)

func _insert_into_children_or_self(entry: Dictionary) -> void:
	for child in children:
		if child.bounds.encloses(entry.bounds):
			child.insert(entry.item, entry.bounds)
			return
	objects.append(entry)

func _split() -> void:
	var half_size := bounds.size / 2.0
	var x := bounds.position.x
	var y := bounds.position.y

	children = [
		QuadTree.new(Rect2(Vector2(x, y), half_size), depth + 1),
		QuadTree.new(Rect2(Vector2(x + half_size.x, y), half_size), depth + 1),
		QuadTree.new(Rect2(Vector2(x, y + half_size.y), half_size), depth + 1),
		QuadTree.new(Rect2(Vector2(x + half_size.x, y + half_size.y), half_size), depth + 1)
	]
