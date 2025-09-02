// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlock.h>
#include <coolConsole/coolConsole.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>


#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>

#include <debug/dag_debug.h>
#include <memory/dag_mem.h>
#include <3d/dag_materialData.h>
#include <3d/dag_texMgr.h>
#include <math/dag_mesh.h>
#include <libTools/util/strUtil.h>

#include <libTools/dagFileRW/dagExporter.h>
#include <perfMon/dag_cpuFreq.h>

#include <debug/dag_debug.h>
#include <de3_interface.h>
#include <vector>

#include "plugin.h"
#include "box_csg.h"

#include <EditorCore/ec_IEditorCore.h>
#include "csg.h"
#include "de3_box_vs_tri.h"

using editorcore_extapi::dagGeom;


CSGPlugin *CSGPlugin::self = NULL;

CSGPlugin::CSGPlugin() { self = this; }


CSGPlugin::~CSGPlugin() { self = NULL; }


//==============================================================================

bool CSGPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  objEd.initUi(toolbar_id);
  return true;
}

bool CSGPlugin::end()
{
  objEd.closeUi();
  return true;
}

void CSGPlugin::actObjects(float dt)
{
  if (!isVisible)
    return;

  if (DAGORED2->curPlugin() == this)
    objEd.update(dt);
}


void CSGPlugin::beforeRenderObjects(IGenViewportWnd *vp)
{
  if (!isVisible)
    return;
  objEd.beforeRender();
}


void CSGPlugin::renderObjects()
{
  if (isVisible)
  {
    objEd.objRender();
  }
}

void CSGPlugin::renderTransObjects()
{
  if (isVisible)
    objEd.objRenderTr();
}


struct CSGNode
{
  StaticGeometryNode *node;
  void *poly;
  int id;
  BBox3 box;
  CSGNode(StaticGeometryNode *n, void *p, int i, BBox3 &b) : node(n), poly(p), id(i), box(b) {}
};

static bool cancelCSG = false;

// could be even parallalized
static void *recursive_union(BaseCSG *csg, CSGNode *at, int cnt)
{
  if (!cnt)
    return 0;
  if (cnt == 1)
    return at->poly;
  int lcnt = cnt / 2;
  bool del_left, del_right;
  void *left = recursive_union(csg, at, lcnt);
  void *right = recursive_union(csg, at + lcnt, cnt - lcnt);
  void *opres = NULL;
  DAGOR_TRY
  {
    if (left && right && !cancelCSG)
      opres = csg->op(left, right);
  }
  DAGOR_CATCH(...)
  {
    cancelCSG = true;
    DAEDITOR3.conWarning("Error: An unknown error has occurred. Skip CSG.");
  }

  csg->delete_poly(left);  // no memory clearing is faster
  csg->delete_poly(right); // no memory clearing is faster
  return opres;
}


