// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dasBinding.h>

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <EASTL/type_traits.h>


#define ADD_FIELD(field) addField<DAS_BIND_MANAGED_FIELD(field)>(#field);

#define BIND_MEMBER(CLASS_NAME, FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                 \
  {                                                                                                                              \
    using memberMethod = DAS_CALL_MEMBER(CLASS_NAME::FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(CLASS_NAME::FUNC_NAME)); \
  }

#define BIND_MEMBER_SIGNATURE(CLASS_NAME, FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)    \
  {                                                                                      \
    using memberMethod = das::das_call_member<SIGNATURE, &CLASS_NAME::FUNC_NAME>;        \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT,      \
      "das::das_call_member<" #SIGNATURE ", &" #CLASS_NAME "::" #FUNC_NAME ">::invoke"); \
  }


namespace darg
{

struct RenderStateAnnotation : das::ManagedStructureAnnotation<::darg::RenderState>
{
  RenderStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RenderState", ml, "::darg::RenderState")
  {
    ADD_FIELD(opacity);
  }
};

struct ElemRenderDataAnnotation : das::ManagedStructureAnnotation<::darg::ElemRenderData>
{
  ElemRenderDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ElemRenderData", ml, "::darg::ElemRenderData")
  {
    ADD_FIELD(pos);
    ADD_FIELD(size);
  }
};

struct ElementAnnotation : das::ManagedStructureAnnotation<Element>
{
  ElementAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Element", ml, "Element") { ADD_FIELD(props); }
};

struct PropertiesAnnotation : das::ManagedStructureAnnotation<::darg::Properties>
{
  PropertiesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Properties", ml, "::darg::Properties") {}
};


/* Properties */

int props_get_int(const ::darg::Properties &props, const char *key, int def)
{
  return props.getInt(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

float props_get_float(const ::darg::Properties &props, const char *key, float def)
{
  return props.getFloat(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

bool props_get_bool(const ::darg::Properties &props, const char *key, bool def)
{
  return props.getBool(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

E3DCOLOR props_get_color(const ::darg::Properties &props, const char *key, E3DCOLOR def)
{
  return props.getColor(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

class ModuleDarg : public das::Module
{
public:
  ModuleDarg() : das::Module("darg")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorStdGuiRender"));

    addAnnotation(das::make_smart<ElemRenderDataAnnotation>(lib));
    addAnnotation(das::make_smart<RenderStateAnnotation>(lib));
    addAnnotation(das::make_smart<PropertiesAnnotation>(lib));
    addAnnotation(das::make_smart<ElementAnnotation>(lib));


    // Properties

    das::addExtern<DAS_BIND_FUN(props_get_int)>(*this, lib, "getInt", das::SideEffects::none, "::darg::props_get_int");
    das::addExtern<DAS_BIND_FUN(props_get_float)>(*this, lib, "getFloat", das::SideEffects::none, "::darg::props_get_float");
    das::addExtern<DAS_BIND_FUN(props_get_bool)>(*this, lib, "getBool", das::SideEffects::none, "::darg::props_get_bool");
    das::addExtern<DAS_BIND_FUN(props_get_color)>(*this, lib, "getColor", das::SideEffects::none, "::darg::props_get_color");
    BIND_MEMBER(::darg::Properties, getFontId, "getFontId", das::SideEffects::none);
    BIND_MEMBER(::darg::Properties, getFontSize, "getFontSize", das::SideEffects::none);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <daRg/dasBinding.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace darg

REGISTER_MODULE_IN_NAMESPACE(ModuleDarg, darg);
