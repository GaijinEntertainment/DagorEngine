//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <daInput/input_api.h>
#include <osApiWrappers/dag_critSec.h>
#include <startup/dag_inpDevClsDrv.h>

using DigitalAction = dainput::DigitalAction;
using AnalogStickAction = dainput::AnalogStickAction;
using AnalogAxisAction = dainput::AnalogAxisAction;

using SingleButtonId = dainput::SingleButtonId;
using DigitalActionBinding = dainput::DigitalActionBinding;
using AnalogAxisActionBinding = dainput::AnalogAxisActionBinding;
using AnalogStickActionBinding = dainput::AnalogStickActionBinding;

MAKE_TYPE_FACTORY(DigitalAction, DigitalAction);
MAKE_TYPE_FACTORY(AnalogStickAction, AnalogStickAction);
MAKE_TYPE_FACTORY(AnalogAxisAction, AnalogAxisAction);

MAKE_TYPE_FACTORY(SingleButtonId, SingleButtonId);
MAKE_TYPE_FACTORY(DigitalActionBinding, DigitalActionBinding);
MAKE_TYPE_FACTORY(AnalogAxisActionBinding, AnalogAxisActionBinding);
MAKE_TYPE_FACTORY(AnalogStickActionBinding, AnalogStickActionBinding);

namespace bind_dascript
{
inline unsigned get_last_used_device_mask() { return dainput::get_last_used_device_mask(); }

inline uint16_t single_button_id_get_devId(const SingleButtonId &b) { return b.devId; }
inline uint16_t single_button_id_get_btnId(const SingleButtonId &b) { return b.btnId; }

inline uint16_t digital_action_binding_get_devId(const DigitalActionBinding &b) { return b.devId; }
inline uint16_t digital_action_binding_get_ctrlId(const DigitalActionBinding &b) { return b.ctrlId; }
inline uint16_t digital_action_binding_get_eventType(const DigitalActionBinding &b) { return b.eventType; }
inline uint16_t digital_action_binding_get_axisCtrlThres(const DigitalActionBinding &b) { return b.axisCtrlThres; }
inline uint16_t digital_action_binding_get_btnCtrl(const DigitalActionBinding &b) { return b.btnCtrl; }
inline uint16_t digital_action_binding_get_stickyToggle(const DigitalActionBinding &b) { return b.stickyToggle; }
inline uint16_t digital_action_binding_get_unordCombo(const DigitalActionBinding &b) { return b.unordCombo; }
inline uint16_t digital_action_binding_get_modCnt(const DigitalActionBinding &b) { return b.modCnt; }

inline uint16_t analog_axis_action_binding_get_devId(const AnalogAxisActionBinding &b) { return b.devId; }
inline uint16_t analog_axis_action_binding_get_axisId(const AnalogAxisActionBinding &b) { return b.axisId; }
inline uint16_t analog_axis_action_binding_get_invAxis(const AnalogAxisActionBinding &b) { return b.invAxis; }
inline uint16_t analog_axis_action_binding_get_axisRelativeVal(const AnalogAxisActionBinding &b) { return b.axisRelativeVal; }
inline uint16_t analog_axis_action_binding_get_instantIncDecBtn(const AnalogAxisActionBinding &b) { return b.instantIncDecBtn; }
inline uint16_t analog_axis_action_binding_get_quantizeValOnIncDecBtn(const AnalogAxisActionBinding &b)
{
  return b.quantizeValOnIncDecBtn;
}
inline uint16_t analog_axis_action_binding_get_modCnt(const AnalogAxisActionBinding &b) { return b.modCnt; }

inline void global_cls_drv_update_cs_lock(const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at)
{
  WinAutoLock guard(global_cls_drv_update_cs);
  context->invoke(block, nullptr, nullptr, at);
}

inline uint16_t analog_stick_action_binding_get_devId(const AnalogStickActionBinding &b) { return b.devId; }
inline uint16_t analog_stick_action_binding_get_axisXId(const AnalogStickActionBinding &b) { return b.axisXId; }
inline uint16_t analog_stick_action_binding_get_axisYId(const AnalogStickActionBinding &b) { return b.axisYId; }
inline uint16_t analog_stick_action_binding_get_axisXinv(const AnalogStickActionBinding &b) { return b.axisXinv; }
inline uint16_t analog_stick_action_binding_get_axisYinv(const AnalogStickActionBinding &b) { return b.axisYinv; }
inline uint16_t analog_stick_action_binding_get_modCnt(const AnalogStickActionBinding &b) { return b.modCnt; }
} // namespace bind_dascript
