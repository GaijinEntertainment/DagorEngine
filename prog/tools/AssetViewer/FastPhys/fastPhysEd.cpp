#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetHlp.h>
#include <assets/assetExporter.h>

#include <animChar/dag_animCharacter2.h>

#include <libTools/fastPhysData/fp_edpoint.h>
#include <libTools/fastPhysData/fp_edclipper.h>
#include <libTools/fastPhysData/fp_data.h>
#include <libTools/util/strUtil.h>

#include <EditorCore/ec_cm.h>
#include <libTools/util/makeBindump.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_roDataBlock.h>
#include <coolConsole/coolConsole.h>

#include <debug/dag_debug3d.h>

#include <libTools/fastPhysData/fp_edbone.h>

#include <propPanel2/comWnd/list_dialog.h>

#include <winGuiWrapper/wgw_dialogs.h>


#include "../av_appwnd.h"
#include "../av_cm.h"

#include "fastPhys.h"
#include "fastPhysEd.h"
#include "fastPhysObject.h"
#include "fastPhysChar.h"
#include <math/dag_geomNodeUtils.h>

//------------------------------------------------------------------

enum
{
  CM_SHOW_CHAR = CM_PLUGIN_BASE + 1000,
  CM_SHOW_BONES,
  CM_SHOW_SIMDEBUG,
  CM_SHOW_DEBUG,

  CM_ACTION_PANEL,

  CM_CREATE_POINTS_AT_PIVOTS,
  CM_CREATE_CLIPPERS_AT_PIVOTS,
  CM_SIMULATE,
  CM_REVERT,
  CM_CREATE_BONE,

  CM_EXPORT_DUMP,
};


//------------------------------------------------------------------

FastPhysEditor::FastPhysEditor(FastPhysPlugin &plugin) :
  mPlugin(plugin),
  points(tmpmem),
  undoSystem(NULL),
  physSystem(NULL),
  selProcess(false),
  charRoot(NULL),
  isSimulationActive(false),
  isSimDebugVisible(true),
  isObjectDebugVisible(true),
  isSkeletonVisible(false)
{
  clearAll();

  physSystem = new FastPhysSystem();
  undoSystem = get_app().getUndoSystem();
}


FastPhysEditor::~FastPhysEditor()
{
  del_it(physSystem);
  undoSystem = NULL;
}


void FastPhysEditor::clearAll()
{
  mSkeletonName = "";

  unselectAll();
  clear_and_shrink(points);
  // removeAllObjects(false);
  clear_and_shrink(objects);

  charRoot = NULL;

  initAction = new FpdContainerAction("init");
  updateAction = new FpdContainerAction("update");

  if (getUndoSystem())
  {
    getUndoSystem()->clear();
    getUndoSystem()->setDirty(false);
  }
}


void FastPhysEditor::removeObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  __super::removeObjects(obj, num, use_undo);

  // fix actions
  // removeDeadActions(initAction);
  // removeDeadActions(updateAction);

  mPlugin.refillActionTree();
}


/*
void FastPhysEditor::removeDeadActions(FpdAction *action)
{
  FpdContainerAction *c_action = getContainerAction(action);
  if (c_action)
  {
    const PtrTab<FpdAction> &subact= c_action->getSubActions();
    int i = 0;

    while (i < subact.size())
    {
      FpdObject *object = subact[i]->getObject();
      if (object)
      {
        bool need_remove = true;
        for (int j = 0; j < objects.size(); ++j)
        {
          if (!objects[j]->isSubOf(IFPObject::HUID))
            continue;

          IFPObject *_obj = (IFPObject *) objects[j].get();
          if (_obj->getObject() == object)
          {
            need_remove = false;
            break;
          }
        }

        if (need_remove)
        {
          c_action->removeActions(i, 1);
          continue;
        }
      }

      removeDeadActions(subact[i]);
      ++i;
    }
  }
}
*/


IFPObject *FastPhysEditor::getWrapObjectByName(const char *name)
{
  IFPObject *obj = NULL;

  for (int i = 0; i < objects.size(); ++i)
  {
    if (!objects[i]->isSubOf(IFPObject::HUID))
      continue;

    obj = (IFPObject *)objects[i].get();

    if (obj && stricmp(obj->getObject()->getName(), name) == 0)
    {
      return obj;
    }
  }

  return NULL;
}


