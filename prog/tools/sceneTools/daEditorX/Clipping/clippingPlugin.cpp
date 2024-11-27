// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>

#include "clippingPlugin.h"
#include "clippingCm.h"
#include "dagListDlg.h"
#include "clipping_builder.h"
#include "collision_panel.h"

#include <oldEditor/de_util.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <de3_huid.h>
#include <de3_editorEvents.h>

#include <coolConsole/coolConsole.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/fileUtils.h>
#include <de3_entityFilter.h>

#include <util/dag_bitArray.h>
#include <scene/dag_physMat.h>
#include <scene/dag_frtdumpInline.h>
#include <shaders/dag_shaderMesh.h>

#include <math/dag_capsule.h>
#include <perfMon/dag_visClipMesh.h>
#include <obsolete/dag_cfg.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_log.h>
#include <math/dag_boundingSphere.h>

#include <io.h>

#include <propPanel/control/container.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <sepGui/wndPublic.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <sepGui/wndGlobal.h>
#include "physMesh.h"
#include <generic/dag_tab.h>
#include "de3_box_vs_tri.h"
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>

using hdpi::_pxScaled;

enum
{
  ID_DAG_FILES = 100,
  ID_PLUGIN_BASE,
};

static Tab<TMatrix> csgBox(midmem);

static __forceinline void errorReport(ILogWriter *rep, const char *text, ILogWriter::MessageType msg_type = ILogWriter::ERROR)
{
  if (rep)
  {
    debug(text);
    rep->addMessage(msg_type, text);
  }
}

static class QuietIgnoreTexResolver : public ITextureNameResolver
{
public:
  virtual bool resolveTextureName(const char *, String &out_name)
  {
    out_name = "";
    return true;
  }
} ignore_tex_resolver;

const char *ClippingPlugin::getPhysMatPath(ILogWriter *rep)
{
  const char *f = DAGORED2 ? DAGORED2->getWorkspace().getPhysmatPath() : NULL;

  if (f && f[0])
  {
    if (::dd_file_exist(f))
      return f;

    errorReport(rep, String(255, "PhysMat: can't find physmat file: '%s'", f));
    return NULL;
  }

  errorReport(rep, "PhysMat: no set 'physmat' in application.blk", ILogWriter::WARNING);
  return NULL;
}

static bool is_p3_eq(const Point3 &a, const Point3 &b) { return (a - b).lengthSq() < 1e-8f; }
static bool is_bad_wtm(const TMatrix &tm)
{
  if (lengthSq(tm.getcol(0)) < 1e-8 || lengthSq(tm.getcol(1)) < 1e-8 || lengthSq(tm.getcol(2)) < 1e-8)
    return true;
  return false;
}


Tab<int> PhysMesh::vmap(tmpmem);
Tab<int> PhysMesh::pmmap(tmpmem);
Tab<int> PhysMesh::facePerMat(tmpmem);
Tab<int> PhysMesh::vertPerMat(tmpmem);
Tab<int> PhysMesh::objPerMat(tmpmem);
FastNameMapEx PhysMesh::sceneMatNames;

namespace cook
{
float gridStep = 20, minSmallOverlap = 0.1, minMutualOverlap = 0.95;
int minFaceCnt = 8;
} // namespace cook

Tab<BBox3> phys_actors_bbox(midmem);
Tab<TMatrix> phys_box_actors(midmem);
Tab<Point4> phys_sph_actors(midmem);
Tab<TMatrix> phys_cap_actors(midmem);

//==============================================================================
ClippingPlugin::ClippingPlugin() :
  isVisible(false),
  clipDag(midmem),
  toolBarId(0),
  clipDagNew(midmem),
  panelClient(NULL),
  vcmRad(50.0),
  curPhysEngType(PHYSENG_Bullet),
  mPanelVisible(false)
{
  showGcBox = showGcSph = showGcCap = showGcMesh = true;
  showVcm = true;
  showDags = false;
  showVcmWire = true;
  gameFrt = NULL;
  showGameFrt = false;
  collisionReady = false;
  dagRtDumpReady = false;
}


//==============================================================================
void ClippingPlugin::registered()
{
  DataBlock blk;
  isValidPhysMatBlk(getPhysMatPath(&(DAGORED2->getConsole())), blk, &(DAGORED2->getConsole()));

  ::register_custom_collider(this);
}


//==============================================================================
void ClippingPlugin::unregistered()
{
  for (int i = 0; i < clipDagNew.size(); ++i)
  {
    String dagPath;
    getDAGPath(dagPath, clipDagNew[i]);

    remove((const char *)dagPath);
  }

  ::unregister_custom_collider(this);

  del_it(panelClient);
}


//==============================================================================
void ClippingPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();

  wndManager.addViewportAccelerator(CM_IMPORT, 'O', true);
  wndManager.addViewportAccelerator(CM_COMPILE_GAME_CLIPPING, 'B', true);
  wndManager.addViewportAccelerator(CM_COLLISION_SHOW_PROPS, 'P');
}


//==============================================================================
bool ClippingPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();

  mainMenu->addItem(menu_id, CM_IMPORT, "Add collision from DAG...\tCtrl+O");
  mainMenu->addItem(menu_id, CM_VIEW_DAG_LIST, "View DAG list...");
  mainMenu->addItem(menu_id, CM_CLEAR_DAG_LIST, "Clear DAG list");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_COLLISION_SHOW_PROPS, "Show collision params\tP");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_COMPILE_CLIPPING, "Compile collision...");
  mainMenu->addItem(menu_id, CM_COMPILE_GAME_CLIPPING, "Compile collision for game (PC)...\tCtrl+B");

  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);

  toolbar->setEventHandler(this);
  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL, "");

  tool->createButton(CM_IMPORT, "Add collision from DAG (Ctrl+O)");
  tool->setButtonPictures(CM_IMPORT, "import_dag");

  tool->createButton(CM_CLEAR_DAG_LIST, "Clear DAG list");
  tool->setButtonPictures(CM_CLEAR_DAG_LIST, "clear");
  tool->createSeparator(0);

  tool->createCheckBox(CM_COLLISION_SHOW_PROPS, "Show collision params (P)");
  tool->setButtonPictures(CM_COLLISION_SHOW_PROPS, "show_panel");
  tool->createSeparator(0);

  tool->createButton(CM_COMPILE_CLIPPING, "Compile collision");
  tool->setButtonPictures(CM_COMPILE_CLIPPING, "compile");

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->registerWindowHandler(this);

  ::set_vcm_visible(isVisible ? showVcm : false);
  if (mPanelVisible && panelClient)
    panelClient->showPropPanel(true);

  return true;
}


//==============================================================================
bool ClippingPlugin::end()
{
  mPanelVisible = panelClient->isVisible();

  if (mPanelVisible)
    panelClient->showPropPanel(false);

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->unregisterWindowHandler(this);
  ::set_vcm_visible(false);
  return true;
}

//==============================================================================

void *ClippingPlugin::onWmCreateWindow(int type)
{
  switch (type)
  {
    case PROPBAR_EDITOR_WTYPE:
    {
      if (panelClient->getPanelWindow())
        return nullptr;

      PropPanel::PanelWindowPropertyControl *_panel_window = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
      if (_panel_window)
      {
        panelClient->setPanelWindow(_panel_window);
        _panel_window->setEventHandler(panelClient);
        panelClient->setPanelParams();
      }

      PropPanel::ContainerPropertyControl *toolBar = DAGORED2->getCustomPanel(toolBarId);
      if (toolBar)
        toolBar->setBool(CM_COLLISION_SHOW_PROPS, panelClient->isVisible());
      return _panel_window;
    }
    break;
  }

  return nullptr;
}


