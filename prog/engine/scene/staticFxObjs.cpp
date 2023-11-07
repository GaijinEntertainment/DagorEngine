#include <scene/dag_staticFxObjs.h>
#include <scene/dag_objsToPlace.h>
#include <fx/dag_baseFxClasses.h>

#include <ioSys/dag_roDataBlock.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>

#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <3d/dag_drv3d.h>
#include <scene/dag_occlusionMap.h>
#include <debug/dag_log.h>

static Tab<Effect> objects(midmem_ptr());

dag::ConstSpan<Effect> StaticFxObjects::get_effects_list() { return objects; }

void StaticFxObjects::on_bindump_unload(unsigned bindump_id)
{
  for (int i = objects.size() - 1; i >= 0; i--)
  {
    if (objects[i].bindumpIndex == bindump_id)
    {
      del_it(objects[i].fx);
      erase_items(objects, i, 1);
    }
  }
}

void StaticFxObjects::on_bindump_renderable_change(unsigned bindump_id, bool renderable)
{
  for (int i = objects.size() - 1; i >= 0; i--)
  {
    if (objects[i].bindumpIndex == bindump_id)
      objects[i].bindumpRenderable = renderable;
  }
}

void StaticFxObjects::clear()
{
  for (int i = 0; i < objects.size(); i++)
    del_it(objects[i].fx);
  objects.clear();
}

static __forceinline bool is_sphere_visible(BSphere3 &sp, const Frustum &frustum)
{
  vec3f center = v_ldu(&sp.c.x);
  vec3f rad = v_splat_w(center);
  if (!frustum.testSphereB(center, rad))
    return false;
  if (current_occlusion && current_occlusion->isOccludedSphere(center, rad))
    return false;
  return true;
}

void StaticFxObjects::render(int render_type, const TMatrix &view_itm)
{
  if (render_type == FX_RENDER_BEFORE)
  {
    mat44f gtm;
    d3d::getglobtm(gtm);
    Frustum frustum(gtm);
    for (int i = 0; i < objects.size(); i++)
    {
      Effect &e = objects[i];
      e.visible = e.bindumpRenderable && is_sphere_visible(e.sph, frustum);

      if (e.visible)
        e.fx->render(FX_RENDER_BEFORE, view_itm);
    }
  }
  else
  {
    for (int i = 0; i < objects.size(); i++)
    {
      Effect &e = objects[i];
      if (e.visible)
        e.fx->render(render_type, view_itm);
    }
  }
}

void StaticFxObjects::update(real dt)
{
  for (int i = 0; i < objects.size(); i++)
  {
    Effect &e = objects[i];
    if (e.visible || e.updateWhenInvisible)
      e.fx->update(dt);
  }
}


void StaticFxObjects::set_raytracer(IEffectRayTracer *rt)
{
  for (int i = 0; i < objects.size(); i++)
    objects[i].fx->setParam(HUID_RAYTRACER, rt);
}


void StaticFxObjects::on_device_reset()
{
  for (int i = 0; i < objects.size(); i++)
    objects[i].fx->onDeviceReset();
}

static void createOneStaticFxObject(const RoDataBlock *b, BaseEffectObject *base_fx, TMatrix &tm, const char *name,
  unsigned bindump_id, bool renderable)
{
  BaseEffectObject *fx = (BaseEffectObject *)base_fx->clone();

  if (!b->getBool("emmiterEditing", false))
    fx->setParam(HUID_TM, &tm);
  else
  {
    TMatrix fm, em;

    fm.setcol(0, Point3(length(tm.getcol(0)), 0, 0));
    fm.setcol(1, Point3(0, length(tm.getcol(1)), 0));
    fm.setcol(2, Point3(0, 0, length(tm.getcol(2))));
    fm.setcol(3, tm.getcol(3));
    fx->setParam(HUID_TM, &fm);

    em.setcol(0, normalize(tm.getcol(0)));
    em.setcol(1, normalize(tm.getcol(1)));
    em.setcol(2, normalize(tm.getcol(2)));
    em.setcol(3, Point3(0, 0, 0));

    fx->setParam(HUID_EMITTER_TM, &em);
  }

  Effect e;
  e.fx = fx;
#if DAGOR_DBGLEVEL > 0
  size_t len = min(strlen(name), size_t(e.NAME_LEN));
  memcpy(e.name, name, len);
  e.name[len] = '\0';
#endif
  BSphere3 sphere;
  fx->getParam(HUID_STATICVISSPHERE, &sphere);
  if (!sphere.isempty())
  {
    float maxScaleSq = max(lengthSq(tm.getcol(0)), max(lengthSq(tm.getcol(1)), lengthSq(tm.getcol(2))));
    float scale = sqrt(maxScaleSq);
    e.sph = BSphere3(sphere.c * tm, sphere.r * scale);

    if (is_equal_float(sphere.r, 10))
      logwarn("StaticFxObjects: default visSphere radius: %s", name);
  }
  else
    e.sph = BSphere3(tm.getcol(3), b->getReal("maxRadius", 10.0f));

  e.visible = true;
  e.updateWhenInvisible = b->getBool("updateWhenInvisible", false);
  e.bindumpIndex = bindump_id;
  e.bindumpRenderable = renderable;
  objects.push_back(e);
}

void StaticFxObjects::init(const RoDataBlock &blk, unsigned bindump_id /*= -1*/, bool renderable /*= true*/)
{
  if (!objects.capacity())
    objects.reserve(16 << 10);

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const RoDataBlock *b = blk.getBlock(i);
    const char *name = b->getStr("name", NULL);

    if (name && ::get_resource_type_id(name) == EffectGameResClassId)
    {
      GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
      BaseEffectObject *base_fx = (BaseEffectObject *)::get_game_resource(handle);
      if (base_fx)
      {
        TMatrix tm = b->getTm("matrix", TMatrix::IDENT);

        createOneStaticFxObject(b, base_fx, tm, name, bindump_id, renderable);
      }
      ::release_game_resource(handle);
    }
  }
}

void StaticFxObjects::init(const ObjectsToPlace &o, unsigned bindump_id /*= -1*/, bool renderable /*= true*/)
{
  if (!objects.capacity())
    objects.reserve(16 << 10);

  for (int j = 0; j < o.objs.size(); j++)
  {
    const char *name = o.objs[j].resName;
    GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
    BaseEffectObject *base_fx = (BaseEffectObject *)::get_game_resource(handle);
    if (!base_fx)
      continue;

    for (int k = 0; k < o.objs[j].tm.size(); k++)
    {
      TMatrix tm = o.objs[j].tm[k];
      const RoDataBlock &b = *o.objs[j].addData.getBlock(k);

      createOneStaticFxObject(&b, base_fx, tm, name, bindump_id, renderable);
    }
    ::release_game_resource(handle);
  }
}
