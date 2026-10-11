// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>

#include "collisionPlugin.h"
#include "collisionCm.h"
#include "dagListDlg.h"
#include "collision_builder.h"
#include "collision_panel.h"

#include <oldEditor/de_util.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <de3_huid.h>
#include <de3_editorEvents.h>

#include <coolConsole/coolConsole.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/fileUtils.h>
#include <de3_entityFilter.h>

#include <util/dag_bitArray.h>
#include <scene/dag_physMat.h>
#include <shaders/dag_shaderMesh.h>

#include <math/dag_capsule.h>
#include <debug/dag_collResDebugDraw.h>
#include <obsolete/dag_cfg.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_log.h>
#include <math/dag_boundingSphere.h>

#include <propPanel/control/container.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_wndGlobal.h>
#include <generic/dag_tab.h>
#include "de3_box_vs_tri.h"
#include <3d/dag_render.h>

#if _TARGET_PC_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

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
  bool resolveTextureName(const char *, String &out_name) override
  {
    out_name = "";
    return true;
  }
} ignore_tex_resolver;

const char *CollisionPlugin::getPhysMatPath(ILogWriter *rep)
{
  const char *f = DAGORED2 ? DAGORED2->getWorkspace().getPhysmatPath() : NULL;

  if (f && f[0])
  {
    if (::dd_file_exist(f))
      return f;

    errorReport(rep, String(255, "PhysMat: can't find physmat file: '%s'", f));
    return NULL;
  }

  errorReport(rep, String(0, "PhysMat: no set 'physmat' in %s", DAGORED2->getWorkspace().getAppBlkShortName()), ILogWriter::WARNING);
  return NULL;
}

static bool is_p3_eq(const Point3 &a, const Point3 &b) { return (a - b).lengthSq() < 1e-8f; }
static bool is_bad_wtm(const TMatrix &tm)
{
  if (lengthSq(tm.getcol(0)) < 1e-8f || lengthSq(tm.getcol(1)) < 1e-8f || lengthSq(tm.getcol(2)) < 1e-8f)
    return true;
  return false;
}


//==============================================================================
CollisionPlugin::CollisionPlugin() :
  isVisible(false), clipDag(midmem), toolBarId(0), clipDagNew(midmem), panelClient(NULL), vcmRad(50.0), mPanelVisible(false)
{
  showVcm = true;
  showDags = false;
  showVcmWire = true;
  showGameFrt = false;
  collisionReady = false;
  dagCollReady = false;
}


//==============================================================================
void CollisionPlugin::registered()
{
  DataBlock blk;
  isValidPhysMatBlk(getPhysMatPath(&(DAGORED2->getConsole())), blk, &(DAGORED2->getConsole()));

  ::register_custom_collider(this);
}


//==============================================================================
void CollisionPlugin::unregistered()
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
void CollisionPlugin::registerEditorCommands(IEditorCommandSystem &command_system)
{
  command_system.addCommand(EditorCommandIds::IMPORT, ImGuiMod_Ctrl | ImGuiKey_O);
  command_system.addCommand(EditorCommandIds::VIEW_DAG_LIST);
  command_system.addCommand(EditorCommandIds::CLEAR_DAG_LIST);
  command_system.addCommand(EditorCommandIds::COLLISION_SHOW_PROPS, ImGuiKey_P);
  command_system.addCommand(EditorCommandIds::COMPILE_COLLISION);
  command_system.addCommand(EditorCommandIds::COMPILE_GAME_COLLISION, ImGuiMod_Ctrl | ImGuiKey_B);
}