bool ClippingPlugin::onWmDestroyWindow(void *window)
{
  if (window == panelClient->getPanelWindow())
  {
    mainPanelState.reset();
    panelClient->getPanelWindow()->saveState(mainPanelState);

    PropPanel::PanelWindowPropertyControl *_panel_window = panelClient->getPanelWindow();
    panelClient->setPanelWindow(NULL);

    IEditorCoreEngine::get()->deleteCustomPanel(_panel_window);

    PropPanel::ContainerPropertyControl *toolBar = DAGORED2->getCustomPanel(toolBarId);
    if (toolBar)
      toolBar->setBool(CM_COLLISION_SHOW_PROPS, panelClient->isVisible());

    return true;
  }

  return false;
}

void ClippingPlugin::updateImgui()
{
  if (DAGORED2->curPlugin() == this)
  {
    PropPanel::PanelWindowPropertyControl *panelWindow = panelClient ? panelClient->getPanelWindow() : nullptr;
    if (panelWindow)
    {
      bool open = true;
      DAEDITOR3.imguiBegin(*panelWindow, &open);
      panelWindow->updateImgui();
      DAEDITOR3.imguiEnd();

      if (!open && panelClient)
      {
        panelClient->showPropPanel(false);
        EDITORCORE->managePropPanels();
      }
    }
  }
}

//==============================================================================

void *ClippingPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  RETURN_INTERFACE(huid, IClipping);
  return NULL;
}


//==============================================================================
void ClippingPlugin::setVisible(bool vis)
{
  isVisible = vis;

  ::set_vcm_visible((isVisible && this == DAGORED2->curPlugin()) ? showVcm : false);
  DAGORED2->invalidateViewportCache();
}

void ClippingPlugin::renderObjects()
{
  begin_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  if (showGcMesh && phys_actors_bbox.size())
    for (int i = 0; i < phys_actors_bbox.size(); i++)
      draw_cached_debug_box(phys_actors_bbox[i], E3DCOLOR(((i & 30) >> 4) * 80, ((i & 0xC) >> 2) * 80, (i & 3) * 80));

  if (showGcBox && phys_box_actors.size())
    for (int i = 0; i < phys_box_actors.size(); i++)
      draw_cached_debug_box(
        phys_box_actors[i].getcol(3) - phys_box_actors[i].getcol(0) - phys_box_actors[i].getcol(1) - phys_box_actors[i].getcol(2),
        phys_box_actors[i].getcol(0) * 2, phys_box_actors[i].getcol(1) * 2, phys_box_actors[i].getcol(2) * 2,
        E3DCOLOR(255, 255, 255, 255));

  if (showGcSph && phys_sph_actors.size())
    for (int i = 0; i < phys_sph_actors.size(); i++)
      draw_cached_debug_sphere(Point3::xyz(phys_sph_actors[i]), phys_sph_actors[i].w, E3DCOLOR(255, 0, 255, 255));

  if (showGcCap && phys_cap_actors.size())
    for (int i = 0; i < phys_cap_actors.size(); i++)
    {
      Capsule cap;
      float r = phys_cap_actors[i].getcol(0).length();
      float l = phys_cap_actors[i].getcol(1).length();

      cap.set(phys_cap_actors[i].getcol(3) - phys_cap_actors[i].getcol(1) * 0.5,
        phys_cap_actors[i].getcol(3) + phys_cap_actors[i].getcol(1) * 0.5, r);

      draw_cached_debug_capsule_w(cap, E3DCOLOR(255, 255, 0, 255));
    }

  end_draw_cached_debug_lines();

  if (showDags)
  {
    makeDagPreviewCollision(false);
    begin_draw_cached_debug_lines();
    collisionpreview::drawCollisionPreview(collision, TMatrix::IDENT, E3DCOLOR(255, 0, 128));
    end_draw_cached_debug_lines();
  }
}

bool ClippingPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_PostRenderObjects && showVcm)
  {
    if (showGameFrt)
      makeGameFrtPreviewCollision(false);
    if (FastRtDump *rt = showGameFrt ? gameFrt : DagorPhys::getFastRtDump())
      ::render_visclipmesh(*rt, ::grs_cur_view.pos);
  }
  return false;
}

//===============================================================================
bool ClippingPlugin::recreatePanel()
{
  if (!panelClient)
    panelClient = new (uimem) CollisionPropPanelClient(this, rtStg, curPhysEngType);

  panelClient->showPropPanel(!panelClient->isVisible());

  return true;
}


//==============================================================================
void ClippingPlugin::importClipDag()
{
  String location(::de_get_sdk_dir());
  if (location == "")
    location = sgg::get_exe_path_full();

  String dagName = wingw::file_open_dlg(NULL, "Add collision from DAG", "DAG files|*.dag|All files|*.*", "dag", location);

  if (!dagName.length())
    return;

  dagRtDumpReady = false;
  ::simplify_fname(dagName);

  String oldDagName(dagName);

  dagName = ::get_file_name(dagName);

  for (int i = 0; i < clipDag.size(); ++i)
    if (stricmp(dagName, clipDag[i]) == 0)
    {
      wingw::message_box(wingw::MBS_EXCL, "Import DAG", "DAG already added: \"%s\"", (const char *)dagName);
      return;
    }

  String dagPath;
  getDAGPath(dagPath, dagName);

  if (!stricmp(dagPath, dagName))
  {
    wingw::message_box(wingw::MBS_EXCL, "Import error", "Couldn't import collision DAG to itself.");
    return;
  }

  if (dag_copy_file((const char *)oldDagName, (const char *)dagPath))
  {
    clipDag.push_back(dagName);
    clipDagNew.push_back(dagName);
    makeDagPreviewCollision(true);
  }
}

static const char *deducePhysMat(Mesh &m, Node &n, const char *phmat_str)
{
  if (phmat_str && phmat_str[0])
    return phmat_str;

  if (n.mat->subMatCount() && m.face.size())
  {
    static String name;
    int mat = m.face[0].mat % n.mat->subMatCount();
    if (n.mat->getSubMat(mat) && ::getPhysMatNameFromMatName(n.mat->getSubMat(mat)->matName, name))
      return name[0] ? (char *)name : NULL;
  }

  return NULL;
}

