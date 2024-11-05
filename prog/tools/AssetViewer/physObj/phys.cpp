// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys.h"
#include "../av_appwnd.h"

#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_workspace.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <scene/dag_physMat.h>
#include <shaders/dag_shaderMesh.h>
#include <debug/dag_debug3d.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>
#include <math/dag_rayIntersectBox.h>
#include <phys/dag_physResource.h>

static bool needSimulate = false;
static int physType = 0;
static bool isGridVisibleLast = false;
PtrTab<PhysicsResource> physsimulator::simObjRes;

static int objIdx = -1, subBodyIdx = -1;
static Point3 contactPointLocal(0, 0, 0);
static Point3 contactPoint2World(0, 0, 0);
static real contactDist = 0.f;
static float curSimDt = 0.01;

#define CALL_PHYS_FUNC(FUNC_SUFFIX, ARGS, UNSUPP_STMNT)                     \
  switch (physType)                                                         \
  {                                                                         \
    case physsimulator::PHYS_BULLET: phys_bullet_##FUNC_SUFFIX ARGS; break; \
    case physsimulator::PHYS_JOLT: phys_jolt_##FUNC_SUFFIX ARGS; break;     \
    default: UNSUPP_STMNT;                                                  \
  }

#define CALL_PHYS_FUNC_RET(FUNC_SUFFIX, ARGS, UNSUPP_RET)                   \
  switch (physType)                                                         \
  {                                                                         \
    case physsimulator::PHYS_BULLET: return phys_bullet_##FUNC_SUFFIX ARGS; \
    case physsimulator::PHYS_JOLT: return phys_jolt_##FUNC_SUFFIX ARGS;     \
    default: return UNSUPP_RET;                                             \
  }

static inline void phys_close_all()
{
  phys_bullet_close();
  phys_jolt_close();
}

static inline bool getPhysTm(int obj_idx, int sub_body_idx, TMatrix &ptm, bool &obj_active)
{
  CALL_PHYS_FUNC(get_phys_tm, (obj_idx, sub_body_idx, ptm, obj_active), return false);
  return true;
}


namespace boxrender
{
static Tab<TMatrix> tms(midmem);
static Tab<E3DCOLOR> tms_color(midmem);
static DynRenderBuffer *drb = NULL;
static DynRenderBuffer::Vertex boxVert[8];
static int boxInd[36];
static int boxqi[] = {0, 1, 3, 2, 4, 5, 7, 6, 0, 1, 5, 4, 3, 2, 6, 7, 0, 4, 6, 2, 1, 5, 7, 3};

const BBox3 normBox(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));


static inline void init()
{
  drb = new DynRenderBuffer("editor_helper_no_tex_blend");

  TMatrix tm(TMatrix::IDENT);
  tm.setcol(0, tm.getcol(0) * 8.f);
  tm.setcol(1, tm.getcol(1) * 0.2);
  tm.setcol(2, tm.getcol(2) * 4.f);
  tm.setcol(3, 0, -2.15, -2);
  tms.push_back(tm);
  tms_color.push_back(E3DCOLOR(100, 0, 0, 63));

  tm.setcol(3, 0, -2.15, +2);
  tms.push_back(tm);
  tms_color.push_back(E3DCOLOR(100, 0, 60, 63));

  tm = TMatrix::IDENT;
  tm.setcol(0, tm.getcol(0) * 500);
  tm.setcol(2, tm.getcol(2) * 500);
  tm.setcol(3, 0, -2.75, 0);
  tms.push_back(tm);
  tms_color.push_back(E3DCOLOR(0, 0, 100, 63));

  memset(boxVert, 0, sizeof(boxVert));
  for (int i = 0; i < 6; i++)
  {
    int *ind = boxInd + i * 6;
    ind[0] = boxqi[i * 4 + 0];
    ind[1] = boxqi[i * 4 + 1];
    ind[2] = boxqi[i * 4 + 2];
    ind[3] = boxqi[i * 4 + 0];
    ind[4] = boxqi[i * 4 + 2];
    ind[5] = boxqi[i * 4 + 3];
  }
}
static inline void renderBox(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c)
{
  for (int i = 0; i < 8; i++)
  {
    boxVert[i].pos = tm * normBox.point(i);
    boxVert[i].color = c;
  }

  db.addInd(boxInd, 36, db.addVert(boxVert, 8));
}

