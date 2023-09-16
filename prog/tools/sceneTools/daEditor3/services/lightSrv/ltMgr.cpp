#include "ltMgr.h"
#include <scene/dag_sh3LtMgr.h>
#include <oldEditor/de_interface.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_debug.h>

class LtMgrForLightService::StaticRayHit : public SH3LightingManager::IRayTracer
{
public:
  virtual bool rayHit(const Point3 &p, const Point3 &norm_dir, float max_dist)
  {
    return DAGORED2->shadowRayHitTest(p, norm_dir, max_dist);
  }
  virtual bool rayTrace(const Point3 &p, const Point3 &norm_dir, float max_dist, float &dist)
  {
    dist = max_dist;

    // when in trouble, uncomment 3 lines below to get non-soft sun shading (old behaviour)
    // if ( !rayHit(p,norm_dir,max_dist))
    //  return false;
    // dist = 0.0; return true;

    if (!DAGORED2->shadowRayHitTest(p, norm_dir, max_dist))
      return false;

    float a = 0, b = max_dist, c = (a + b) / 2;
    while (fabs(b - a) > 0.5)
    {
      if (DAGORED2->shadowRayHitTest(p + norm_dir * a, norm_dir, c - a))
        b = c + 0.1;
      else
        a = c - 0.1;

      c = (a + b) / 2;
    }

    dist = c;
    return true;
  }
};

static LtMgrForLightService::StaticRayHit tracer;

LtMgrForLightService::~LtMgrForLightService() { del_it(mgr); }

void LtMgrForLightService::save(const char *fname, StaticGeometryContainer &geom)
{
  FullFileSaveCB cwr(fname);
  cwr.writeInt(_MAKE4C('LMNG'));
  cwr.writeInt(_MAKE4C('v4'));
  mkbindump::BinDumpSaveCB bdcwr(128 << 10, _MAKE4C('PC'), false);
  exportShlt(bdcwr, geom);
  bdcwr.copyDataTo(cwr);
}

bool LtMgrForLightService::load(const char *fname)
{
  del_it(mgr);

  FullFileLoadCB crd(fname);
  if (!crd.fileHandle)
    return false;

  DAGOR_TRY
  {
    if (crd.readInt() != _MAKE4C('LMNG'))
      return false;
    if (crd.readInt() != _MAKE4C('v4'))
      return false;
    if (crd.beginTaggedBlock() != _MAKE4C('SH3L'))
      return false;

    mgr = new SH3LightingManager;
    mgr->tracer = &tracer;
    SH3LightingManager::min_power_of_light_use = 0.008;

    if (mgr->loadLtDataBinary(crd, -1) == -1)
    {
      debug("light manager loading failed!");
      del_it(mgr);
      return false;
    }

    crd.endBlock();
  }
  DAGOR_CATCH(IGenLoad::LoadException)
  {
    debug("Light manager loading failed");
    del_it(mgr);
    return false;
  }

  return true;
}