static bool addBoxCollision(IClippingDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
{
  if (!(rt->supportMask & rt->SUPPORT_BOX))
    return false;

  BBox3 box;
  for (int vi = 0; vi < m.vert.size(); vi++)
    box += m.vert[vi];

  TMatrix box_tm;
  Point3 w = box.width() * 0.5;

  box_tm.setcol(0, n.wtm.getcol(0) * w.x);
  box_tm.setcol(1, n.wtm.getcol(1) * w.y);
  box_tm.setcol(2, n.wtm.getcol(2) * w.z);
  box_tm.setcol(3, n.wtm * box.center());
  rt->addBox(box_tm, 0, 1, deducePhysMat(m, n, phmat_str));
  return true;
}
static bool addSphereCollision(IClippingDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
{
  if (!(rt->supportMask & rt->SUPPORT_SPHERE))
    return false;

  BSphere3 sph = ::mesh_bounding_sphere(m.vert.data(), m.vert.size());
  float l0 = lengthSq(n.wtm.getcol(0));
  float l1 = lengthSq(n.wtm.getcol(1));
  float l2 = lengthSq(n.wtm.getcol(2));
  if (l0 < l1)
    l0 = l2 > l1 ? l2 : l1;
  else if (l0 < l2)
    l0 = l2;
  rt->addSphere(n.wtm * sph.c, sqrt(l0) * sph.r, 0, 1, deducePhysMat(m, n, phmat_str));
  return true;
}
static bool addCapsuleCollision(IClippingDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
{
  if (!(rt->supportMask & rt->SUPPORT_CAPSULE))
    return false;

  BBox3 box;
  for (int vi = 0; vi < m.vert.size(); vi++)
    box += m.vert[vi];

  Capsule c;
  c.set(box);
  c.transform(n.wtm);
  rt->addCapsule(c, 0, 1, deducePhysMat(m, n, phmat_str));
  return true;
}

//==============================================================================
void ClippingPlugin::addClipNode(Node &n, IClippingDumpBuilder *rt)
{
  if (n.flags & NODEFLG_RCVSHADOW && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.mesh && !is_bad_wtm(n.wtm))
    {
      if (!(n.flags & NODEFLG_RENDERABLE) && strnicmp(n.name, "capsule", 7) == 0)
      {
        BBox3 b;
        for (int i = 0; i < mh.mesh->vert.size(); ++i)
          b += mh.mesh->vert[i];

        Capsule c;

        c.set(b);
        c.transform(n.wtm);

        int pmid = PHYSMAT_DEFAULT;
        {
          DataBlock blk;
          dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST);

          if (blk.paramExists("collision"))
          {
            DAEDITOR3.conWarning("explicit collision type in node '%s' of imported DAG is not supported, ignored", n.name.str());
            return;
          }

          if (blk.paramExists("phmat"))
          {
            const char *nm = blk.getStr("phmat", NULL);
            if (nm && strlen(nm))
            {
              pmid = PhysMat::getMaterialId(nm);
              if (pmid == PHYSMAT_DEFAULT)
                logerr("material %s is not defined in physmat_index!", nm);
            }
          }
          else
          {
            CfgReader cfg;
            cfg.readtext(n.script);
            cfg.getdiv("phmat");
            const char *nm = cfg.getstr("phmat", NULL);
            if (nm && strlen(nm))
            {
              pmid = PhysMat::getMaterialId(nm);
              if (pmid == PHYSMAT_DEFAULT)
                logerr("material %s is not defined in physmat_index!", nm);
            }
          }
        }

        rt->addCapsule(c, -1, 1, (pmid == PHYSMAT_DEFAULT) ? NULL : PhysMat::getMaterial(pmid).name.str());
        n.setobj(NULL);

        return;
      }
      else
      {
        int pmid = PHYSMAT_DEFAULT;
        bool pmid_force = false;
        const char *phmat_str = NULL;

        if (n.script)
        {
          DataBlock blk;
          dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST);

          if (blk.paramExists("phmat_force") || blk.paramExists("phmat"))
          {
            const char *n = blk.getStr("phmat_force", NULL);
            if (n && strlen(n))
              pmid_force = true;
            else
              n = blk.getStr("phmat", NULL);

            if (n && strlen(n))
            {
              phmat_str = n;
              pmid = PhysMat::getMaterialId(n);
              if (pmid == PHYSMAT_DEFAULT)
                logerr("material %s is not defined in physmat_index!", n);
            }
          }
          else
          {
            CfgReader cfg;
            cfg.readtext(n.script);

            if (cfg.getdiv("phmat"))
            {
              const char *n = cfg.getstr("phmat_force", NULL);
              if (n)
                pmid_force = true;
              else
                n = cfg.getstr("phmat", NULL);

              if (n)
              {
                phmat_str = n;
                pmid = PhysMat::getMaterialId(n);
                if (pmid == PHYSMAT_DEFAULT)
                  logerr("material %s is not defined in physmat_index!", n);
              }
            }
          }

          const char *primitive = blk.getStr("collision", NULL);
          bool add_not_mesh = false;
          if (!primitive || stricmp(primitive, "mesh") == 0 || stricmp(primitive, "convex") == 0)
            add_not_mesh = false;
          else if (stricmp(primitive, "box") == 0)
            add_not_mesh = addBoxCollision(rt, *mh.mesh, n, phmat_str);
          else if (stricmp(primitive, "sphere") == 0)
            add_not_mesh = addSphereCollision(rt, *mh.mesh, n, phmat_str);
          else if (stricmp(primitive, "capsule") == 0)
            add_not_mesh = addCapsuleCollision(rt, *mh.mesh, n, phmat_str);
          else
            DAEDITOR3.conWarning("unknown collision type: <%s>, ignored", primitive);
          if (!add_not_mesh)
            rt->addMesh(*mh.mesh, n.mat, n.wtm, 0, 1, pmid, pmid_force);
        }
        else
          rt->addMesh(*mh.mesh, n.mat, n.wtm, 0, 1, pmid, pmid_force);
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    if (n.child[i])
      addClipNode(*n.child[i], rt);
}

static void addCollisionRecurs(collisionpreview::Collision &c, Node &n)
{
  if (n.flags & NODEFLG_RCVSHADOW && n.obj && !is_bad_wtm(n.wtm) && n.obj->isSubOf(OCID_MESHHOLDER) && ((MeshHolderObj *)n.obj)->mesh)
  {
    Mesh &m = *((MeshHolderObj *)n.obj)->mesh;

    if (!(n.flags & NODEFLG_RENDERABLE) && strnicmp(n.name, "capsule", 7) == 0)
    {
      collisionpreview::addCapsuleCollision(c.capsule.push_back(), m, n.wtm);
      n.setobj(NULL);
      return;
    }

    if (!n.script)
    {
      DataBlock blk;
      dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST);

      const char *primitive = blk.getStr("collision", NULL);
      bool add_not_mesh = false;
      if (!primitive || stricmp(primitive, "mesh") == 0 || stricmp(primitive, "convex") == 0)
        collisionpreview::addMeshCollision(c.ltMesh.push_back(), m, n.wtm);
      else if (stricmp(primitive, "box") == 0)
        collisionpreview::addBoxCollision(c.box.push_back(), m, n.wtm);
      else if (stricmp(primitive, "sphere") == 0)
        collisionpreview::addSphereCollision(c.sphere.push_back(), m, n.wtm);
      else if (stricmp(primitive, "capsule") == 0)
        collisionpreview::addCapsuleCollision(c.capsule.push_back(), m, n.wtm);
      else
        DAEDITOR3.conWarning("unknown collision type: <%s>, ignored", primitive);
    }
    else
      collisionpreview::addMeshCollision(c.ltMesh.push_back(), m, n.wtm);
  }

  for (int i = 0; i < n.child.size(); ++i)
    addCollisionRecurs(c, *n.child[i]);
}

void ClippingPlugin::makeDagPreviewCollision(bool force_remake)
{
  if (!force_remake && collisionReady)
    return;
  collision.clear();

  for (int i = 0; i < clipDag.size(); ++i)
  {
    AScene sc;
    String dagPath;

    getDAGPath(dagPath, clipDag[i]);

    ITextureNameResolver *prev_resolver = ::get_global_tex_name_resolver();
    ::set_global_tex_name_resolver(&ignore_tex_resolver);
    if (!::load_ascene(dagPath, sc, LASF_NULLMATS | LASF_MATNAMES, false))
    {
      DAEDITOR3.conError("Invalid collision DAG: %s", dagPath.str());
      ::set_global_tex_name_resolver(prev_resolver);
      continue;
    }
    ::set_global_tex_name_resolver(prev_resolver);

    sc.root->calc_wtm();

    addCollisionRecurs(collision, *sc.root);
  }
  collisionReady = true;
}

void ClippingPlugin::makeGameFrtPreviewCollision(bool force_remake)
{
  if (!force_remake && gameFrt)
    return;
  del_it(gameFrt);
  gameFrt = getFrt(true);
}


void ClippingPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) { onPluginMenuClickInternal(pcb_id, panel); }
bool ClippingPlugin::onPluginMenuClick(unsigned id) { return onPluginMenuClickInternal(id, nullptr); }

//==============================================================================

bool ClippingPlugin::onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel)
{
  switch (id)
  {
    case CM_CLEAR_DAG_LIST:
      if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Clear list", "Do you really want to clear DAG list?") ==
          wingw::MB_ID_YES)
        clearDags();
      return true;

    case CM_IMPORT: importClipDag(); return true;

    case CM_COLLISION_SHOW_PROPS:
      recreatePanel();
      EDITORCORE->managePropPanels();
      break;

    case CM_COMPILE_CLIPPING:
      if (!compileEditClip())
        wingw::message_box(wingw::MBS_EXCL, "Compilation failed", "Failed to compile collision");
      else
        DAGORED2->invalidateViewportCache();
      return true;

    case CM_COMPILE_GAME_CLIPPING:
    {
      PropPanel::DialogWindow *myDlg = DAGORED2->createDialog(_pxScaled(300), _pxScaled(560), "Compile game collision (PC)");
      PropPanel::ContainerPropertyControl *myPanel = myDlg->getPanel();
      fillExportPanel(*myPanel);
      if (myDlg->showDialog() == PropPanel::DIALOG_ID_OK)
        if (!compileGameClip(myPanel, _MAKE4C('PC')))
          wingw::message_box(wingw::MBS_EXCL, "Compilation failed", "Failed to compile game collision");

      makeGameFrtPreviewCollision(true);
      DAGORED2->invalidateViewportCache();
      DAGORED2->deleteDialog(myDlg);

      return true;
    }
    case CM_VIEW_DAG_LIST:
      if (clipDag.size())
      {
        DagListDlg dl(clipDag);
        dl.showDialog();
      }
      else
        wingw::message_box(wingw::MBS_HAND, "DAG list", "DAG list is empty.");

      return true;
  }

  return false;
}


