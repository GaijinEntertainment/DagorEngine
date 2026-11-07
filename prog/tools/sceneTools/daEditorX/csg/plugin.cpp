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
#include <libTools/meshCSG/meshCSG.h>
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


void CSGPlugin::processGeometry(StaticGeometryContainer &cont)
{
  DAEDITOR3.conNote("Start CSG");

  Tab<mesh_csg::Input> inputs(tmpmem);
  Tab<BBox3> boxes(tmpmem);
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    if (node.mesh->getRefCount() > 1)
    {
      // clone mesh when it is used by more than one node
      StaticGeometryMesh *m = new StaticGeometryMesh;
      m->mats = node.mesh->mats;
      m->mesh = node.mesh->mesh;
      node.mesh = m;
    }
    inputs.push_back(mesh_csg::Input{&node.mesh->mesh, node.wtm, node.name.str()});
    BBox3 box;
    for (const Point3 &v : node.mesh->mesh.vert)
      box += node.wtm * v;
    boxes.push_back(box);
  }

  // refused nodes, open ones too, keep faces
  mesh_csg::Result r;
  const bool ok = mesh_csg::evaluate(inputs, mesh_csg::Op::Union, {}, r);
  for (const eastl::string &w : r.warnings)
    DAEDITOR3.conNote("CSG: %s", w.c_str());
  for (int i = 0; i < r.refused.size(); ++i)
    if (!r.refused[i].empty())
      DAEDITOR3.conWarning("CSG: node %s keeps all its faces: %s", cont.nodes[i]->name.str(), r.refused[i].c_str());
  if (ok)
    for (int i = 0; i < cont.nodes.size(); ++i)
      mesh_csg::keep_surface_faces(r, i, cont.nodes[i]->mesh->mesh);
  else
    DAEDITOR3.conWarning("CSG Error: %s", r.error.c_str());


  BBox3 bbox;
  for (int b = 0; b < objEd.objectCount(); b++)
  {
    BoxCSG *p = RTTI_cast<BoxCSG>(objEd.getObject(b));
    if (p)
    {
      TMatrix boxWtm = p->getWtm();
      Plane3 box[6];
      generateBox(boxWtm, box, bbox);

      for (int i = 0; i < cont.nodes.size(); ++i)
      {
        if (!(bbox & boxes[i]))
          continue;

        TMatrix tm = cont.nodes[i]->wtm;
        MeshData &mesh = cont.nodes[i]->mesh->mesh;

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

  for (int i = cont.nodes.size() - 1; i >= 0; --i)
  {
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

void CSGPlugin::registerEditorCommands(IEditorCommandSystem &command_system) { objEd.registerEditorCommands(command_system); }

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
