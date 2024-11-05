// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"

#include <oldEditor/de_clipping.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <de3_entityFilter.h>

#include <sceneRay/dag_sceneRay.h>

#include <scene/dag_physMat.h>
#include <scene/dag_frtdump.h>
#include <scene/dag_frtdumpInline.h>

#include <math/dag_mesh.h>
#include <ioSys/dag_dataBlock.h>

#include <math/dag_capsule.h>
#include <math/dag_math3d.h>

#include <libTools/dagFileRW/dagFileNode.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>


bool(__stdcall *external_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm) = NULL;
static FastRtDump *rtdump = NULL;


static Tab<IDagorEdCustomCollider *> colliders(midmem_ptr());
static Tab<IDagorEdCustomCollider *> activeColliders(midmem_ptr());
static Tab<IDagorEdCustomCollider *> activeShadows(midmem_ptr());


bool DagorPhys::use_only_visible_colliders = false;


void register_custom_collider(IDagorEdCustomCollider *coll)
{
  if (!coll)
    return;

  for (int i = 0; i < colliders.size(); ++i)
    if (colliders[i] == coll)
      return;

  ::colliders.push_back(coll);
  ::activeColliders.push_back(coll);
  ::activeShadows.push_back(coll);
  DAEDITOR3.conNote("add collider <%s>, %d total", coll->getColliderName(), colliders.size());
}


void unregister_custom_collider(IDagorEdCustomCollider *coll)
{
  if (!coll)
    return;
  int i;

  for (i = 0; i < colliders.size(); ++i)
    if (colliders[i] == coll)
    {
      erase_items(colliders, i, 1);
      break;
    }

  for (i = 0; i < ::activeColliders.size(); ++i)
    if (::activeColliders[i] == coll)
    {
      erase_items(::activeColliders, i, 1);
      break;
    }

  for (i = 0; i < ::activeShadows.size(); ++i)
    if (::activeShadows[i] == coll)
    {
      erase_items(::activeShadows, i, 1);
      break;
    }
}


int get_custom_colliders_count() { return colliders.size(); }


IDagorEdCustomCollider *get_custom_collider(int idx)
{
  if (idx >= 0 && idx < ::colliders.size())
    return ::colliders[idx];

  return NULL;
}


void enable_custom_collider(const char *name)
{
  if (!name)
    return;

  for (int i = 0; i < ::colliders.size(); ++i)
    if (!::strcmp(colliders[i]->getColliderName(), name))
    {
      for (int j = 0; j < ::activeColliders.size(); ++j)
        if (::activeColliders[j] == colliders[i])
          return;

      ::activeColliders.push_back(colliders[i]);
      return;
    }

  IGenEditorPlugin *p = DAGORED2->getPluginByMenuName(name);
  if (!p)
  {
    DAEDITOR3.conError("cannot enable collider <%s>", name);
    return;
  }

  IObjEntityFilter *filter = p->queryInterface<IObjEntityFilter>();
  if (!filter)
  {
    DAEDITOR3.conError("collider <%s> is not filter", name);
    return;
  }
  if (!filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
  {
    DAEDITOR3.conError("filter <%s> doesnt support collision filtering", name);
    return;
  }

  filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION);
}


void disable_custom_collider(const char *name)
{
  if (!name)
    return;

  for (int i = 0; i < ::activeColliders.size(); ++i)
    if (!::strcmp(::activeColliders[i]->getColliderName(), name))
    {
      erase_items(::activeColliders, i, 1);
      return;
    }

  IGenEditorPlugin *p = DAGORED2->getPluginByMenuName(name);
  if (!p)
  {
    DAEDITOR3.conError("cannot disable collider <%s>", name);
    return;
  }

  IObjEntityFilter *filter = p->queryInterface<IObjEntityFilter>();
  if (!filter)
  {
    DAEDITOR3.conError("collider <%s> is not filter", name);
    return;
  }
  if (!filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
  {
    DAEDITOR3.conError("filter <%s> doesnt support collision filtering", name);
    return;
  }

  filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION, false);
}


void set_enabled_colliders(const Tab<String> &names)
{
  activeColliders.clear();

  for (int ni = 0; ni < names.size(); ++ni)
  {
    bool ignoreName = false;

    for (int ai = 0; ai < activeColliders.size(); ++ai)
    {
      if (!stricmp(activeColliders[ai]->getColliderName(), names[ni]))
      {
        ignoreName = true;
        break;
      }
    }

    if (ignoreName)
      continue;

    ignoreName = true;

    for (int ci = 0; ci < colliders.size(); ++ci)
    {
      if (!stricmp(names[ni], colliders[ci]->getColliderName()))
      {
        activeColliders.push_back(colliders[ci]);
        break;
      }
    }
  }
}


