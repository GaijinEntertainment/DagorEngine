#include <phys/dag_physics.h>
#include <phys/dag_physObject.h>
#include <phys/dag_physRagdoll.h>
#include "phys.h"
#include <de3_lightService.h>
#include <3d/dag_render.h>
#include <shaders/dag_dynSceneRes.h>
#include <render/dag_cur_view.h>
#include <EditorCore/ec_interface.h>
#include <de3_dynRenderService.h>
#include <phys/dag_physDebug.h>

static PhysWorld *pw = NULL;
static bool pw_owned = false;
static bool inited = false;
static Tab<PhysSystemInstance *> simPhysSys;
static Tab<DynamicPhysObject *> simObj;
static PhysBody *simBody = NULL;
static bool mustDelSimObj = false;

static Tab<PhysBody *> stBody(midmem);

static inline PhysBody *getPhysBody(int obj_idx, int sub_body_idx)
{
  if (simBody)
    return simBody;
  if (obj_idx < simPhysSys.size())
    if (auto *physSys = simPhysSys[obj_idx])
      return physSys->getBody(sub_body_idx);
  return NULL;
}

static bool phys_get_phys_tm(int obj_idx, int sub_body_idx, TMatrix &phys_tm, bool &obj_active)
{
  if (simBody)
  {
    simBody->getTm(phys_tm);
    obj_active = simBody->isActive();
    return true;
  }

  if (obj_idx < simPhysSys.size())
    if (auto *physSys = simPhysSys[obj_idx])
      if (physSys->getBody(sub_body_idx))
      {
        physSys->getBody(sub_body_idx)->getTm(phys_tm);
        obj_active = physSys->getBody(sub_body_idx)->isActive();
        return true;
      }
  obj_active = false;
  return false;
}

static __forceinline void phys_simulate(real dt)
{
  if (!get_external_phys_world())
    pw->simulate(dt);
}
static __forceinline void phys_clear_phys_world()
{
  if (mustDelSimObj)
    clear_all_ptr_items(simObj);
  mustDelSimObj = false;
  simPhysSys.clear();
  physsimulator::simObjRes.clear();
  simBody = NULL;

  clear_all_ptr_items(stBody);
  if (pw_owned)
    del_it(pw);
  pw = nullptr;
  pw_owned = false;
}

static PhysBody *createStaticBody(const TMatrix &tm, real sz_x, real sz_y, real sz_z)
{
  PhysBodyCreationData pbcd;
  pbcd.autoMask = false, pbcd.group = pbcd.mask = 3;
  PhysBoxCollision coll(sz_x, sz_y, sz_z);
  return new PhysBody(pw, 0, &coll, tm, pbcd);
}

static __forceinline void phys_set_render_lod(int render_lod)
{
  for (DynamicPhysObject *dpo : simObj)
  {
    for (int modelIndex = 0; modelIndex < dpo->getModelCount(); ++modelIndex)
    {
      DynamicRenderableSceneInstance *model = dpo->getModel(modelIndex);
      const int lodCount = model->getLodsCount();
      const int finalLod = (render_lod >= 0 && lodCount > 1) ? clamp(render_lod, 0, lodCount - 1) : -1;
      model->setLod(finalLod);
    }
  }
}

