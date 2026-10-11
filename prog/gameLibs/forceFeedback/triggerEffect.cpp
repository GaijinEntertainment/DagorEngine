// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <forceFeedback/forceFeedback.h>
#include <osApiWrappers/dag_critSec.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiJoystick.h>

namespace force_feedback
{
namespace trigger_effect
{
void set(const HumanInput::TriggerEffectParam &param)
{
  WinAutoLock joyGuard(global_cls_drv_update_cs);
  if (HumanInput::IGenJoystick *joy = ::global_cls_drv_joy ? ::global_cls_drv_joy->getDefaultJoystick() : nullptr)
    joy->setTriggerEffect(param);
}
} // namespace trigger_effect
} // namespace force_feedback
