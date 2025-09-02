// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <coolConsole/coolConsole.h>

#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_IEditorCore.h>

#include <debug/dag_debug.h>

#include "plugin.h"


IvyGenPlugin *IvyGenPlugin::self = NULL;


IvyGenPlugin::IvyGenPlugin()
{
  self = this;
  firstBegin = true;
}


IvyGenPlugin::~IvyGenPlugin()
{
  self = NULL;
  editorcore_extapi::dagRender->releaseManagedTex(IvyObject::texPt);
}


//==============================================================================
bool IvyGenPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  objEd.initUi(toolbar_id);

  if (firstBegin)
  {
    IvyObject::initCollisionCallback();

    firstBegin = false;
    objEd.invalidateObjectProps();
    DAGORED2->repaint();
  }

  return true;
}


bool IvyGenPlugin::end()
{
  for (int i = 0; i < objEd.objectCount(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objEd.getObject(i));
    if (p)
      p->stopAutoGrow();
  }

  objEd.closeUi();
  return true;
}


void IvyGenPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  objEd.load(blk);
  objEd.updateToolbarButtons();
}


void IvyGenPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) { objEd.save(blk); }


void IvyGenPlugin::clearObjects() { objEd.removeAllObjects(false); }


void IvyGenPlugin::actObjects(float dt)
{
  if (!isVisible)
    return;

  if (DAGORED2->curPlugin() == self)
    objEd.update(dt);
}


void IvyGenPlugin::beforeRenderObjects(IGenViewportWnd *vp)
{
  if (!isVisible)
    return;
  objEd.beforeRender();
}


void IvyGenPlugin::renderObjects() { objEd.render(); }


//==============================================================================
void IvyGenPlugin::renderTransObjects()
{
  if (!isVisible)
    return;

  objEd.renderTrans();
}

void *IvyGenPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IGatherStaticGeometry);
  return NULL;
}

void IvyGenPlugin::handleViewportAcceleratorCommand(unsigned id) { objEd.onClick(id, nullptr); }

void IvyGenPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();
  objEd.registerViewportAccelerators(wndManager);
}