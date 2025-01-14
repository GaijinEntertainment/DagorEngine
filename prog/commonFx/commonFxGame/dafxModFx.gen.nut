from "dagor.math" import Color3, Color4, Point2, Point3

let {glob_decl_text, declare_struct, begin_declare_params, end_declare_params} = require("params.gen.nut")

glob_decl_text.append("#include <math/dag_gradientBoxParams.h>\n");

begin_declare_params("DafxModFx");

// generic
declare_struct("FxValueGradOpt", 3,
[
  { name="enabled", type="bool", defVal=0 },
  { name="use_part_idx_as_key", type="bool", defVal=0 },
  { name="grad", type="gradient_box" },
]);

declare_struct("FxValueCurveOpt", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="curve", type="cubic_curve" },
]);

// spawn
declare_struct("FxSpawnFixed", 1,
[
  { name="count", type="int", defVal=1 },
]);

declare_struct("FxSpawnLinear", 2,
[
  { name="count_min", type="int", defVal=1 },
  { name="count_max", type="int", defVal=1 },
]);

declare_struct("FxSpawnBurst", 2,
[
  { name="count_min", type="int", defVal=1 },
  { name="count_max", type="int", defVal=1 },
  { name="cycles", type="int", defVal=0 },
  { name="period", type="real", defVal=1 },
]);

declare_struct("FxDistanceBased", 2,
[
  { name="elem_limit", type="int", defVal=10 },
  { name="distance", type="real", defVal=1 },
  { name="idle_period", type="real", defVal=0 },
]);

declare_struct("FxSpawn", 2,
[
  { name="type", type="list", list=["linear", "burst", "distance_based", "fixed"] },
  { name="linear", type="FxSpawnLinear" },
  { name="burst", type="FxSpawnBurst" },
  { name="distance_based", type="FxDistanceBased" },
  { name="fixed", type="FxSpawnFixed" },
]);

// life
declare_struct("FxLife", 1,
[
  { name="part_life_min", type="real", defVal=1 },
  { name="part_life_max", type="real", defVal=1 },
  { name="part_life_rnd_offset", type="real", defVal=0 },
  { name="inst_life_delay", type="real", defVal=0 },
]);

// pos
declare_struct("FxInitPosSphere", 1,
[
  { name="radius", type="real", defVal=1 },
]);

declare_struct("FxInitPosSphereSector", 1,
[
  { name="vec", type="Point3", defVal=Point3(0, 1, 0) },
  { name="radius", type="real", defVal=1 },
  { name="sector", type="real", defVal=0.5 },
  { name="random_burst", type="real", defVal=0 },
]);

declare_struct("FxInitPosCylinder", 2,
[
  { name="vec", type="Point3", defVal=Point3(0, 1, 0) },
  { name="radius", type="real", defVal=1 },
  { name="height", type="real", defVal=1 },
  { name="random_burst", type="real", defVal=0 },
]);

declare_struct("FxInitPosBox", 1,
[
  { name="width", type="real", defVal=1 },
  { name="height", type="real", defVal=1 },
  { name="depth", type="real", defVal=1 },
]);

declare_struct("FxInitPosCone", 1,
[
  { name="vec", type="Point3", defVal=Point3(0, 1, 0) },
  { name="width_top", type="real", defVal=1 },
  { name="width_bottom", type="real", defVal=0.5},
  { name="height", type="real", defVal=1 },
  { name="random_burst", type="real", defVal=0 },
]);


declare_struct("FxInitGpuPlacement", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="placement_threshold", type="real", defVal=0 },
  { name="use_hmap", type="bool", defVal=1 },
  { name="use_depth_above", type="bool", defVal=0 },
  { name="use_water", type="bool", defVal=0 },
  { name="use_water_flowmap", type="bool", defVal=0 },
  { name="use_normal", type="bool", defVal=0 }, // unused?
]);

declare_struct("FxPos", 6,
[
  { name="enabled", type="bool", defVal=1 },
  { name="attach_last_part_to_emitter", type="bool", defVal=0 },
  { name="type", type="list", list=["sphere", "cylinder", "cone", "box", "sphere_sector"] },
  { name="volume", type="real", defVal=0 },
  { name="offset", type="Point3", defVal=Point3(0, 0, 0) },
  { name="sphere", type="FxInitPosSphere" },
  { name="cylinder", type="FxInitPosCylinder" },
  { name="cone", type="FxInitPosCone" },
  { name="box", type="FxInitPosBox"},
  { name="sphere_sector", type="FxInitPosSphereSector" },
  { name="gpu_placement", type="FxInitGpuPlacement" },
]);