FpdObject *FastPhysEditor::getEdObjectByName(const char *name)
{
  if (IFPObject *obj = getWrapObjectByName(name))
    return obj->getObject();

  return NULL;
}


IFPObject *FastPhysEditor::addObject(FpdObject *o)
{
  if (!o)
    return NULL;

  IFPObject *wo = IFPObject::createFPObject(o, *this);
  if (!wo)
    return NULL;

  objects.push_back(wo);
  //__super::addObject(wo, true);

  FpdPoint *po = rtti_cast<FpdPoint>(o);
  if (po)
    points.push_back(po);

  return wo;
}


FpdPoint *FastPhysEditor::getPointByName(const char *name)
{
  if (!name)
    return NULL;

  for (int i = 0; i < points.size(); ++i)
    if (stricmp(points[i]->getName(), name) == 0)
      return points[i];

  return NULL;
}


FpdBone *FastPhysEditor::getBoneByName(const char *name)
{
  if (!name)
    return NULL;

  for (int i = 0; i < objects.size(); ++i)
  {
    if (!objects[i]->isSubOf(IFPObject::HUID))
      continue;

    IFPObject *wobj = (IFPObject *)objects[i].get();

    if (wobj)
    {
      FpdObject *obj = wobj->getObject();

      if (obj && obj->isSubOf(FpdBone::HUID) && stricmp(obj->getName(), name) == 0)
      {
        return (FpdBone *)obj;
      }
    }
  }
  return NULL;
}


void FastPhysEditor::onCharWtmChanged()
{
  for (int i = 0; i < objectCount(); ++i)
  {
    RenderableEditableObject *o = getObject(i);
    if (!o || !o->isSubOf(IFPObject::HUID))
      continue;

    FpdObject *obj = ((IFPObject *)o)->getObject();

    if (obj && obj->isSubOf(FpdPoint::HUID))
      ((FpdPoint *)obj)->recalcPos();

    if (obj && obj->isSubOf(FpdClipper::HUID))
      ((FpdClipper *)obj)->recalcTm();
  }
}


bool FastPhysEditor::loadCharacter(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log)
{
  int char_atype = mgr.getAssetTypeId("animChar");

  if (!sa_name.length())
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton name <%s>", sa_name.str());
    return false;
  }

  Tab<int> types(tmpmem);
  types.push_back(char_atype);

  Tab<int> a_indexes(tmpmem);
  a_indexes = mgr.getFilteredAssets(types);

  for (int i = 0; i < a_indexes.size(); ++i)
  {
    DagorAsset &asset = mgr.getAsset(a_indexes[i]);
    const DataBlock &blk = asset.props;

    SimpleString sk_name(blk.getStr("skeleton", ""));

    if (strcmp(sk_name, sa_name) != 0)
      continue;

    //------------

    int skeleton_atype = mgr.getAssetTypeId("skeleton");
    DagorAsset *sk_asset = mgr.findAsset(sa_name, skeleton_atype);

    SimpleString model_name(blk.getStr("dynModel", ""));
    int dyn_model_atype = mgr.getAssetTypeId("dynModel");
    DagorAsset *model_asset = mgr.findAsset(model_name, dyn_model_atype);

    if (!sk_asset || !model_asset)
      return false;

    AnimV20::AnimCharCreationProps cp;

    // dabuildcache::invalidate_asset(asset, true);

    /*
    cp.useCharDep = asset.props.getBool("useCharDep",false);
    if (cp.useCharDep)
    {
      cp.pyOfs = asset.props.getReal("root_py_ofs", 0);
      cp.pxScale = asset.props.getReal("root_px_scale", 1);
      cp.pxScale = asset.props.getReal("root_py_scale", 1);
      cp.pxScale = asset.props.getReal("root_pz_scale", 1);
      cp.sScale = asset.props.getReal("root_s_scale", 1);
      cp.rootNode = asset.props.getStr("rootNode", "");
    }
    cp.centerNode = asset.props.getStr("center", "Pelvis");
    cp.noAnimDist = asset.props.getReal("no_anim_dist", 150);
    cp.noRenderDist = asset.props.getReal("no_render_dist", 300);
    cp.bsphRad = asset.props.getReal("bsphere_rad", 10);
    */

    AnimV20::IAnimCharacter2 *animchar = new AnimCharV20::IAnimCharacter2;
    int modelId = dabuildcache::get_asset_res_id(*model_asset);
    int skeletonId = dabuildcache::get_asset_res_id(*sk_asset);
    int physId = -1;
    int acId = -1; // dabuildcache::get_asset_res_id(*scene.assetMain);

    if (!animchar->load(cp, modelId, skeletonId, acId, physId))
      destroy_it(animchar);
    else
    {
      // animchar->reset();
      // animchar->recalcWtm();
      animchar->beforeRender();
    }

    /*
    const char *fp_name = asset.props.getStr("ref_fastPhys", NULL);
    if (fp_name)
    {
      FastPhysSystem *fpRes = (FastPhysSystem*)
        ::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(fp_name), FastPhysGameResClassId);

      if (!fpRes)
        log.addMessage(ILogWriter::ERROR, "cannot find fastphys <%s> for animChar: %s",
          fp_name, asset.getNameTypified());
      else
        animchar->setFastPhysSystem(fpRes);
    }
    */

    objects.push_back(charRoot = new FastPhysCharRoot(animchar, *this));
    return true;
  }

  return false;
}


