// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "prefabs.h"
#include "pf_cm.h"

#include "../av_util.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>

#include <EditorCore/ec_interface.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_occludersFromGeom.h>

#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeomUi/nodeFlags.h>

#include <libTools/dtx/filename.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/meshUtil.h>
#include <coolConsole/coolConsole.h>

#include <shaders/dag_shaders.h>

#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>

#define CENTER_LINES_LEN 40.0


static int rendEntGeomMask = -1;
static int collisionMask = -1;


//==============================================================================
PrefabsPlugin::PrefabsPlugin() : geom(NULL), showDag(false), changedNodes(midmem), nodeModif(NULL) {}


//==============================================================================
PrefabsPlugin::~PrefabsPlugin() {}


//==============================================================================
void PrefabsPlugin::registered()
{
  ::init_geom_object_lighting();

  geom = new (midmem) GeomObject;
  G_ASSERT(geom);

  geom->setTm(TMatrix::IDENT);
  geom->setUseNodeVisRange(true);

  nodeModif = new (midmem) NodeFlagsModfier(*this, "ObjLib");

  rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
  collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
}


//==============================================================================
void PrefabsPlugin::unregistered()
{
  // destruction
  del_it(geom);
  del_it(nodeModif);
}


//==============================================================================
bool PrefabsPlugin::begin(DagorAsset *asset)
{
  curAssetDagFname = dagFilesList.getName(dagFilesList.addNameId(asset->getTargetFilePath()));

  openDag(curAssetDagFname);
  return true;
}


//==============================================================================
bool PrefabsPlugin::end() { return true; }


//==============================================================================
void PrefabsPlugin::renderOccluders()
{
  const StaticGeometryContainer *geomContainer = geom->getGeometryContainer();
  if (geomContainer)
  {
    NodeParamsTab nodeFlags;
    bool alreadyInMap = false;

    if (!getNodeFlags(nodeFlags, alreadyInMap))
      return;


    for (int i = 0; i < geomContainer->nodes.size(); ++i)
    {
      if (nodeFlags.size() <= i)
        break;

      if (!nodeFlags[i].showOccluders)
        continue;

      const StaticGeometryNode *node = geomContainer->nodes[i];

      if (!node)
        continue;

      TMatrix box;
      Point3 quad[4];
      GetOccluderFromGeomResultType result = getOcclFromGeomNode(*node, curAssetDagFname, box, quad);
      if (result == GetOccluderFromGeomResultType::Box)
      {
        begin_draw_cached_debug_lines(false, false);
        set_cached_debug_lines_wtm(box);
        draw_cached_debug_box(BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)), 0x80808080);
        end_draw_cached_debug_lines();
      }
      else if (result == GetOccluderFromGeomResultType::Quad)
      {
        begin_draw_cached_debug_lines(false, false);
        draw_cached_debug_quad(quad, 0x80808080);
        end_draw_cached_debug_lines();
      }
    }
  }
}

void PrefabsPlugin::renderTransObjects()
{
  if (showDag)
  {
    geom->setTm(TMatrix::IDENT);

    unsigned mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

    if ((mask & collisionMask) == collisionMask)
      geom->renderEdges(false);
  }

  renderOccluders();
}

void PrefabsPlugin::renderGeometry(Stage stage)
{
  if (!showDag || !getVisible())
    return;

  unsigned mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  geom->setTm(TMatrix::IDENT);
  switch (stage)
  {
    case STG_RENDER_STATIC_OPAQUE:
    case STG_RENDER_SHADOWS:
      if ((mask & rendEntGeomMask) == rendEntGeomMask)
        geom->render();
      break;

    case STG_RENDER_STATIC_TRANS:
      if ((mask & rendEntGeomMask) == rendEntGeomMask)
        geom->renderTrans();
      break;

    default: break;
  }
}

//==============================================================================
bool PrefabsPlugin::supportAssetType(const DagorAsset &asset) const
{
  return strcmp(asset.getMgr().getAssetTypeName(asset.getType()), "prefab") == 0;
}


//==============================================================================
/*
void PrefabsPlugin::drawCenter(IGenViewportWnd *wnd, CtlDC &dc)
{
  if (showDag)
  {
    const Point3 pos = Point3(0,0,0);
    Point2 center, ax, ay, az, temp;

    if (wnd->worldToClient(pos, center))
    {
      wnd->worldToClient(pos + Point3(0.01, 0, 0), temp);
      ax = temp - center;
      wnd->worldToClient(pos + Point3(0, 0.01, 0), temp);
      ay = temp - center;
      wnd->worldToClient(pos + Point3(0, 0, 0.01), temp);
      az = temp - center;

      real xl = ::length(ax);
      real yl = ::length(ay);
      real zl = ::length(az);

      const real multiply = CENTER_LINES_LEN / __max(__max(xl, yl), zl);

      if (xl < 1e-5)
        xl = 1e-5;
      if (yl < 1e-5)
        yl = 1e-5;
      if (zl < 1e-5)
        zl = 1e-5;

      ax *= multiply;
      ay *= multiply;
      az *= multiply;

      dc.selectColor(PC_LtRed);
      dc.line(center.x, center.y, center.x + ax.x, center.y + ax.y);

      dc.selectColor(PC_LtGreen);
      dc.line(center.x, center.y, center.x + ay.x, center.y + ay.y);

      dc.selectColor(PC_LtBlue);
      dc.line(center.x, center.y, center.x + az.x, center.y + az.y);
    }
  }
}
*/