static inline void drawBox()
{
  const int cnt = tms.size();

  G_ASSERT(cnt == tms_color.size());

  drb->clearBuf();
  for (int i = 0; i < cnt; i++)
    renderBox(*drb, tms[i], tms_color[i]);

  set_cached_debug_lines_wtm(TMatrix::IDENT);
  d3d::settm(TM_WORLD, TMatrix::IDENT);
  drb->flushToBuffer(BAD_TEXTUREID);
  drb->flush();

  begin_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  for (int i = 0; i < cnt; i++)
  {
    Point3 ax = tms[i].getcol(0);
    Point3 ay = tms[i].getcol(1);
    Point3 az = tms[i].getcol(2);

    draw_cached_debug_box(tms[i].getcol(3) - (ax + ay + az) / 2.f, ax, ay, az, E3DCOLOR(0, 255, 0));
  }

  end_draw_cached_debug_lines();
}
} // namespace boxrender

static inline void renderCollision(PhysicsResource *simObjRes, int obj_idx)
{
  bool active;
  int cnt = simObjRes->getBodies().size();
  for (int i = 0; i < cnt; i++)
  {
    TMatrix ptm;
    if (!getPhysTm(obj_idx, i, ptm, active))
      return;

    const E3DCOLOR color(active ? 0 : 255, active ? 255 : 0, 0);
    const PhysicsResource::Body &body = simObjRes->getBodies()[i];

    set_cached_debug_lines_wtm(ptm);

    int scnt = body.sphColl.size();
    for (int j = 0; j < scnt; j++)
      draw_cached_debug_sphere(body.capColl[j].center, body.capColl[j].radius, color);

    int bcnt = body.boxColl.size();
    for (int j = 0; j < bcnt; j++)
    {
      const TMatrix &m = body.boxColl[j].tm;
      const Point3 &s = body.boxColl[j].size;
      Point3 ax = m.getcol(0) * s.x;
      Point3 ay = m.getcol(1) * s.y;
      Point3 az = m.getcol(2) * s.z;

      draw_cached_debug_box(m.getcol(3) - (ax + ay + az) / 2.f, ax, ay, az, color);
    }

    int ccnt = body.capColl.size();
    for (int j = 0; j < ccnt; j++)
    {
      const Point3 &p = body.capColl[j].center;
      const real &r = body.capColl[j].radius;
      const Point3 e = body.capColl[j].extent - r * normalize(body.capColl[j].extent);

      Capsule c;
      c.set(p - e, p + e, r);

      draw_cached_debug_capsule_w(c, color);
    }
  }
}
static inline void renderCollision()
{
  using physsimulator::simObjRes;
  for (int i = 0; i < simObjRes.size(); i++)
    renderCollision(simObjRes[i], i);
}
static inline void renderCmass(PhysicsResource *simObjRes, int obj_idx)
{
  int cnt = simObjRes->getBodies().size();
  bool active;
  for (int i = 0; i < cnt; i++)
  {
    TMatrix tm;
    if (!getPhysTm(obj_idx, i, tm, active))
      return;

    set_cached_debug_lines_wtm(TMatrix::IDENT);

    Point3 c = tm.getcol(3);

    draw_debug_line(c, c + tm.getcol(0), E3DCOLOR(255, 000, 000));
    draw_debug_line(c, c + tm.getcol(1), E3DCOLOR(000, 255, 000));
    draw_debug_line(c, c + tm.getcol(2), E3DCOLOR(000, 000, 255));
  }
}
static inline void renderCmass()
{
  using physsimulator::simObjRes;
  for (int i = 0; i < simObjRes.size(); i++)
    renderCmass(simObjRes[i], i);
}

static inline void renderSpring()
{
  if (objIdx == -1)
    return;

  set_cached_debug_lines_wtm(TMatrix::IDENT);

  TMatrix ptm;
  bool active;
  if (!getPhysTm(objIdx, subBodyIdx, ptm, active))
    return;

  Point3 p1 = contactPointLocal * ptm;
  Point3 &p2 = contactPoint2World;

  draw_debug_line(p1, p2, E3DCOLOR(0, 200, 200));
}