//==============================================================================
void CollisionPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();

  wndManager.addViewportAccelerator(CM_IMPORT, EditorCommandIds::IMPORT);
  wndManager.addViewportAccelerator(CM_VIEW_DAG_LIST, EditorCommandIds::VIEW_DAG_LIST);
  wndManager.addViewportAccelerator(CM_CLEAR_DAG_LIST, EditorCommandIds::CLEAR_DAG_LIST);
  wndManager.addAccelerator(CM_COLLISION_SHOW_PROPS, EditorCommandIds::COLLISION_SHOW_PROPS);
  wndManager.addViewportAccelerator(CM_COMPILE_COLLISION, EditorCommandIds::COMPILE_COLLISION);
  wndManager.addViewportAccelerator(CM_COMPILE_GAME_COLLISION, EditorCommandIds::COMPILE_GAME_COLLISION);
}


//==============================================================================
bool CollisionPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();
  IEditorCommandSystem *commandSystem = DAGORED2->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);

  commandSystem->addMenuItem(*mainMenu, menu_id, CM_IMPORT, EditorCommandIds::IMPORT, "Add collision from DAG...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_VIEW_DAG_LIST, EditorCommandIds::VIEW_DAG_LIST, "View DAG list...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_CLEAR_DAG_LIST, EditorCommandIds::CLEAR_DAG_LIST, "Clear DAG list");
  mainMenu->addSeparator(menu_id);
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_COLLISION_SHOW_PROPS, EditorCommandIds::COLLISION_SHOW_PROPS,
    "Show collision params");
  mainMenu->addSeparator(menu_id);
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_COMPILE_COLLISION, EditorCommandIds::COMPILE_COLLISION, "Compile collision...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_COMPILE_GAME_COLLISION, EditorCommandIds::COMPILE_GAME_COLLISION,
    "Compile collision for game (PC)...");

  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);

  toolbar->setEventHandler(this);
  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL);

  commandSystem->createToolbarButton(*tool, CM_IMPORT, EditorCommandIds::IMPORT, "Add collision from DAG");
  tool->setButtonPictures(CM_IMPORT, "import_dag");

  commandSystem->createToolbarButton(*tool, CM_CLEAR_DAG_LIST, EditorCommandIds::CLEAR_DAG_LIST, "Clear DAG list");
  tool->setButtonPictures(CM_CLEAR_DAG_LIST, "clear");
  tool->createSeparator(0);

  commandSystem->createToolbarToggleButton(*tool, CM_COLLISION_SHOW_PROPS, EditorCommandIds::COLLISION_SHOW_PROPS,
    "Show collision params");
  tool->setButtonPictures(CM_COLLISION_SHOW_PROPS, "show_panel");
  tool->createSeparator(0);

  commandSystem->createToolbarButton(*tool, CM_COMPILE_COLLISION, EditorCommandIds::COMPILE_COLLISION, "Compile collision");
  tool->setButtonPictures(CM_COMPILE_COLLISION, "compile");

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->registerWindowHandler(this);

  if (mPanelVisible && panelClient)
    panelClient->showPropPanel(true);

  return true;
}


//==============================================================================
bool CollisionPlugin::end()
{
  mPanelVisible = panelClient && panelClient->isVisible();

  if (mPanelVisible)
    panelClient->showPropPanel(false);

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->unregisterWindowHandler(this);
  return true;
}

//==============================================================================