bool FastPhysEditor::loadSkeleton(DagorAssetMgr &mgr, SimpleString sa_name, ILogWriter &log)
{
  int skeleton_atype = mgr.getAssetTypeId("skeleton");

  if (!sa_name.length())
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton name <%s>", sa_name.str());
    return false;
  }

  DagorAsset *sk_asset = mgr.findAsset(sa_name, skeleton_atype);

  if (!sk_asset)
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton asset <%s>", sa_name.str());
    return false;
  }

  IDagorAssetExporter *e = mgr.getAssetExporter(skeleton_atype);
  if (!e)
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton exporter plugin");
    return false;
  }

  mkbindump::BinDumpSaveCB cwr(16 << 10, _MAKE4C('PC'), false);
  if (!e->exportAsset(*sk_asset, cwr, log))
  {
    log.addMessage(ILogWriter::ERROR, "can't build skeleton asset <%s> data", sa_name.str());
    return false;
  }

  MemoryLoadCB crd(cwr.getMem(), false);
  nodeTree.load(crd);

  return true;
}


void FastPhysEditor::revert()
{
  DagorAsset *asset = mPlugin.getAsset();
  if (asset)
  {
    const DataBlock blk(asset->getTargetFilePath());
    asset->props.setFrom(&blk);

    load(*asset);
    mPlugin.refillActionTree();
    mPlugin.fillPluginPanel();
  }
}


void FastPhysEditor::load(const DagorAsset &asset)
{
  clearAll();

  CoolConsole &log = ::get_app().getConsole();
  const DataBlock blk = asset.props;

  // load skeleton
  mSkeletonName = blk.getStr("skeleton", "");
  if (!loadSkeleton(asset.getMgr(), mSkeletonName, log))
    return;

  if (!loadCharacter(asset.getMgr(), mSkeletonName, log))
    return;

  // load objects
  int objNameId = blk.getNameId("obj");
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &sb = *blk.getBlock(i);
    if (sb.getBlockNameId() != objNameId)
      continue;

    FpdObject *obj = IFpdLoad::create_object_by_class(sb.getStr("class", NULL));
    if (!obj)
    {
      log.addMessage(ILogWriter::ERROR, "Error loading FastPhys object");
      continue;
    }

    obj->setName(sb.getStr("name", "Object"));
    obj->load(sb, *this);

    addObject(obj);
  }

  // load actions
  initAction->load(*blk.getBlockByNameEx("initAction"), *this);
  updateAction->load(*blk.getBlockByNameEx("updateAction"), *this);

  updateToolbarButtons();
}


void FastPhysEditor::save(DataBlock &blk)
{
  blk.setStr("skeleton", mSkeletonName);

  // save objects
  for (int i = 0; i < objects.size(); ++i)
  {
    RenderableEditableObject *o = getObject(i);
    if (!o || !o->isSubOf(IFPObject::HUID))
      continue;

    FpdObject *obj = ((IFPObject *)o)->getObject();

    if (!obj)
      continue;

    DataBlock &sb = *blk.addNewBlock("obj");
    sb.setStr("name", obj->getName());

    obj->save(sb, nodeTree);
  }

  // save actions
  initAction->save(*blk.addBlock("initAction"), nodeTree);
  updateAction->save(*blk.addBlock("updateAction"), nodeTree);
}