//==============================================================================
void PrefabsPlugin::clearObjects()
{
  changedNodes.eraseall();

  showDag = false;
  dagName = "";

  repaintView();
}


//==============================================================================
void PrefabsPlugin::onSaveLibrary() { saveChangedDags(); }


//==============================================================================
void PrefabsPlugin::onLoadLibrary() { changedNodes.eraseall(); }


//==============================================================================
bool PrefabsPlugin::getSelectionBox(BBox3 &box) const
{
  if (showDag && geom)
  {
    box = geom->getBoundBox();
    return true;
  }

  return false;
}


//==============================================================================

void PrefabsPlugin::openDag(const char *fname)
{
  if (stricmp(dagName, fname) != 0)
  {
    if (geom->loadFromDag(fname, &getMainConsole()))
    {
      showDag = true;

      StaticGeometryContainer &cont = *geom->getGeometryContainer();
      CoolConsole &con = getMainConsole();
      bool errMeshes = false;

      for (int ni = 0; ni < cont.nodes.size(); ++ni)
      {
        const StaticGeometryNode *node = cont.nodes[ni];

        if (node && node->mesh && !::validate_mesh(node->name, node->mesh->mesh, con))
          errMeshes = true;
      }

      if (errMeshes)
      {
        wingw::message_box(wingw::MBS_EXCL, "Invalid mesh", "Object node(s) containes invalid mesh. See console for details.");

        con.showConsole();

        showDag = false;
      }
      else
      {
        NodeParamsTab nodeFlags;
        bool alreadyInMap = false;

        if (getNodeFlags(nodeFlags, alreadyInMap))
        {
          setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);

          Bitarray nf(tmpmem);
          nf.resize(nodeFlags.size());

          for (int i = 0; i < nodeFlags.size(); ++i)
            nf.set(i, nodeFlags[i].flags & StaticGeometryNode::FLG_RENDERABLE);

          geom->recompile(&nf);
        }
      }
    }
    else
    {
      wingw::message_box(wingw::MBS_EXCL, "Unable to open DAG", "Unable to open DAG\n\"%s\"", fname);

      showDag = false;
    }

    dagName = fname;
  }
  else
    showDag = true;

  geom->setTm(TMatrix::IDENT);
}


//==============================================================================
void PrefabsPlugin::centerSOObject()
{
  BBox3 bound;

  geom->setTm(TMatrix::IDENT);

  repaintView();
}


//==============================================================================
void PrefabsPlugin::autoAjustSOObject()
{
  BBox3 bound;

  geom->setTm(TMatrix::IDENT);

  repaintView();
}


//==============================================================================
void PrefabsPlugin::resetSOObjectPivot()
{
  geom->setTm(TMatrix::IDENT);

  repaintView();
}


//==============================================================================
bool PrefabsPlugin::getNodeFlags(NodeParamsTab &tab, bool &already_in_map)
{
  already_in_map = changedNodes.get(curAssetDagFname, tab);

  if (already_in_map)
    return true;
  else
  {
    const StaticGeometryContainer *cont = geom->getGeometryContainer();

    if (cont)
    {
      tab.resize(cont->nodes.size());

      for (int ni = 0; ni < cont->nodes.size(); ++ni)
      {
        const StaticGeometryNode *node = cont->nodes[ni];
        tab[ni].flags = node->flags;
        tab[ni].visRange = node->visRange;
        tab[ni].light = node->lighting;
        tab[ni].ltMul = node->ltMul;
        tab[ni].vltMul = node->vltMul;
        tab[ni].linkedRes = linkedNames.addNameId(node->linkedResource);
        tab[ni].lodRange = node->lodRange;
        tab[ni].lodName = linkedNames.addNameId(node->topLodName);
        tab[ni].showOccluders = true; // Not reading this from file, default to true
      }

      return (bool)tab.size();
    }
  }

  return false;
}


