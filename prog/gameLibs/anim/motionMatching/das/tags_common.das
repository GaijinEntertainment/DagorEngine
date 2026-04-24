module tags_common shared

require ecs
require daslib/strings_boost
require MotionMatching
require DagorSystem

def get_tag_index(controller : MotionMatchingController; tag_name : string) : int
  assert(controller.dataBase != null)
  return get_tag_index(*controller.dataBase, tag_name)

def get_tag_index(data_base : AnimationDataBase; tag_name : string) : int
  let tagIdx = data_base |> getTagIdx(tag_name)
  if tagIdx < 0
    logerr("MM: unknown tag '{tag_name}'")
  return tagIdx

def set_tag(var controller : MotionMatchingController; tag_name : string; value : bool)
  let tagIdx = get_tag_index(controller, tag_name)
  set_tag(controller, tagIdx, value)

def set_tag(var controller : MotionMatchingController; tag_idx : int; value : bool)
  if tag_idx < 0
    return
  if value
    controller.currentTags |> requireTag(uint(tag_idx))
  else
    controller.currentTags |> excludeTag(uint(tag_idx))

def remove_tag(var controller : MotionMatchingController; tag_name : string)
  let tagIdx = get_tag_index(controller, tag_name)
  remove_tag(controller, tagIdx)

def remove_tag(var controller : MotionMatchingController; tag_idx : int)
  if tag_idx < 0
    return
  controller.currentTags |> removeTag(uint(tag_idx))
