// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotDagorImgui.h>
#include <dasModules/dagorTexture3d.h>

DAS_BASE_BIND_ENUM(ImGuiState, ImGuiState, OFF, ACTIVE, OVERLAY)

namespace bind_dascript
{
das::mutex functionsRegistryMutex;
ImguiRegistry functionsRegistry;
das::mutex windowsRegistryMutex;
ImguiRegistry windowsRegistry;

class DagorImguiModule final : public das::Module
{
  static constexpr const char *moduleName = "DagorImgui";

public:
  das::smart_ptr<bind_dascript::RegisterImguiFunctionAnnotation> funcReg;
  das::smart_ptr<bind_dascript::RegisterImguiWindowAnnotation> windowReg;

public:
  DagorImguiModule() : das::Module(moduleName)
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addEnumeration(das::make_smart<EnumerationImGuiState>());
    funcReg = das::make_smart<bind_dascript::RegisterImguiFunctionAnnotation>(functionsRegistry, functionsRegistryMutex);
    addAnnotation(funcReg);
    windowReg = das::make_smart<bind_dascript::RegisterImguiWindowAnnotation>(windowsRegistry, windowsRegistryMutex);
    addAnnotation(windowReg);
    das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) {
      auto mod = das::Module::require(moduleName);
      if (mod == nullptr)
        return;
      auto dagorImguiMod = static_cast<DagorImguiModule *>(mod);
      dagorImguiMod->funcReg->unloadContext(ctx);
      dagorImguiMod->windowReg->unloadContext(ctx);
    });

    das::addExtern<DAS_BIND_FUN(imgui_set_bold_font)>(*this, lib, "imgui_set_bold_font", das::SideEffects::modifyExternal,
      "::imgui_set_bold_font");
    das::addExtern<DAS_BIND_FUN(imgui_set_mono_font)>(*this, lib, "imgui_set_mono_font", das::SideEffects::modifyExternal,
      "::imgui_set_mono_font");
    das::addExtern<DAS_BIND_FUN(imgui_set_default_font)>(*this, lib, "imgui_set_default_font", das::SideEffects::modifyExternal,
      "::imgui_set_default_font");
    das::addExtern<DAS_BIND_FUN(imgui_window_set_visible)>(*this, lib, "imgui_window_set_visible", das::SideEffects::modifyExternal,
      "::imgui_window_set_visible");
    das::addExtern<DAS_BIND_FUN(imgui_window_is_visible)>(*this, lib, "imgui_window_is_visible", das::SideEffects::accessExternal,
      "::imgui_window_is_visible");
    das::addExtern<DAS_BIND_FUN(imgui_get_state)>(*this, lib, "imgui_get_state", das::SideEffects::accessExternal,
      "::imgui_get_state");
    das::addExtern<DAS_BIND_FUN(imgui_save_blk)>(*this, lib, "imgui_save_blk", das::SideEffects::accessExternal, "::imgui_save_blk");
    das::addExtern<DAS_BIND_FUN(bind_dascript::imgui_get_blk), das::SimNode_ExtFuncCallRef>(*this, lib, "imgui_get_blk",
      das::SideEffects::accessExternal, "::bind_dascript::imgui_get_blk");
    das::addExtern<DAS_BIND_FUN(::imgui_apply_style_from_blk)>(*this, lib, "imgui_apply_style_from_blk",
      das::SideEffects::modifyExternal, "::imgui_apply_style_from_blk");
    das::addExtern<DAS_BIND_FUN(::convert_dag_res_id_to_imgui)>(*this, lib, "convert_dag_res_id_to_imgui", das::SideEffects::none,
      "::convert_dag_res_id_to_imgui");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorImgui.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorImguiModule, bind_dascript);
