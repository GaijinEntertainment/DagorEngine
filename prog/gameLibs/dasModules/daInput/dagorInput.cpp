#include <dasModules/aotDagorInput.h>

struct DigitalActionAnnotation final : das::ManagedStructureAnnotation<dainput::DigitalAction, false>
{
  DigitalActionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DigitalAction", ml)
  {
    cppName = " ::dainput::DigitalAction";
    addField<DAS_BIND_MANAGED_FIELD(bState)>("bState");
    addField<DAS_BIND_MANAGED_FIELD(bActive)>("bActive");
    addField<DAS_BIND_MANAGED_FIELD(bActivePrev)>("bActivePrev");
  }
};

struct AnalogStickActionAnnotation final : das::ManagedStructureAnnotation<dainput::AnalogStickAction, false>
{
  AnalogStickActionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnalogStickAction", ml)
  {
    cppName = " ::dainput::AnalogStickAction";
    addField<DAS_BIND_MANAGED_FIELD(x)>("x");
    addField<DAS_BIND_MANAGED_FIELD(y)>("y");
    addField<DAS_BIND_MANAGED_FIELD(bActive)>("bActive");
    addField<DAS_BIND_MANAGED_FIELD(bActivePrev)>("bActivePrev");
  }
};

struct AnalogAxisActionAnnotation final : das::ManagedStructureAnnotation<dainput::AnalogAxisAction, false>
{
  AnalogAxisActionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnalogAxisAction", ml)
  {
    cppName = " ::dainput::AnalogAxisAction";
    addField<DAS_BIND_MANAGED_FIELD(x)>("x");
    addField<DAS_BIND_MANAGED_FIELD(bActive)>("bActive");
    addField<DAS_BIND_MANAGED_FIELD(bActivePrev)>("bActivePrev");
  }
};

struct SingleButtonIdAnnotation final : das::ManagedStructureAnnotation<dainput::SingleButtonId, false>
{
  SingleButtonIdAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SingleButtonId", ml)
  {
    cppName = " ::dainput::SingleButtonId";
  }
};

struct DigitalActionBindingAnnotation final : das::ManagedStructureAnnotation<dainput::DigitalActionBinding, false>
{
  DigitalActionBindingAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DigitalActionBinding", ml)
  {
    cppName = " ::dainput::DigitalActionBinding";
    addField<DAS_BIND_MANAGED_FIELD(mod)>("mod");
  }
};

struct AnalogAxisActionBindingAnnotation final : das::ManagedStructureAnnotation<dainput::AnalogAxisActionBinding, false>
{
  AnalogAxisActionBindingAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnalogAxisActionBinding", ml)
  {
    cppName = " ::dainput::AnalogAxisActionBinding";
    addField<DAS_BIND_MANAGED_FIELD(mod)>("mod");
    addField<DAS_BIND_MANAGED_FIELD(minBtn)>("minBtn");
    addField<DAS_BIND_MANAGED_FIELD(maxBtn)>("maxBtn");
    addField<DAS_BIND_MANAGED_FIELD(incBtn)>("incBtn");
    addField<DAS_BIND_MANAGED_FIELD(decBtn)>("decBtn");
    addField<DAS_BIND_MANAGED_FIELD(deadZoneThres)>("deadZoneThres");
    addField<DAS_BIND_MANAGED_FIELD(nonLin)>("nonLin");
    addField<DAS_BIND_MANAGED_FIELD(maxVal)>("maxVal");
    addField<DAS_BIND_MANAGED_FIELD(relIncScale)>("relIncScale");
  }
};

struct AnalogStickActionBindingAnnotation final : das::ManagedStructureAnnotation<dainput::AnalogStickActionBinding, false>
{
  AnalogStickActionBindingAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnalogStickActionBinding", ml)
  {
    cppName = " ::dainput::AnalogStickActionBinding";
    addField<DAS_BIND_MANAGED_FIELD(mod)>("mod");
    addField<DAS_BIND_MANAGED_FIELD(minXBtn)>("minXBtn");
    addField<DAS_BIND_MANAGED_FIELD(maxXBtn)>("maxXBtn");
    addField<DAS_BIND_MANAGED_FIELD(minYBtn)>("minYBtn");
    addField<DAS_BIND_MANAGED_FIELD(maxYBtn)>("maxYBtn");
    addField<DAS_BIND_MANAGED_FIELD(deadZoneThres)>("deadZoneThres");
    addField<DAS_BIND_MANAGED_FIELD(axisSnapAngK)>("axisSnapAngK");
    addField<DAS_BIND_MANAGED_FIELD(nonLin)>("nonLin");
    addField<DAS_BIND_MANAGED_FIELD(maxVal)>("maxVal");
    addField<DAS_BIND_MANAGED_FIELD(sensScale)>("sensScale");
  }
};

