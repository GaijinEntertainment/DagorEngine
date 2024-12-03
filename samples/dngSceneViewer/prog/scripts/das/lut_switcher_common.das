module lut_switcher_common shared
require ecs

def add_grading(
                item_id : uint;
                var color_grading_items : ecs::Array;
                color_grading : Object)
  push(color_grading_items, color_grading)
  var itemObj = getRW_ecs_object(color_grading_items[length(color_grading_items) - 1])
  set(*itemObj, "unique_id", int(item_id))

def remove_grading(item_id : uint;
                   var color_grading_items : ecs::Array)
  var idx = length(color_grading_items)
  while --idx >= 0
    var itemObj = getRW_ecs_object(color_grading_items[idx])
    if uint(get_int(*itemObj, "unique_id") ?? 0) == item_id
      let timingsPtr = get_Point3(*itemObj, "timings")
      if timingsPtr != null && (*timingsPtr).z > 0f
        let timings = *timingsPtr
        let lifeTime = get_float(*itemObj, "life_time") ?? 0f
        if lifeTime < timings.x + timings.y
          set(*itemObj, "life_time", timings.x + timings.y) // advance to end
      else
        erase(color_grading_items, idx)

def amend_object_from(var to; from)
  for field in from
    set(to, field.key, field.value)

def update_object_existing_fields_from(var to; from)
  for field in from
    if has(to, field.key)
      deref(to[field.key]) := field.value

def switch_color_grading(item_id : uint;
                         var color_grading_items : Array;
                         color_grading : Object;
                         is_enabled : bool)
  if is_enabled
    add_grading(item_id, color_grading_items, color_grading)
  else
    remove_grading(item_id, color_grading_items)

def has_color_grading(color_grading_items : Array; item_id : uint)
  for it in color_grading_items
    if uint(get_int(get_ecs_object(it), "unique_id") ?? 0) == item_id
      return true
  return false
