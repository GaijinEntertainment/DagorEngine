#include "plugIn.h"

#include <oldEditor/de_interface.h>
#include <oldEditor/de_cm.h>
#include <oldEditor/de_util.h>
#include <de3_lightService.h>
#include <de3_occludersFromGeom.h>
#include <de3_entityFilter.h>

#include <dllPluginCore/core.h>

#include <sepGui/wndPublic.h>

#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/dagUtil.h>

#include <libTools/util/de_TextureName.h>

#include <libTools/dtx/dtx.h>

#include <libTools/staticGeom/geomObject.h>

#include <coolConsole/coolConsole.h>

#include <shaders/dag_shaders.h>
// #include <texConverter/TextureConverterDlg.h>
#include <debug/dag_debug.h>

#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>


#define DAG_FILENAME "single.dag"


static int dagFileNameId = -1;
static unsigned collisionSubtypeMask = 0, rendEntGeomMask = 0;


enum
{
  CM_TOOL = CM_PLUGIN_BASE + 0,
  CM_DAG_OPTIMIZE,
  CM_IMPORT_WITHOUT_TEX,
  CM_SAVE_GEOMETRY,
  CM_RESET_GEOMETRY,
};


StaticGeometryPlugin::StaticGeometryPlugin() : doResetGeometry(false), geom(NULL), loadingFailed(false)
{
  objsVisible = true;
  loaded = false;
  collisionBuilt = false;

  geom = dagGeom->newGeomObject(midmem);
  geom->setUseNodeVisRange(true);
  collisionSubtypeMask = 1 << DAEDITOR3.registerEntitySubTypeId("collision");
  rendEntGeomMask = 1 << DAEDITOR3.registerEntitySubTypeId("rend_ent_geom");
}


//==============================================================================
StaticGeometryPlugin::~StaticGeometryPlugin() { dagGeom->deleteGeomObject(geom); }


//==============================================================================
String StaticGeometryPlugin::getPluginFilePath(const char *fileName) { return DAGORED2->getPluginFilePath(this, fileName); }


//==============================================================================
void StaticGeometryPlugin::setVisible(bool vis)
{
  objsVisible = vis;

  if (objsVisible && !loaded)
    loadGeometry();
}


//==============================================================================
bool StaticGeometryPlugin::begin(int toolbar_id, unsigned menu_id)
{
  // menu

  IMenu *mainMenu = DAGORED2->getWndManager()->getMainMenu();

  mainMenu->addItem(menu_id, CM_IMPORT_WITHOUT_TEX, "Import DAG...\tCtrl+O");
  mainMenu->addItem(menu_id, CM_DAG_OPTIMIZE, "Optimize DAG");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_RESET_GEOMETRY, "Erase geometry");

  // toolbar
  PropertyContainerControlBase *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  toolbar->setEventHandler(this);

  PropertyContainerControlBase *tool = toolbar->createToolbarPanel(CM_TOOL, "");

  tool->createButton(CM_IMPORT_WITHOUT_TEX, "Import DAG (Ctrl+O)");
  tool->setButtonPictures(CM_IMPORT_WITHOUT_TEX, "import_dag");

  tool->createButton(CM_DAG_OPTIMIZE, "Optimize DAG");
  tool->setButtonPictures(CM_DAG_OPTIMIZE, "dag_optimize");
  tool->createSeparator(0);
  tool->createButton(CM_RESET_GEOMETRY, "Erase geometry");
  tool->setButtonPictures(CM_RESET_GEOMETRY, "clear");

  return true;
}


//==============================================================================
void StaticGeometryPlugin::clearObjects()
{
  dagGeom->geomObjectClear(*geom);
  loaded = false;
}


//==============================================================================
void StaticGeometryPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  local_data.setStr("lastImported", ::make_path_relative(lastImportFilename, DAGORED2->getSdkDir()));

  if (doResetGeometry)
    resetGeometry();
}


//==============================================================================
void StaticGeometryPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  lastImportFilename = local_data.getStr("lastImported", lastImportFilename);
  lastImportFilename = ::find_in_base_smart(lastImportFilename, DAGORED2->getSdkDir(), lastImportFilename);
}


//==============================================================================
bool StaticGeometryPlugin::getSelectionBox(BBox3 &bounds) const
{
  if (!loaded)
    return false;

  bounds = dagGeom->geomObjectGetBoundBox(*geom);
  return true;
}


//==============================================================================
void StaticGeometryPlugin::renderGeometry(Stage stage)
{
  if (!objsVisible)
    return;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (!(st_mask & rendEntGeomMask))
    return;

  switch (stage)
  {
    case STG_RENDER_STATIC_OPAQUE:
    case STG_RENDER_TO_CLIPMAP: dagGeom->geomObjectRender(*geom); break;

    case STG_RENDER_STATIC_TRANS: dagGeom->geomObjectRenderTrans(*geom); break;

    case STG_RENDER_SHADOWS:
      dagGeom->geomObjectRender(*geom);
      // dagGeom->geomObjectRenderTrans(*geom);
      break;
  }
}

