// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dasBinding.h>

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <EASTL/type_traits.h>
#include "stdRendObj.h"


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

struct ElementAnnotation : das::ManagedStructureAnnotation<::darg::Element>
{
  ElementAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Element", ml, "::darg::Element") { ADD_FIELD(props); }
};

struct PropertiesAnnotation : das::ManagedStructureAnnotation<::darg::Properties>
{
  PropertiesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Properties", ml, "::darg::Properties") {}
};

struct PictureAnnotation : das::ManagedStructureAnnotation<::darg::Picture>
{
  PictureAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Picture", ml, "::darg::Picture") {}
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

::darg::Picture *props_get_picture(const ::darg::Properties &props, const char *key)
{
  return props.getPicture(Sqrat::Object(key, props.scriptDesc.GetVM()));
}

DataBlock *props_get_blk(const ::darg::Properties &props, const char *key)
{
  return props.getBlk(Sqrat::Object(key, props.scriptDesc.GetVM()));
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
    addBuiltinDependency(lib, require("DagorDataBlock"));

    addAnnotation(das::make_smart<ElemRenderDataAnnotation>(lib));
    addAnnotation(das::make_smart<RenderStateAnnotation>(lib));
    addAnnotation(das::make_smart<PropertiesAnnotation>(lib));
    addAnnotation(das::make_smart<ElementAnnotation>(lib));
    addAnnotation(das::make_smart<PictureAnnotation>(lib));


    // Properties

    das::addExtern<DAS_BIND_FUN(props_get_int)>(*this, lib, "getInt", das::SideEffects::none, "::darg::props_get_int");
    das::addExtern<DAS_BIND_FUN(props_get_float)>(*this, lib, "getFloat", das::SideEffects::none, "::darg::props_get_float");
    das::addExtern<DAS_BIND_FUN(props_get_bool)>(*this, lib, "getBool", das::SideEffects::none, "::darg::props_get_bool");
    das::addExtern<DAS_BIND_FUN(props_get_color)>(*this, lib, "getColor", das::SideEffects::none, "::darg::props_get_color");
    das::addExtern<DAS_BIND_FUN(props_get_picture)>(*this, lib, "getPicture", das::SideEffects::none, "::darg::props_get_picture");
    das::addExtern<DAS_BIND_FUN(props_get_blk)>(*this, lib, "getBlk", das::SideEffects::none, "::darg::props_get_blk");
    das::addExtern<void (*)(StdGuiRender::GuiContext &, Picture *, Point2, Point2, E3DCOLOR), ::darg::render_picture>(*this, lib,
      "render_picture", das::SideEffects::modifyExternal, "::darg::render_picture");
    das::addExtern<void (*)(StdGuiRender::GuiContext &, Picture *, Point2, Point2, E3DCOLOR, float, float, bool, bool, float),
      ::darg::render_picture>(*this, lib, "render_picture", das::SideEffects::modifyExternal, "::darg::render_picture");
    BIND_MEMBER(::darg::Properties, getFontId, "getFontId", das::SideEffects::none);
    BIND_MEMBER(::darg::Properties, getFontSize, "getFontSize", das::SideEffects::none);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <daRg/dasBinding.h>\n";
    tw << "#include <daRg/stdRendObj.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace darg

REGISTER_MODULE_IN_NAMESPACE(ModuleDarg, darg);