static __forceinline void phys_create_phys_world(DynamicPhysObjectData *base_obj, float base_plane_ht0, float base_plane_ht1,
  int inst_count, int scene_type, float interval, int render_lod)
{
  if (pw)
    return;

  pw = get_external_phys_world();
  pw_owned = !pw;
  if (pw_owned)
    pw = new PhysWorld();

  TMatrix nullPlaneTm(TMatrix::IDENT);
  nullPlaneTm.setcol(3, 0, min(base_plane_ht0, base_plane_ht1) - 0.7, 0);
  stBody.push_back(createStaticBody(nullPlaneTm, 500.f, 1.f, 500.f));
  nullPlaneTm.setcol(3, 0, base_plane_ht0 - 0.1, -2);
  stBody.push_back(createStaticBody(nullPlaneTm, 8.f, 0.2, 4.f));
  nullPlaneTm.setcol(3, 0, base_plane_ht1 - 0.1, +2);
  stBody.push_back(createStaticBody(nullPlaneTm, 8.f, 0.2, 4.f));

  pw->setFixedTimeStep(-1.f / 60.0f);

  if (!base_obj)
    return;

  if (!interval)
  {
    BBox3 box;
    for (auto &m : base_obj->models)
      box += m->getLocalBoundingBox();
    if (scene_type == physsimulator::SCENE_TYPE_STACK)
      interval = box.width().y * 1.05;
    else if (scene_type == physsimulator::SCENE_TYPE_GROUP)
      interval = max(box.width().x * 1.05, box.width().z * 1.05);
  }

  // create DynamicPhysObject from initial data
  TMatrix tm = TMatrix::IDENT;
  for (int i = 0; i < inst_count; i++)
  {
    if (scene_type == physsimulator::SCENE_TYPE_STACK)
      tm.setcol(3, Point3(0, interval * i, 0));
    else if (scene_type == physsimulator::SCENE_TYPE_GROUP)
      tm.setcol(3, Point3(interval * ((i + 5) / 10) * (((i / 5) & 1) ? -1.f : +1.f), 0, interval * (i % 5)));

    DynamicPhysObject *po = new (midmem) DynamicPhysObject;
    po->init(base_obj, pw, tm);
    simObj.push_back(po);

    mustDelSimObj = true;
    simPhysSys.push_back(po->getPhysSys());
    physsimulator::simObjRes.push_back(base_obj->physRes);
  }

  if (render_lod >= 0)
    phys_set_render_lod(render_lod);
}
static __forceinline void phys_set_phys_body(void *phbody) { simBody = (PhysBody *)phbody; }

static __forceinline void phys_init()
{
  if (inited || get_external_phys_world())
    return;

  ::init_physics_engine();
  physdbg::init<PhysWorld>();
  inited = true;
}

static __forceinline void phys_close()
{
  if (!inited || get_external_phys_world())
    return;

  phys_clear_phys_world();

  physdbg::term<PhysWorld>();
  ::close_physics_engine();
  inited = false;
}

