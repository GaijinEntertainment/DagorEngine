#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotGpuReadbackQuery.h>


namespace bind_dascript
{
class GpuReadbackQueryModule final : public das::Module
{
public:
  GpuReadbackQueryModule() : das::Module("gpuReadbackQuery")
  {
    das::ModuleLibrary lib(this);

    addEnumeration(das::make_smart<EnumerationGpuReadbackResultState>());

    das::addExtern<DAS_BIND_FUN(is_gpu_readback_query_successful)>(*this, lib, "is_gpu_readback_query_successful",
      das::SideEffects::none, "::is_gpu_readback_query_successful");

    das::addExtern<DAS_BIND_FUN(is_gpu_readback_query_failed)>(*this, lib, "is_gpu_readback_query_failed", das::SideEffects::none,
      "::is_gpu_readback_query_failed");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGpuReadbackQuery.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(GpuReadbackQueryModule, bind_dascript);
