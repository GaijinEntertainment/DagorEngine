#include <dasModules/aotDagorFindFiles.h>

namespace bind_dascript
{

static char dagorFindFiles_das[] =
#include "dagorFindFiles.das.inl"
  ;

class DagorFindFiles final : public das::Module
{
public:
  DagorFindFiles() : das::Module("DagorFindFiles")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(das_find_files_in_folder)>(*this, lib, "find_files_in_folder", das::SideEffects::accessExternal,
      "bind_dascript::das_find_files_in_folder");
    das::addExtern<DAS_BIND_FUN(das_find_file_in_vromfs)>(*this, lib, "find_file_in_vromfs", das::SideEffects::accessExternal,
      "bind_dascript::das_find_file_in_vromfs");
    compileBuiltinModule("dagorFindFiles.das", (unsigned char *)dagorFindFiles_das, sizeof(dagorFindFiles_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorFindFiles.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorFindFiles, bind_dascript);
