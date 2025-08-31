// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <render/world/portalRenderer.h>


MAKE_TYPE_FACTORY(PortalParams, PortalParams);

struct PortalParamsAnnotation final : das::ManagedStructureAnnotation<PortalParams, false>
{
  PortalParamsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PortalParams", ml)
  {
    cppName = " ::PortalParams";
    addField<DAS_BIND_MANAGED_FIELD(portalRange)>("portalRange");
    addField<DAS_BIND_MANAGED_FIELD(isBidirectional)>("isBidirectional");
    addField<DAS_BIND_MANAGED_FIELD(useDenseFog)>("useDenseFog");
    addField<DAS_BIND_MANAGED_FIELD(fogStartDist)>("fogStartDist");
  }
  bool isLocal() const override { return true; }
};

namespace bind_dascript
{
class PortalRendererModule final : public das::Module
{
public:
  PortalRendererModule() : das::Module("PortalRenderer")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<PortalParamsAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(portal_renderer_mgr::update_portal)>(*this, lib, "update_portal", das::SideEffects::accessExternal,
      "portal_renderer_mgr::update_portal");
    das::addExtern<DAS_BIND_FUN(portal_renderer_mgr::allocate_portal)>(*this, lib, "allocate_portal", das::SideEffects::accessExternal,
      "portal_renderer_mgr::allocate_portal");
    das::addExtern<DAS_BIND_FUN(portal_renderer_mgr::free_portal)>(*this, lib, "free_portal", das::SideEffects::accessExternal,
      "portal_renderer_mgr::free_portal");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <render/world/portalRenderer.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PortalRendererModule, bind_dascript);