namespace physsimulator
{
real springFactor = 1000.f;
real damperFactor = 0.2;

static void addImpulse(int obj_idx, int sub_body_idx, const Point3 &local_pos, const Point3 &p2_world, real spring_factor,
  real damper_factor, real dt)
{
  TMatrix ptm;
  bool active;
  if (!getPhysTm(obj_idx, sub_body_idx, ptm, active))
    return;

  Point3 contactPoint = local_pos * ptm;
  Point3 delta = p2_world - contactPoint;

  CALL_PHYS_FUNC(add_impulse, (obj_idx, sub_body_idx, contactPoint, delta, spring_factor, damper_factor, dt), break);
}

void simulate(real dt)
{
  curSimDt = dt;
  if (!needSimulate)
    return;

  CALL_PHYS_FUNC(simulate, (dt), break);

  if (objIdx != -1)
    addImpulse(objIdx, subBodyIdx, contactPointLocal, contactPoint2World, springFactor, damperFactor, dt);
}

int getDefaultPhys()
{
  const char *collision = ::get_app().getWorkspace().getCollisionName();
  if ((stricmp(collision, "bullet") == 0) || (stricmp(collision, "DagorBullet") == 0))
    return PHYS_BULLET;
  else
  {
    CoolConsole &log = ::get_app().getConsole();
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
  else if (newPhys == physType)
    return true;
  else if (physType != PHYS_DEFAULT)
    phys_close_all();
  physType = newPhys;

  CALL_PHYS_FUNC(init, (), break);
  return true;
}

void init()
{
  if (!boxrender::drb)
    boxrender::init();
}

bool begin(DynamicPhysObjectData *base_obj, int phys_type, int inst_count, int scene_type, float interval, float base_plane_ht0,
  float base_plane_ht1)
{
  if (!initPhys(phys_type))
  {
    needSimulate = false;
    return false;
  }

  CALL_PHYS_FUNC(init, (), break);
  CALL_PHYS_FUNC(create_phys_world, (base_obj, base_plane_ht0, base_plane_ht1, inst_count, scene_type, interval), break);
  needSimulate = true;

  isGridVisibleLast = get_app().isGridVisible();
  if (boxrender::tms.size() > 1)
  {
    boxrender::tms[0].setcol(3, Point3(0, base_plane_ht0 - 0.1, -2));
    boxrender::tms[1].setcol(3, Point3(0, base_plane_ht1 - 0.1, +2));
  }
  if (boxrender::tms.size() > 2)
    boxrender::tms[2].setcol(3, Point3(0, min(base_plane_ht0, base_plane_ht1) - 0.7, 0));

  get_app().renderGrid(false);

  return true;
}
void *getPhysWorld()
{
  CALL_PHYS_FUNC_RET(get_phys_world, (), nullptr);
  return NULL;
}
void setTargetObj(void *phys_body, const char *res)
{
  simObjRes.clear();
  if (res)
  {
    DynamicPhysObjectData *podata =
      (DynamicPhysObjectData *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(res), PhysObjGameResClassId);
    if (podata)
    {
      simObjRes.push_back(podata->physRes);
      release_game_resource((GameResource *)podata);
    }
    else
    {
      CoolConsole &log = ::get_app().getConsole();
      log.addMessage(ILogWriter::ERROR, "cannot load physob res: <%s>", res);
      log.showConsole();
    }
  }

  CALL_PHYS_FUNC(set_phys_body, (phys_body), break);
}

void end()
{
  setTargetObj(NULL, NULL);
  CALL_PHYS_FUNC(clear_phys_world, (), break);
  CALL_PHYS_FUNC(close, (), break);

  get_app().renderGrid(isGridVisibleLast);

  needSimulate = false;
}

void close()
{
  phys_close_all();

  del_it(boxrender::drb);
}

void beforeRender() { CALL_PHYS_FUNC(before_render, (), break); }

void renderTrans(bool render_collision, bool render_geom, bool bodies, bool body_center, bool constraints, bool constraints_refsys)
{
  if (render_geom)
    CALL_PHYS_FUNC(render_trans, (), break);
  if (!simObjRes.empty())
  {
    begin_draw_cached_debug_lines();

    if (render_collision)
      renderCollision();

    if (body_center && (physType != physsimulator::PHYS_BULLET && physType != physsimulator::PHYS_JOLT))
      renderCmass();

    renderSpring();

    end_draw_cached_debug_lines();
  }

  boxrender::drawBox();
  CALL_PHYS_FUNC(render_debug, (bodies, body_center, constraints, constraints_refsys), break);
}

void render() { CALL_PHYS_FUNC(render, (), break); }

static bool traceCollision(PhysicsResource *simObjRes, int o_idx, const Point3 &pt, Point3 dir, Point3 &cp, int &obj_idx,
  int &sub_body_idx, real &dist)
{
  G_ASSERT(fabs(length(dir) - 1.0) < 0.001);
  real maxt = 100.f;

  int cnt = simObjRes->getBodies().size();
  bool ret = false;
  for (int i = 0; i < cnt; i++)
  {
    TMatrix ptm;
    bool active;
    if (!getPhysTm(o_idx, i, ptm, active))
      return false;

    const PhysicsResource::Body &body = simObjRes->getBodies()[i];

    int ccnt = body.capColl.size();
    for (int j = 0; j < ccnt; j++)
    {
      const Point3 &p = body.capColl[j].center;
      const real &r = body.capColl[j].radius;
      const Point3 e = body.capColl[j].extent - r * normalize(body.capColl[j].extent);

      Capsule c;
      c.set(p - e, p + e, r);
      c.transform(ptm);

      Point3 norm;
      if (c.traceRay(pt, dir, maxt, norm))
      {
        dist = maxt;
        cp = pt + dir * dist;
        obj_idx = o_idx, sub_body_idx = i;

        ret = true;
      }
    }

    int scnt = body.sphColl.size();
    for (int j = 0; j < scnt; j++)
    {
      Point3 c = body.capColl[j].center;

      Capsule capsule(c, c, body.capColl[j].radius);
      capsule.transform(ptm);

      Point3 norm;
      if (capsule.traceRay(pt, dir, maxt, norm))
      {
        dist = maxt;
        cp = pt + dir * dist;
        obj_idx = o_idx, sub_body_idx = i;

        ret = true;
      }
    }

    int bcnt = body.boxColl.size();
    for (int j = 0; j < bcnt; j++)
    {
      TMatrix m = ptm * body.boxColl[j].tm;
      const Point3 &s = body.boxColl[j].size;
      Point3 hs = s / 2.f;
      BBox3 box(-hs, +hs);

      if (ray_intersect_box(pt, dir, box, m, maxt))
      {
        dist = maxt;
        cp = pt + dir * dist;
        obj_idx = o_idx, sub_body_idx = i;

        ret = true;
      }
    }
  }

  return ret;
}
static bool traceCollision(const Point3 &pt, Point3 dir, Point3 &cp, int &obj_idx, int &sub_body_idx, real &dist)
{
  if (simObjRes.empty())
    return false;
  bool ret = false;
  obj_idx = sub_body_idx = -1;
  for (int i = 0; i < simObjRes.size(); i++)
    ret |= traceCollision(simObjRes[i], i, pt, dir, cp, obj_idx, sub_body_idx, dist);
  return ret;
}

bool shootAtObject(const Point3 &pt, const Point3 &dir, float bullet_impulse)
{
  Point3 shotPt;
  int obj_idx, sub_body_idx;
  real dist;

  if (traceCollision(pt, dir, shotPt, obj_idx, sub_body_idx, dist))
  {
    CALL_PHYS_FUNC(add_impulse, (obj_idx, sub_body_idx, shotPt, dir, 0, bullet_impulse, -0.01), return false);
    return true;
  }

  return false;
}

bool connectSpring(const Point3 &pt, const Point3 &dir)
{
  if (traceCollision(pt, dir, contactPoint2World, objIdx, subBodyIdx, contactDist))
  {
    TMatrix ptm;
    bool active;
    if (!getPhysTm(objIdx, subBodyIdx, ptm, active))
    {
      objIdx = subBodyIdx = -1;
      return false;
    }

    contactPointLocal = inverse(ptm) * contactPoint2World;
    return true;
  }
  else
  {
    objIdx = subBodyIdx = -1;
    return false;
  }
}

bool disconnectSpring()
{
  if (objIdx != -1)
  {
    objIdx = subBodyIdx = -1;
    return true;
  }
  return false;
}

void setSpringCoord(const Point3 &pt, const Point3 &dir) { contactPoint2World = pt + dir * contactDist; }
} // namespace physsimulator
