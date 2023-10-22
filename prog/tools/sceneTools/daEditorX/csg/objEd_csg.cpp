#include "objEd_csg.h"
#include "box_csg.h"
#include "plugIn.h"

#include <dllPluginCore/core.h>
#include <EditorCore/ec_ObjectCreator.h>
#include <coolConsole/coolConsole.h>
#include <oldEditor/de_cm.h>
#include <de3_editorEvents.h>

#include <3d/dag_drv3d.h>

#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <propPanel2/comWnd/dialog_window.h>

#include <debug/dag_debug.h>

enum
{
  CM_ = CM_PLUGIN_BASE + 1,
  CM_CREATE_OCCLUDER_BOX
};


ObjEd::ObjEd() : cloneMode(false), objCreator(NULL)
{
  BoxCSG::initStatics();
  ptDynBuf = dagRender->newDynRenderBuffer("editor_helper_no_tex_blend", midmem);
  secTime = 0;
  showLocalBoxCSG = true;
}


ObjEd::~ObjEd() { dagRender->deleteDynRenderBuffer(ptDynBuf); }


void ObjEd::fillToolBar(PropertyContainerControlBase *toolbar)
{
  PropertyContainerControlBase *tb1 = toolbar->createToolbarPanel(0, "");

  addButton(tb1, CM_CREATE_OCCLUDER_BOX, "create_box", "Create box Occluder (1)", true);

  tb1->createSeparator();

  __super::fillToolBar(toolbar);

  PropertyContainerControlBase *tb2 = toolbar->createToolbarPanel(0, "");
}


void ObjEd::addButton(PropertyContainerControlBase *tb, int id, const char *bmp_name, const char *hint, bool check)
{
  if (id == CM_OBJED_DROP || id == CM_OBJED_MODE_SURF_MOVE)
    return;
  __super::addButton(tb, id, bmp_name, hint, check);
}


void ObjEd::updateToolbarButtons()
{
  __super::updateToolbarButtons();
  setRadioButton(CM_CREATE_OCCLUDER_BOX, getEditMode());
}

void ObjEd::reset()
{
  setCreateBySampleMode(NULL);
  del_it(objCreator);
  showLocalBoxCSG = true;
}

void ObjEd::beforeRender() { __super::beforeRender(); }


void ObjEd::objRender()
{
  bool curPlugin = DAGORED2->curPlugin() == CSGPlugin::self;
  dagRender->startLinesRender();
  if (showLocalBoxCSG)
  {
    for (int i = 0; i < objects.size(); i++)
    {
      BoxCSG *p = RTTI_cast<BoxCSG>(objects[i]);
      if (p)
        p->render();
    }
  }
  dagRender->endLinesRender();

  if (objCreator)
    objCreator->render();
}


void ObjEd::objRenderTr()
{
  bool curPlugin = DAGORED2->curPlugin() == CSGPlugin::self;
  if (!curPlugin)
    return;

  TMatrix4 gtm;
  d3d::getglobtm(gtm);

  dagRender->dynRenderBufferClearBuf(*ptDynBuf);

  int cnt = 0;

  if (showLocalBoxCSG)
    for (int i = 0; i < objects.size(); i++)
    {
      BoxCSG *p = RTTI_cast<BoxCSG>(objects[i]);
      if (p)
      {
        p->renderCommonGeom(*ptDynBuf, gtm);

        cnt++;
        if (cnt >= 512)
          dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
      }
    }

  dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
  dagRender->dynRenderBufferFlush(*ptDynBuf);
}


void ObjEd::gizmoStarted() { __super::gizmoStarted(); }


void ObjEd::gizmoEnded(bool apply)
{
  if (cloneMode && selection.size())
  {
    getUndoSystem()->accept("Clone object(s)");

    cloneMode = false;

    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
    isGizmoStarted = false;
  }
  else
    __super::gizmoEnded(apply);
}