void FastPhysEditor::exportBinaryDump()
{
  String fileName(wingw::file_save_dlg(EDITORCORE->getWndManager()->getMainWindow(), "Export FastPhys binary dump",
    "FastPhys files|*.fastphys.bin|All files|*.*", "fastphys.bin"));

  if (!fileName.length())
    return;

  FullFileSaveCB cb(fileName);

  mkbindump::BinDumpSaveCB bdcwr(16 << 20, _MAKE4C('PC'), false);
  export_fast_phys(bdcwr);

  bdcwr.copyDataTo(cb);
  cb.close();
}


void FastPhysEditor::export_fast_phys(mkbindump::BinDumpSaveCB &cb)
{
  saveResource();
  DagorAsset *asset = mPlugin.getAsset();
  CoolConsole &log = ::get_app().getConsole();

  IDagorAssetExporter *e = asset->getMgr().getAssetExporter(asset->getType());
  if (!e)
  {
    log.addMessage(ILogWriter::ERROR, "can't get fastPhys exporter plugin");
    return;
  }

  if (!e->exportAsset(*asset, cb, log))
  {
    log.addMessage(ILogWriter::ERROR, "can't build fastPhys asset <%s> data", asset->getName());
    return;
  }

  log.addMessage(ILogWriter::NOTE, "fastPhys asset <%s> was exported", asset->getName());
}


void FastPhysEditor::saveResource()
{
  DataBlock db;
  save(db);

  DagorAsset *asset = mPlugin.getAsset();
  if (asset)
    asset->props = db;
}


FpdContainerAction *FastPhysEditor::getContainerAction(FpdAction *action)
{
  if (action->isSubOf(FpdContainerAction::HUID))
  {
    return (FpdContainerAction *)action;
  }

  return NULL;
}


bool FastPhysEditor::hasSubAction(FpdContainerAction *container, FpdAction *action)
{
  if (container)
  {
    const PtrTab<FpdAction> &subActions = container->getSubActions();

    for (int i = 0; i < subActions.size(); ++i)
    {
      if (subActions[i] == action)
        return true;

      FpdContainerAction *sub_c_action = getContainerAction(subActions[i]);

      if (sub_c_action && hasSubAction(sub_c_action, action))
        return true;
    }
  }

  return false;
}


void FastPhysEditor::startSimulation()
{
  mkbindump::BinDumpSaveCB saveCb(1 << 20, _MAKE4C('PC'), false);

  try
  {
    export_fast_phys(saveCb);
  }
  catch (IGenSave::SaveException)
  {
    wingw::message_box(wingw::MBS_EXCL, "Error", "Error exporting FastPhys system");
    return;
  }

  MemoryLoadCB loadCb(saveCb.getMem(), false);

  charRoot->character->setFastPhysSystem(NULL);
  del_it(physSystem);
  physSystem = new FastPhysSystem();

  try
  {
    loadCb.beginBlock();
    physSystem->load(loadCb);
    loadCb.endBlock();
  }
  catch (IGenLoad::LoadException)
  {
    wingw::message_box(wingw::MBS_EXCL, "Error", "Error loading FastPhys system");
    del_it(physSystem);
    return;
  }

  charRoot->updateNodeTm();
  charRoot->character->reset();
  charRoot->character->recalcWtm();

  const GeomNodeTree &tree = charRoot->character->getNodeTree();
  clear_and_resize(savedNodeTms, tree.nodeCount());

  for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
    savedNodeTms[i.index()] = tree.getNodeTm(i);

  charRoot->character->setFastPhysSystem(physSystem);

  getUndoSystem()->begin();
  unselectAll();
  charRoot->selectObject();
  getUndoSystem()->accept("Start simulation");

  isSimulationActive = true;
  updateToolbarButtons();
}


