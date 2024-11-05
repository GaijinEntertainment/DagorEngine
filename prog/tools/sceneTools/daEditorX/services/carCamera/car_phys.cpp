// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "car_phys.h"

#include "../../de_appwnd.h"
#include <oldEditor/de_workspace.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_workspace.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <scene/dag_physMat.h>
#include <shaders/dag_shaderMesh.h>
#include <debug/dag_debug3d.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>
#include <math/dag_rayIntersectBox.h>
#include <phys/dag_physResource.h>

#include "../Clipping/clippingPlugin.h"
#include "../Clipping/clippingCm.h"

#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

#include <scene/dag_frtdumpMgr.h>


extern void phys_bullet_init();
extern void phys_bullet_close();
extern void phys_bullet_simulate(real dt);
extern void phys_bullet_create_phys_world(DynamicPhysObjectData *base_obj);
extern void *phys_bullet_get_phys_world();
extern void phys_bullet_set_phys_body(void *phbody);
extern void phys_bullet_clear_phys_world();
extern void phys_bullet_before_render();
extern void phys_bullet_render_trans();
extern void phys_bullet_render();
extern bool phys_bullet_get_phys_tm(int body_id, TMatrix &phys_tm, bool &obj_active);
extern void phys_bullet_add_impulse(int body_ind, const Point3 &pos, const Point3 &delta, real spring_factor, real damper_factor,
  real dt);
extern bool phys_bullet_load_collision(IGenLoad &crd);
extern void phys_bullet_install_tracer(bool (*traceray)(const Point3 &p, const Point3 &d, float &mt, Point3 &out_n, int &out_pmid));


static bool needSimulate = false;
static int physType = 0;
static Ptr<PhysicsResource> simObjRes = NULL;
static float curSimDt = 0.01;

static FastRtDumpManager phys_frt;
static inline bool phys_traceray_normal(const Point3 &p, const Point3 &dir, real &t, Point3 &n, int &pmid)
{
  return phys_frt.traceray(p, dir, t, pmid, n);
}


namespace carphyssimulator
{
real springFactor = 1.f;
real damperFactor = 0.2;

void simulate(real dt)
{
  curSimDt = dt;
  if (!needSimulate)
    return;

  if (physType == PHYS_BULLET)
    phys_bullet_simulate(dt);
}

int getDefaultPhys()
{
  const char *collision = DAGORED2->getWorkspace().getCollisionName();
  if ((stricmp(collision, "bullet") == 0) || (stricmp(collision, "DagorBullet") == 0))
    return PHYS_BULLET;
  else
  {
    CoolConsole &log = DAGORED2->getConsole();
    log.addMessage(ILogWriter::ERROR, "unsupported collision name: '%s', switching to '%s'", collision, "bullet");
    log.showConsole();
    return PHYS_BULLET;
  }
}

static bool initPhys(int phys_type)
{
  int newPhys = (phys_type == PHYS_DEFAULT) ? getDefaultPhys() : phys_type;
  if (newPhys == PHYS_DEFAULT)
    return false;
  if (physType != PHYS_DEFAULT)
  {
    phys_bullet_close();
  }
  physType = newPhys;

  if (physType == PHYS_BULLET)
    phys_bullet_init();

  return true;
}

void init() {}


bool buildCollisions(mkbindump::BinDumpSaveCB &cwr, unsigned target_code)
{
  IBinaryDataBuilder *collisionBuilder = NULL;
  IGenEditorPlugin *plug = NULL;

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    plug = DAGORED2->getPlugin(i);

    if (!strcmp(plug->getInternalName(), "clipping"))
    {
      collisionBuilder = plug->queryInterface<IBinaryDataBuilder>();
      break;
    }
  }