void *CollisionPlugin::onWmCreateWindow(int type)
{
  switch (type)
  {
    case PROPBAR_EDITOR_WTYPE:
    {
      if (!panelClient || panelClient->getPanelWindow())
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


bool CollisionPlugin::onWmDestroyWindow(void *window)
{
  if (panelClient && window == panelClient->getPanelWindow())
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

void CollisionPlugin::updateImgui()
{
  if (DAGORED2->curPlugin() == this)
  {
    PropPanel::PanelWindowPropertyControl *panelWindow = panelClient ? panelClient->getPanelWindow() : nullptr;
    if (panelWindow)
    {
      bool open = true;
      if (auto window = IDaEditor3Engine::ScopedImguiWindow(*panelWindow, &open); window.canSubmitContents())
        panelWindow->updateImgui();

      if (!open && panelClient)
      {
        panelClient->showPropPanel(false);
        EDITORCORE->managePropPanels();
      }
    }
  }
}

//==============================================================================

void *CollisionPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  RETURN_INTERFACE(huid, ICollision);
  return NULL;
}


//==============================================================================
void CollisionPlugin::setVisible(bool vis)
{
  isVisible = vis;
  DAGORED2->invalidateViewportCache();
}

void CollisionPlugin::renderObjects()
{
  if (showDags)
  {
    makeDagPreviewCollision(false);
    begin_draw_cached_debug_lines();
    collisionpreview::drawCollisionPreview(collision, TMatrix::IDENT, E3DCOLOR(255, 0, 128));
    end_draw_cached_debug_lines();
  }
}

bool CollisionPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_PostRenderObjects && showVcm && isVisible && DAGORED2->curPlugin() == this)
  {
    if (showGameFrt)
      makeGameFrtPreviewCollision(false);
    const CollisionResource *coll = showGameFrt ? gameStaticColl.get() : DagorPhys::getClipCollision();
    IGenViewportWnd *vp = DAGORED2->getRenderViewport();
    if (coll && vp)
    {
      TMatrix cameraTm;
      vp->getCameraTransform(cameraTm);
      bbox3f box;
      v_bbox3_init_by_bsph(box, v_ldu(&cameraTm.getcol(3).x), v_splats(vcmRad));
      ::draw_collres_debug(*coll, box, CollisionNode::PHYS_COLLIDABLE, showVcmWire);
    }
  }
  return false;
}

//===============================================================================
bool CollisionPlugin::recreatePanel()
{
  if (!panelClient)
    panelClient = new (uimem) CollisionPropPanelClient(this, rtStg);

  panelClient->showPropPanel(!panelClient->isVisible());

  return true;
}


//==============================================================================
void CollisionPlugin::importClipDag()
{
  String location(::de_get_sdk_dir());
  if (location == "")
    location = sgg::get_exe_path_full();

  String dagName = wingw::file_open_dlg(NULL, "Add collision from DAG", "DAG files|*.dag|All files|*.*", "dag", location);

  if (!dagName.length())
    return;

  dagCollReady = false;
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

static bool addBoxCollision(ICollisionDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
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
static bool addSphereCollision(ICollisionDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
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
static bool addCapsuleCollision(ICollisionDumpBuilder *rt, Mesh &m, Node &n, const char *phmat_str)
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
void CollisionPlugin::addClipNode(Node &n, ICollisionDumpBuilder *rt, const char *dag_fn, bool report_stats)
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

        if (rt->supportMask & rt->SUPPORT_CAPSULE)
          rt->addCapsule(c, -1, 1, (pmid == PHYSMAT_DEFAULT) ? NULL : PhysMat::getMaterial(pmid).name.str());
        else
          DAEDITOR3.conWarning("node <%s> is a capsule, which this cook cannot make: it is left out", n.name.str());
        n.setobj(NULL);

        return;
      }
      else
      {
        int pmid = PHYSMAT_DEFAULT;
        bool pmid_force = false;
        const char *phmat_str = NULL;
        bool add_not_mesh = false;

        if (!n.script.empty())
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
        }

        if (!add_not_mesh)
        {
          if (report_stats)
            DAEDITOR3.conNote("collision mesh(%5d faces, %4d verts) %s(%s) at %@ pmid=%d[%s] %s", //
              mh.mesh->face.size(), mh.mesh->vert.size(), n.name, dag_fn, n.wtm.getcol(3),        //
              pmid, PhysMat::getMaterial(pmid).name, pmid_force ? "(forced)" : "");
          rt->addMesh(*mh.mesh, n.mat, n.wtm, 0, 1, pmid, pmid_force);
        }
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    if (n.child[i])
      addClipNode(*n.child[i], rt, dag_fn, report_stats);
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

    if (!n.script.empty())
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

void CollisionPlugin::makeDagPreviewCollision(bool force_remake)
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


// The decompressor reports the size it wrote, so its destination only has to swallow the bytes.
struct NullSaveCB final : public IGenSave
{
  void write(const void *, int) override {}
  int tell() override { return 0; }
  void seekto(int) override {}
  void seektoend(int) override {}
  const char *getTargetName() override { return "<size counter>"; }
  void beginBlock() override {}
  void endBlock(unsigned) override {}
  int getBlockLevel() override { return 0; }
  void flush() override {}
};

void CollisionPlugin::makeGameFrtPreviewCollision(bool force_remake)
{
  if (!force_remake && gameStaticColl)
    return;
  gameStaticColl = nullptr;
  String clipFname;
  getGameClipPath(clipFname, _MAKE4C('PC'));
  FullFileLoadCB fl(clipFname);
  if (!fl.fileHandle)
  {
    debug("Unable to find file %s", clipFname.str());
    return;
  }
  // a truncated or empty clip throws on the first read; the preview must survive that
  DAGOR_TRY { gameStaticColl = new CollisionResource(fl, -1, "game static collision"); }
  DAGOR_CATCH(IGenLoad::LoadException)
  {
    debug("File %s is not a readable static collision", clipFname.str());
    gameStaticColl = nullptr;
    return;
  }
  if (gameStaticColl->getAllNodes().empty())
  {
    debug("File %s holds no static collision", clipFname.str());
    gameStaticColl = nullptr;
  }
}


void CollisionPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) { onPluginMenuClickInternal(pcb_id, panel); }
bool CollisionPlugin::onPluginMenuClick(unsigned id) { return onPluginMenuClickInternal(id, nullptr); }

//==============================================================================

bool CollisionPlugin::onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel)
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

    case CM_COMPILE_COLLISION:
      if (!compileEditClip())
        wingw::message_box(wingw::MBS_EXCL, "Compilation failed", "Failed to compile collision");
      else
        DAGORED2->invalidateViewportCache();
      return true;

    case CM_COMPILE_GAME_COLLISION:
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
void CollisionPlugin::getClipPath(String &path, unsigned target_code, const char *suffix) const
{
  char name_prefix[64] = {"game_clip"};

  uint64_t tc_storage = 0;
  if (target_code != _MAKE4C('PC'))
    sprintf(name_prefix + strlen(name_prefix), "-%s", mkbindump::get_target_str(target_code, tc_storage));

  strcat(name_prefix, suffix);
  path = DAGORED2->getPluginFilePath(this, name_prefix);
}


//==============================================================================
void CollisionPlugin::getGameClipPath(String &path, unsigned target_code) const { getClipPath(path, target_code, ".scol.bin"); }


//==============================================================================
void CollisionPlugin::getWaterClipPath(String &path, unsigned target_code) const { getClipPath(path, target_code, ".wcol.bin"); }


//==============================================================================
void CollisionPlugin::getCollisionFiles(Tab<String> &files, unsigned target_code) const
{
  String path;
  getGameClipPath(path, target_code);

  files.push_back(path);

  String waterPath;
  getWaterClipPath(waterPath, target_code);
  if (::dd_file_exist(waterPath)) // a level without water cooks no water stream
    files.push_back(waterPath);
}


//==============================================================================
bool CollisionPlugin::validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params)
{
  getPhysMatPath(&rep);

  if (!compileGameClip(params, target))
  {
    rep.addMessage(ILogWriter::ERROR, "Game collision was not compiled");
    return false;
  }

  String clipFnameGame;
  getGameClipPath(clipFnameGame, target);

  if (!::dd_file_exist(clipFnameGame))
  {
    rep.addMessage(ILogWriter::ERROR, "Couldn't load game collision from \"%s\"\n", clipFnameGame);

    logerr("Can't open game collision from \"%s\"", clipFnameGame.str());
    return false;
  }

  return true;
}