void FastPhysEditor::stopSimulation()
{
  charRoot->setWtm(TMatrix::IDENT);

  charRoot->character->setFastPhysSystem(NULL);
  charRoot->character->reset();

  GeomNodeTree &tree = charRoot->character->getNodeTree();

  if (savedNodeTms.size() != 0)
  {
    G_ASSERT(savedNodeTms.size() == tree.nodeCount());
    for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
      tree.getNodeTm(i) = savedNodeTms[i.index()];

    clear_and_shrink(savedNodeTms);
  }

  nodeTree.invalidateWtm();
  charRoot->character->recalcWtm();

  onCharWtmChanged();

  // IResEditorEngine::get()->updateViewports();

  isSimulationActive = false;
  updateToolbarButtons();
}


void FastPhysEditor::renderSkeleton()
{
  static const E3DCOLOR NODE_COLOR(125, 200, 0);
  static const E3DCOLOR LINK_COLOR(255, 255, 0);

  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  for (dag::Index16 i(1), ie(nodeTree.nodeCount()); i < ie; ++i)
  {
    // points

    Point3 pos = nodeTree.getNodeWposRelScalar(i);
    draw_debug_sph(pos, 0.03, NODE_COLOR);

    // links

    const int nodeChildCnt = nodeTree.getChildCount(i);
    for (int j = 0; j < nodeChildCnt; j++)
    {
      Point3 c_pos = nodeTree.getNodeWposRelScalar(nodeTree.getChildNodeIdx(i, j));
      draw_cached_debug_line(pos, c_pos, LINK_COLOR);
    }
  }
}

TMatrix FastPhysEditor::getRootWtm() { return charRoot.get() ? charRoot->getWtm() : TMatrix::IDENT; }

void FastPhysEditor::renderGeometry(int stage)
{
  for (int i = 0; i < objects.size(); ++i)
    if (FastPhysCharRoot *cr = RTTI_cast<FastPhysCharRoot>(objects[i]))
      cr->renderGeometry(stage);
}

void FastPhysEditor::render()
{
  ::begin_draw_cached_debug_lines();

  __super::render();

  if (isSimulationActive && isSimDebugVisible && physSystem)
    physSystem->debugRender();

  // skeleton render
  // if (!isSimulationActive && isSkeletonVisible)
  //  renderSkeleton();

  ::end_draw_cached_debug_lines();
}


void FastPhysEditor::update(real dt)
{
  // Do not handle update when this is not the active plugin because
  // calling ObjectEditor::updateGizmo prevents other plugins from using
  // gizmos.
  if (get_app().curPlugin() != &mPlugin)
    return;

  __super::update(dt);

  // updateGizmo();

  /*
  if (isSimulationActive && physSystem)
  {
    //physSystem->update(dt);
    IResEditorEngine::get()->updateViewports();
  }
  */
}


IFPObject *FastPhysEditor::getSelObject()
{
  if (selectedCount() == 1)
  {
    RenderableEditableObject *o = getSelected(0);
    IFPObject *wobj = NULL;

    if (!o || !o->isSubOf(IFPObject::HUID) || !(wobj = (IFPObject *)o))
      return NULL;

    return wobj;
  }

  return NULL;
}


void FastPhysEditor::updateSelection()
{
  if (!selProcess)
  {
    selProcess = true;
    mPlugin.updateActionPanel();
    selProcess = false;
  }

  mPlugin.refillPanel();
}


void FastPhysEditor::selectionChanged() { updateSelection(); }


void FastPhysEditor::updateToolbarButtons()
{
  __super::updateToolbarButtons();

  if (charRoot)
    setButton(CM_SHOW_CHAR, charRoot->getCharVisible());
  setButton(CM_SHOW_BONES, isSkeletonVisible);
  /*
  setButton(toolBar, CM_ACTION_PANEL, actionPropBar?actionPropBar->isVisible():false);
  setButton(toolBar, CM_SIMULATE, isSimulationActive);
  */
  setButton(CM_SHOW_SIMDEBUG, isSimDebugVisible);
  setButton(CM_SHOW_DEBUG, isObjectDebugVisible);
}


