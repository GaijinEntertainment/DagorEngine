// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "input/dasModules/touchInput.h"

namespace bind_dascript
{
class TouchInputModule final : public das::Module
{
public:
  TouchInputModule() : das::Module("TouchInput")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorInput"));

    das::addExtern<DAS_BIND_FUN(press_button)>(*this, lib, "press_button", das::SideEffects::modifyExternal,
      "bind_dascript::press_button");
    das::addExtern<DAS_BIND_FUN(release_button)>(*this, lib, "release_button", das::SideEffects::modifyExternal,
      "bind_dascript::release_button");
    das::addExtern<DAS_BIND_FUN(is_button_pressed)>(*this, lib, "is_button_pressed", das::SideEffects::accessExternal,
      "bind_dascript::is_button_pressed");
    das::addExtern<DAS_BIND_FUN(set_stick_value)>(*this, lib, "set_stick_value", das::SideEffects::modifyExternal,
      "bind_dascript::set_stick_value");
    das::addExtern<DAS_BIND_FUN(get_stick_value)>(*this, lib, "get_stick_value", das::SideEffects::accessExternal,
      "bind_dascript::get_stick_value");
    das::addExtern<DAS_BIND_FUN(set_axis_value)>(*this, lib, "set_axis_value", das::SideEffects::modifyExternal,
      "bind_dascript::set_axis_value");
    das::addExtern<DAS_BIND_FUN(get_axis_value)>(*this, lib, "get_axis_value", das::SideEffects::accessExternal,
      "bind_dascript::get_axis_value");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"input/dasModules/touchInput.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(TouchInputModule, bind_dascript);