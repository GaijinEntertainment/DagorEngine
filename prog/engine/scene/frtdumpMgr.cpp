#include <scene/dag_frtdumpMgr.h>
#include <scene/dag_frtdumpInline.h>
#include <sceneRay/dag_sceneRay.h>
#include <debug/dag_debug.h>

FastRtDumpManager::FastRtDumpManager(int max_dump_reserve) : rt(midmem_ptr()), rtId(midmem_ptr())
{
  generation = 0;
  rt.reserve(max_dump_reserve);
  rtId.reserve(max_dump_reserve);
}
FastRtDumpManager::~FastRtDumpManager()
{
  delAllRtDumps();
  clear_all_ptr_items(rt);
  clear_and_shrink(rtId);
}

int FastRtDumpManager::allocRtDump()
{
  int id = -1;

  for (int i = 0; i < rt.size(); i++)
    if (!rt[i]->isDataValid())
    {
      id = i;
      break;
    }

  if (id == -1)
  {
    rt.push_back(new FastRtDump());
    id = append_items(rtId, 1);
  }

  return id;
}

int FastRtDumpManager::loadRtDump(IGenLoad &crd, unsigned rt_id)
{
  int id = allocRtDump();

  rt[id]->loadData(crd);
  if (rt[id]->isDataValid())
  {
    generation++;
    rtId[id] = rt_id;
    return id;
  }

  return -1;
}
int FastRtDumpManager::addRtDump(StaticSceneRayTracer *_rt, unsigned rt_id)
{
  int id = allocRtDump();

  rt[id]->setData(_rt);
  if (rt[id]->isDataValid())
  {
    generation++;
    rtId[id] = rt_id;
    return id;
  }

  return -1;
}

void FastRtDumpManager::delRtDump(unsigned rt_id)
{
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rtId[i] == rt_id)
    {
      generation++;
      rtId[i] = -1;
      rt[i]->unloadData();

      // remove empty trailing slots
      for (i = rt.size() - 1; i >= 0; i--)
        if (!rt[i]->isDataValid())
        {
          erase_ptr_items(rt, i, 1);
          erase_items(rtId, i, 1);
        }
        else
          break;
      return;
    }
}

void FastRtDumpManager::delAllRtDumps()
{
  generation++;
  clear_all_ptr_items(rt);
  rtId.clear();
}


//
// raytracing and collision
//

// native version of StaticSceneRayTracer interface
bool FastRtDumpManager::rayhit(const Point3 &p, const Point3 &dir, real t)
{
  real dirlen = dir.length();
  t *= dirlen;
  return rayhitNormalized(p, dirlen != 0.0 ? (dir / dirlen) : dir, t);
}
bool FastRtDumpManager::rayhitNormalized(const Point3 &p, const Point3 &normDir, real t)
{
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->getRt().rayhitNormalized(p, normDir, t))
      return true;
  return false;
}
bool FastRtDumpManager::traceray(const Point3 &p, const Point3 &dir, real &t)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->getRt().traceray(p, dir, t) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->getRt().tracerayNormalized(p, normDir, t) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->getRt().clipCapsule(cap, cp1, cp2, md, movedirNormalized) != -1)
      ret = true;
  return ret;
}

// pmid-oriented version of StaticSceneRayTracer interface
bool FastRtDumpManager::traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->traceray(p, dir, t, out_pmid) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->tracerayNormalized(p, normDir, t, out_pmid) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->clipCapsule(cap, cp1, cp2, md, movedirNormalized, out_pmid) != -1)
      ret = true;
  return ret;
}
// pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
bool FastRtDumpManager::traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->traceray(p, dir, t, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->tracerayNormalized(p, normDir, t, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  int &out_pmid, Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->clipCapsule(cap, cp1, cp2, md, movedirNormalized, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}

// Custom (usage flags) pmid-oriented version of StaticSceneRayTracer interface
bool FastRtDumpManager::traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->traceray(custom, p, dir, t, out_pmid) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->tracerayNormalized(custom, p, normDir, t, out_pmid) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized, int &out_pmid)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->clipCapsule(custom, cap, cp1, cp2, md, movedirNormalized, out_pmid) != -1)
      ret = true;
  return ret;
}
// Custom (usage flags)  pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
bool FastRtDumpManager::traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->traceray(custom, p, dir, t, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid,
  Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->tracerayNormalized(custom, p, normDir, t, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}
bool FastRtDumpManager::clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized, int &out_pmid, Point3 &out_norm)
{
  bool ret = false;
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->clipCapsule(custom, cap, cp1, cp2, md, movedirNormalized, out_pmid, out_norm) != -1)
      ret = true;
  return ret;
}

bool FastRtDumpManager::rayhit(int custom, const Point3 &p, const Point3 &dir, real t)
{
  real dirlen = dir.length();
  t *= dirlen;
  return rayhitNormalized(custom, p, dirlen != 0.0 ? (dir / dirlen) : dir, t);
}
bool FastRtDumpManager::rayhitNormalized(int custom, const Point3 &p, const Point3 &normDir, real t)
{
  for (int i = 0; i < rt.size(); i++)
    if (rt[i]->isDataValid() && rt[i]->rayhitNormalized(custom, p, normDir, t))
      return true;
  return false;
}