//==============================================================================
void ClippingPlugin::getGameClipPath(String &path, unsigned target_code) const
{
  char name_prefix[64] = {"game_clip"};

  if (target_code != _MAKE4C('PC'))
    sprintf(name_prefix + strlen(name_prefix), "-%s", mkbindump::get_target_str(target_code));

  switch (curPhysEngType)
  {
    case PHYSENG_DagorFastRT: strcat(name_prefix, ".frt.bin"); break;
    case PHYSENG_Bullet: strcat(name_prefix, ".bt.bin"); break;
  }
  path = DAGORED2->getPluginFilePath(this, name_prefix);
}


//==============================================================================
void ClippingPlugin::getClippingFiles(Tab<String> &files, unsigned target_code) const
{
  String path;
  getGameClipPath(path, target_code);

  files.push_back(path);

  if (curPhysEngType == PHYSENG_Bullet)
  {
    remove_trailing_string(path, ".bin");
    path += "-frt.bin";
    files.push_back(path);
  }
}


//==============================================================================
bool ClippingPlugin::validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params)
{
  switch (curPhysEngType)
  {
    case PHYSENG_DagorFastRT:
    {
      getPhysMatPath(&rep);
      break;
    }

    case PHYSENG_Bullet: break;
    default:
    {
      rep.addMessage(ILogWriter::ERROR, "Unknown collision PhysEngine type");
      return false;
    }
  }

  if (!compileGameClip(params, target))
  {
    rep.addMessage(ILogWriter::ERROR, "Game collision was not compiled");
    return false;
  }

  String clipFnameGame;
  getGameClipPath(clipFnameGame, target);

  if (!::dd_file_exist(clipFnameGame))
  {
    rep.addMessage(ILogWriter::ERROR, "Couldn't load raytracer dump from \"%s\"\n", clipFnameGame);

    logerr("Can't open raytracer dump from \"%s\"", clipFnameGame.str());
    return false;
  }

  if (curPhysEngType == PHYSENG_Bullet)
  {
    String frt_name(clipFnameGame);
    remove_trailing_string(frt_name, ".bin");
    frt_name += "-frt.bin";
    if (!::dd_file_exist(frt_name))
    {
      rep.addMessage(ILogWriter::ERROR, "Can't open raytracer dump to '%s'\n", frt_name);

      logerr("Can't open raytracer dump from \"%s\"", frt_name.str());

      return false;
    }
  }
  return true;
}

//==============================================================================
bool ClippingPlugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *)
{
  Tab<String> files(tmpmem);
  getClippingFiles(files, cwr.getTarget());

  if (files.size() == 2)
  {
    file_ptr_t fp = df_open(files[1], DF_READ);
    if (!fp)
      return false;

    if (df_length(fp) > 0)
    {
      cwr.beginTaggedBlock(_MAKE4C('FRT'));
      copy_file_to_stream(fp, cwr.getRawWriter(), df_length(fp));
      cwr.endBlock();
    }

    ::df_close(fp);
  }

  if (curPhysEngType == PHYSENG_Bullet && stricmp(DagorPhys::get_collision_name(), "DagorBullet") != 0)
    return true;

  {
    file_ptr_t fp = df_open(files[0], DF_READ);
    if (!fp)
      return false;

    int label;
    switch (curPhysEngType)
    {
      case PHYSENG_DagorFastRT: label = _MAKE4C('FRT'); break;
      case PHYSENG_Bullet: label = _MAKE4C('B_RT'); break;
    }

    if (df_length(fp) > 0)
    {
      cwr.beginTaggedBlock(label);
      copy_file_to_stream(fp, cwr.getRawWriter(), df_length(fp));
      cwr.endBlock();
    }
    ::df_close(fp);
  }

  return true;
}


//==============================================================================
void ClippingPlugin::fillExportPanel(PropPanel::ContainerPropertyControl &params)
{
  params.createStatic(-1, "Game collision members:");

  if (clipDag.size())
    params.createCheckBox(ID_DAG_FILES, "DAG files", disabledGamePlugins.getNameId("DAG files") == -1);

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

    if (check_collision_provider(plugin))
    {
      const char *plugName = plugin->getMenuCommandName();
      params.createCheckBox(ID_PLUGIN_BASE + i, plugName, disabledGamePlugins.getNameId(plugName) == -1);
    }
  }
}


//==============================================================================
bool ClippingPlugin::checkMetrics(const DataBlock &metrics_blk)
{
  CoolConsole &con = DAGORED2->getConsole();

  Tab<String> files(tmpmem);
  getClippingFiles(files, _MAKE4C('PC')); //== FIXME: should check metrics for current platform

  const int maxSize = metrics_blk.getReal("max_size", 0) * 1024 * 1024;
  int totalSize = 0;
  int compressed_frt = 0;

  for (int i = 0; i < files.size(); ++i)
  {
    file_ptr_t f = ::df_open(files[i], DF_READ);

    if (f)
    {
      // debug("file %s: %d", files[i].str(), df_length(f));
      if (trail_stricmp(files[i], "-frt.bin") && df_length(f) >= 20)
      {
        int sz;
        ::df_seek_to(f, 16);
        ::df_read(f, &sz, 4);
        if (sz < 0)
          sz = -sz;
        totalSize += sz;
        compressed_frt += df_length(f);
      }
      else
        totalSize += df_length(f);
      ::df_close(f);
    }
  }

  if (totalSize > maxSize)
  {
    DAEDITOR3.conError("Metrics validation failed: collision file(s) size %s (%i bytes) more "
                       "than maximum %s (%i bytes), compressed FRT=%dK (for leafSize=%@ levels=%d gridStep=%.1f)",
      ::bytes_to_mb(totalSize), totalSize, ::bytes_to_mb(maxSize), maxSize, compressed_frt >> 10, rtStg.leafSize(), rtStg.levels,
      rtStg.gridStep);

    return false;
  }

  return true;
}