void ObjEd::save(DataBlock &blk)
{
  for (int i = 0; i < objects.size(); i++)
  {
    BoxCSG *p = RTTI_cast<BoxCSG>(objects[i]);
    if (p)
      p->save(*blk.addNewBlock("CSG_Box"));
  }
}


void ObjEd::load(const DataBlock &blk)
{
  int nid = blk.getNameId("CSG_Box");

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &cb = *blk.getBlock(i);
    if (cb.getBlockNameId() != nid)
      continue;

    Ptr<BoxCSG> s = new BoxCSG();
    s->load(cb);
    addObject(s, false);
  }
}


void ObjEd::getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types)
{
  names.clear();

  if (types.empty())
    return;

  bool showIvys = false;

  for (int ti = 0; ti < types.size(); ++ti)
  {
    if (types[ti] == 0)
      showIvys = true;
  }

  for (int i = 0; i < objects.size(); i++)
  {
    BoxCSG *p = RTTI_cast<BoxCSG>(objects[i]);
    if (p && showIvys)
    {
      names.push_back() = p->getName();

      if (p->isSelected())
        sel_names.push_back() = p->getName();
    }
  }
}


void ObjEd::getTypeNames(Tab<String> &names)
{
  names.resize(1);
  names[0] = "Occluders";
}


void ObjEd::onSelectedNames(const Tab<String> &names)
{
  getUndoSystem()->begin();
  unselectAll();

  for (int ni = 0; ni < names.size(); ++ni)
  {
    RenderableEditableObject *o = getObjectByName(names[ni]);
    o->selectObject();
  }

  getUndoSystem()->accept("Select");
  updateGizmo();
}


void ObjEd::update(real dt)
{
  updateGizmo();
  if (objCreator && objCreator->isFinished())
  {
    if (objCreator->isOk())
    {
      if (getEditMode() == CM_CREATE_OCCLUDER_BOX)
      {
        // create box
        BoxCSG *obj = new BoxCSG();
        objCreator->matrix.setcol(3, objCreator->matrix * Point3(0, 0.5, 0));
        obj->setWtm(objCreator->matrix);

        getUndoSystem()->begin();
        addObject(obj);
        getUndoSystem()->accept("Create box csg");

        onClick(CM_CREATE_OCCLUDER_BOX, NULL);
      }
    }
    else
      onClick(CM_OBJED_MODE_SELECT, NULL);
  }
}

static int last_mx = 0, last_my = 0, last_inside = 0;
static IGenViewportWnd *last_wnd = NULL;

bool ObjEd::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
  {
    last_mx = x;
    last_my = y;
    last_inside = inside;
    last_wnd = wnd;
    // return objCreator->handleMouseMove(wnd, x, y, inside, buttons, key_modif^CTRL_PRESSED, true);
    return objCreator->handleMouseMove(wnd, x, y, inside, buttons, !wingw::is_key_pressed(wingw::V_CONTROL), true);
  }
  return __super::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}


bool ObjEd::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
  return __super::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}


bool ObjEd::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
  return __super::handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
}


bool ObjEd::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
  return __super::handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
}


void ObjEd::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  if (!wingw::is_special_pressed())
  {
    if (wingw::is_key_pressed('1'))
      onClick(CM_CREATE_OCCLUDER_BOX, NULL);
    if (wingw::is_key_pressed(wingw::V_ESC) && objCreator)
      onClick(CM_OBJED_MODE_SELECT, NULL);
  }

  __super::handleKeyPress(wnd, vk, modif);
}

void ObjEd::handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) { __super::handleKeyRelease(wnd, vk, modif); }


void ObjEd::onClick(int pcb_id, PropPanel2 *panel)
{
  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE:
      del_it(objCreator);
      setEditMode(pcb_id);
      break;

    case CM_CREATE_OCCLUDER_BOX:
      showLocalBoxCSG = true;
      del_it(objCreator);
      setEditMode(pcb_id);
      objCreator = dagGeom->newBoxCreator();
      last_wnd = NULL;
      break;
  }

  updateGizmo();
  updateToolbarButtons();

  __super::onClick(pcb_id, panel);
}