//==============================================================================
bool CollisionPlugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *)
{
  Tab<String> files(tmpmem);
  getCollisionFiles(files, cwr.getTarget());

  {
    file_ptr_t fp = df_open(files[0], DF_READ);
    if (!fp)
      return false;

    if (df_length(fp) > 0)
    {
      cwr.beginTaggedBlock(_MAKE4C('SCol'));
      copy_file_to_stream(fp, cwr.getRawWriter(), df_length(fp));
      cwr.endBlock();
    }
    ::df_close(fp);
  }

  // the second file is the water stream, listed only when the cook wrote one
  if (files.size() > 1)
  {
    file_ptr_t fp = df_open(files[1], DF_READ);
    if (!fp)
      return false;

    if (df_length(fp) > 0)
    {
      cwr.beginTaggedBlock(_MAKE4C('WCol'));
      copy_file_to_stream(fp, cwr.getRawWriter(), df_length(fp));
      cwr.endBlock();
    }
    ::df_close(fp);
  }

  return true;
}


//==============================================================================
void CollisionPlugin::fillExportPanel(PropPanel::ContainerPropertyControl &params)
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
bool CollisionPlugin::checkMetrics(const DataBlock &metrics_blk)
{
  CoolConsole &con = DAGORED2->getConsole();

  Tab<String> files(tmpmem);
  getCollisionFiles(files, _MAKE4C('PC')); //== FIXME: should check metrics for current platform

  const int maxSize = metrics_blk.getReal("max_size", 0) * 1024 * 1024;
  int totalSize = 0;
  int packedSize = 0;

  for (int i = 0; i < files.size(); ++i)
  {
    file_ptr_t f = ::df_open(files[i], DF_READ);

    if (f)
    {
      // debug("file %s: %d", files[i].str(), df_length(f));
      if (df_length(f) >= 8)
      {
        // the metric is the unpacked size, and a ZSTD block keeps none: decode to count it
        LFileGeneralLoadCB crd(f);
        crd.readInt();
        unsigned compr = btag_compr::NONE;
        const int blockLen = crd.beginBlock(&compr);
        int sz = blockLen;
        if (compr == btag_compr::ZSTD)
        {
          NullSaveCB sink;
          sz = (int)zstd_stream_decompress_data(sink, crd, blockLen);
        }
        totalSize += sz;
        packedSize += df_length(f);
      }
      else
        totalSize += df_length(f);
      ::df_close(f);
    }
  }

  if (totalSize > maxSize)
  {
    DAEDITOR3.conError("Metrics validation failed: collision file(s) size %s (%i bytes) more "
                       "than maximum %s (%i bytes), packed=%dK",
      ::bytes_to_mb(totalSize), totalSize, ::bytes_to_mb(maxSize), maxSize, packedSize >> 10);

    return false;
  }

  return true;
}


