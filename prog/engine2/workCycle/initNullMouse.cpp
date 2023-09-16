#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiCreate.h>
#include <humanInput/dag_hiPointing.h>
#include <3d/dag_drv3d.h>

void dagor_init_mouse_null()
{
  if (!global_cls_drv_pnt)
  {
    global_cls_drv_pnt = HumanInput::createNullMouseClassDriver();
    if (HumanInput::IGenPointing *ms = global_cls_drv_pnt->getDevice(0))
    {
      int w = 16, h = 16;
      d3d::get_screen_size(w, h);
      ms->setClipRect(4, 4, w - 8, h - 8);
    }
  }
}