namespace bind_dascript
{
class DagorInputModule final : public das::Module
{
public:
  DagorInputModule() : das::Module("DagorInput")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<DigitalActionAnnotation>(lib));
    addAnnotation(das::make_smart<AnalogStickActionAnnotation>(lib));
    addAnnotation(das::make_smart<AnalogAxisActionAnnotation>(lib));

    addAnnotation(das::make_smart<SingleButtonIdAnnotation>(lib));
    addAnnotation(das::make_smart<DigitalActionBindingAnnotation>(lib));
    addAnnotation(das::make_smart<AnalogAxisActionBindingAnnotation>(lib));
    addAnnotation(das::make_smart<AnalogStickActionBindingAnnotation>(lib));

    G_STATIC_ASSERT(sizeof(dainput::action_handle_t) == sizeof(uint16_t));
    auto pType = das::make_smart<das::TypeDecl>(das::Type::tUInt16);
    pType->alias = "action_handle_t";
    addAlias(pType);

    das::addExtern<DAS_BIND_FUN(dainput::get_actions_binding_column_active)>(*this, lib, "get_actions_binding_column_active",
      das::SideEffects::accessExternal, "dainput::get_actions_binding_column_active");
    das::addExtern<DAS_BIND_FUN(dainput::get_action_handle)>(*this, lib, "get_action_handle", das::SideEffects::accessExternal,
      "dainput::get_action_handle");
    das::addExtern<DAS_BIND_FUN(dainput::get_action_name)>(*this, lib, "get_action_name", das::SideEffects::accessExternal,
      "dainput::get_action_name");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_last_used_device_mask)>(*this, lib, "get_last_used_device_mask",
      das::SideEffects::accessExternal, "bind_dascript::get_last_used_device_mask");
    das::addExtern<DAS_BIND_FUN(dainput::is_action_active)>(*this, lib, "is_action_active", das::SideEffects::accessExternal,
      "dainput::is_action_active");
    das::addExtern<DAS_BIND_FUN(dainput::is_action_enabled)>(*this, lib, "is_action_enabled", das::SideEffects::accessExternal,
      "dainput::is_action_enabled");
    das::addExtern<DAS_BIND_FUN(dainput::set_action_enabled)>(*this, lib, "set_action_enabled", das::SideEffects::modifyExternal,
      "dainput::set_action_enabled");
    das::addExtern<DAS_BIND_FUN(dainput::set_action_mask_immediate)>(*this, lib, "set_action_mask_immediate",
      das::SideEffects::modifyExternal, "dainput::set_action_mask_immediate")
      ->args({"action_handle", "is_enabled"});
    das::addExtern<DAS_BIND_FUN(dainput::is_action_mask_immediate)>(*this, lib, "is_action_mask_immediate",
      das::SideEffects::accessExternal, "dainput::is_action_mask_immediate")
      ->args({"action_handle"});
    das::addExtern<DAS_BIND_FUN(dainput::get_digital_action_binding)>(*this, lib, "get_digital_action_binding",
      das::SideEffects::accessExternal, "dainput::get_digital_action_binding");
    das::addExtern<DAS_BIND_FUN(dainput::get_analog_axis_action_binding)>(*this, lib, "get_analog_axis_action_binding",
      das::SideEffects::accessExternal, "dainput::get_analog_axis_action_binding");
    das::addExtern<DAS_BIND_FUN(dainput::get_analog_stick_action_binding)>(*this, lib, "get_analog_stick_action_binding",
      das::SideEffects::accessExternal, "dainput::get_analog_stick_action_binding");
    das::addExtern<DAS_BIND_FUN(dainput::get_analog_stick_action_smooth_value)>(*this, lib, "get_analog_stick_action_smooth_value",
      das::SideEffects::accessExternal, "dainput::get_analog_stick_action_smooth_value");
    das::addExtern<DAS_BIND_FUN(dainput::set_analog_stick_action_smooth_value)>(*this, lib, "set_analog_stick_action_smooth_value",
      das::SideEffects::accessExternal, "dainput::set_analog_stick_action_smooth_value");
    das::addExtern<DAS_BIND_FUN(dainput::get_actions_count)>(*this, lib, "get_actions_count", das::SideEffects::accessExternal,
      "dainput::get_actions_count");
    das::addExtern<DAS_BIND_FUN(dainput::get_action_handle_by_ord)>(*this, lib, "get_action_handle_by_ord",
      das::SideEffects::accessExternal, "dainput::get_action_handle_by_ord");
    das::addExtern<DAS_BIND_FUN(dainput::check_bindings_conflicts_one)>(*this, lib, "check_bindings_conflicts_one",
      das::SideEffects::accessExternal, "dainput::check_bindings_conflicts_one");
    das::addExtern<DAS_BIND_FUN(dainput::reset_digital_action_sticky_toggle)>(*this, lib, "reset_digital_action_sticky_toggle",
      das::SideEffects::accessExternal, "dainput::reset_digital_action_sticky_toggle");
    das::addExtern<const DigitalAction &(*)(dainput::action_handle_t), dainput::get_digital_action_state, das::SimNode_ExtFuncCallRef>(
      *this, lib, "get_digital_action_state", das::SideEffects::accessExternal, "dainput::get_digital_action_state");
    das::addExtern<const AnalogStickAction &(*)(dainput::action_handle_t), dainput::get_analog_stick_action_state,
      das::SimNode_ExtFuncCallRef>(*this, lib, "get_analog_stick_action_state", das::SideEffects::accessExternal,
      "dainput::get_analog_stick_action_state");
    das::addExtern<const AnalogAxisAction &(*)(dainput::action_handle_t), dainput::get_analog_axis_action_state,
      das::SimNode_ExtFuncCallRef>(*this, lib, "get_analog_axis_action_state", das::SideEffects::accessExternal,
      "dainput::get_analog_axis_action_state");
    das::addExtern<DAS_BIND_FUN(dainput::get_action_set_handle)>(*this, lib, "get_action_set_handle", das::SideEffects::accessExternal,
      "dainput::get_action_set_handle");
    das::addExtern<DAS_BIND_FUN(dainput::activate_action_set)>(*this, lib, "activate_action_set", das::SideEffects::modifyExternal,
      "dainput::activate_action_set");
    das::addExtern<DAS_BIND_FUN(dainput::set_analog_axis_action_state)>(*this, lib, "set_analog_axis_action_state",
      das::SideEffects::modifyExternal, "dainput::set_analog_axis_action_state");
    das::addExtern<DAS_BIND_FUN(dainput::send_action_event)>(*this, lib, "send_action_event", das::SideEffects::modifyExternal,
      "dainput::send_action_event");
    das::addExtern<DAS_BIND_FUN(dainput::dump_action_sets)>(*this, lib, "dump_action_sets", das::SideEffects::modifyExternal,
      "dainput::dump_action_sets");
    das::addExtern<DAS_BIND_FUN(dainput::dump_action_bindings)>(*this, lib, "dump_action_bindings", das::SideEffects::modifyExternal,
      "dainput::dump_action_bindings");
    das::addExtern<DAS_BIND_FUN(dainput::dump_action_sets_stack)>(*this, lib, "dump_action_sets_stack",
      das::SideEffects::modifyExternal, "dainput::dump_action_sets_stack");

    das::addExtern<DAS_BIND_FUN(bind_dascript::single_button_id_get_devId)>(*this, lib, "single_button_id_get_devId",
      das::SideEffects::none, "bind_dascript::single_button_id_get_devId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::single_button_id_get_btnId)>(*this, lib, "single_button_id_get_btnId",
      das::SideEffects::none, "bind_dascript::single_button_id_get_btnId");

    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_devId)>(*this, lib, "digital_action_binding_get_devId",
      das::SideEffects::none, "bind_dascript::digital_action_binding_get_devId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_ctrlId)>(*this, lib, "digital_action_binding_get_ctrlId",
      das::SideEffects::none, "bind_dascript::digital_action_binding_get_ctrlId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_eventType)>(*this, lib,
      "digital_action_binding_get_eventType", das::SideEffects::none, "bind_dascript::digital_action_binding_get_eventType");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_axisCtrlThres)>(*this, lib,
      "digital_action_binding_get_axisCtrlThres", das::SideEffects::none, "bind_dascript::digital_action_binding_get_axisCtrlThres");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_btnCtrl)>(*this, lib, "digital_action_binding_get_btnCtrl",
      das::SideEffects::none, "bind_dascript::digital_action_binding_get_btnCtrl");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_stickyToggle)>(*this, lib,
      "digital_action_binding_get_stickyToggle", das::SideEffects::none, "bind_dascript::digital_action_binding_get_stickyToggle");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_unordCombo)>(*this, lib,
      "digital_action_binding_get_unordCombo", das::SideEffects::none, "bind_dascript::digital_action_binding_get_unordCombo");
    das::addExtern<DAS_BIND_FUN(bind_dascript::digital_action_binding_get_modCnt)>(*this, lib, "digital_action_binding_get_modCnt",
      das::SideEffects::none, "bind_dascript::digital_action_binding_get_modCnt");

    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_devId)>(*this, lib,
      "analog_axis_action_binding_get_devId", das::SideEffects::none, "bind_dascript::analog_axis_action_binding_get_devId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_axisId)>(*this, lib,
      "analog_axis_action_binding_get_axisId", das::SideEffects::none, "bind_dascript::analog_axis_action_binding_get_axisId");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_invAxis)>(*this, lib,
      "analog_axis_action_binding_get_invAxis", das::SideEffects::none, "bind_dascript::analog_axis_action_binding_get_invAxis");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_axisRelativeVal)>(*this, lib,
      "analog_axis_action_binding_get_axisRelativeVal", das::SideEffects::none,
      "bind_dascript::analog_axis_action_binding_get_axisRelativeVal");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_instantIncDecBtn)>(*this, lib,
      "analog_axis_action_binding_get_instantIncDecBtn", das::SideEffects::none,
      "bind_dascript::analog_axis_action_binding_get_instantIncDecBtn");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_quantizeValOnIncDecBtn)>(*this, lib,
      "analog_axis_action_binding_get_quantizeValOnIncDecBtn", das::SideEffects::none,
      "bind_dascript::analog_axis_action_binding_get_quantizeValOnIncDecBtn");
    das::addExtern<DAS_BIND_FUN(bind_dascript::analog_axis_action_binding_get_modCnt)>(*this, lib,
      "analog_axis_action_binding_get_modCnt", das::SideEffects::none, "bind_dascript::analog_axis_action_binding_get_modCnt");

    das::addExtern<DAS_BIND_FUN(bind_dascript::global_cls_drv_update_cs_lock)>(*this, lib, "global_cls_drv_update_cs_lock",
      das::SideEffects::modifyExternal, "bind_dascript::global_cls_drv_update_cs_lock");

    das::addConstant(*this, "BTN_pressed", uint32_t(dainput::DigitalActionBinding::BTN_pressed));
    das::addConstant(*this, "BTN_pressed_long", uint32_t(dainput::DigitalActionBinding::BTN_pressed_long));
    das::addConstant(*this, "BTN_pressed2", uint32_t(dainput::DigitalActionBinding::BTN_pressed2));
    das::addConstant(*this, "BTN_pressed3", uint32_t(dainput::DigitalActionBinding::BTN_pressed3));
    das::addConstant(*this, "BTN_released", uint32_t(dainput::DigitalActionBinding::BTN_released));
    das::addConstant(*this, "BTN_released_short", uint32_t(dainput::DigitalActionBinding::BTN_released_short));
    das::addConstant(*this, "BTN_released_long", uint32_t(dainput::DigitalActionBinding::BTN_released_long));

    das::addConstant(*this, "BAD_ACTION_HANDLE", dainput::action_handle_t(dainput::BAD_ACTION_HANDLE));
    das::addConstant(*this, "BAD_ACTION_SET_HANDLE", dainput::action_handle_t(dainput::BAD_ACTION_SET_HANDLE));
    das::addConstant(*this, "TYPEGRP_DIGITAL", uint16_t(dainput::TYPEGRP_DIGITAL));
    das::addConstant(*this, "TYPEGRP_AXIS", uint16_t(dainput::TYPEGRP_AXIS));
    das::addConstant(*this, "TYPEGRP_STICK", uint16_t(dainput::TYPEGRP_STICK));

    das::addConstant(*this, "DEV_USED_mouse", uint32_t(dainput::DEV_USED_mouse));
    das::addConstant(*this, "DEV_USED_kbd", uint32_t(dainput::DEV_USED_kbd));
    das::addConstant(*this, "DEV_USED_gamepad", uint32_t(dainput::DEV_USED_gamepad));
    das::addConstant(*this, "DEV_USED_touch", uint32_t(dainput::DEV_USED_touch));

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorInput.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorInputModule, bind_dascript);
