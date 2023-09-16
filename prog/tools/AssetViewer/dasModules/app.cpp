#include <daScript/daScript.h>

// should be extern and have another behavior for aot
char const *get_game_name() { return "asset_viewer"; }

namespace bind_dascript
{
class AppModule final : public das::Module
{
public:
  AppModule() : das::Module("app")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(get_game_name)>(*this, lib, "get_game_name", das::SideEffects::accessExternal, "::get_game_name");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &) const override { return das::ModuleAotType::cpp; }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(AppModule, bind_dascript);