//==============================================================================
void ClippingPlugin::clearObjects()
{
  clipDag.clear();
  editClipPlugins.reset();
  disabledGamePlugins.reset();
  curPhysEngType = PHYSENG_DagorFastRT;

  close_tps_physmat();
  DagorPhys::close_clipping();
  initClipping(false);
  clear_and_shrink(phys_actors_bbox);
  clear_and_shrink(phys_box_actors);
  clear_and_shrink(phys_sph_actors);
  clear_and_shrink(phys_cap_actors);
  collisionReady = false;
}


//=============================================================================
bool ClippingPlugin::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
bool ClippingPlugin::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
bool ClippingPlugin::handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
bool ClippingPlugin::handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
void ClippingPlugin::handleViewChange(IGenViewportWnd *wnd) {}


//==============================================================================
void ClippingPlugin::handleViewportPaint(IGenViewportWnd *wnd) {}


//==============================================================================
bool ClippingPlugin::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
void ClippingPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  local_data.setReal("vcm_rad", get_vcm_rad());
  local_data.setBool("showGcBox", showGcBox);
  local_data.setBool("showGcSph", showGcSph);
  local_data.setBool("showGcCap", showGcCap);
  local_data.setBool("showGcMesh", showGcMesh);
  local_data.setBool("showVcm", showVcm);
  local_data.setBool("showDags", showDags);
  local_data.setBool("showGameFrt", showGameFrt);

  int i;
  for (i = 0; i < clipDag.size(); ++i)
    blk.addStr("dag", clipDag[i]);

  blk.setPoint3("rt_leafSize", rtStg.leafSize());
  blk.setInt("rt_levels", rtStg.levels);
  blk.setReal("rt_gridStep", rtStg.gridStep);
  blk.setReal("rt_minMutualOverlap", rtStg.minMutualOverlap);
  blk.setReal("rt_minSmallOverlap", rtStg.minSmallOverlap);
  blk.setInt("rt_minFaceCnt", rtStg.minFaceCnt);

  DataBlock *plugBlk = blk.addBlock("EditClipPlugNames");

  iterate_names(editClipPlugins, [&](int, const char *name) { plugBlk->addStr("name", name); });


  DataBlock *disableBlk = blk.addNewBlock("disabled_game_collision");

  if (disableBlk)
    iterate_names(disabledGamePlugins, [&](int, const char *name) { disableBlk->addStr("name", name); });

  clipDagNew.clear();
}


//==============================================================================
void ClippingPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  vcmRad = local_data.getReal("vcm_rad", get_vcm_rad());
  showGcBox = local_data.getBool("showGcBox", true);
  showGcSph = local_data.getBool("showGcSph", true);
  showGcCap = local_data.getBool("showGcCap", true);
  showGcMesh = local_data.getBool("showGcMesh", true);
  showVcm = local_data.getBool("showVcm", true);
  showDags = local_data.getBool("showDags", false);
  showGameFrt = local_data.getBool("showGameFrt", false);

  if (vcmRad != get_vcm_rad())
    set_vcm_rad(vcmRad);

  curPhysEngType = PHYSENG_Bullet;
  const char *collisName = DagorPhys::get_collision_name();

  if (collisName)
  {
    if (stricmp(collisName, "Dagor") == 0)
      curPhysEngType = PHYSENG_DagorFastRT;
    else if (!stricmp(collisName, "Bullet") || !stricmp(collisName, "DagorBullet"))
      curPhysEngType = PHYSENG_Bullet;
    else
    {
      DAEDITOR3.conError("unknown collision type: <%s>, switching to <%s>", collisName, "Bullet");
      curPhysEngType = PHYSENG_Bullet;
    }
  }

  int dagNid = blk.getNameId("dag");
  int i;

  for (i = 0; i < blk.paramCount(); ++i)
    if (blk.getParamNameId(i) == dagNid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      String dagPath;
      String dagFile(blk.getStr(i));

      getDAGPath(dagPath, dagFile);
      if (access((const char *)dagPath, 4))
      {
        String msg(128, "Unable to open DAG file %s.\nFile name removed from DAG list.", (const char *)dagPath);

        wingw::message_box(wingw::MBS_EXCL, "Unable to open collision DAG", msg);
      }
      else
        clipDag.push_back(dagFile);
    }

  rtStg.defaults();
  rtStg.leafSize() = blk.getPoint3("rt_leafSize", rtStg.leafSize());
  rtStg.levels = blk.getInt("rt_levels", rtStg.levels);
  rtStg.gridStep = blk.getReal("rt_gridStep", rtStg.gridStep);
  rtStg.minMutualOverlap = blk.getReal("rt_minMutualOverlap", rtStg.minMutualOverlap);
  rtStg.minSmallOverlap = blk.getReal("rt_minSmallOverlap", rtStg.minSmallOverlap);
  rtStg.minFaceCnt = blk.getInt("rt_minFaceCnt", rtStg.minFaceCnt);

  const DataBlock *plugBlk = blk.getBlockByName("EditClipPlugNames");

  if (plugBlk)
    for (i = 0; i < plugBlk->paramCount(); ++i)
      editClipPlugins.addNameId(plugBlk->getStr(i));

  plugBlk = blk.getBlockByName("DisabledCustomColliders");

  if (plugBlk)
  {
    String plugName;

    for (i = 0; i < plugBlk->paramCount(); ++i)
    {
      plugName = plugBlk->getStr(i);
      if (plugName.length())
        disableCustomColliders.addNameId(plugName);
    }
  }

  disabledGamePlugins.reset();

  const DataBlock *disableBlk = blk.getBlockByName("disabled_game_collision");

  if (disableBlk)
  {
    for (int i = 0; i < disableBlk->paramCount(); ++i)
    {
      const char *name = disableBlk->getStr(i);
      if (name && *name)
        disabledGamePlugins.addNameId(name);
    }
  }

  initClipping(false);
  setVisible(isVisible);
  dagRtDumpReady = false;
}


static void addMeshCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n, bool for_game, bool need_kill);
static void addBoxCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n);
static void addSphereCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n);
static void addCapsuleCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n);
static void addConvexCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n);
static const char *deducePhysMat(StaticGeometryNode &n)
{
  const char *physmat = n.script.getStr("phmat", NULL);
  if (physmat && physmat[0])
    return physmat;

  if (n.mesh->mats.size() && n.mesh->mesh.face.size())
  {
    static String name;
    int mat = n.mesh->mesh.face[0].mat % n.mesh->mats.size();
    if (n.mesh->mats[mat] && ::getPhysMatNameFromMatName(n.mesh->mats[mat]->name, name))
      return name[0] ? (char *)name : NULL;
  }

  return NULL;
}