void FastPhysEditor::fillToolBar(PropPanel2 *toolbar)
{
  __super::fillToolBar(toolbar);

  PropertyContainerControlBase *tb = toolbar->createToolbarPanel(0, "");

  // addButton(tb, CM_CLOSE_CURRENT_EDITOR, "close_editor", "Exit FastPhys Editor");
  // tb->createSeparator();

  addButton(tb, CM_REVERT, "import_blk", "Revert changes");
  tb->createSeparator();

  //__super::fillToolBar();

  // addButton(tb, CM_ACTION_PANEL, "show_panel_gi", "Show/hide actions panel", true);
  // tb->createSeparator();

  addButton(tb, CM_SHOW_DEBUG, "res_geomnodetree", "Show/hide debug", true);
  addButton(tb, CM_SHOW_CHAR, "create_actor", "Show/hide character", true);
  addButton(tb, CM_SHOW_BONES, "create_actor", "Show/hide skeleton", true);
  tb->createSeparator();

  addButton(tb, CM_CREATE_POINTS_AT_PIVOTS, "create_point", "Create points at pivots");
  addButton(tb, CM_CREATE_BONE, "create_box", "Create bone linking 2 selected points (B)");
  addButton(tb, CM_CREATE_CLIPPERS_AT_PIVOTS, "create_sphere", "Create clippers at pivots");
  tb->createSeparator();

  addButton(tb, CM_SIMULATE, "compile", "Start/stop simulation test (Enter)", true);
  addButton(tb, CM_SHOW_SIMDEBUG, "create_object", "Show/hide simulation debug", true);
  tb->createSeparator();

  addButton(tb, CM_EXPORT_DUMP, "export_blk", "Export binary dump");

  updateToolbarButtons();
}


void FastPhysEditor::addButton(PropPanel2 *toolbar, int id, const char *bmp_name, const char *hint, bool check)
{
  if (id == CM_OBJED_OBJPROP_PANEL)
    return;

  __super::addButton(toolbar, id, bmp_name, hint, check);
}

void FastPhysEditor::setWind(const Point3 &vel, float power, float turb)
{
  if (physSystem)
  {
    physSystem->windVel = vel;
    physSystem->windPower = power;
    physSystem->windTurb = turb;
  }
}

void FastPhysEditor::getWind(Point3 &vel, float &power, float &turb)
{
  vel.zero();
  power = turb = 0.f;
  if (physSystem)
  {
    vel = physSystem->windVel;
    power = physSystem->windPower;
    turb = physSystem->windTurb;
  }
}


void FastPhysEditor::onClick(int pcb_id, PropPanel2 *panel)
{
  __super::onClick(pcb_id, panel);

  switch (pcb_id)
  {
    case CM_REVERT:
      if (wingw::message_box(wingw::MBS_YESNO, "Revert FastPhys asset", "Revert asset data to original, removing all changes?") ==
          wingw::MB_ID_YES)
        revert();
      break;

    case CM_EXPORT_DUMP: exportBinaryDump(); break;

    case CM_SHOW_SIMDEBUG:
      isSimDebugVisible = !isSimDebugVisible;
      updateToolbarButtons();
      break;

    case CM_SHOW_DEBUG:
      isObjectDebugVisible = !isObjectDebugVisible;
      updateToolbarButtons();
      break;


    case CM_SHOW_CHAR:
      charRoot->setCharVisible(!charRoot->getCharVisible());
      updateToolbarButtons();
      break;

    case CM_SHOW_BONES:
      isSkeletonVisible = !isSkeletonVisible;
      updateToolbarButtons();
      break;

    case CM_CREATE_POINTS_AT_PIVOTS: createPointsAtPivots(); break;

    case CM_CREATE_BONE: linkSelectedPointsWithBone(); break;

    case CM_CREATE_CLIPPERS_AT_PIVOTS: createClippersAtPivots(); break;

    case CM_SIMULATE:
      if (!isSimulationActive)
        startSimulation();
      else
        stopSimulation();
      break;
  }
}


void FastPhysEditor::refillPanel() { mPlugin.refillPanel(); }


AnimV20::IAnimCharacter2 *FastPhysEditor::getAnimChar() const
{
  if (charRoot)
    return charRoot->character;

  return NULL;
}


// creators

bool FastPhysEditor::showNodeList(Tab<int> &sels)
{
  Tab<String> _names(tmpmem), _sel_names(tmpmem);

  for (dag::Index16 i(1), ie(nodeTree.nodeCount()); i < ie; ++i)
    _names.push_back() = nodeTree.getNodeName(i);

  MultiListDialog dlg("Select points", hdpi::_pxScaled(300), hdpi::_pxScaled(400), _names, _sel_names);
  dlg.setSelectionTab(&sels);
  dlg.showDialog();

  return sels.size();
}