void set_disabled_colliders(const Tab<String> &names)
{
  activeColliders = colliders;

  for (int ni = 0; ni < names.size(); ++ni)
  {
    for (int ai = 0; ai < activeColliders.size(); ++ai)
    {
      if (!stricmp(activeColliders[ai]->getColliderName(), names[ni]))
      {
        erase_items(activeColliders, ai, 1);
        break;
      }
    }
  }
}


void get_enabled_colliders(Tab<String> &names)
{
  names.clear();

  for (int i = 0; i < activeColliders.size(); ++i)
    names.push_back() = activeColliders[i]->getColliderName();
}


void get_disabled_colliders(Tab<String> &names)
{
  names.clear();

  for (int i = 0; i < colliders.size(); ++i)
  {
    if (!::is_custom_collider_enabled(colliders[i]))
      names.push_back() = colliders[i]->getColliderName();
  }
}


bool is_custom_collider_enabled(const IDagorEdCustomCollider *collider)
{
  for (int i = 0; i < ::activeColliders.size(); ++i)
    if (::activeColliders[i] == collider)
      return true;

  return false;
}


static bool editorCollUsed = true;
static Tab<IDagorEdCustomCollider *> editorColliders(tmpmem);
static unsigned editorFilterMask = 0;
void set_custom_colliders(dag::ConstSpan<IDagorEdCustomCollider *> colliders, unsigned filter_mask)
{
  if (editorCollUsed)
  {
    editorColliders = activeColliders;
    editorFilterMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    editorCollUsed = false;
  }
  activeColliders = colliders;
  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, filter_mask);
}
void restore_editor_colliders()
{
  if (editorCollUsed)
    return;

  activeColliders = editorColliders;
  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, editorFilterMask);
  editorCollUsed = true;
}

dag::ConstSpan<IDagorEdCustomCollider *> get_current_colliders(unsigned &filter_mask)
{
  filter_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  return activeColliders;
}

void enable_custom_shadow(const char *name)
{
  if (!name)
    return;

  for (int i = 0; i < ::colliders.size(); ++i)
    if (!::strcmp(colliders[i]->getColliderName(), name))
    {
      for (int j = 0; j < ::activeShadows.size(); ++j)
        if (::activeShadows[j] == ::colliders[i])
          return;

      ::activeShadows.push_back(::colliders[i]);
    }
}


void disable_custom_shadow(const char *name)
{
  if (!name)
    return;

  for (int i = 0; i < ::activeShadows.size(); ++i)
    if (!::strcmp(activeShadows[i]->getColliderName(), name))
    {
      erase_items(::activeShadows, i, 1);
      break;
    }
}


bool is_custom_shadow_enabled(const IDagorEdCustomCollider *collider)
{
  for (int i = 0; i < ::activeShadows.size(); ++i)
    if (::activeShadows[i] == collider)
      return true;

  return false;
}


static bool customTraceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm, bool use_only_visible)
{
  bool hit = false;

  for (int i = 0; i < ::activeColliders.size(); ++i)
  {
    if (!use_only_visible || (use_only_visible && ::activeColliders[i]->isColliderVisible()))
    {
      if (::activeColliders[i]->traceRay(p, dir, maxt, norm))
        hit = true;
    }
  }

  return hit;
}

static void customClipCapsule(Capsule &c, Point3 &lpt, Point3 &wpt, real &md, const Point3 &norm)
{
  for (int i = 0; i < ::activeColliders.size(); ++i)
  {
    activeColliders[i]->clipCapsule(c, lpt, wpt, md, norm);
  }
}


bool DagorEdAppWindow::shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt)
{
  for (int i = 0; i < ::activeShadows.size(); ++i)
    if (::activeShadows[i]->shadowRayHitTest(p, dir, maxt))
    {
      return true;
    }

  return false;
}


//==============================================================================
StaticSceneRayTracer *DagorPhys::load_binary_raytracer(IGenLoad &crd)
{
  if (!rtdump)
    rtdump = new (midmem) FastRtDump(crd);
  else
    rtdump->loadData(crd);

  if (!rtdump->isDataValid())
    del_it(rtdump);

  if (rtdump)
    return &rtdump->getRt();

  return nullptr;
}


//==============================================================================
void DagorPhys::setup_collider_params(int mode, const BBox3 &area)
{
  for (int i = 0; i < ::activeColliders.size(); ++i)
  {
    activeColliders[i]->setupColliderParams(mode, area);
  }
}

