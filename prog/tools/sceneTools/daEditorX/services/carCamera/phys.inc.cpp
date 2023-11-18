#include <de3_lightService.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <EditorCore/ec_interface.h>

static PhysWorld *pw = NULL;
static bool inited = false;
static DynamicPhysObject *simObj = NULL;
static PhysBody *simBody = NULL;
static bool mustDelSimObj = false;

static Tab<PhysBody *> stBody(midmem);

static inline PhysBody *getPhysBody(int ind)
{
  if (simBody)
    return simBody;
  if (simObj && simObj->getPhysSys())
    return simObj->getPhysSys()->getBody(ind);
  return NULL;
}

static bool phys_get_phys_tm(int body_id, TMatrix &phys_tm, bool &obj_active)
{
  if (simBody)
  {
    simBody->getTm(phys_tm);
    obj_active = simBody->isActive();
    return true;
  }

  if (simObj && simObj->getPhysSys() && simObj->getPhysSys()->getBody(body_id))
  {
    simObj->getPhysSys()->getBody(body_id)->getTm(phys_tm);
    obj_active = simObj->getPhysSys()->getBody(body_id)->isActive();
    return true;
  }
  obj_active = false;
  return false;
}

static __forceinline void phys_simulate(real dt) { pw->simulate(dt); }
static __forceinline void phys_clear_phys_world()
{
  if (mustDelSimObj)
    del_it(simObj);
  mustDelSimObj = false;
  simObj = NULL;
  simBody = NULL;

  clear_all_ptr_items(stBody);
  del_it(pw);
}

static PhysBody *createStaticBody(const TMatrix &tm, real sz_x, real sz_y, real sz_z)
{
  PhysBodyCreationData pbcd;
  pbcd.autoMask = false, pbcd.group = pbcd.mask = 3;
  PhysBoxCollision coll(sz_x, sz_y, sz_z);
  return new PhysBody(pw, 0, &coll, tm, pbcd);
}

static __forceinline void phys_create_phys_world(DynamicPhysObjectData *base_obj)
{
  if (pw)
    return;

  pw = new PhysWorld(0.9f, 0.7f, 0.4f, 1.0f);

  return;

  TMatrix nullPlaneTm(TMatrix::IDENT);
  nullPlaneTm.setcol(3, 0, -2.75, 0);
  stBody.push_back(createStaticBody(nullPlaneTm, 500.f, 1.f, 500.f));
  nullPlaneTm.setcol(3, 0, -2.15f, 0);
  stBody.push_back(createStaticBody(nullPlaneTm, 8.f, 0.2, 8.f));

  if (!base_obj)
    return;

  // create DynamicPhysObject from initial data
  simObj = new (midmem) DynamicPhysObject;
  simObj->init(base_obj, pw, TMatrix::IDENT);

  mustDelSimObj = true;
}
static __forceinline void phys_set_phys_body(void *phbody) { simBody = (PhysBody *)phbody; }

static __forceinline void phys_init()
{
  if (inited)
    return;

  ::init_physics_engine();
  inited = true;
}

static __forceinline void phys_close()
{
  if (!inited)
    return;

  phys_clear_phys_world();

  ::close_physics_engine();
  inited = false;
}

static __forceinline void phys_before_render()
{
  if (simObj)
    simObj->beforeRender(::grs_cur_view.pos);
}
static __forceinline void phys_render_trans()
{
  if (simObj)
    simObj->renderTrans();
}
static __forceinline void phys_render()
{
  if (simObj)
    simObj->render();
}

static __forceinline void add_impulse(int body_ind, const Point3 &pos, const Point3 &delta, real spring_factor, real damper_factor,
  real dt) // for unit mass
{
  PhysBody *pb = getPhysBody(body_ind);
  if (!pb)
    return;

  real mass = pb->getMass();
  if (dt < 0)
  {
    pb->addImpulse(pos, delta * mass * -dt);
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
