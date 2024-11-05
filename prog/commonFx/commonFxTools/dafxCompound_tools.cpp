// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <dafxCompound_decl.h>


ScriptHelpers::TunedElement *LightfxShadowParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("is_dynamic_light", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("shadows_for_dynamic_objects", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("shadows_for_gpu_objects", false));
  elems.push_back(ScriptHelpers::create_tuned_int_param("quality", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("shrink", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("priority", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *LightfxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_int_param("ref_slot", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("allow_game_override", false));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_radius", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life_time", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fade_time", 0));
  elems.push_back(LightfxShadowParams::createTunedElement("shadow"));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *ModFxQuality::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("low_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("medium_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("high_quality", true));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *ModfxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(22);

  elems.push_back(ScriptHelpers::create_tuned_int_param("ref_slot", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("rotation", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distance_lag", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_part_count", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_life", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_radius", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_rotation_speed", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_start", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_add", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_drag", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_drag_to_rad", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_mass", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("mod_velocity_wind_scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("mod_color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("global_life_time_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("global_life_time_max", 0));
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
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(6);

    enumEntries[0].name = "default";
    enumEntries[0].value = 0;
    enumEntries[1].name = "lowres";
    enumEntries[1].value = 1;
    enumEntries[2].name = "highres";
    enumEntries[2].value = 2;
    enumEntries[3].name = "distortion";
    enumEntries[3].value = 3;
    enumEntries[4].name = "water_proj";
    enumEntries[4].value = 4;
    enumEntries[5].name = "underwater";
    enumEntries[5].value = 5;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("render_group", enumEntries));
  }
  elems.push_back(ModFxQuality::createTunedElement("quality"));

  return ScriptHelpers::create_tuned_struct(name, 10, elems);
}


ScriptHelpers::TunedElement *CFxGlobalParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_real_param("spawn_range_limit", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("max_instances", 100));
  elems.push_back(ScriptHelpers::create_tuned_int_param("player_reserved", 10));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_number", 3));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_radius", 5));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *CompModfx::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(21);

  elems.push_back(create_tuned_array("array", ModfxParams::createTunedElement("ModfxParams")));
  set_tuned_array_member_to_show_in_caption(*elems.back(), "ref_slot");

  elems.push_back(ScriptHelpers::create_tuned_real_param("instance_life_time_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("instance_life_time_max", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx_scale_min", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx_scale_max", 1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx1", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx2", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx3", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx4", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx5", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx6", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx7", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx8", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx9", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx10", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx11", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx12", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx13", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx14", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx15", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx16", "Effects"));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


class DafxCompoundEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "DafxCompound"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(CompModfx::createTunedElement("CompModfx_data"));
    elems.push_back(LightfxParams::createTunedElement("LightfxParams_data"));
    elems.push_back(CFxGlobalParams::createTunedElement("CFxGlobalParams_data"));

    return ScriptHelpers::create_tuned_group("params", 3, elems);
  }
};

static DafxCompoundEffectTools tools;


void register_dafx_compound_fx_tools() { ::register_effect_class_tools(&tools); }
