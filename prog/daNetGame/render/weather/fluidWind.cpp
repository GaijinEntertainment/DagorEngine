// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fluidWind.h"
#include <render/wind/fluidWind.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_Point3.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>

static const Point3 FLUID_WIND_VOLUME_SIZE(64, 32, 64);
static const int FLUID_WIND_DIM_X = 32;
static const int FLUID_WIND_DIM_Y = 32;
static const int FLUID_WIND_DIM_Z = 16;

static eastl::unique_ptr<FluidWind> fluid_wind;

CONSOLE_BOOL_VAL("render", fluid_wind_debug_output, false);

void init_fluid_wind()
{
  bool fluidWindEnabled = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("fluidWind", true);

  if (fluidWindEnabled)
  {
    const DataBlock *fluidWindBlk = dgs_get_game_params()->getBlockByNameEx("fluidWind");

    FluidWind::Desc desc;
    desc.worldSize = fluidWindBlk->getPoint3("volumeSize", FLUID_WIND_VOLUME_SIZE);
    desc.dimX = fluidWindBlk->getInt("dimX", FLUID_WIND_DIM_X);
    desc.dimY = fluidWindBlk->getInt("dimY", FLUID_WIND_DIM_Y);
    desc.dimZ = fluidWindBlk->getInt("dimZ", FLUID_WIND_DIM_Z);
    fluid_wind.reset(new FluidWind(desc));
  }
}
FluidWind *get_fluid_wind() { return fluid_wind.get(); }
void close_fluid_wind() { fluid_wind.reset(); }

void update_fluid_wind(float dt, const Point3 &origin)
{
  if (!(fluid_wind && dt > 0.0f))
    return;

  {
    TIME_D3D_PROFILE(wind_effects);
    fluid_wind->update(dt, origin);
  }
#if DAGOR_DBGLEVEL > 0
  if (fluid_wind_debug_output.get())
    fluid_wind->renderDebug();
#endif
}