// radius
declare_struct("FxRadius", 2,
[
  { name="enabled", type="bool", defVal=1 },
  { name="rad_min", type="real", defVal=1 },
  { name="rad_max", type="real", defVal=1 },
  { name="apply_emitter_transform", type="bool", defVal=0 },
  { name="over_part_life", type="FxValueCurveOpt" },
]);

// color
declare_struct("FxColorCurveOpt", 4,
[
  { name="enabled", type="bool", defVal=0 },
  { name="mask", type="E3DCOLOR", defVal=Color4(0, 0, 0, 255) },
  { name="use_part_idx_as_key", type="bool", defVal=0 },
  { name="use_threshold", type="bool", defVal=0 },
  { name="curve", type="cubic_curve" },
]);

declare_struct("FxMaskedCurveOpt", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="mask", type="E3DCOLOR", defVal=Color4(255, 255, 255, 255) },
  { name="curve", type="cubic_curve" },
]);

declare_struct("FxAlphaByVelocity", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="vel_min", type="real", defVal=0 },
  { name="vel_max", type="real", defVal=1 },
  { name="use_emitter_velocity", type="bool", defVal=0 },
  { name="velocity_alpha_curve", type="FxValueCurveOpt" }
]);

declare_struct("FxColor", 5,
[
  { name="enabled", type="bool", defVal=0 },
  { name="allow_game_override", type="bool", defVal=0 },
  { name="gamma_correction", type="bool", defVal=0 },
  { name="col_min", type="E3DCOLOR", defVal=Color4(128, 128, 128, 255) },
  { name="col_max", type="E3DCOLOR", defVal=Color4(128, 128, 128, 255) },
  { name="grad_over_part_life", type="FxValueGradOpt" },
  { name="curve_over_part_life", type="FxColorCurveOpt" },
  { name="curve_over_part_idx", type="FxMaskedCurveOpt" },
  { name="alpha_by_velocity", type="FxAlphaByVelocity" },
]);

declare_struct("FxRotation", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="uv_rotation", type="bool", defVal=0 },
  { name="start_min", type="real", defVal=0 },
  { name="start_max", type="real", defVal=180 },

  { name="dynamic", type="bool", defVal=0 },
  { name="vel_min", type="real", defVal=0 },
  { name="vel_max", type="real", defVal=360 },
  { name="over_part_life", type="FxValueCurveOpt" },
]);

// vel
declare_struct("FxInitVelocityVec", 1,
[
  { name="vec", type="Point3", defVal=Point3(0, 1, 0) },
]);

declare_struct("FxInitVelocityPoint", 1,
[
  { name="offset", type="Point3", defVal=Point3(0, 0, 0) },
]);

declare_struct("FxInitVelocityCone", 1,
[
  { name="vec", type="Point3", defVal=Point3(0, 1, 0) },
  { name="offset", type="Point3", defVal=Point3(0, -1, 0) },
  { name="width_top", type="real", defVal=1 },
  { name="width_bottom", type="real", defVal=0.5},
  { name="height", type="real", defVal=1 },
  { name="center_power", type="real", defVal=1 },
  { name="border_power", type="real", defVal=1 },
]);

declare_struct("FxVelocityStart", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="vel_min", type="real", defVal=1 },
  { name="vel_max", type="real", defVal=1 },
  { name="vec_rnd", type="real", defVal=0 },
  { name="type", type="list", list=["point", "vec", "start_shape"] },
  { name="point", type="FxInitVelocityPoint" },
  { name="vec", type="FxInitVelocityVec" },
]);

declare_struct("FxVelocityAdd", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="apply_emitter_transform", type="bool", defVal=1 },
  { name="vel_min", type="real", defVal=1 },
  { name="vel_max", type="real", defVal=1 },
  { name="vec_rnd", type="real", defVal=0 },
  { name="type", type="list", list=["point", "vec", "cone"] },
  { name="point", type="FxInitVelocityPoint" },
  { name="vec", type="FxInitVelocityVec" },
  { name="cone", type="FxInitVelocityCone" },
]);

declare_struct("FxCollision", 5,
[
  { name="enabled", type="bool", defVal=0 },
  { name="radius_mod", type="real", defVal=1 },
  { name="reflect_power", type="real", defVal=1 },
  { name="energy_loss", type="real", defVal=0.5 },
  { name="emitter_deadzone", type="real", defVal=0 },
  { name="collision_decay", type="real", defVal=-1 },
  { name="collide_with_depth", type="bool", defVal=1 },
  { name="collide_with_depth_above", type="bool", defVal=0 },
  { name="collide_with_hmap", type="bool", defVal=0 },
  { name="collide_with_water", type="bool", defVal=0 },
  { name="collision_fadeout_radius_min", type="real", defVal=0 },
  { name="collision_fadeout_radius_max", type="real", defVal=0 },
  { name="stop_rotation_on_collision", type="bool", defVal=1 },
]);