static __forceinline void phys_before_render()
{
  for (auto *o : simObj)
    o->beforeRender(::grs_cur_view.pos);

  auto rs = EDITORCORE->queryEditorInterface<IDynRenderService>();
  if (rs && rs->getRenderType() == IDynRenderService::RTYPE_DNG_BASED)
  {
    for (auto *o : simObj)
      for (int i = 0, ie = o->getModelCount(); i < ie; ++i)
      {
        DynamicRenderableSceneInstance *sceneInstance = o->getModel(i);
        for (uint32_t n = 0; n < sceneInstance->getNodeCount(); ++n)
        {
          TMatrix wtm = sceneInstance->getNodeWtm(n);
          wtm.setcol(3, wtm.getcol(3) - ::grs_cur_view.pos);
          sceneInstance->setPrevNodeWtm(n, wtm);
        }
      }
  }
}
static __forceinline void phys_render(IDynRenderService::Stage stage)
{
  if (auto *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
    for (auto *o : simObj)
      for (int i = 0, ie = o->getModelCount(); i < ie; ++i)
        rs->renderOneDynModelInstance(o->getModel(i), stage);
}
static __forceinline void phys_render_debug(bool bodies, bool body_center, bool constraints, bool constraints_refsys)
{
  physdbg::RenderFlags flags;
  if (bodies)
    flags |= physdbg::RenderFlag::BODIES;
  if (body_center)
    flags |= physdbg::RenderFlag::BODY_CENTER;
  if (constraints)
    flags |= physdbg::RenderFlag::CONSTRAINTS | physdbg::RenderFlag::CONSTRAINT_LIMITS;
  if (constraints_refsys)
    flags |= physdbg::RenderFlag::CONSTRAINT_REFSYS;
  if (unsigned(flags))
    physdbg::renderWorld(pw, flags);
}

static __forceinline void add_impulse(int obj_idx, int sub_body_idx, const Point3 &pos, const Point3 &delta, real spring_factor,
  real damper_factor, real dt) // for unit mass
{
  PhysBody *pb = getPhysBody(obj_idx, sub_body_idx);
  if (!pb)
    return;

  real mass = pb->getMass();
  if (dt < 0)
  {
    pb->addImpulse(pos, delta * (damper_factor > 0 ? damper_factor : mass * -dt));
    return;
  }

  float delta_l = length(delta) - 0.2;
  static float lastLen = delta_l;
  float force = 0;

  if (delta_l > 0)
  {
    force += mass * (delta_l * spring_factor - damper_factor * (lastLen - delta_l) / dt);
    lastLen = delta_l;
  }
  else
    lastLen = 0;

  pb->addImpulse(pos, normalize(delta) * force * dt);
}
static __forceinline void *phys_start_ragdoll(AnimV20::AnimcharBaseComponent *animChar, AnimV20::AnimcharFinalMat44 *finalWtm,
  const Point3 &vel0)
{
  PhysRagdoll *ragdoll = PhysRagdoll::create(animChar->getPhysicsResource(), pw);
  G_ASSERT_RETURN(ragdoll, NULL);

  ragdoll->setRecalcWtms(true);
  ragdoll->setContinuousCollisionMode(true);

  ragdoll->setStartAddLinVel(vel0);
  animChar->setTm(finalWtm->nwtm[0]);
  animChar->act(0.f, true);
  animChar->copyNodesTo(*finalWtm);
  ragdoll->startRagdoll();
  animChar->setPostController(ragdoll);
  physsimulator::simObjRes.push_back(ragdoll->getPhysRes());
  simPhysSys.push_back(ragdoll->getPhysSys());
  return ragdoll;
}

#define IMPLEMENT_PHYS_API(PHYS)                                                                                               \
  void phys_##PHYS##_simulate(real dt) { phys_simulate(dt); }                                                                  \
  void phys_##PHYS##_clear_phys_world() { phys_clear_phys_world(); }                                                           \
  void phys_##PHYS##_create_phys_world(DynamicPhysObjectData *base_obj, float h0, float h1, int inst_count, int scene_type,    \
    float interval, int render_lod)                                                                                            \
  {                                                                                                                            \
    phys_create_phys_world(base_obj, h0, h1, inst_count, scene_type, interval, render_lod);                                    \
  }                                                                                                                            \
  void *phys_##PHYS##_get_phys_world() { return pw; }                                                                          \
  void phys_##PHYS##_set_phys_body(void *phbody) { phys_set_phys_body(phbody); }                                               \
  void phys_##PHYS##_init() { phys_init(); }                                                                                   \
  void phys_##PHYS##_close() { phys_close(); }                                                                                 \
  void phys_##PHYS##_before_render() { phys_before_render(); }                                                                 \
  void phys_##PHYS##_render() { phys_render(IDynRenderService::Stage::STG_RENDER_DYNAMIC_OPAQUE); }                            \
  void phys_##PHYS##_render_decals() { phys_render(IDynRenderService::Stage::STG_RENDER_DYNAMIC_DECALS); }                     \
  void phys_##PHYS##_render_trans() { phys_render(IDynRenderService::Stage::STG_RENDER_DYNAMIC_TRANS); }                       \
  void phys_##PHYS##_render_debug(bool b, bool bc, bool c, bool cr) { phys_render_debug(b, bc, c, cr); }                       \
  void phys_##PHYS##_set_render_lod(int render_lod) { phys_set_render_lod(render_lod); }                                       \
  bool phys_##PHYS##_get_phys_tm(int obj_idx, int sub_body_idx, TMatrix &phys_tm, bool &obj_active)                            \
  {                                                                                                                            \
    return phys_get_phys_tm(obj_idx, sub_body_idx, phys_tm, obj_active);                                                       \
  }                                                                                                                            \
  void phys_##PHYS##_add_impulse(int obj_idx, int sub_body_idx, const Point3 &pos, const Point3 &delta, real spring_factor,    \
    real damper_factor, real dt)                                                                                               \
  {                                                                                                                            \
    add_impulse(obj_idx, sub_body_idx, pos, delta, spring_factor, damper_factor, dt);                                          \
  }                                                                                                                            \
  void *phys_##PHYS##_start_ragdoll(AnimV20::AnimcharBaseComponent *ac, AnimV20::AnimcharFinalMat44 *fWtm, const Point3 &vel0) \
  {                                                                                                                            \
    return phys_start_ragdoll(ac, fWtm, vel0);                                                                                 \
  }                                                                                                                            \
  void phys_##PHYS##_delete_ragdoll(void *&ragdoll)                                                                            \
  {                                                                                                                            \
    if (!ragdoll)                                                                                                              \
      return;                                                                                                                  \
    auto rd = (PhysRagdoll *)ragdoll;                                                                                          \
    erase_item_by_value(physsimulator::simObjRes, rd->getPhysRes());                                                           \
    erase_item_by_value(simPhysSys, rd->getPhysSys());                                                                         \
    delete rd;                                                                                                                 \
    ragdoll = NULL;                                                                                                            \
  }
