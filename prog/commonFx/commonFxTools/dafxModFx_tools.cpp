// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <dafxModFx_decl.h>


ScriptHelpers::TunedElement *FxValueGradOpt::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_part_idx_as_key", false));
  elems.push_back(ScriptHelpers::create_tuned_gradient_box("grad"));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *FxValueCurveOpt::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("curve", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxSpawnFixed::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  elems.push_back(ScriptHelpers::create_tuned_int_param("count", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxSpawnLinear::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_int_param("count_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("count_max", 1));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxSpawnBurst::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_int_param("count_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("count_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("cycles", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("period", 1));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxDistanceBased::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_int_param("elem_limit", 10));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distance", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("idle_period", 0));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxSpawn::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(4);

    enumEntries[0].name = "linear";
    enumEntries[0].value = 0;
    enumEntries[1].name = "burst";
    enumEntries[1].value = 1;
    enumEntries[2].name = "distance_based";
    enumEntries[2].value = 2;
    enumEntries[3].name = "fixed";
    enumEntries[3].value = 3;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(FxSpawnLinear::createTunedElement("linear"));
  elems.push_back(FxSpawnBurst::createTunedElement("burst"));
  elems.push_back(FxDistanceBased::createTunedElement("distance_based"));
  elems.push_back(FxSpawnFixed::createTunedElement("fixed"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxLife::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_real_param("part_life_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("part_life_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("part_life_rnd_offset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("inst_life_delay", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitPosSphere::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitPosSphereSector::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("vec", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sector", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("random_burst", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitPosCylinder::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("vec", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("random_burst", 0));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxInitPosBox::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_real_param("width", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("depth", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitPosCone::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("vec", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("width_top", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("width_bottom", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("random_burst", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitGpuPlacement::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("placement_threshold", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_hmap", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_depth_above", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_water", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_water_flowmap", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_normal", false));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxPos::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(11);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("attach_last_part_to_emitter", false));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(5);

    enumEntries[0].name = "sphere";
    enumEntries[0].value = 0;
    enumEntries[1].name = "cylinder";
    enumEntries[1].value = 1;
    enumEntries[2].name = "cone";
    enumEntries[2].value = 2;
    enumEntries[3].name = "box";
    enumEntries[3].value = 3;
    enumEntries[4].name = "sphere_sector";
    enumEntries[4].value = 4;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("volume", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("offset", Point3(0, 0, 0)));
  elems.push_back(FxInitPosSphere::createTunedElement("sphere"));
  elems.push_back(FxInitPosCylinder::createTunedElement("cylinder"));
  elems.push_back(FxInitPosCone::createTunedElement("cone"));
  elems.push_back(FxInitPosBox::createTunedElement("box"));
  elems.push_back(FxInitPosSphereSector::createTunedElement("sphere_sector"));
  elems.push_back(FxInitGpuPlacement::createTunedElement("gpu_placement"));

  return ScriptHelpers::create_tuned_struct(name, 6, elems);
}


ScriptHelpers::TunedElement *FxRadius::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rad_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rad_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("apply_emitter_transform", false));
  elems.push_back(FxValueCurveOpt::createTunedElement("over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxColorCurveOpt::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("mask", E3DCOLOR(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_part_idx_as_key", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_threshold", false));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("curve", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *FxMaskedCurveOpt::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("mask", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("curve", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxAlphaByVelocity::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_emitter_velocity", false));
  elems.push_back(FxValueCurveOpt::createTunedElement("velocity_alpha_curve"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxColor::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(9);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("allow_game_override", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("gamma_correction", false));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("col_min", E3DCOLOR(128, 128, 128)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("col_max", E3DCOLOR(128, 128, 128)));
  elems.push_back(FxValueGradOpt::createTunedElement("grad_over_part_life"));
  elems.push_back(FxColorCurveOpt::createTunedElement("curve_over_part_life"));
  elems.push_back(FxMaskedCurveOpt::createTunedElement("curve_over_part_idx"));
  elems.push_back(FxAlphaByVelocity::createTunedElement("alpha_by_velocity"));

  return ScriptHelpers::create_tuned_struct(name, 5, elems);
}


ScriptHelpers::TunedElement *FxRotation::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("uv_rotation", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("start_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("start_max", 180));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("dynamic", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_max", 360));
  elems.push_back(FxValueCurveOpt::createTunedElement("over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxInitVelocityVec::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("vec", Point3(0, 1, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitVelocityPoint::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("offset", Point3(0, 0, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxInitVelocityCone::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("vec", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("offset", Point3(0, -1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("width_top", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("width_bottom", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("center_power", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("border_power", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxVelocityStart::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vec_rnd", 0));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "point";
    enumEntries[0].value = 0;
    enumEntries[1].name = "vec";
    enumEntries[1].value = 1;
    enumEntries[2].name = "start_shape";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(FxInitVelocityPoint::createTunedElement("point"));
  elems.push_back(FxInitVelocityVec::createTunedElement("vec"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxVelocityAdd::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(9);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("apply_emitter_transform", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vel_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vec_rnd", 0));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "point";
    enumEntries[0].value = 0;
    enumEntries[1].name = "vec";
    enumEntries[1].value = 1;
    enumEntries[2].name = "cone";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(FxInitVelocityPoint::createTunedElement("point"));
  elems.push_back(FxInitVelocityVec::createTunedElement("vec"));
  elems.push_back(FxInitVelocityCone::createTunedElement("cone"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxCollision::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(13);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius_mod", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("reflect_power", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("energy_loss", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("emitter_deadzone", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("collision_decay", -1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("collide_with_depth", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("collide_with_depth_above", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("collide_with_hmap", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("collide_with_water", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("collision_fadeout_radius_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("collision_fadeout_radius_max", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("stop_rotation_on_collision", true));

  return ScriptHelpers::create_tuned_struct(name, 5, elems);
}


ScriptHelpers::TunedElement *FxWind::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("directional_force", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("directional_freq", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulence_force", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulence_freq", 0));
  elems.push_back(FxValueCurveOpt::createTunedElement("atten"));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("impulse_wind", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("impulse_wind_force", 10));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxForceFieldVortex::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(11);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("axis_direction", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("direction_rnd", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("axis_position", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("position_rnd", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rotation_speed_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rotation_speed_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("pull_speed_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("pull_speed_max", 1));
  elems.push_back(FxValueCurveOpt::createTunedElement("rotation_speed_over_part_life"));
  elems.push_back(FxValueCurveOpt::createTunedElement("pull_speed_over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxForceFieldNoise::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "velocity_add";
    enumEntries[0].value = 0;
    enumEntries[1].name = "pos_offset";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("pos_scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("power_scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("power_rnd", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("power_per_part_rnd", 0));
  elems.push_back(FxValueCurveOpt::createTunedElement("power_curve"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxForceField::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(FxForceFieldVortex::createTunedElement("vortex"));
  elems.push_back(FxForceFieldNoise::createTunedElement("noise"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxVelocityGpuPlacement::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height_max", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("climb_power", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("reserved", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_hmap", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_depth_above", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_water", false));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxCameraVelocity::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("velocity_weight", 0.02));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxVelocity::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(17);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mass", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("drag_coeff", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("drag_to_rad_k", 1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("apply_gravity", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("gravity_transform", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("apply_parent_velocity", false));
  elems.push_back(FxVelocityStart::createTunedElement("start"));
  elems.push_back(FxVelocityAdd::createTunedElement("add"));
  elems.push_back(FxForceField::createTunedElement("force_field"));
  elems.push_back(FxValueCurveOpt::createTunedElement("decay"));
  elems.push_back(FxValueCurveOpt::createTunedElement("drag_curve"));
  elems.push_back(FxCollision::createTunedElement("collision"));
  elems.push_back(FxVelocityGpuPlacement::createTunedElement("gpu_hmap_limiter"));
  elems.push_back(FxWind::createTunedElement("wind"));
  elems.push_back(FxCameraVelocity::createTunedElement("camera_velocity"));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(4);

    enumEntries[0].name = "default";
    enumEntries[0].value = 0;
    enumEntries[1].name = "disabled";
    enumEntries[1].value = 1;
    enumEntries[2].name = "per_emitter";
    enumEntries[2].value = 2;
    enumEntries[3].name = "per_particle";
    enumEntries[3].value = 3;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("gravity_zone", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 14, elems);
}


ScriptHelpers::TunedElement *FxPlacement::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("placement_threshold", -1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_hmap", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_depth_above", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_water", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("align_normals_offset", -1));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *FxTextureAnim::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("animated_flipbook", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("speed_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("speed_max", 1));
  elems.push_back(FxValueCurveOpt::createTunedElement("over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxColorMatrix::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("red", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("green", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("blue", E3DCOLOR(0, 0, 255)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("alpha", E3DCOLOR(0, 0, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxColorRemapDyanmic::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxColorRemap::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("apply_base_color", true));
  elems.push_back(ScriptHelpers::create_tuned_gradient_box("grad"));
  elems.push_back(FxColorRemapDyanmic::createTunedElement("dynamic"));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("second_mask_enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("second_mask_apply_base_color", false));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *FxTexture::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(17);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("enable_aniso", false));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("tex_0", "Texture"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("tex_1", "Texture"));
  elems.push_back(ScriptHelpers::create_tuned_int_param("frames_x", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("frames_y", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("frame_scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("start_frame_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("start_frame_max", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("flip_x", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("flip_y", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("random_flip_x", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("random_flip_y", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("disable_loop", false));
  elems.push_back(FxColorMatrix::createTunedElement("color_matrix"));
  elems.push_back(FxColorRemap::createTunedElement("color_remap"));
  elems.push_back(FxTextureAnim::createTunedElement("animation"));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *FxEmission::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("value", 0));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("mask", E3DCOLOR(255, 255, 255)));
  elems.push_back(FxValueCurveOpt::createTunedElement("over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxThermalEmission::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("value", 0));
  elems.push_back(FxValueCurveOpt::createTunedElement("over_part_life"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxLighting::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(10);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(5);

    enumEntries[0].name = "none";
    enumEntries[0].value = 0;
    enumEntries[1].name = "uniform";
    enumEntries[1].value = 1;
    enumEntries[2].name = "view_disc";
    enumEntries[2].value = 2;
    enumEntries[3].name = "sphere";
    enumEntries[3].value = 3;
    enumEntries[4].name = "normal_map";
    enumEntries[4].value = 4;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("translucency", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sphere_normal_radius", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sphere_normal_power", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("normal_softness", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("specular_enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("specular_power", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("specular_strength", 1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("ambient_enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("external_lights_enabled", false));

  return ScriptHelpers::create_tuned_struct(name, 5, elems);
}


ScriptHelpers::TunedElement *FxRenderShapeBillboard::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(10);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(7);

    enumEntries[0].name = "screen";
    enumEntries[0].value = 0;
    enumEntries[1].name = "view_pos";
    enumEntries[1].value = 1;
    enumEntries[2].name = "velocity";
    enumEntries[2].value = 2;
    enumEntries[3].name = "static_aligned";
    enumEntries[3].value = 3;
    enumEntries[4].name = "adaptive_aligned";
    enumEntries[4].value = 4;
    enumEntries[5].name = "velocity_motion";
    enumEntries[5].value = 5;
    enumEntries[6].name = "start_velocity";
    enumEntries[6].value = 6;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("orientation", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("static_aligned_up_vec", Point3(0, 1, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("static_aligned_right_vec", Point3(1, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("cross_fade_mul", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("cross_fade_pow", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("cross_fade_threshold", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point2_param("pivot_offset", Point2(0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("velocity_to_length", 1));
  elems.push_back(ScriptHelpers::create_tuned_Point2_param("velocity_to_length_clamp", Point2(1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point2_param("screen_view_clamp", Point2(0, 0)));

  return ScriptHelpers::create_tuned_struct(name, 9, elems);
}


ScriptHelpers::TunedElement *FxRenderShapeRibbon::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "side_only";
    enumEntries[0].value = 0;
    enumEntries[1].name = "side_and_head";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "relative";
    enumEntries[0].value = 0;
    enumEntries[1].name = "static";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("uv_mapping", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("uv_tile", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("side_fade_threshold", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("side_fade_pow", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("head_fade_threshold", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("head_fade_pow", 1));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *FxRenderShape::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_real_param("aspect", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("camera_offset", 0));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "billboard";
    enumEntries[0].value = 0;
    enumEntries[1].name = "ribbon";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(FxRenderShapeBillboard::createTunedElement("billboard"));
  elems.push_back(FxRenderShapeRibbon::createTunedElement("ribbon"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxBlending::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "alpha_blend";
    enumEntries[0].value = 0;
    enumEntries[1].name = "premult_alpha";
    enumEntries[1].value = 1;
    enumEntries[2].name = "additive";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxRenderGroup::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(5);

    enumEntries[0].name = "lowres";
    enumEntries[0].value = 0;
    enumEntries[1].name = "highres";
    enumEntries[1].value = 1;
    enumEntries[2].name = "distortion";
    enumEntries[2].value = 2;
    enumEntries[3].name = "water_proj";
    enumEntries[3].value = 3;
    enumEntries[4].name = "underwater";
    enumEntries[4].value = 4;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *FxRenderShaderDummy::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  elems.push_back(ScriptHelpers::create_tuned_real_param("placeholder", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxRenderShaderDistortion::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_real_param("distortion_strength", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distortion_fade_range", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distortion_fade_power", 0));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("distortion_rgb", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distortion_rgb_strength", 1));

  return ScriptHelpers::create_tuned_struct(name, 5, elems);
}


ScriptHelpers::TunedElement *FxRenderShaderVolShape::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_real_param("thickness", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius_pow", 2));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxRenderVolfogInjection::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("weight_rgb", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("weight_alpha", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FxRenderShader::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(10);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("reverse_part_order", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("shadow_caster", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("allow_screen_proj_discard", true));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(6);

    enumEntries[0].name = "modfx_default";
    enumEntries[0].value = 0;
    enumEntries[1].name = "modfx_bboard_atest";
    enumEntries[1].value = 1;
    enumEntries[2].name = "modfx_bboard_distortion";
    enumEntries[2].value = 2;
    enumEntries[3].name = "modfx_vol_shape";
    enumEntries[3].value = 3;
    enumEntries[4].name = "modfx_bboard_rain";
    enumEntries[4].value = 4;
    enumEntries[5].name = "modfx_bboard_rain_distortion";
    enumEntries[5].value = 5;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("shader", enumEntries));
  }
  elems.push_back(FxRenderShaderDummy::createTunedElement("modfx_bboard_render"));
  elems.push_back(FxRenderShaderDummy::createTunedElement("modfx_bboard_render_atest"));
  elems.push_back(FxRenderShaderDistortion::createTunedElement("modfx_bboard_distortion"));
  elems.push_back(FxRenderShaderVolShape::createTunedElement("modfx_bboard_vol_shape"));
  elems.push_back(FxRenderShaderDummy::createTunedElement("modfx_bboard_rain"));
  elems.push_back(FxRenderShaderDistortion::createTunedElement("modfx_bboard_rain_distortion"));

  return ScriptHelpers::create_tuned_struct(name, 11, elems);
}


ScriptHelpers::TunedElement *FxDepthMask::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("use_part_radius", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("depth_softness", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("znear_softness", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("znear_clip", 0.1));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *FxPartTrimming::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("reversed", false));
  elems.push_back(ScriptHelpers::create_tuned_int_param("steps", 4));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fade_mul", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fade_pow", 1));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *FxGlobalParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_real_param("spawn_range_limit", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("max_instances", 100));
  elems.push_back(ScriptHelpers::create_tuned_int_param("player_reserved", 10));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "default";
    enumEntries[0].value = 0;
    enumEntries[1].name = "world_space";
    enumEntries[1].value = 1;
    enumEntries[2].name = "local_space";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("transform_type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("emission_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_number", 3));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_radius", 5));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *FxQuality::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("low_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("medium_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("high_quality", true));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


class DafxModFxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "DafxModFx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(21);

    elems.push_back(FxSpawn::createTunedElement("FxSpawn_data"));
    elems.push_back(FxLife::createTunedElement("FxLife_data"));
    elems.push_back(FxPos::createTunedElement("FxPos_data"));
    elems.push_back(FxRadius::createTunedElement("FxRadius_data"));
    elems.push_back(FxColor::createTunedElement("FxColor_data"));
    elems.push_back(FxRotation::createTunedElement("FxRotation_data"));
    elems.push_back(FxVelocity::createTunedElement("FxVelocity_data"));
    elems.push_back(FxPlacement::createTunedElement("FxPlacement_data"));
    elems.push_back(FxTexture::createTunedElement("FxTexture_data"));
    elems.push_back(FxEmission::createTunedElement("FxEmission_data"));
    elems.push_back(FxThermalEmission::createTunedElement("FxThermalEmission_data"));
    elems.push_back(FxLighting::createTunedElement("FxLighting_data"));
    elems.push_back(FxRenderShape::createTunedElement("FxRenderShape_data"));
    elems.push_back(FxBlending::createTunedElement("FxBlending_data"));
    elems.push_back(FxDepthMask::createTunedElement("FxDepthMask_data"));
    elems.push_back(FxRenderGroup::createTunedElement("FxRenderGroup_data"));
    elems.push_back(FxRenderShader::createTunedElement("FxRenderShader_data"));
    elems.push_back(FxRenderVolfogInjection::createTunedElement("FxRenderVolfogInjection_data"));
    elems.push_back(FxPartTrimming::createTunedElement("FxPartTrimming_data"));
    elems.push_back(FxGlobalParams::createTunedElement("FxGlobalParams_data"));
    elems.push_back(FxQuality::createTunedElement("FxQuality_data"));

    return ScriptHelpers::create_tuned_group("params", 13, elems);
  }
};

static DafxModFxEffectTools tools;


void register_dafx_modfx_fx_tools() { ::register_effect_class_tools(&tools); }