//==============================================================================
void CollisionPlugin::clearObjects()
{
  showVcm = true;
  showDags = false;
  showVcmWire = true;
  showGameFrt = false;
  vcmRad = 50.0;

  clipDag.clear();
  editClipPlugins.reset();
  disableCustomColliders.reset();
  disabledGamePlugins.reset();
  collision.clear();
  rtStg.defaults();
  gameStaticColl = nullptr;

  DagorPhys::close_collision();
  collisionReady = false;
  dagCollReady = false;
}


//=============================================================================
bool CollisionPlugin::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
bool CollisionPlugin::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
bool CollisionPlugin::handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
bool CollisionPlugin::handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
void CollisionPlugin::handleViewChange(IGenViewportWnd *wnd) {}


//==============================================================================
void CollisionPlugin::handleViewportPaint(IGenViewportWnd *wnd) {}


//==============================================================================
bool CollisionPlugin::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }


//==============================================================================
void CollisionPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  local_data.setReal("vcm_rad", vcmRad);
  local_data.setBool("showVcm", showVcm);
  local_data.setBool("showDags", showDags);
  local_data.setBool("showGameFrt", showGameFrt);

  int i;
  for (i = 0; i < clipDag.size(); ++i)
    blk.addStr("dag", clipDag[i]);

  blk.setPoint3("rt_leafSize", rtStg.leafSize);
  blk.setInt("rt_levels", rtStg.levels);

  DataBlock *plugBlk = blk.addBlock("EditClipPlugNames");

  iterate_names(editClipPlugins, [&](int, const char *name) { plugBlk->addStr("name", name); });


  DataBlock *disableBlk = blk.addNewBlock("disabled_game_collision");

  if (disableBlk)
    iterate_names(disabledGamePlugins, [&](int, const char *name) { disableBlk->addStr("name", name); });

  clipDagNew.clear();
}


