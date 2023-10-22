#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <humanInput/dag_hiCreate.h>
#include <humanInput/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>
#include <generic/dag_initOnDemand.h>

class JoystickRestartProcXInput : public SRestartProc
{
public:
  virtual const char *procname() { return "joystick@xinput"; }
  JoystickRestartProcXInput() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  void startup() { global_cls_drv_joy = HumanInput::createXinputJoystickClassDriver(); }

  void shutdown()
  {
    if (global_cls_drv_joy)
      global_cls_drv_joy->destroy();
    global_cls_drv_joy = NULL;
  }
};

static InitOnDemand<JoystickRestartProcXInput> joy_rproc;

void dagor_init_joystick_xinput()
{
  joy_rproc.demandInit();

  add_restart_proc(joy_rproc);
}
