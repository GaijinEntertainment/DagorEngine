// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/dag_baseFxClasses.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>

namespace
{
class StubFx : public BaseEffectObject
{
  virtual ~StubFx() {}

  BaseParamScript *clone() override { return new StubFx(); };
  void loadParamsData(const char *, int, BaseParamScriptLoadCB *) override {}

  void update(float) override {}
  void drawEmitter(const Point3 &) override {}
  void render(unsigned, const TMatrix &) override {}

  void setParam(unsigned, void *) override {}
  void *getParam(unsigned id, void *value) override
  {
    switch (id)
    {
      case _MAKE4C('FXLX'): *(Point2 *)value = Point2{3, 5}; break;
      case _MAKE4C('FXLM'): *(int *)value = 100; break;
      default: return nullptr;
    }
    return value;
  }
};

class OutdatedFxFactory : public BaseEffectFactory
{
  const char *name;

public:
  OutdatedFxFactory(const char *_name) : name(_name) {}
  BaseParamScript *createObject() override { return new StubFx{}; }

  void destroyFactory() override {}
  const char *getClassName() override { return name; }
};
} // namespace

static OutdatedFxFactory g_flowps2_factory{"FlowPs2"};
static OutdatedFxFactory g_compoundps_factory{"CompoundPs"};

void register_outdatedFx_factory(bool enabled)
{
  if (!enabled)
    return;

  register_effect_factory(&g_flowps2_factory);
  register_effect_factory(&g_compoundps_factory);
}