void StaticGeometryPlugin::renderTransObjects()
{
  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (st_mask & collisionSubtypeMask)
  {
    if (loaded && !collisionBuilt)
    {
      DAEDITOR3.conNote("Building StaticGeometry collision preview");
      collisionpreview::assembleCollisionFromNodes(collision, geom->getGeometryContainer()->nodes, false);
      collisionBuilt = true;
    }

    dagRender->startLinesRender();
    collisionpreview::drawCollisionPreview(collision, TMatrix::IDENT, E3DCOLOR(0, 255, 128));
    dagRender->endLinesRender();
  }
}

//==============================================================================
void *StaticGeometryPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IGatherStaticGeometry);
  RETURN_INTERFACE(huid, IOccluderGeomProvider);
  RETURN_INTERFACE(huid, IRenderingService);
  RETURN_INTERFACE(huid, ILightingChangeClient);
  return NULL;
}


//==============================================================================
String StaticGeometryPlugin::getImportDagPath() const
{
  const String openPath = ::find_in_base_smart(lastImportFilename, DAGORED2->getSdkDir(), ::make_full_path(DAGORED2->getSdkDir(), ""));

  return wingw::file_open_dlg(DAGORED2->getWndManager()->getMainWindow(), "Open DAG file",
    "DAG files (*.dag)|*.dag|All files (*.*)|*.*", "dag", openPath);
}


//==============================================================================
void StaticGeometryPlugin::importDag()
{
  String filename(getImportDagPath());

  if (filename.length())
  {
    resetGeometry();

    StaticGeometryContainer *gc = dagGeom->newStaticGeometryContainer(tmpmem);
    dagGeom->staticGeometryContainerImportDag(*gc, filename, getPluginFilePath(DAG_FILENAME));
    dagGeom->deleteStaticGeometryContainer(gc);

    lastImportFilename = filename;

    if (objsVisible)
    {
      loadGeometry();
      DAGORED2->repaint();
    }
  }
}


//==============================================================================

void StaticGeometryPlugin::onClick(int pcb_id, PropPanel2 *panel) { onPluginMenuClick(pcb_id); }


bool StaticGeometryPlugin::onPluginMenuClick(unsigned id)
{
  switch (id)
  {
    case CM_IMPORT_WITHOUT_TEX: importDag(); return true;

    case CM_DAG_OPTIMIZE:
    {
      CoolConsole &con = DAGORED2->getConsole();
      const String dagFileName(getPluginFilePath(DAG_FILENAME));
      loadGeometry();
      if (loaded)
      {
        con.setActionDesc(String(128, "Optimize DAG '%s'...", dagFileName.str()));
        dagGeom->geomObjectSaveToDag(*geom, dagFileName);
      }
      else
        con.addMessage(ILogWriter::NOTE, "Unable to load SGeometry DAG, optimize aborted");

      dagConsole->endProgress(con);
      return true;
    }

    case CM_RESET_GEOMETRY:
      if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Erase geometry", "Erase all SGeomerty?") == wingw::MB_ID_YES)
      {
        dagGeom->geomObjectClear(*geom);
        doResetGeometry = true;
        DAGORED2->repaint();
      }
      return true;
  }

  return false;
}


//==============================================================================
void StaticGeometryPlugin::resetGeometry()
{
  clearObjects();

  ::dd_erase(getPluginFilePath(DAG_FILENAME));

  alefind_t ff;
  bool ok;

  for (ok = ::dd_find_first(getPluginFilePath("*.dtx"), DA_FILE, &ff); ok; ok = ::dd_find_next(&ff))
    ::dd_erase(getPluginFilePath(ff.name));

  ::dd_find_close(&ff);

  for (ok = ::dd_find_first(getPluginFilePath("*.dds"), DA_FILE, &ff); ok; ok = ::dd_find_next(&ff))
    ::dd_erase(getPluginFilePath(ff.name));

  ::dd_find_close(&ff);

  DAGORED2->repaint();
  doResetGeometry = false;
  loaded = false;
}


void StaticGeometryPlugin::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  // Ctrl
  if (modif & wingw::M_CTRL)
  {
    switch (vk)
    {
      case ('O'): onPluginMenuClick(CM_IMPORT_WITHOUT_TEX); break;
    }
  }
}


void StaticGeometryPlugin::handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) {}


bool StaticGeometryPlugin::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


bool StaticGeometryPlugin::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


bool StaticGeometryPlugin::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


bool StaticGeometryPlugin::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


bool StaticGeometryPlugin::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


bool StaticGeometryPlugin::handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
bool StaticGeometryPlugin::handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}


//==============================================================================
bool StaticGeometryPlugin::handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }


//==============================================================================
bool StaticGeometryPlugin::handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }


//==============================================================================
void StaticGeometryPlugin::handleViewportPaint(IGenViewportWnd *wnd) {}


//==============================================================================
void StaticGeometryPlugin::handleViewChange(IGenViewportWnd *wnd) {}