//==============================================================================
bool DagorPhys::trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt)
{
  return ::customTraceRay(p, dir, maxt, NULL, use_only_visible_colliders);
}


//==============================================================================
bool DagorPhys::trace_ray_static(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm)
{
  return ::customTraceRay(p, dir, maxt, &norm, use_only_visible_colliders);
}


//==============================================================================
bool DagorPhys::ray_hit_static(const Point3 &p, const Point3 &dir, real maxt)
{
  if (::customTraceRay(p, dir, maxt, NULL, use_only_visible_colliders))
    return true;

  return false;
}

//==============================================================================
real DagorPhys::clip_capsule_static(Capsule &c, Point3 &cap_pt, Point3 &world_pt)
{
  real md = MAX_REAL;
  ::customClipCapsule(c, cap_pt, world_pt, md, Point3(0, 0, 0));

  if (!rtdump)
    return md;

  int phmatid;
  const int idx = rtdump->clipCapsule(c, cap_pt, world_pt, md, Point3(0, 0, 0), phmatid);

  return md;
}


//==============================================================================
real DagorPhys::clip_capsule_static(Capsule &c, Point3 &cap_pt, Point3 &world_pt, Point3 &norm)
{
  if (!rtdump)
    return MAX_REAL;

  int phmatid;
  real md = MAX_REAL;
  const int idx = rtdump->clipCapsule(c, cap_pt, world_pt, md, Point3(0, 0, 0), phmatid, norm);

  return md;
}


//==============================================================================
BBox3 DagorPhys::get_bounding_box()
{
  if (!rtdump)
    return BBox3();

  return rtdump->getRt().getBox();
}


//==============================================================================
void init_tps_physmat(const char *physmatFileName) { PhysMat::init(physmatFileName); }


//==============================================================================
void close_tps_physmat() { PhysMat::release(); }


//==============================================================================
void DagorPhys::close_clipping() { del_it(rtdump); }


//==============================================================================
void DagorPhys::init_clipping_binary(StaticSceneRayTracer *rt)
{
  del_it(rtdump);

  if (!rt)
    return;


  rtdump = new (midmem) FastRtDump(rt);
  DEBUG_CTX("binary rtdump: %p", rt);
}


//==============================================================================
FastRtDump *DagorPhys::getFastRtDump() { return rtdump; }


bool fill_custom_colliders_list(PropPanel::ContainerPropertyControl &panel, const char *grp_caption, int grp_pid, int collider_pid,
  bool shadow, bool open_grp)
{
  PropPanel::ContainerPropertyControl *grp = panel.createGroup(grp_pid, grp_caption);

  if (!grp)
    return false;

  const int colCnt = ::get_custom_colliders_count();

  for (int i = 0; i < colCnt; ++i)
  {
    const IDagorEdCustomCollider *collider = ::get_custom_collider(i);

    if (collider)
    {
      bool enabled = (shadow) ? ::is_custom_shadow_enabled(collider) : ::is_custom_collider_enabled(collider);

      grp->createCheckBox(collider_pid + i, collider->getColliderName(), enabled ? 1 : 0);
    }
  }

  panel.setBool(grp_pid, !open_grp);

  return true;
}


bool on_pp_collider_check(int pid, const PropPanel::ContainerPropertyControl &panel, int collider_pid, bool shadow)
{
  if (pid < collider_pid)
    return false;

  if (pid >= collider_pid + colliders.size())
    return false;

  Tab<IDagorEdCustomCollider *> &active = shadow ? activeShadows : activeColliders;

  IDagorEdCustomCollider *collider = colliders[pid - collider_pid];
  int colliderActive = 0;
  int colliderIdx = -1;
  int i;

  for (i = 0; i < active.size(); ++i)
  {
    if (active[i] == collider)
    {
      colliderActive = 1;
      colliderIdx = i;
      break;
    }
  }

  int enable = panel.getBool(pid);

  if (enable != colliderActive)
  {
    if (enable)
      active.push_back(collider);
    else
      erase_items(active, colliderIdx, 1);
  }

  return true;
}


const char *DagorPhys::get_collision_name() { return DAGORED2->getWorkspace().getCollisionName(); }

void reset_colliders_data()
{
  colliders.clear();
  activeColliders.clear();
  activeShadows.clear();
  DagorPhys::use_only_visible_colliders = false;

  editorCollUsed = true;
  editorColliders.clear();
  editorFilterMask = 0;

  del_it(rtdump);
}