declare_struct("FxWind", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="directional_force", type="real", defVal=1 },
  { name="directional_freq", type="real", defVal=0 },
  { name="turbulence_force", type="real", defVal=0 },
  { name="turbulence_freq", type="real", defVal=0 },
  { name="atten", type="FxValueCurveOpt" },
  { name="impulse_wind", type="bool", defVal=0 },
  { name="impulse_wind_force", type="real", defVal=10 },
]
);

declare_struct("FxForceFieldVortex", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="axis_direction", type="Point3", defVal=Point3(0, 1, 0) },
  { name="direction_rnd", type="real", defVal=0 },
  { name="axis_position", type="Point3", defVal=Point3(0, 0, 0) },
  { name="position_rnd", type="Point3", defVal=Point3(0, 0, 0) },
  { name="rotation_speed_min", type="real", defVal=1 },
  { name="rotation_speed_max", type="real", defVal=1 },
  { name="pull_speed_min", type="real", defVal=1 },
  { name="pull_speed_max", type="real", defVal=1 },
  { name="rotation_speed_over_part_life", type="FxValueCurveOpt" },
  { name="pull_speed_over_part_life", type="FxValueCurveOpt" },
]
);

declare_struct("FxForceFieldNoise", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="type", type="list", list=["velocity_add", "pos_offset"] },
  { name="pos_scale", type="real", defVal=1 },
  { name="power_scale", type="real", defVal=1 },
  { name="power_rnd", type="real", defVal=1 },
  { name="power_per_part_rnd", type="real", defVal=0 },
  { name="power_curve", type="FxValueCurveOpt" },
]
);


declare_struct("FxForceField", 2,
[
  { name="vortex", type="FxForceFieldVortex" },
  { name="noise", type="FxForceFieldNoise" },
]
);

declare_struct("FxVelocityGpuPlacement", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="height_min", type="real", defVal=0 },
  { name="height_max", type="real", defVal=0 },
  { name="climb_power", type="real", defVal=0 },
  { name="reserved", type="real", defVal=0 },
  { name="use_hmap", type="bool", defVal=1 },
  { name="use_depth_above", type="bool", defVal=0 },
  { name="use_water", type="bool", defVal=0 },
]
);

declare_struct("FxCameraVelocity", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="velocity_weight", type="real", defVal=0.02 },
]
);

declare_struct("FxVelocity", 14,
[
  { name="enabled", type="bool", defVal=0 },
  { name="mass", type="real", defVal=1 },
  { name="drag_coeff", type="real", defVal=0.5 },
  { name="drag_to_rad_k", type="real", defVal=1 },
  { name="apply_gravity", type="bool", defVal=1 },
  { name="gravity_transform", type="bool", defVal=0 },
  { name="apply_parent_velocity", type="bool", defVal=0 },
  { name="start", type="FxVelocityStart" },
  { name="add", type="FxVelocityAdd" },
  { name="force_field", type="FxForceField" },
  { name="decay", type="FxValueCurveOpt" },
  { name="drag_curve", type="FxValueCurveOpt" },
  { name="collision", type="FxCollision" },
  { name="gpu_hmap_limiter" type="FxVelocityGpuPlacement" },
  { name="wind", type="FxWind" },
  { name="camera_velocity", type="FxCameraVelocity"}
  { name="gravity_zone", type="list", list=["default", "disabled", "per_emitter", "per_particle"] },
]);

declare_struct("FxPlacement", 3,
[
  { name="enabled", type="bool", defVal=0 },
  { name="placement_threshold", type="real", defVal=-1 },
  { name="use_hmap", type="bool", defVal=0 },
  { name="use_depth_above", type="bool", defVal=1 },
  { name="use_water", type="bool", defVal=1 },
  { name="align_normals_offset", type="real", defVal=-1 },
]);

declare_struct("FxTextureAnim", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="animated_flipbook", type="bool", defVal=0 },
  { name="speed_min", type="real", defVal=1 },
  { name="speed_max", type="real", defVal=1 },

  { name="over_part_life", type="FxValueCurveOpt" },
]);