static void markTris(Bitarray &used_faces, const TMatrix &boxWtm, StaticGeometryNode &n)
{
  Plane3 box[6];
  BBox3 bbox;
  generateBox(boxWtm, box, bbox);

  TMatrix tm = n.wtm;
  MeshData &mesh = n.mesh->mesh;

  Tab<bool> inside(tmpmem);
  inside.resize(mesh.vert.size());
  for (int i = 0; i < mesh.vert.size(); ++i)
  {
    Point3 mv;
    mv = tm * mesh.vert[i];
    inside[i] = isInside(box, mv);
  }

  for (int i = mesh.face.size() - 1; i >= 0; --i)
  {
    if (used_faces.get(i) && inside[mesh.face[i].v[0]] && inside[mesh.face[i].v[1]] && inside[mesh.face[i].v[2]])
    {
      // mesh.removeFacesFast(i,1);
      used_faces.set(i, 0);
    }
  }
}


static void addMeshCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n, bool for_game, bool need_kill)
{
  // remove degenerate triangles
  Mesh mesh = n.mesh->mesh;
  const TMatrix *wtm = &n.wtm;
  if (need_kill)
  {
    for (int i = 0; i < mesh.vert.size(); ++i)
      mesh.vert[i] = n.wtm * mesh.vert[i];
    wtm = &TMatrix::IDENT;
    mesh.kill_unused_verts(0.0005f);
    mesh.kill_bad_faces();
  }
  Bitarray used_mats;
  if (for_game)
  {
    used_mats.resize(n.mesh->mats.size());
    for (int mi = 0; mi < n.mesh->mats.size(); ++mi)
    {
      //==fixme: name could be moved to application.blk
      if ((n.flags & StaticGeometryNode::FLG_RENDERABLE) && mi < n.mesh->mats.size() && n.mesh->mats[mi] &&
          strstr(n.mesh->mats[mi]->className.str(), "land_mesh") && !strstr(n.mesh->mats[mi]->className.str(), "land_mesh_clipmap"))
        used_mats.set(mi, 0); // this mesh was already added to landMesh/landRay, skip it here
      else
        used_mats.set(mi, 1);
    }
  }
  Bitarray used_faces;
  used_faces.resize(mesh.face.size());
  for (int ntri = 0; ntri < mesh.face.size(); ntri++)
  {
    Face &face = mesh.face[ntri];
    int mat = face.mat;
    if (mat >= used_mats.size())
      mat = used_mats.size() - 1;
    if (mat >= 0)
    {
      if (!used_mats[mat])
      {
        used_faces.set(ntri, 0);
        continue;
      }
    }

    Point3 v[3];
    for (int z = 0; z < 3; z++)
      v[z] = mesh.vert[face.v[z]];

    if (is_p3_eq(v[1], v[0]) || is_p3_eq(v[2], v[0]) || is_p3_eq(v[1], v[2]))
      used_faces.set(ntri, 0);
    else
      used_faces.set(ntri, 1);
  }

  for (int b = 0; b < csgBox.size(); b++)
    markTris(used_faces, csgBox[b], n);

  mesh.removeFacesFast(used_faces);
  if (mesh.face.size())
    rt->addMesh(mesh, NULL, *wtm, 0, 1, PHYSMAT_DEFAULT, false, n.mesh);
  else if (used_mats.size())
  {
    bool has_used_mat = false;
    for (int mi = 0; mi < used_mats.size(); ++mi)
      if (used_mats[mi])
        has_used_mat = true;
    if (!has_used_mat)
      debug("skip mesh in <%s> for %d mats are *land_mesh*", n.name, used_mats.size());
  }
}
static void addBoxCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n)
{
  if (!(rt->supportMask & rt->SUPPORT_BOX))
  {
    if (rt->supportMask & rt->SUPPORT_CONVEX)
      addConvexCollision(rt, n);
    else
      addMeshCollision(rt, n, false, false);
    return;
  }

  BBox3 box;
  Mesh &m = n.mesh->mesh;
  for (int vi = 0; vi < m.vert.size(); vi++)
    box += m.vert[vi];

  TMatrix box_tm;
  Point3 w = box.width() * 0.5;
  /*float len;

  len = length(n.wtm.getcol(0)*w.x);
  if (len < 0.1)
    w.x *= 0.1/len;
  len = length(n.wtm.getcol(1)*w.y);
  if (w.y < 0.1)
    w.y *= 0.1/len;
  len = length(n.wtm.getcol(2)*w.z);
  if (w.z < 0.1)
    w.z *= 0.1/len;*/

  box_tm.setcol(0, n.wtm.getcol(0) * w.x);
  box_tm.setcol(1, n.wtm.getcol(1) * w.y);
  box_tm.setcol(2, n.wtm.getcol(2) * w.z);
  box_tm.setcol(3, n.wtm * box.center());
  rt->addBox(box_tm, 0, 1, deducePhysMat(n));
}
static void addSphereCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n)
{
  if (!(rt->supportMask & rt->SUPPORT_SPHERE))
  {
    if (rt->supportMask & rt->SUPPORT_CONVEX)
      addConvexCollision(rt, n);
    else
      addMeshCollision(rt, n, false, false);
    return;
  }

  Mesh &m = n.mesh->mesh;
  BSphere3 sph = ::mesh_bounding_sphere(m.vert.data(), m.vert.size());
  float l0 = lengthSq(n.wtm.getcol(0));
  float l1 = lengthSq(n.wtm.getcol(1));
  float l2 = lengthSq(n.wtm.getcol(2));
  if (l0 < l1)
    l0 = l2 > l1 ? l2 : l1;
  else if (l0 < l2)
    l0 = l2;
  rt->addSphere(n.wtm * sph.c, sqrt(l0) * sph.r, 0, 1, deducePhysMat(n));
}
static void addCapsuleCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n)
{
  if (!(rt->supportMask & rt->SUPPORT_CAPSULE))
  {
    if (rt->supportMask & rt->SUPPORT_CONVEX)
      addConvexCollision(rt, n);
    else
      addMeshCollision(rt, n, false, false);
    return;
  }

  BBox3 box;
  Mesh &m = n.mesh->mesh;
  for (int vi = 0; vi < m.vert.size(); vi++)
    box += m.vert[vi];

  Capsule c;
  c.set(box);
  c.transform(n.wtm);
  rt->addCapsule(c, 0, 1, deducePhysMat(n));
}
static void addConvexCollision(IClippingDumpBuilder *rt, StaticGeometryNode &n)
{
  if (!(rt->supportMask & rt->SUPPORT_CONVEX))
    return;

  // remove degenerate triangles
  Mesh mesh = n.mesh->mesh;
  for (int ntri = 0; ntri < mesh.face.size(); ntri++)
  {
    Face &face = mesh.face[ntri];

    Point3 v[3];
    for (int z = 0; z < 3; z++)
      v[z] = mesh.vert[face.v[z]];

    Point3 v1 = v[1] - v[0];
    Point3 v2 = v[2] - v[0];

    const real lenSq = sqrtf(v1.lengthSq() * v2.lengthSq());
    if (lenSq < 0.00001)
      erase_items(mesh.face, ntri--, 1);
  }
  rt->addConvexHull(mesh, NULL, n.wtm, 0, 1, PHYSMAT_DEFAULT, false, n.mesh);
}