// point

IFPObject *FastPhysEditor::createPoint(const char *from_name, const Point3 &pos, dag::Index16 node)
{
  // make unique name
  String name(from_name);

  while (getObjectByName(name))
    ::make_next_numbered_name(name, 2);

  // create point
  FpdPoint *obj = new FpdPoint();
  obj->setName(name);
  as_point3(&obj->localPos) = pos;
  obj->node = node;
  obj->nodeWtm = get_node_wtm_rel_ptr(nodeTree, node);
  obj->initActions(getContainerAction(initAction), getContainerAction(updateAction), *this);
  obj->recalcPos();

  return addObject(obj);
}


void FastPhysEditor::createPointsAtPivots()
{
  Tab<int> selIndexes(tmpmem);

  if (!showNodeList(selIndexes))
    return;

  // create points
  getUndoSystem()->begin();

  unselectAll();

  for (int i = 0; i < selIndexes.size(); ++i)
  {
    auto node = dag::Index16(selIndexes[i] + 1);
    if (!node)
      continue;

    String name(nodeTree.getNodeName(node));
    name += ".pt";

    IFPObject *obj = createPoint(name, Point3(0, 0, 0), node);
    obj->selectObject();
  }

  getUndoSystem()->accept("Create points");

  mPlugin.refillActionTree();
}


// bone

void FastPhysEditor::linkSelectedPointsWithBone()
{
  if (selectedCount() != 2)
    return;

  if (!getSelected(0)->isSubOf(IFPObject::HUID))
    return;
  if (!getSelected(1)->isSubOf(IFPObject::HUID))
    return;

  FpdObject *obj1 = ((IFPObject *)getSelected(0))->getObject();
  FpdObject *obj2 = ((IFPObject *)getSelected(1))->getObject();

  if (!obj1->isSubOf(FpdPoint::HUID))
    return;
  if (!obj2->isSubOf(FpdPoint::HUID))
    return;

  FpdPoint *p1 = (FpdPoint *)obj1;
  FpdPoint *p2 = (FpdPoint *)obj2;

  // create
  getUndoSystem()->begin();

  unselectAll();

  // make unique name
  String name("Bone01");

  while (getObjectByName(name))
    ::make_next_numbered_name(name, 2);

  // create bone
  FpdBone *obj = new FpdBone();
  obj->setName(name);
  obj->point1Name = p1->getName();
  obj->point2Name = p2->getName();

  IFPObject *wobj = addObject(obj);
  obj->initActions(getContainerAction(initAction), getContainerAction(updateAction), *this);

  wobj->selectObject();

  getUndoSystem()->accept("Create bone");

  mPlugin.refillActionTree();
  // updateActionPanel();
}


// clipper

IFPObject *FastPhysEditor::createClipper(const char *from_name, const Point3 &pos, dag::Index16 node)
{
  // make unique name
  String name(from_name);

  while (getObjectByName(name))
    ::make_next_numbered_name(name, 2);

  // create object
  FpdClipper *obj = new FpdClipper();
  obj->setName(name);
  obj->localTm.setcol(3, pos);
  obj->node = node;
  obj->nodeWtm = get_node_wtm_rel_ptr(nodeTree, node);
  obj->initActions(getContainerAction(initAction), getContainerAction(updateAction), *this);
  obj->recalcTm();

  return addObject(obj);
}


void FastPhysEditor::createClippersAtPivots()
{
  Tab<int> selIndexes(tmpmem);

  if (!showNodeList(selIndexes))
    return;

  // create objects
  getUndoSystem()->begin();

  unselectAll();

  for (int i = 0; i < selIndexes.size(); ++i)
  {
    auto node = dag::Index16(selIndexes[i] + 1);
    if (!node)
      continue;

    String name(nodeTree.getNodeName(node));
    name += ".clip";

    IFPObject *obj = createClipper(name, Point3(0, 0, 0), node);
    obj->selectObject();
  }

  getUndoSystem()->accept("Create clippers");

  mPlugin.refillActionTree();
  // updateActionPanel();
}


//------------------------------------------------------------------
