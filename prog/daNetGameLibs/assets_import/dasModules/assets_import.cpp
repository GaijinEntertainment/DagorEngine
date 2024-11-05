// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assets_import.h"


DAS_BASE_BIND_ENUM(AssetLoadingStatus, AssetLoadingStatus, NotLoaded, Loading, Loaded, LoadedWithErrors);

struct DagorAssetAnnotation final : das::ManagedStructureAnnotation<DagorAsset, false, false>
{
  DagorAssetAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DagorAsset", ml, "DagorAsset")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getType)>("typeId", "getType");
    addProperty<DAS_BIND_MANAGED_PROP(getTypeStr)>("typeName", "getTypeStr");
    addProperty<DAS_BIND_MANAGED_PROP(getName)>("name", "getName");
    addField<DAS_BIND_MANAGED_FIELD(props)>("props", "props");
  }
};

struct DagorAssetMgrAnnotation final : das::ManagedStructureAnnotation<DagorAssetMgr, false, false>
{
  DagorAssetMgrAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DagorAssetMgr", ml, "DagorAssetMgr")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getTexAssetTypeId)>("texAssetTypeId", "getTexAssetTypeId");
  }
};

namespace bind_dascript
{
void os_shell_execute_hidden(const char *op, const char *file, const char *params, const char *dir)
{
  ::os_shell_execute(op, file, params, dir, false, OpenConsoleMode::Hide);
}

class AssetsImportModule final : public das::Module
{
public:
  AssetsImportModule() : das::Module("AssetsImport")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<DagorAssetAnnotation>(lib));
    addAnnotation(das::make_smart<DagorAssetMgrAnnotation>(lib));
    addEnumeration(das::make_smart<EnumerationAssetLoadingStatus>());

    using method_getName = DAS_CALL_MEMBER(DagorAsset::getName);
    das::addExtern<DAS_CALL_METHOD(method_getName)>(*this, lib, "getName", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(DagorAsset::getName));

    das::addExtern<DAS_BIND_FUN(bind_dascript::asset_getSrcFilePath)>(*this, lib, "getSrcFilePath", das::SideEffects::accessExternal,
      "bind_dascript::asset_getSrcFilePath");

    using method_findAsset =
      das::das_call_member<DagorAsset *(::DagorAssetMgr::*)(const char *, int) const, &::DagorAssetMgr::findAsset>;
    das::addExtern<DAS_CALL_METHOD(method_findAsset)>(*this, lib, "findAsset", das::SideEffects::modifyExternal,
      "das_call_member<DagorAsset*(::DagorAssetMgr::*)(const char *, int) const, &::DagorAssetMgr::findAsset>::invoke");

    using method_findAsset2 = das::das_call_member<DagorAsset *(::DagorAssetMgr::*)(const char *) const, &::DagorAssetMgr::findAsset>;
    das::addExtern<DAS_CALL_METHOD(method_findAsset2)>(*this, lib, "findAsset", das::SideEffects::modifyExternal,
      "das_call_member<DagorAsset*(::DagorAssetMgr::*)(const char *) const, &::DagorAssetMgr::findAsset>::invoke");

    using method_getAssetTypeId = DAS_CALL_MEMBER(DagorAssetMgr::getAssetTypeId);
    das::addExtern<DAS_CALL_METHOD(method_getAssetTypeId)>(*this, lib, "getAssetTypeId", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(DagorAssetMgr::getAssetTypeId));

    using method_getAsset = DAS_CALL_MEMBER(DagorAssetMgr::getAsset);
    das::addExtern<DAS_CALL_METHOD(method_getAsset)>(*this, lib, "getAsset", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(DagorAssetMgr::getAsset));

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_filtered_assets)>(*this, lib, "getFilteredAssets", das::SideEffects::modifyExternal,
      "bind_dascript::get_filtered_assets");

    das::addExtern<DAS_BIND_FUN(dabuildcache::invalidate_asset)>(*this, lib, "invalidate_asset", das::SideEffects::modifyExternal,
      "dabuildcache::invalidate_asset");

    das::addExtern<DAS_BIND_FUN(bind_dascript::iterate_dag_textures)>(*this, lib, "iterate_dag_textures",
      das::SideEffects::modifyExternal, "bind_dascript::iterate_dag_textures");
    das::addExtern<DAS_BIND_FUN(bind_dascript::copy_file)>(*this, lib, "copy_file", das::SideEffects::modifyExternal,
      "bind_dascript::copy_file");

    das::addExtern<DAS_BIND_FUN(get_asset_status)>(*this, lib, "get_asset_status", das::SideEffects::modifyArgumentAndAccessExternal,
      "get_asset_status");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_asset_source_files)>(*this, lib, "get_asset_source_files",
      das::SideEffects::modifyExternal, "bind_dascript::get_asset_source_files");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_asset_dependencies)>(*this, lib, "get_asset_dependencies",
      das::SideEffects::modifyExternal, "bind_dascript::get_asset_dependencies");

    das::addExtern<DAS_BIND_FUN(bind_dascript::os_shell_execute_hidden)>(*this, lib, "os_shell_execute",
      das::SideEffects::modifyExternal, "bind_dascript::os_shell_execute_hidden");


    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/assets_import.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
AUTO_REGISTER_MODULE_IN_NAMESPACE(AssetsImportModule, bind_dascript);

extern das::Module *register_Module_StdDlg(); // REGISTER_MODULE_IN_NAMESPACE(Module_StdDlg,das);
das::ModulePullHelper Module_StdDlgRegisterHelper(&register_Module_StdDlg);

size_t AssetsImportModule_pull = 0;