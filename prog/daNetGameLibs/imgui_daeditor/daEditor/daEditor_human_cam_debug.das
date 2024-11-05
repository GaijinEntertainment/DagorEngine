options no_aot
require ecs
require DagorMath

[es(tag=render, after=cockpit_pre_animchar_offset, REQUIRE=watchedByPlr, REQUIRE=daeditor__selected)]
def cockpit_post_animchar_offset(info : UpdateStageInfoAct; var transform : float3x4; human_cam__calculatedOffset : DPoint3; human_cam__offsetMult : float)
  transform[3] = transform[3] - float3(human_cam__calculatedOffset * human_cam__offsetMult)
