#include <ioSys/dag_dataBlock.h>
#include <coolConsole/coolConsole.h>

#include <sepGui/wndPublic.h>

#include <debug/dag_debug.h>

#include "plugIn.h"


IvyGenPlugin *IvyGenPlugin::self = NULL;


IvyGenPlugin::IvyGenPlugin()
{
  self = this;
  firstBegin = true;
}


IvyGenPlugin::~IvyGenPlugin() { self = NULL; }


//==============================================================================
bool IvyGenPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropertyContainerControlBase *toolbar = DAGORED2->getCustomPanel(toolbar_id);
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