//==============================================================================
bool ClippingPlugin::compileClipping(bool for_game, Tab<int> &plugs, int physeng_type, unsigned target_code)
{
  Tab<String> clipPlug(tmpmem);

  IClippingDumpBuilder *rt;
  if (!PhysMat::getMaterials().size())
  {
    DAEDITOR3.conError("No phys materials found - collision expoprt aborted.\n"
                       "Check for properly configured physmat.blk path in application.blk");
    return false;
  }

  switch (physeng_type)
  {
    case PHYSENG_DagorFastRT: rt = ::create_dagor_raytracer_dump_builder(); break;
    case PHYSENG_Bullet:
      rt = ::create_bullet_clipping_dump_builder(stricmp(DagorPhys::get_collision_name(), "DagorBullet") == 0);
      break;
  }

  if (!rt)
  {
    DEBUG_CTX("Cannot create physeng collision dump builder: %d", physeng_type);
    return false;
  }

  rt->start(rtStg);

  int i;

  if (plugs.size() && plugs[0] == -1)
  {
    clipPlug.push_back() = "DAG files";

    for (i = 0; i < clipDag.size(); ++i)
    {
      AScene sc;
      String dagPath;

      getDAGPath(dagPath, clipDag[i]);

      ITextureNameResolver *prev_resolver = ::get_global_tex_name_resolver();
      ::set_global_tex_name_resolver(&ignore_tex_resolver);
      if (!::load_ascene(dagPath, sc, LASF_NULLMATS | LASF_MATNAMES, false))
      {
        debug("Invalid collision DAG, compilation aborted");
        ::set_global_tex_name_resolver(prev_resolver);
        return false;
      }
      ::set_global_tex_name_resolver(prev_resolver);

      sc.root->calc_wtm();

      addClipNode(*sc.root, rt);
    }

    erase_items(plugs, 0, 1);
  }

  int prevSubtype = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);

  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, 0);
  for (i = 0; i < plugs.size(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(plugs[i]);
    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();

    if (strcmp(plugin->getInternalName(), "csg") == 0)
    {
      StaticGeometryContainer geoCont;

      if (for_game)
        geom->gatherStaticCollisionGeomGame(geoCont);
      else
        geom->gatherStaticCollisionGeomEditor(geoCont);

      for (int j = 0; j < geoCont.nodes.size(); ++j)
      {
        StaticGeometryNode &n = *geoCont.nodes[j];
        if (is_bad_wtm(n.wtm))
          continue;
        csgBox.resize(csgBox.size() + 1);
        csgBox.back() = n.wtm;
      }
      continue;
    }

    if (geom)
      continue;

    IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

    if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
      filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION, true);
  }

  DataBlock app_blk(DAGORED2->getWorkspace().getAppPath());
  const char *mgr_type = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getStr("type", NULL);
  bool removeInvisibleFacesLand = false;
  static int lmeshObj = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj");

  if (mgr_type && strcmp(mgr_type, "aces") == 0)
    removeInvisibleFacesLand =
      app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("scnExport")->getBool("removeUndegroundFaces", true);

  for (i = 0; i < plugs.size(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(plugs[i]);
    clipPlug.push_back() = plugin->getMenuCommandName();

    if (strcmp(plugin->getInternalName(), "csg") == 0)
      continue;

    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
    if (!geom)
      continue;

    StaticGeometryContainer geoCont;

    if (for_game)
      geom->gatherStaticCollisionGeomGame(geoCont);
    else
      geom->gatherStaticCollisionGeomEditor(geoCont);

    // preprocessing
    bool need_kill = false;
    if (removeInvisibleFacesLand /*&& (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT) & lmeshObj)*/)
      for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
        if (IGenEditorPlugin *plug = DAGORED2->getPlugin(i))
          if (plug->queryInterfacePtr(HUID_IPostProcessGeometry))
          {
            plug->queryInterface<IPostProcessGeometry>()->processGeometry(geoCont);
            need_kill = true;
          }

    for (int j = 0; j < geoCont.nodes.size(); ++j)
    {
      StaticGeometryNode &n = *geoCont.nodes[j];
      if (is_bad_wtm(n.wtm))
        continue;

      const char *primitive = geoCont.nodes[j]->script.getStr("collision", NULL);
      if (!primitive || stricmp(primitive, "mesh") == 0)
      {
        addMeshCollision(rt, n, for_game, need_kill);
      }
      else if (stricmp(primitive, "box") == 0)
        addBoxCollision(rt, n);
      else if (stricmp(primitive, "sphere") == 0)
        addSphereCollision(rt, n);
      else if (stricmp(primitive, "capsule") == 0)
        addCapsuleCollision(rt, n);
      else if (stricmp(primitive, "convex") == 0)
        addConvexCollision(rt, n);
      else
        DAEDITOR3.conWarning("unknown collision type: <%s>, ignored", primitive);
    }
  }

  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, prevSubtype);

  String tmpFname;
  String clipFname;

  if (for_game)
    getGameClipPath(clipFname, target_code);
  else
    getEditorClipPath(clipFname);

  tmpFname = DAGORED2->getPluginFilePath(this, "temp_rtdump");

  {
    FullFileSaveCB cwr(clipFname);
    bool ok = cwr.fileHandle && rt->finishAndWrite(tmpFname, cwr, target_code);

    if (!ok)
    {
      debug("Errors while compile collision");
      rt->destroy();
      return false;
    }
  }

  if (rt->getSeparateRayTracer())
  {
    IClippingDumpBuilder *sep_rt = rt->getSeparateRayTracer();
    String frt_fname(clipFname);
    remove_trailing_string(frt_fname, ".bin");
    frt_fname += "-frt.bin";

    FullFileSaveCB cwr(frt_fname);
    bool ok = cwr.fileHandle && sep_rt->finishAndWrite(tmpFname, cwr, target_code);

    if (!ok)
    {
      rt->destroy();
      return false;
    }
  }

  rt->destroy();

  clear_and_shrink(csgBox);

  if (!for_game)
    initClipping(false);

  return true;
}


//==============================================================================
bool ClippingPlugin::initClipping(bool for_game)
{
  String clipFname;

  if (for_game)
  {
    getGameClipPath(clipFname, _MAKE4C('PC'));
    remove_trailing_string(clipFname, ".bin");
    clipFname += "-frt.bin";
  }
  else
    getEditorClipPath(clipFname);

  close_tps_physmat();
  DagorPhys::close_clipping();

  ::init_tps_physmat(ClippingPlugin::getPhysMatPath());

  if (IDagorEd2Engine::get())
  {
    if (!panelClient)
      panelClient = new (uimem) CollisionPropPanelClient(this, rtStg, curPhysEngType);
  }

  FullFileLoadCB fl((const char *)clipFname);

  debug("Try to load collision from file %s", (const char *)clipFname);

  if (!fl.fileHandle)
    return false;

  bool success = true;

  try
  {
    if (!DagorPhys::load_binary_raytracer(fl))
      success = false;
    else
      debug("Collision successfully loaded");
  }
  catch (...)
  {
    success = false;
  }

  if (!success)
    debug("Unable to load file %s. Perhaps file is corrupted.", (const char *)clipFname);

  return success;
}

FastRtDump *ClippingPlugin::getFrt(bool game_frt)
{
  String clipFname;
  if (game_frt)
  {
    getGameClipPath(clipFname, _MAKE4C('PC'));
    remove_trailing_string(clipFname, ".bin");
    clipFname += "-frt.bin";
  }
  else
    getEditorClipPath(clipFname);

  FullFileLoadCB fl((const char *)clipFname);

  if (!fl.fileHandle)
  {
    debug("Unable to find file %s. Perhaps file is corrupted.", (const char *)clipFname);
    return NULL;
  }

  bool success = true;

  FastRtDump *frt = new (midmem) FastRtDump;

  try
  {
    frt->loadData(fl);
  }
  catch (...)
  {
    success = false;
  }

  if (!success)
    debug("Unable to loadData from file %s. Perhaps file is corrupted.", (const char *)clipFname);

  if (!frt->isDataValid())
  {
    delete frt;
    debug("File %s is corrupted - isDataValid=false", (const char *)clipFname);
    return NULL;
  }
  return frt;
}