declare_struct("FxColorMatrix", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="red", type="E3DCOLOR", defVal=Color4(255, 0, 0, 0) },
  { name="green", type="E3DCOLOR", defVal=Color4(0, 255, 0, 0) },
  { name="blue", type="E3DCOLOR", defVal=Color4(0, 0, 255, 0) },
  { name="alpha", type="E3DCOLOR", defVal=Color4(0, 0, 0, 255) },
]);

declare_struct("FxColorRemapDyanmic", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="scale", type="real", defVal=1 },
]);

declare_struct("FxColorRemap", 3,
[
  { name="enabled", type="bool", defVal=0 },
  { name="apply_base_color", type="bool", defVal=1 },
  { name="grad", type="gradient_box" },
  { name="dynamic", type="FxColorRemapDyanmic" },

  { name="second_mask_enabled", type="bool", defVal=0 },
  { name="second_mask_apply_base_color", type="bool", defVal=0 },
]);

declare_struct("FxTexture", 4,
[
  { name="enabled", type="bool", defVal=0 },
  { name="enable_aniso", type="bool", defVal=0 },
  { name="tex_0", type="ref_slot", slotType="Texture" },
  { name="tex_1", type="ref_slot", slotType="Texture" },

  { name="frames_x", type="int", defVal=1 },
  { name="frames_y", type="int", defVal=1 },
  { name="frame_scale", type="real", defVal=1 },
  { name="start_frame_min", type="int", defVal=0 },
  { name="start_frame_max", type="int", defVal=0 },
  { name="flip_x", type="bool", defVal=0 },
  { name="flip_y", type="bool", defVal=0 },
  { name="random_flip_x", type="bool", defVal=0 },
  { name="random_flip_y", type="bool", defVal=0 },
  { name="disable_loop", type="bool", defVal=0 },
  { name="color_matrix", type="FxColorMatrix" },
  { name="color_remap", type="FxColorRemap" },
  { name="animation", type="FxTextureAnim" },
]);

declare_struct("FxEmission", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="value", type="real", defVal=0 },
  { name="mask", type="E3DCOLOR", defVal=Color4(255, 255, 255, 0) },
  { name="over_part_life", type="FxValueCurveOpt" },
]);

declare_struct("FxThermalEmission", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="value", type="real", defVal=0 },
  { name="over_part_life", type="FxValueCurveOpt" },
]);

declare_struct("FxLighting", 5,
[
  { name="type", type="list", list=["none", "uniform", "view_disc", "sphere", "normal_map"] },
  { name="translucency", type="real", defVal=0 },
  { name="sphere_normal_radius", type="real", defVal=1 },
  { name="sphere_normal_power", type="real", defVal=1 },
  { name="normal_softness", type="real", defVal=0 },
  { name="specular_enabled", type="bool", defVal=0 },
  { name="specular_power", type="real", defVal=1 },
  { name="specular_strength", type="real", defVal=1 },
  { name="ambient_enabled", type="bool", defVal=0 },
  { name="external_lights_enabled", type="bool", defVal=0 },
]);

declare_struct("FxRenderShapeBillboard", 9,
[
  { name="orientation", type="list", list=["screen", "view_pos", "velocity", "static_aligned", "adaptive_aligned", "velocity_motion", "start_velocity"] },
  { name="static_aligned_up_vec", type="Point3", defVal=Point3(0, 1, 0) },
  { name="static_aligned_right_vec", type="Point3", defVal=Point3(1, 0, 0) },
  { name="cross_fade_mul", type="real", defVal=0 },
  { name="cross_fade_pow", type="int", defVal=1 },
  { name="cross_fade_threshold", type="real", defVal=0 },
  { name="pivot_offset", type="Point2", defVal=Point2(0, 0) },
  { name="velocity_to_length", type="real", defVal=1 },
  { name="velocity_to_length_clamp", type="Point2", defVal=Point2(1, 1) },
  { name="screen_view_clamp", type="Point2", defVal=Point2(0, 0) },
]);

declare_struct("FxRenderShapeRibbon", 3,
[
  { name="type", type="list", list=["side_only", "side_and_head"] },
  { name="uv_mapping", type="list", list=["relative", "static"] },
  { name="uv_tile", type="real", defVal=1 },
  { name="side_fade_threshold", type="real", defVal=1 },
  { name="side_fade_pow", type="real", defVal=1 },
  { name="head_fade_threshold", type="real", defVal=1 },
  { name="head_fade_pow", type="real", defVal=1 },
]);

declare_struct("FxRenderShape", 2,
[
  { name="aspect", type="real", defVal=1 },
  { name="camera_offset", type="real", defVal=0 },
  { name="type", type="list", list=["billboard", "ribbon"] },
  { name="billboard", type="FxRenderShapeBillboard" },
  { name="ribbon", type="FxRenderShapeRibbon"},
]);

