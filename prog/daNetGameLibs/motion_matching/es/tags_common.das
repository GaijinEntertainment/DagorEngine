module tags_common shared

require ecs
require daslib/strings_boost
require MotionMatching
require DagorSystem

let INVALID_TAG_IDX = uint(-1)

def get_tag_index(controller : MotionMatchingController; tag_name : string) : uint
  assert(controller.dataBase != null)
  let tagIdx = *controller.dataBase |> getTagIdx(tag_name)
  if tagIdx < 0
    logerr("MM: unknown tag '{tag_name}'")
  return uint(tagIdx)

def set_tag(var controller : MotionMatchingController; tag_name : string; value : bool)
  let tagIdx = get_tag_index(controller, tag_name)
  if tagIdx == INVALID_TAG_IDX
    return
  if value
    controller.currentTags |> requireTag(tagIdx)
  else
    controller.currentTags |> excludeTag(tagIdx)

def remove_tag(var controller : MotionMatchingController; tag_name : string)
  let tagIdx = get_tag_index(controller, tag_name)
  if tagIdx == INVALID_TAG_IDX
    return
  controller.currentTags |> removeTag(tagIdx)
