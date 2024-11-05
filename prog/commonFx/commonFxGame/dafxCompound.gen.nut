from "dagor.math" import Color3, Color4, Point2, Point3

from "params.gen.nut" import *

begin_declare_params("DafxCompound");

declare_struct("LightfxShadowParams", 1,
[
  { name="enabled", type="bool", defVal=false },
  { name="is_dynamic_light", type="bool", defVal=false },
  { name="shadows_for_dynamic_objects", type="bool", defVal=false },
  { name="shadows_for_gpu_objects", type="bool", defVal=false },
  { name="quality", type="int", defVal=0 },
  { name="shrink", type="int", defVal=0 },
  { name="priority", type="int", defVal=0 },
]);

declare_struct("LightfxParams", 4,
[
  { name="ref_slot", type="int", defVal=0 },
  { name="allow_game_override", type="bool", defVal=0 },
  { name="offset", type="Point3", defVal=Point3(0,0,0) },
  { name="mod_scale", type="real", defVal=1 },
  { name="mod_radius", type="real", defVal=1 },
  { name="life_time", type="real", defVal=0 },
  { name="fade_time", type="real", defVal=0 },

  { name="shadow", type="LightfxShadowParams" },
]);

declare_struct("ModFxQuality", 1,
[
  { name="low_quality", type="bool", defVal=1 },
  { name="medium_quality", type="bool", defVal=1 },
  { name="high_quality", type="bool", defVal=1 },
]);

declare_struct("ModfxParams", 10,
[
  { name="ref_slot", type="int", defVal=0 },
  { name="offset", type="Point3", defVal=Point3(0,0,0) },
  { name="scale", type="Point3", defVal=Point3(1,1,1) },
  { name="rotation",type="Point3", defVal=Point3(0,0,0) },
  { name="delay", type="real", defVal=0 },
  { name="distance_lag", type="real", defVal=0 },

  { name="mod_part_count", type="real", defVal=1 },
  { name="mod_life", type="real", defVal=1 },
  { name="mod_radius", type="real", defVal=1 },
  { name="mod_rotation_speed", type="real", defVal=1 },

  { name="mod_velocity_start", type="real", defVal=1 },
  { name="mod_velocity_add", type="real", defVal=1 },
  { name="mod_velocity_drag", type="real", defVal=1 },
  { name="mod_velocity_drag_to_rad", type="real", defVal=1 },
  { name="mod_velocity_mass", type="real", defVal=1 },
  { name="mod_velocity_wind_scale", type="real", defVal=1 },

  { name="mod_color", type="E3DCOLOR", defVal=Color4(255, 255, 255, 255) },

  { name="global_life_time_min", type="real", defVal=0 },
  { name="global_life_time_max", type="real", defVal=0 },
  { name="transform_type", type="list", list=["default", "world_space", "local_space"] },
  { name="render_group", type="list", list=["default", "lowres", "highres", "distortion", "water_proj", "underwater"] },

  { name="quality", type="ModFxQuality" },
]);

declare_struct("CFxGlobalParams", 3,
[
  { name="spawn_range_limit", type="real", defVal=0 },
  { name="max_instances", type="int", defVal=100 },
  { name="player_reserved", type="int", defVal=10 },
  { name="one_point_number", type="real", defVal=3 },
  { name="one_point_radius", type="real", defVal=5 },
]);

declare_struct("CompModfx", 3,
[
  { name="array", type="dyn_array", elemType="ModfxParams", memberToShowInCaption="ref_slot" },

  { name="instance_life_time_min", type="real", defVal=0 },
  { name="instance_life_time_max", type="real", defVal=0 },

  { name="fx_scale_min", type="real", defVal=1 },
  { name="fx_scale_max", type="real", defVal=1 },

  { name="fx1", type="ref_slot", slotType="Effects" },
  { name="fx2", type="ref_slot", slotType="Effects" },
  { name="fx3", type="ref_slot", slotType="Effects" },
  { name="fx4", type="ref_slot", slotType="Effects" },
  { name="fx5", type="ref_slot", slotType="Effects" },
  { name="fx6", type="ref_slot", slotType="Effects" },
  { name="fx7", type="ref_slot", slotType="Effects" },
  { name="fx8", type="ref_slot", slotType="Effects" },
  { name="fx9", type="ref_slot", slotType="Effects" },
  { name="fx10", type="ref_slot", slotType="Effects" },
  { name="fx11", type="ref_slot", slotType="Effects" },
  { name="fx12", type="ref_slot", slotType="Effects" },
  { name="fx13", type="ref_slot", slotType="Effects" },
  { name="fx14", type="ref_slot", slotType="Effects" },
  { name="fx15", type="ref_slot", slotType="Effects" },
  { name="fx16", type="ref_slot", slotType="Effects" },
]);

end_declare_params("dafx_compound",3, [
  {struct="CompModfx"},
  {struct="LightfxParams"},
  {struct="CFxGlobalParams"}
]);