//==================================================================================================
void PrefabsPlugin::setNodeFlags(const char *entry, const NodeParamsTab &tab, bool in_map)
{
  const StaticGeometryContainer *cont = geom->getGeometryContainer();

  for (int ni = 0; ni < cont->nodes.size(); ++ni)
  {
    cont->nodes[ni]->flags = tab[ni].flags;
    cont->nodes[ni]->visRange = tab[ni].visRange;
    cont->nodes[ni]->lighting = tab[ni].light;
    cont->nodes[ni]->ltMul = tab[ni].ltMul;
    cont->nodes[ni]->vltMul = tab[ni].vltMul;
    cont->nodes[ni]->linkedResource = linkedNames.getName(tab[ni].linkedRes);
    cont->nodes[ni]->lodRange = tab[ni].lodRange;
    cont->nodes[ni]->topLodName = linkedNames.getName(tab[ni].lodName);
  }

  in_map ? changedNodes.set(entry, tab) : changedNodes.add(entry, tab);
}


//==================================================================================================
void PrefabsPlugin::saveChangedDags()
{
  if (!changedNodes.size())
  {
    changedNodes.eraseall();
    linkedNames.reset();
    return;
  }

  const char *fname = NULL;
  NodeParamsTab nodeFlags;
  StaticGeometryContainer cont;

  CoolConsole &con = getMainConsole();

  con.setActionDesc("Saving modified prefabs...");
  con.setTotal(changedNodes.size());

  for (bool ok = changedNodes.getFirst(fname, nodeFlags); ok; ok = changedNodes.getNext(fname, nodeFlags))
  {
    // const String fname(::lib_file(entry->dagFile));
    if (cont.loadFromDag(fname, &con))
    {
      debug("saving file \"%s\"", fname);

      int ni;
      for (ni = 0; ni < cont.nodes.size(); ++ni)
      {
        cont.nodes[ni]->flags = nodeFlags[ni].flags;
        cont.nodes[ni]->visRange = nodeFlags[ni].visRange;
        cont.nodes[ni]->lighting = nodeFlags[ni].light;
        cont.nodes[ni]->ltMul = nodeFlags[ni].ltMul;
        cont.nodes[ni]->vltMul = nodeFlags[ni].vltMul;
        cont.nodes[ni]->linkedResource = linkedNames.getName(nodeFlags[ni].linkedRes);
        cont.nodes[ni]->lodRange = nodeFlags[ni].lodRange;
        cont.nodes[ni]->topLodName = linkedNames.getName(nodeFlags[ni].lodName);
      }

      cont.exportDag(fname);

      for (ni = 0; ni < cont.nodes.size(); ++ni)
        del_it(cont.nodes[ni]);

      cont.nodes.clear();
    }

    con.incDone();
  }

  changedNodes.eraseall();
  linkedNames.reset();
}


// IGenEventHandler
//==============================================================================

void PrefabsPlugin::onNodeFlagsChanged(int node_idx, int or_flags, int and_flags)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].flags |= or_flags;
  nodeFlags[node_idx].flags &= and_flags;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);

  Bitarray nf(tmpmem);
  nf.resize(nodeFlags.size());

  for (int i = 0; i < nodeFlags.size(); ++i)
    nf.set(i, nodeFlags[i].flags & StaticGeometryNode::FLG_RENDERABLE);

  geom->recompile(&nf);
  repaintView();
}


//==============================================================================
void PrefabsPlugin::onVisRangeChanged(int node_idx, real vis_range)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].visRange = vis_range;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);
  repaintView();
}


//==============================================================================
void PrefabsPlugin::onLightingChanged(int node_idx, StaticGeometryNode::Lighting light)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].light = light;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);

  geom->recompile();
  repaintView();
}


//==============================================================================
void PrefabsPlugin::onLtMulChanged(int node_idx, real lt_mul)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].ltMul = lt_mul;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);
}


//==============================================================================
void PrefabsPlugin::onVltMulChanged(int node_idx, int vlt_mul)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].vltMul = vlt_mul;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);
}


//==============================================================================
void PrefabsPlugin::onLinkedResChanged(int node_idx, const char *res_name)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].linkedRes = -1;

  if (res_name)
    nodeFlags[node_idx].linkedRes = linkedNames.addNameId(res_name);

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);
}


//==============================================================================
void PrefabsPlugin::onTopLodChanged(int node_idx, const char *top_lod_name)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].lodName = -1;

  if (top_lod_name)
    nodeFlags[node_idx].lodName = linkedNames.addNameId(top_lod_name);

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);

  geom->recompile();
  repaintView();
}


//==============================================================================
void PrefabsPlugin::onLodRangeChanged(int node_idx, int lod_range)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].lodRange = lod_range;

  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);

  geom->recompile();
  repaintView();
}

void PrefabsPlugin::onShowOccludersChanged(int node_idx, bool show_occluders)
{
  NodeParamsTab nodeFlags;
  bool alreadyInMap = false;

  if (!getNodeFlags(nodeFlags, alreadyInMap))
    return;

  if (node_idx >= nodeFlags.size())
    return;

  nodeFlags[node_idx].showOccluders = show_occluders;
  setNodeFlags(curAssetDagFname, nodeFlags, alreadyInMap);
}
