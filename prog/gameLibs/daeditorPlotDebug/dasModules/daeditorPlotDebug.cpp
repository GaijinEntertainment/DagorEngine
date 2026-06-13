// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../daeditor_plot_debug.h"

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{
class DaeditorPlotDebugModule final : public das::Module
{
public:
  DaeditorPlotDebugModule() : das::Module("DaeditorPlotDebug")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(daeditor_plot_debug_signal)>(*this, lib, "daeditor_plot_debug_signal",
      das::SideEffects::modifyExternal, "daeditor_plot_debug_signal")
      ->args({"value", "slot"})
      ->arg_init(1, new das::ExprConstInt(0));
    das::addExtern<DAS_BIND_FUN(daeditor_plot_debug_get_signal)>(*this, lib, "daeditor_plot_debug_get_signal",
      das::SideEffects::accessExternal, "daeditor_plot_debug_get_signal")
      ->args({"slot"})
      ->arg_init(0, new das::ExprConstInt(0));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"daeditorPlotDebug/daeditor_plot_debug.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
AUTO_REGISTER_MODULE_IN_NAMESPACE(DaeditorPlotDebugModule, bind_dascript);

size_t DaeditorPlotDebugModule_pull = 0;