//==============================================================================
bool ClippingPlugin::compileEditClip()
{
  int lines = count_collision_provider();
  if (clipDag.size())
    ++lines;

  eastl::unique_ptr<PropPanel::DialogWindow> myDlg(
    DAGORED2->createDialog(_pxScaled(300), _pxScaled(70 + lines * 30), "Collision members"));
  PropPanel::ContainerPropertyControl *myPanel = myDlg->getPanel();

  int i;
  if (clipDag.size())
    myPanel->createCheckBox(ID_DAG_FILES, "DAG files", editClipPlugins.getNameId("DAG files") >= 0);

  for (i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plug = DAGORED2->getPlugin(i);

    if (check_collision_provider(plug))
      myPanel->createCheckBox(ID_PLUGIN_BASE + i, plug->getMenuCommandName(),
        editClipPlugins.getNameId(plug->getMenuCommandName()) >= 0);
  }

  Tab<int> plugs(tmpmem);

  if (myDlg->showDialog() != PropPanel::DIALOG_ID_OK)
    return false;
  else
  {
    if (myPanel->getBool(ID_DAG_FILES))
      plugs.push_back(-1);

    for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
      if (myPanel->getBool(ID_PLUGIN_BASE + i))
        plugs.push_back(i);
  }

  if (plugs.empty())
    return false;

  return compileClipping(false, plugs, PHYSENG_DagorFastRT, _MAKE4C('PC'));
}

void ClippingPlugin::prepareDAGcollision()
{
  dagRtDumpReady = true;
  dagRtDump.unloadData();

  IClippingDumpBuilder *rt = ::create_dagor_raytracer_dump_builder();

  if (!rt)
  {
    DAEDITOR3.conError("Cannot create FRT collision dump builder");
    return;
  }

  rt->start(rtStg);

  for (int i = 0; i < clipDag.size(); ++i)
  {
    AScene sc;
    String dagPath;

    getDAGPath(dagPath, clipDag[i]);

    ITextureNameResolver *prev_resolver = ::get_global_tex_name_resolver();
    ::set_global_tex_name_resolver(&ignore_tex_resolver);
    if (!::load_ascene(dagPath, sc, LASF_NULLMATS | LASF_MATNAMES, false))
    {
      DAEDITOR3.conError("Invalid collision DAG <%s>, skipped", dagPath.str());
      ::set_global_tex_name_resolver(prev_resolver);
      continue;
    }
    ::set_global_tex_name_resolver(prev_resolver);

    sc.root->calc_wtm();
    addClipNode(*sc.root, rt);
    DAEDITOR3.conNote("add clip DAG: %s", dagPath.str());
  }

  {
    MemorySaveCB cwr;
    if (rt->finishAndWrite(NULL, cwr, _MAKE4C('PC')))
    {
      if (cwr.getSize())
      {
        MemoryLoadCB crd(cwr.getMem(), false);
        dagRtDump.loadData(crd);
        if (!dagRtDump.isDataValid())
          DAEDITOR3.conError("Errors reading built DAG collision");
      }
      else
        DAEDITOR3.conNote("No data in DAG collision");
    }
    else
      DAEDITOR3.conError("Errors while compiling DAG collision");
  }

  rt->destroy();
}

//==============================================================================
bool ClippingPlugin::compileGameClip(PropPanel::ContainerPropertyControl *panel, unsigned target_code)
{
  if (!panel)
    return false;

  disabledGamePlugins.reset();

  Tab<int> gamePlugs(tmpmem);

  if (panel->getBool(ID_DAG_FILES))
    gamePlugs.push_back(-1);
  else
    disabledGamePlugins.addNameId("DAG files");

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    if (panel->getBool(ID_PLUGIN_BASE + i))
      gamePlugs.push_back(i);
    else if (IGenEditorPlugin *plug = DAGORED2->getPlugin(i))
      disabledGamePlugins.addNameId(plug->getMenuCommandName());

  if (!gamePlugs.size())
    DAEDITOR3.conWarning("No plugins selected to get collision data. Collision will be empty.");

  return compileClipping(true, gamePlugs, curPhysEngType, target_code);
}


//==============================================================================
void ClippingPlugin::clearDags()
{
  String dagPath;

  for (int i = 0; i < clipDag.size(); ++i)
  {
    getDAGPath(dagPath, clipDag[i]);
    remove((const char *)dagPath);
  }

  clipDag.clear();
  dagRtDumpReady = false;
}


//==============================================================================
bool ClippingPlugin::compileClippingWithDialog(bool for_game)
{
  if (!for_game)
    return compileEditClip();

  return false;
}


//==============================================================================
void ClippingPlugin::manageCustomColliders()
{
  iterate_names(disableCustomColliders, [&](int, const char *name) { ::disable_custom_collider(name); });
}


//==============================================================================
bool ClippingPlugin::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (!dagRtDumpReady)
    prepareDAGcollision();
  if (!dagRtDump.isDataValid())
    return false;

  bool clip = false;
  FastRtDump *rt = DagorPhys::getFastRtDump();

  int phmatid;
  Point3 normal;

  if (dagRtDump.traceray(p, dir, maxt, phmatid, norm ? *norm : normal) >= 0)
    return true;

  return false;
}

//==============================================================================
bool ClippingPlugin::isValidPhysMatBlk(const char *file, DataBlock &blk, ILogWriter *rep, DataBlock **mat_blk, DataBlock **def_blk)
{
  if (mat_blk)
    *mat_blk = NULL;
  if (def_blk)
    *def_blk = NULL;

  if (!file)
    return false;

  if (!blk.load(file))
  {
    errorReport(rep, String(255, "PhysMat: cannot load '%s'!", file));
    return false;
  }

  bool ret = true;

  // load materials
  DataBlock *materialsBlk = blk.getBlockByName("PhysMats");
  if (!materialsBlk)
  {
    errorReport(rep, String(255, "PhysMat: 'PhysMats' section required! (%s)", file));
    ret = false;
  }

  if (mat_blk)
    *mat_blk = materialsBlk;

  // default params
  DataBlock *defMatBlk = ret ? materialsBlk->getBlockByName("__DefaultParams") : NULL;
  if (ret && !defMatBlk)
  {
    errorReport(rep, String(255, "PhysMat: '__DefaultParams' section required in 'PhysMats'! (%s)", file));
    ret = false;
  }

  if (def_blk)
    *def_blk = defMatBlk;


  // load InteractPropsList
  DataBlock *propListBlk = blk.getBlockByName("InteractPropsList");
  if (!propListBlk)
  {
    errorReport(rep, String(255, "PhysMat: 'InteractPropsList' section required! (%s)", file));
    ret = false;
  }

  return ret;
}


bool check_collision_provider(IGenEditorPlugin *plugin)
{
  if (!plugin)
    return false;

  IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
  IObjEntityFilter *filter = NULL;

  if (!geom)
  {
    filter = plugin->queryInterface<IObjEntityFilter>();
    if (filter && !filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
      filter = NULL;
  }

  return geom || filter;
}
int count_collision_provider()
{
  int count = 0;
  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    if (check_collision_provider(DAGORED2->getPlugin(i)))
      count++;
  return count;
}
