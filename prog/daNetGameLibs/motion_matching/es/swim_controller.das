require ecs
require math.base
require strings
require DagorMath
require DagorSystem
require AnimV20
require MotionMatching
require %danetlibs/motion_matching/es/mm_events
require %danetlibs/motion_matching/es/tags_common


[es(tag=gameClient, track=(human_net_phys__isSwimming, human_net_phys__isUnderwater))]
def motion_matching_watch_swim(event : Event;
                               var motion_matching__controller : MotionMatchingController;
                               human_net_phys__isSwimming : bool;
                               human_net_phys__isUnderwater : bool;
                               var motion_matching__enabled : bool&;
                               motion_matching__hasSwimming : bool = false)

  if motion_matching__controller.dataBase == null || !motion_matching__hasSwimming
    return
  motion_matching__enabled = human_net_phys__isSwimming
  if human_net_phys__isSwimming
    set_tag(motion_matching__controller, "swim", true)
    set_tag(motion_matching__controller, "swim_underwater", human_net_phys__isUnderwater)
  else
    remove_tag(motion_matching__controller, "swim")
    remove_tag(motion_matching__controller, "swim_underwater")
