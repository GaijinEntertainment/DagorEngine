// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <dafxEmitter_decl.h>
#include <dafxSparks_decl.h>


ScriptHelpers::TunedElement *DafxLinearDistribution::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_real_param("start", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("end", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxRectangleDistribution::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_Point2_param("leftTop", Point2(0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point2_param("rightBottom", Point2(0, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxCubeDistribution::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("center", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("size", Point3(0, 0, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxSphereDistribution::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("center", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxSectorDistribution::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(9);

  elems.push_back(ScriptHelpers::create_tuned_Point3_param("center", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radiusMin", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("radiusMax", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("azimutMax", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("azimutNoise", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("inclinationMin", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("inclinationMax", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("inclinationNoise", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxSparksSimParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(21);

  elems.push_back(DafxCubeDistribution::createTunedElement("pos"));
  elems.push_back(DafxLinearDistribution::createTunedElement("width"));
  elems.push_back(DafxLinearDistribution::createTunedElement("life"));
  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(DafxSectorDistribution::createTunedElement("velocity"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("restitution", 0.2));
  elems.push_back(ScriptHelpers::create_tuned_real_param("friction", 2));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color0", E3DCOLOR(128, 128, 128)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color1", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("color1Portion", 0));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("colorEnd", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("velocityBias", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("dragCoefficient", 1));
  elems.push_back(DafxLinearDistribution::createTunedElement("turbulenceForce"));
  elems.push_back(DafxLinearDistribution::createTunedElement("turbulenceFreq"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("liftForce", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("liftTime", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("spawnNoise", 0));
  elems.push_back(DafxCubeDistribution::createTunedElement("spawnNoisePos"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("hdrScale1", 2));
  elems.push_back(ScriptHelpers::create_tuned_real_param("windForce", 0));

  return ScriptHelpers::create_tuned_struct(name, 6, elems);
}


ScriptHelpers::TunedElement *DafxSparksRenParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "additive";
    enumEntries[0].value = 0;
    enumEntries[1].name = "alpha_blend";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("blending", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("motionScale", 0.05));
  elems.push_back(ScriptHelpers::create_tuned_real_param("hdrScale", 2));
  elems.push_back(ScriptHelpers::create_tuned_real_param("arrowShape", 0));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *DafxSparksGlobalParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_real_param("spawn_range_limit", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("max_instances", 100));
  elems.push_back(ScriptHelpers::create_tuned_int_param("player_reserved", 10));
  elems.push_back(ScriptHelpers::create_tuned_real_param("emission_min", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_number", 3));
  elems.push_back(ScriptHelpers::create_tuned_real_param("one_point_radius", 5));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxSparksQuality::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("low_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("medium_quality", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("high_quality", true));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxRenderGroup::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(1);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "highres";
    enumEntries[0].value = 0;
    enumEntries[1].name = "lowres";
    enumEntries[1].value = 1;
    enumEntries[2].name = "underwater";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


class DafxSparksEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "DafxSparks"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(6);

    elems.push_back(DafxEmitterParams::createTunedElement("DafxEmitterParams_data"));
    elems.push_back(DafxSparksSimParams::createTunedElement("DafxSparksSimParams_data"));
    elems.push_back(DafxSparksRenParams::createTunedElement("DafxSparksRenParams_data"));
    elems.push_back(DafxSparksGlobalParams::createTunedElement("DafxSparksGlobalParams_data"));
    elems.push_back(DafxSparksQuality::createTunedElement("DafxSparksQuality_data"));
    elems.push_back(DafxRenderGroup::createTunedElement("DafxRenderGroup_data"));

    return ScriptHelpers::create_tuned_group("params", 4, elems);
  }
};

static DafxSparksEffectTools tools;


void register_dafx_sparks_fx_tools() { ::register_effect_class_tools(&tools); }