declare_struct("FxBlending", 1,
[
  { name="type", type="list", list=["alpha_blend", "premult_alpha", "additive"] },
]);

declare_struct("FxRenderGroup", 4,
[
  { name="type", type="list", list=["lowres", "highres", "distortion", "water_proj", "underwater"] },
]);

declare_struct("FxRenderShaderDummy", 1,
[
  { name="placeholder", type="real", defVal=1 },
]);

declare_struct("FxRenderShaderDistortion", 5,
[
  { name="distortion_strength", type="real", defVal=1 },
  { name="distortion_fade_range", type="real", defVal=0 },
  { name="distortion_fade_power", type="real", defVal=0 },
  { name="distortion_rgb", type="E3DCOLOR", defVal=Color4(255, 255, 255, 255) },
  { name="distortion_rgb_strength", type="real", defVal=1 },
]);

declare_struct("FxRenderShaderVolShape", 1,
[
  { name="thickness", type="real", defVal=1 },
  { name="radius_pow", type="real", defVal=2 },
]);

declare_struct("FxRenderVolfogInjection", 1,
[
  { name="enabled", type="bool", defVal=0 },
  { name="weight_rgb", type="real", defVal=1 },
  { name="weight_alpha", type="real", defVal=1 },
]);

declare_struct("FxRenderShader", 11,
[
  { name="reverse_part_order", type="bool", defVal=0 },
  { name="shadow_caster", type="bool", defVal=0 },
  { name="allow_screen_proj_discard", type="bool", defVal=1 },
  { name="shader", type="list", list=["modfx_default", "modfx_bboard_atest", "modfx_bboard_distortion", "modfx_vol_shape", "modfx_bboard_rain", "modfx_bboard_rain_distortion"] },
  { name="modfx_bboard_render", type="FxRenderShaderDummy" },
  { name="modfx_bboard_render_atest", type="FxRenderShaderDummy" },
  { name="modfx_bboard_distortion", type="FxRenderShaderDistortion" },
  { name="modfx_bboard_vol_shape", type="FxRenderShaderVolShape" },
  { name="modfx_bboard_rain", type="FxRenderShaderDummy" },
  { name="modfx_bboard_rain_distortion", type="FxRenderShaderDistortion" },
]);

declare_struct("FxDepthMask", 3,
[
  { name="enabled", type="bool", defVal=1 },
  { name="use_part_radius", type="bool", defVal=1 },
  { name="depth_softness", type="real", defVal=0.5 },
  { name="znear_softness", type="real", defVal=1 },
  { name="znear_clip", type="real", defVal=0.1 },
]);

declare_struct("FxPartTrimming", 2,
[
  { name="enabled", type="bool", defVal=0 },
  { name="reversed", type="bool", defVal=0 },
  { name="steps", type="int", defVal=4 },
  { name="fade_mul", type="real", defVal=1 },
  { name="fade_pow", type="real", defVal=1 },
]);

declare_struct("FxGlobalParams", 4,
[
  { name="spawn_range_limit", type="real", defVal=0 },
  { name="max_instances", type="int", defVal=100 },
  { name="player_reserved", type="int", defVal=10 },
  { name="transform_type", type="list", list=["default", "world_space", "local_space"] },
  { name="emission_min", type="real", defVal=0 },
  { name="one_point_number", type="real", defVal=3 },
  { name="one_point_radius", type="real", defVal=5 },
]);

declare_struct("FxQuality", 1,
[
  { name="low_quality", type="bool", defVal=1 },
  { name="medium_quality", type="bool", defVal=1 },
  { name="high_quality", type="bool", defVal=1 },
]);

end_declare_params("dafx_modfx", 13,
[
  {struct="FxSpawn"},
  {struct="FxLife"},
  {struct="FxPos"},
  {struct="FxRadius"},
  {struct="FxColor"},
  {struct="FxRotation"},
  {struct="FxVelocity"},
  {struct="FxPlacement"},
  {struct="FxTexture"},
  {struct="FxEmission"},
  {struct="FxThermalEmission"},
  {struct="FxLighting"},
  {struct="FxRenderShape"},
  {struct="FxBlending"},
  {struct="FxDepthMask"},
  {struct="FxRenderGroup"},
  {struct="FxRenderShader"},
  {struct="FxRenderVolfogInjection"},
  {struct="FxPartTrimming"},
  {struct="FxGlobalParams"},
  {struct="FxQuality"},
]);