//==============================================================================
void CollisionPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  vcmRad = local_data.getReal("vcm_rad", get_collres_debug_draw_radius());
  showVcm = local_data.getBool("showVcm", true);
  showDags = local_data.getBool("showDags", false);
  showGameFrt = local_data.getBool("showGameFrt", false);

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
  rtStg.leafSize = blk.getPoint3("rt_leafSize", rtStg.leafSize);
  rtStg.levels = blk.getInt("rt_levels", rtStg.levels);

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

  initCollision(false);
  setVisible(isVisible);
  dagCollReady = false;
}


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


static void addMeshCollision(ICollisionDumpBuilder *rt, StaticGeometryNode &n, bool for_game, bool need_kill, bool report_stats)
{
  // remove degenerate triangles
  Mesh mesh = n.mesh->mesh;
  const TMatrix *wtm = &n.wtm;
  Point3 pos = n.wtm.getcol(3);
  if (report_stats && mesh.vert.size() && pos.lengthSq() < 1e-6f)
  {
    pos.set(0, 0, 0);
    for (const auto &p : mesh.vert)
      pos += n.wtm * p;
    pos /= mesh.vert.size();
  }

  if (need_kill)
  {
    for (int i = 0; i < mesh.vert.size(); ++i)
      mesh.vert[i] = n.wtm * mesh.vert[i];
    wtm = &TMatrix::IDENT;
    mesh.kill_unused_verts(0.0005f);
    int nPrevFaces = mesh.face.size();
    mesh.kill_sliver_faces(1e-12f, /* 1/1mm */ 1000.f);
    if (mesh.face.size() != nPrevFaces)
      mesh.kill_unused_verts();
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
  {
    int pmid = PHYSMAT_DEFAULT;
    if (const char *nm = n.script.getStr("phmat", nullptr); nm && *nm)
    {
      pmid = PhysMat::getMaterialId(nm);
      if (pmid == PHYSMAT_DEFAULT)
        logerr("material %s is not defined in physmat_index!", nm);
    }

    if (report_stats)
      DAEDITOR3.conNote("collision mesh(%5d faces, %4d verts) %s at %@ pmid=%d[%s]", //
        mesh.face.size(), mesh.vert.size(), n.name, pos, pmid, PhysMat::getMaterial(pmid).name);
    rt->addMesh(mesh, NULL, *wtm, 0, 1, pmid, false, n.mesh);
  }
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
static void addBoxCollision(ICollisionDumpBuilder *rt, StaticGeometryNode &n, bool report_stats)
{
  if (!(rt->supportMask & rt->SUPPORT_BOX))
  {
    addMeshCollision(rt, n, false, false, report_stats);
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
static void addSphereCollision(ICollisionDumpBuilder *rt, StaticGeometryNode &n, bool report_stats)
{
  if (!(rt->supportMask & rt->SUPPORT_SPHERE))
  {
    addMeshCollision(rt, n, false, false, report_stats);
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
static void addCapsuleCollision(ICollisionDumpBuilder *rt, StaticGeometryNode &n, bool report_stats)
{
  if (!(rt->supportMask & rt->SUPPORT_CAPSULE))
  {
    addMeshCollision(rt, n, false, false, report_stats);
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
static void addConvexCollision(ICollisionDumpBuilder *rt, StaticGeometryNode &n, bool report_stats)
{
  G_UNUSED(report_stats);
  // the box, sphere and capsule paths fall back to the mesh; this one drops the node
  if (!(rt->supportMask & rt->SUPPORT_CONVEX))
  {
    DAEDITOR3.conWarning("node <%s> asks for convex collision, which this cook cannot make: it is left out", n.name.str());
    return;
  }

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
    if (lenSq < 0.00001f)
      erase_items(mesh.face, ntri--, 1);
  }
  rt->addConvexHull(mesh, NULL, n.wtm, 0, 1, PHYSMAT_DEFAULT, false, n.mesh);
}

//==============================================================================
bool CollisionPlugin::compileCollision(bool for_game, Tab<int> &plugs, unsigned target_code)
{
  Tab<String> clipPlug(tmpmem);

  ICollisionDumpBuilder *rt;
  if (!PhysMat::getMaterials().size())
  {
    DAEDITOR3.conError("No phys materials found - collision expoprt aborted.\n"
                       "Check for properly configured physmat.blk path in %s",
      DAGORED2->getWorkspace().getAppBlkShortName());
    return false;
  }

  if (for_game)
  {
    // the same gate the asset cook of every other collision reads
    DataBlock appBlk(DAGORED2->getWorkspace().getAppBlkPath());
    const bool joltFail = appBlk.getBlockByNameEx("assets")
                            ->getBlockByNameEx("build")
                            ->getBlockByNameEx("collision")
                            ->getBool("joltDegenerativeTriFailExport", false);
    rt = ::create_static_collision_builder(CollisionPlugin::getPhysMatPath(), joltFail);
  }
  else
    rt = ::create_editor_collision_builder("editor clip");

  if (!rt)
  {
    DEBUG_CTX("Cannot create collision dump builder");
    return false;
  }

  bool report_stats = for_game;

  int i;

  if (report_stats)
    DAEDITOR3.conNote("--- start building collision geom");
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

      addClipNode(*sc.root, rt, dagPath, report_stats);
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

  DataBlock app_blk(DAGORED2->getWorkspace().getAppBlkPath());
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

    if (report_stats)
      DAEDITOR3.conNote("----- collision geom from: %s (%d nodes)", plugin->getMenuCommandName(), geoCont.nodes.size());
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
        addMeshCollision(rt, n, for_game, need_kill, report_stats);
      else if (stricmp(primitive, "box") == 0)
        addBoxCollision(rt, n, report_stats);
      else if (stricmp(primitive, "sphere") == 0)
        addSphereCollision(rt, n, report_stats);
      else if (stricmp(primitive, "capsule") == 0)
        addCapsuleCollision(rt, n, report_stats);
      else if (stricmp(primitive, "convex") == 0)
        addConvexCollision(rt, n, report_stats);
      else
        DAEDITOR3.conWarning("unknown collision type: <%s>, ignored", primitive);
    }
  }

  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, prevSubtype);

  String clipFname;

  if (for_game)
    getGameClipPath(clipFname, target_code);
  else
    getEditorClipPath(clipFname);

  if (report_stats)
    DAEDITOR3.conNote("--- finished gathering the collision geom");

  if (!for_game)
    DagorPhys::close_collision(); // the cook rewrites the clip this holds

  bool ok, wroteAny;
  {
    FullFileSaveCB cwr(clipFname);
    if (!cwr.fileHandle)
      DAEDITOR3.conError("Cannot open %s to write the collision", clipFname.str());
    ok = cwr.fileHandle && rt->finishAndWrite(cwr);
    wroteAny = ok && cwr.tell() > 0;
  }
  if (!for_game)
    ::dd_erase(DAGORED2->getPluginFilePath(this, "editor_clip.frt.bin")); // the tracer-era clip, which nothing reads
  if (!ok || (!wroteAny && !for_game))
    ::dd_erase(clipFname);
  if (!ok)
  {
    debug("Errors while compile collision");
    rt->destroy();
    return false;
  }

  if (for_game)
  {
    // The water stream is a file of its own, and a cook without water must leave none behind. A
    // file that will not open is a failed cook, not a level without water.
    String waterFname;
    getWaterClipPath(waterFname, target_code);
    bool hasWater = false;
    {
      FullFileSaveCB wcwr(waterFname);
      if (!wcwr.fileHandle)
      {
        logerr("Can't open \"%s\" to write the water collision", waterFname.str());
        rt->destroy();
        return false;
      }
      hasWater = rt->finishAndWriteWater(wcwr);
    }
    if (!hasWater)
      ::dd_erase(waterFname);
  }

  rt->destroy();

  clear_and_shrink(csgBox);

  // a cook that wrote nothing erased clipFname, so a reload would only report it missing
  if (!for_game && wroteAny)
    return initCollision(false);

  return true;
}


//==============================================================================
bool CollisionPlugin::initCollision(bool for_game)
{
  String clipFname;

  if (for_game)
  {
    debug("initCollision(for_game): the game clip is not loaded by the editor, nothing installed");
    return false;
  }
  getEditorClipPath(clipFname);

  DagorPhys::close_collision();

  if (IDagorEd2Engine::get())
  {
    if (!panelClient)
      panelClient = new (uimem) CollisionPropPanelClient(this, rtStg);
  }

  FullFileLoadCB fl((const char *)clipFname);

  debug("Try to load collision from file %s", (const char *)clipFname);

  if (!fl.fileHandle)
  {
    DAEDITOR3.conWarning("No %s: compile the collision to trace against it", (const char *)clipFname);
    return false;
  }

  const bool success = DagorPhys::load_binary_collision(fl);
  if (success)
    debug("Collision successfully loaded");
  else
    DAEDITOR3.conError("Unable to load %s: compile the collision again", (const char *)clipFname);

  return success;
}

//==============================================================================
bool CollisionPlugin::compileEditClip()
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

  return compileCollision(false, plugs, _MAKE4C('PC'));
}

void CollisionPlugin::prepareDAGcollision()
{
  dagCollReady = true;
  dagColl = nullptr;

  ICollisionDumpBuilder *rt = ::create_editor_collision_builder("DAG collision");

  if (!rt)
  {
    DAEDITOR3.conError("Cannot create the DAG collision builder");
    return;
  }

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

  dagColl = rt->finishAndBuild();
  if (dagColl && dagColl->getAllNodes().empty())
  {
    dagColl = nullptr;
    DAEDITOR3.conError("DAG collision built an empty resource");
  }

  rt->destroy();
}

//==============================================================================
bool CollisionPlugin::compileGameClip(PropPanel::ContainerPropertyControl *panel, unsigned target_code)
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

  return compileCollision(true, gamePlugs, target_code);
}


//==============================================================================
void CollisionPlugin::clearDags()
{
  String dagPath;

  for (int i = 0; i < clipDag.size(); ++i)
  {
    getDAGPath(dagPath, clipDag[i]);
    remove((const char *)dagPath);
  }

  clipDag.clear();
  dagCollReady = false;
}


//==============================================================================
bool CollisionPlugin::compileCollisionWithDialog(bool for_game)
{
  if (!for_game)
    return compileEditClip();

  return false;
}


//==============================================================================
void CollisionPlugin::manageCustomColliders()
{
  iterate_names(disableCustomColliders, [&](int, const char *name) { ::disable_custom_collider(name); });
}


//==============================================================================
bool CollisionPlugin::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (!dagCollReady)
    prepareDAGcollision();
  if (!dagColl)
    return false;
  return trace_via_unit_dir(dir, maxt,
    [&](const Point3 &unit_dir, real &t) { return dagColl->traceRay(TMatrix::IDENT, p, unit_dir, t, norm); });
}

//==============================================================================
bool CollisionPlugin::isValidPhysMatBlk(const char *file, DataBlock &blk, ILogWriter *rep, DataBlock **mat_blk, DataBlock **def_blk)
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