  if (collisionBuilder)
  {
    if (wingw::message_box(wingw::MBS_OKCANCEL, "Collision build", "Rebuild collisions?") == wingw::MB_ID_OK)
      plug->onPluginMenuClick(CM_COMPILE_GAME_CLIPPING);

    TextureRemapHelper trh(target_code);
    DAEDITOR3.conNote("+++ Exporting ...");
    if (!collisionBuilder->buildAndWrite(cwr, trh, NULL))
    {
      DAEDITOR3.conError("export error");
      DAGORED2->getConsole().endProgress();
      DAEDITOR3.conShow(false);
      return false;
    }
    cwr.beginTaggedBlock(_MAKE4C('END'));
    cwr.endBlock();
    DAEDITOR3.conShow(false);
  }

  return true;
}


bool setCollisionsToWorld(mkbindump::BinDumpSaveCB &cwr)
{
  // loading dump
  DAEDITOR3.conNote("+++ Loading ...");

  MemoryLoadCB crd(cwr.getRawWriter().getMem(), false);
  int tag = 0;
  unsigned bindump_id = 0xFFFFFFFF;

  for (;;)
  {
    tag = crd.beginTaggedBlock();
    bool need_break = false;

    switch (tag)
    {
      case _MAKE4C('FRT'):
      {
        int id = phys_frt.loadRtDump(crd, bindump_id);
        if (id == -1)
          break;
        phys_bullet_install_tracer(&phys_traceray_normal);
      }
      break;

      case _MAKE4C('B_RT'):
        if (!phys_bullet_load_collision(crd))
          break;
        break;

      case _MAKE4C('END'): need_break = true; break;

      default: break;
    }

    crd.endBlock();
    if (need_break)
      break;
  }

  return true;
}


void *getPhysWorld()
{
  if (physType == PHYS_BULLET)
    return phys_bullet_get_phys_world();
  return NULL;
}


void setTargetObj(void *phys_body, const char *res)
{
  simObjRes = NULL;
  if (res)
  {
    DynamicPhysObjectData *podata =
      (DynamicPhysObjectData *)get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(res), PhysObjGameResClassId);
    if (podata)
    {
      simObjRes = podata->physRes;
      release_game_resource((GameResource *)podata);
    }
    else
    {
      CoolConsole &log = DAGORED2->getConsole();
      log.addMessage(ILogWriter::ERROR, "cannot load physob res: <%s>", res);
      log.showConsole();
    }
  }

  if (physType == PHYS_BULLET)
    phys_bullet_set_phys_body(phys_body);
}


bool begin(DynamicPhysObjectData *base_obj, int phys_type)
{
  unsigned target_code = _MAKE4C('PC');
  mkbindump::BinDumpSaveCB cwr(128 << 20, target_code, false);
  CoolConsole &log = DAGORED2->getConsole();

  if (!buildCollisions(cwr, target_code))
  {
    log.addMessage(ILogWriter::ERROR, "cannot build collisions");
    wingw::message_box(wingw::MBS_EXCL, "Collision error",
      "No collision found \n"
      "Please build collisions by menu Plugin->\"Compile collision for game (PC)...\" "
      "in \"Collision\" plugin");
    return false;
  }

  if (!initPhys(phys_type))
  {
    log.addMessage(ILogWriter::ERROR, "cannot init Physics");
    log.showConsole();
    needSimulate = false;
    return false;
  }

  if (physType == PHYS_BULLET)
    phys_bullet_create_phys_world(base_obj);
  needSimulate = true;
  simObjRes = base_obj ? base_obj->physRes : NULL;

  return setCollisionsToWorld(cwr);
}


void end()
{
  setTargetObj(NULL, NULL);
  if (physType == PHYS_BULLET)
    phys_bullet_clear_phys_world();

  needSimulate = false;

  phys_bullet_install_tracer(nullptr);
  phys_frt.delAllRtDumps();
  phys_bullet_close();
}

void close() { phys_bullet_close(); }

void beforeRender()
{
  if (physType == PHYS_BULLET)
    phys_bullet_before_render();
}

void renderTrans(bool render_collision, bool render_geom, bool draw_cmass)
{
  if (render_geom)
  {
    if (physType == PHYS_BULLET)
      phys_bullet_render_trans();
  }
}

void render()
{
  if (physType == PHYS_BULLET)
    phys_bullet_render();
}
} // namespace carphyssimulator