//==============================================================================
void StaticGeometryPlugin::gatherStaticVisualGeometry(StaticGeometryContainer &container)
{
  if (!loaded)
    loadGeometry();

  const StaticGeometryContainer &g = dagGeom->geomObjectGetGeometryContainer(*geom);

  for (int i = 0; i < g.nodes.size(); ++i)
    if (g.nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE)
      container.addNode(dagGeom->newStaticGeometryNode(*g.nodes[i], tmpmem));
}


//==============================================================================
void StaticGeometryPlugin::gatherStaticEnviGeometry(StaticGeometryContainer &container) {}


//==============================================================================
void StaticGeometryPlugin::gatherStaticCollisionGeomGame(StaticGeometryContainer &container)
{
  if (!loaded)
    loadGeometry();

  const StaticGeometryContainer &g = dagGeom->geomObjectGetGeometryContainer(*geom);

  for (int i = 0; i < g.nodes.size(); ++i)
    if (g.nodes[i]->flags & StaticGeometryNode::FLG_COLLIDABLE)
      container.addNode(dagGeom->newStaticGeometryNode(*g.nodes[i], tmpmem));
}


//==============================================================================
void StaticGeometryPlugin::gatherStaticCollisionGeomEditor(StaticGeometryContainer &container)
{
  gatherStaticCollisionGeomGame(container);
}

void StaticGeometryPlugin::gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<IOccluderGeomProvider::Quad> &occl_quads)
{
  Tab<TMatrix> ob(tmpmem);
  Tab<Point3> oq(tmpmem);

  getOcclFromGeom(dagGeom->geomObjectGetGeometryContainer(*geom), "StaticGeom", ob, oq);
  append_items(occl_boxes, ob.size(), ob.data());

  int base = append_items(occl_quads, oq.size() / 4);
  for (int i = 0; i < oq.size() / 4; i++)
    mem_copy_to(make_span_const(oq).subspan(i * 4, 4), occl_quads[base + i].v);
}


//==============================================================================
void StaticGeometryPlugin::onFileChanged(int file_name_id)
{
  if (file_name_id == dagFileNameId)
  {
    loadGeometry();
    DAGORED2->invalidateViewportCache();
  }
}

void StaticGeometryPlugin::loadGeometry()
{
  dagGeom->geomObjectClear(*geom);
  collision.clear();

  String fileName(getPluginFilePath(DAG_FILENAME));

  if (!::dd_file_exist(fileName))
  {
    if (!loadingFailed)
      debug("File %s not exists", (const char *)fileName);
    loadingFailed = true;
    return;
  }

  DAEDITOR3.conNote("Loading StaticGeometry data");
  dd_simplify_fname_c(fileName);
  loaded = dagGeom->geomObjectLoadFromDag(*geom, fileName, &DAGORED2->getConsole());
  collisionBuilt = false;

  if (loaded && (dagFileNameId == -1))
  {
    IFileChangeTracker *tracker = DAGORED2->queryEditorInterface<IFileChangeTracker>();
    if (tracker)
    {
      dagFileNameId = tracker->addFileChangeTracking(fileName);
      tracker->subscribeUpdateNotify(dagFileNameId, this);
    }
  }

  loadingFailed = !loaded;
}


//==============================================================================
void addFileName(const char *fileName, Tab<String> &list)
{
  const String fullName = ::make_full_path(DAGORED2->getGameDir(), fileName);

  if (::dd_file_exist(fullName))
    list.push_back(fullName);
}


bool StaticGeometryPlugin::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (loadingFailed)
    return false;

  if (!loaded)
  {
    loadGeometry();

    if (loadingFailed)
      return false;
  }

  if (maxt <= 0)
    return false;

  return dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm);
}


bool StaticGeometryPlugin::shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt)
{
  if (maxt <= 0)
    return false;

  if (!loaded)
  {
    loadGeometry();

    if (loadingFailed)
      return false;
  }

  return dagGeom->geomObjectShadowRayHitTest(*geom, p + dir * maxt, -dir, maxt);
}


void StaticGeometryPlugin::registered() { DAGORED2->registerCustomCollider(this); }


void StaticGeometryPlugin::unregistered()
{
  IFileChangeTracker *tracker = DAGORED2->queryEditorInterface<IFileChangeTracker>();
  if (tracker && (dagFileNameId != -1))
    tracker->unsubscribeUpdateNotify(dagFileNameId, this);

  DAGORED2->unregisterCustomCollider(this);
}


// back-loop
void set_cached_debug_lines_wtm(const TMatrix &tm) { dagRender->setLinesTm(tm); }

void draw_cached_debug_box(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color)
{
  dagRender->renderBox(p, ax, ay, az, color);
}

void draw_cached_debug_line(const Point3 &p0, const Point3 &p1, E3DCOLOR color) { dagRender->renderLine(p0, p1, color); }

void draw_cached_debug_sphere(const Point3 &center, real rad, E3DCOLOR col, int segs)
{
  dagRender->renderSphere(center, rad, col, segs);
}

void draw_cached_debug_capsule_w(const Capsule &cap, E3DCOLOR c) { dagRender->renderCapsuleW(cap, c); }