void CSGPlugin::processGeometry(StaticGeometryContainer &cont)
{
  BaseCSG *csg = make_new_csg();
  Tab<CSGNode> polys(tmpmem);
  polys.reserve(cont.nodes.size());
  cancelCSG = false;

  DAEDITOR3.conNote("Start CSG");

  BBox3 bbox;
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    void *poly = NULL;
    if (node.mesh->getRefCount() > 1)
    {
      // clone mesh when it is used by more than one node
      if (node.mesh->user_data != (void *)1) // 1 is invalid mesh, 0 - hasn't tried yet, 2 - valid
      {
        DAGOR_TRY { poly = csg->create_poly(&node.wtm, node.mesh->mesh, i, false, bbox); }
        DAGOR_CATCH(...)
        {
          poly = NULL;
          DAEDITOR3.conWarning("Error: An unknown error has occurred. Skip node:%s.", (const char *)node.name);
        }
        if (poly)
        {
          node.mesh->user_data = (void *)2; // valid mesh
          StaticGeometryMesh *m = new StaticGeometryMesh;
          m->mats = node.mesh->mats;
          m->mesh = node.mesh->mesh;
          node.mesh = m;
        }
        else
        {
          node.mesh->user_data = (void *)1; // invalid
        }
      }
    }
    else
    {
      poly = csg->create_poly(&node.wtm, node.mesh->mesh, i, false, bbox);
    }
    if (poly)
      polys.push_back(CSGNode(&node, poly, i, bbox));
  }

  // void* result = recursive_union(csg, polys.data(), polys.size());
  for (int i = 0; i < polys.size(); ++i)
  {
    if (cancelCSG)
      break;

    for (int j = 0; j < polys.size(); ++j)
    {
      if (csg->hasOpenManifolds(polys[i].poly))
        continue;

      if (i != j && (polys[i].box & polys[j].box))
      {
        DAGOR_TRY
        {
          void *prevPoly = polys[i].poly;
          void *poly = csg->op(polys[i].poly, polys[j].poly);
          if (poly && !csg->hasOpenManifolds(poly))
          {
            polys[i].poly = poly;
            csg->delete_poly(prevPoly);
          }
          else
          {
            csg->delete_poly(poly);
          }
        }
        DAGOR_CATCH(...)
        {
          polys[i].poly = NULL;
          DAEDITOR3.conWarning("CSG Error: %s <-> %s", (const char *)polys[i].node->name, (const char *)polys[j].node->name);
          cancelCSG = true;
          break;
        }
      }
    }
  }


  if (!cancelCSG)
  {
    for (int i = 0; i < polys.size(); ++i)
      if (polys[i].poly)
        csg->removeUnusedFaces(polys[i].node->mesh->mesh, polys[i].poly, polys[i].id);
  }


  for (int b = 0; b < objEd.objectCount(); b++)
  {
    BoxCSG *p = RTTI_cast<BoxCSG>(objEd.getObject(b));
    if (p)
    {
      TMatrix boxWtm = p->getWtm();
      Plane3 box[6];
      generateBox(boxWtm, box, bbox);

      for (int i = 0; i < polys.size(); ++i)
      {
        if (!(bbox & polys[i].box))
          continue;

        TMatrix tm = polys[i].node->wtm;
        MeshData &mesh = polys[i].node->mesh->mesh;

        std::vector<bool> inside;
        inside.resize(mesh.vert.size());
        for (int i = 0; i < mesh.vert.size(); ++i)
        {
          Point3 mv;
          mv = tm * mesh.vert[i];
          inside[i] = isInside(box, mv);
        }

        for (int i = mesh.face.size() - 1; i >= 0; --i)
        {
          if (inside[mesh.face[i].v[0]] && inside[mesh.face[i].v[1]] && inside[mesh.face[i].v[2]])
          {
            mesh.removeFacesFast(i, 1);
          }
        }
      }
    }
  }

  // csg->delete_poly(result);

  for (int i = cont.nodes.size() - 1; i >= 0; --i)
  {
    cont.nodes[i]->mesh->user_data = NULL;
    Mesh *mesh = &cont.nodes[i]->mesh->mesh;
    if (cont.nodes[i]->mesh->mats[0]->className == "csg")
    {
      mesh->removeFacesFast(0, mesh->face.size());
    }
    if (mesh->face.size() == 0)
    {
      dagGeom->deleteStaticGeometryNode(cont.nodes[i]);
      erase_items(cont.nodes, i, 1);
    }
  }
  delete_csg(csg);

  DAEDITOR3.conNote("End CSG");
}


//==============================================================================

void *CSGPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IPostProcessGeometry);
  RETURN_INTERFACE(huid, IGatherStaticGeometry);
  return NULL;
}

void CSGPlugin::handleViewportAcceleratorCommand(unsigned id) { objEd.onClick(id, nullptr); }

void CSGPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();
  objEd.registerViewportAccelerators(wndManager);
}

void CSGPlugin::setVisible(bool vis)
{
  isVisible = vis;
  dagGeom->shaderGlobalSetInt(dagGeom->getShaderVariableId("showCSG"), isVisible);
}

bool CSGPlugin::getVisible() const { return isVisible; }

void CSGPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  objEd.load(blk);
  objEd.updateToolbarButtons();
}


void CSGPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) { objEd.save(blk); }


void CSGPlugin::clearObjects()
{
  objEd.removeAllObjects(false);
  objEd.reset();
}

void CSGPlugin::gatherStaticCollisionGeomGame(StaticGeometryContainer &cont)
{
  BBox3 bbox;
  for (int b = 0; b < objEd.objectCount(); b++)
  {
    BoxCSG *p = RTTI_cast<BoxCSG>(objEd.getObject(b));
    if (p)
    {
      TMatrix boxWtm = p->getWtm();

      // fill with matrices
      StaticGeometryNode n;
      n.flags = -1;
      n.wtm = boxWtm;
      n.script.setStr("collision", "csg");
      cont.addNode(dagGeom->newStaticGeometryNode(n, tmpmem));
    }
  }
}

void CSGPlugin::gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) { gatherStaticCollisionGeomGame(cont); }
