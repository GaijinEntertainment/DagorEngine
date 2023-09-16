#include <3d/dag_drvDecl.h>
#include <3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>
#include <render/lightCube.h>
#include <render/lightProbeSpecularCubesContainer.h>
#include <render/sphHarmCalc.h>
#include <render/lightProbe.h>

void LightProbe::init(LightProbeSpecularCubesContainer *containter, bool should_calc_diffuse)
{
  probesContainer = containter;
  hasDiffuse = should_calc_diffuse;
  if (!probesContainer)
    return;
  if (probeId != -1)
    probesContainer->deallocate(probeId);
  invalidate();
  probeId = probesContainer->allocate();
}

const ManagedTex *LightProbe::getCubeTex() const { return probesContainer ? probesContainer->getManagedTex() : nullptr; }

LightProbe::~LightProbe()
{
  if (probesContainer)
    probesContainer->deallocate(probeId);
}

void LightProbe::invalidate()
{
  mem_set_0(sphHarm);
  renderedCubeFaces = 0;
  processedSpecularFaces = 0;
  valid = false;
  position = Point3(-1000000, -100000, +100000);
  if (probesContainer)
    probesContainer->invalidateProbe(this);
}

void LightProbe::update()
{
  if (valid || !probesContainer)
    return;
  probesContainer->addProbeToUpdate(this);
}

const Color4 *LightProbe::getSphHarm() const { return sphHarm.data(); }

void LightProbe::setSphHarm(const Color4 *ptr) { mem_copy_from(sphHarm, ptr); }
