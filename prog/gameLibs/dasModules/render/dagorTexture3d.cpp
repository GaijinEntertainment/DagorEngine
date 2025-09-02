// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dagorTexture3d.h>
#include <daScript/ast/ast_policy_types.h>

DAS_BIND_ENUM_CAST(D3DResourceType);
DAS_BASE_BIND_ENUM(D3DResourceType, D3DResourceType, TEX, CUBETEX, VOLTEX, ARRTEX, CUBEARRTEX, SBUF);

namespace das
{
IMPLEMENT_OP1_EVAL_POLICY(BoolNot, D3DRESID);
IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ, D3DRESID);
IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu, D3DRESID);
}; // namespace das

G_STATIC_ASSERT(D3DRESID::INVALID_ID == 0u);

struct D3DRESIDAnnotation final : das::ManagedValueAnnotation<D3DRESID>
{
  D3DRESIDAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "D3DRESID") { cppName = " ::D3DRESID"; }
  virtual void walk(das::DataWalker &walker, void *data) override { walker.UInt(*reinterpret_cast<unsigned *>(data)); }
  virtual bool hasNonTrivialCtor() const override { return false; }
  virtual bool hasNonTrivialCopy() const override { return false; }
  virtual bool hasNonTrivialDtor() const override { return false; }
};

struct TextureInfoAnnotation final : das::ManagedStructureAnnotation<TextureInfo, false>
{
  TextureInfoAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TextureInfo", ml)
  {
    cppName = " ::TextureInfo";
    addField<DAS_BIND_MANAGED_FIELD(w)>("w");
    addField<DAS_BIND_MANAGED_FIELD(h)>("h");
    addField<DAS_BIND_MANAGED_FIELD(d)>("d");
    addField<DAS_BIND_MANAGED_FIELD(a)>("a");
    addField<DAS_BIND_MANAGED_FIELD(mipLevels)>("mipLevels");
    addField<DAS_BIND_MANAGED_FIELD(type)>("type");
    addField<DAS_BIND_MANAGED_FIELD(cflg)>("cflg");
  }
};

struct BaseTextureAnnotation final : das::ManagedStructureAnnotation<BaseTexture, false>
{
  BaseTextureAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BaseTexture", ml) { cppName = " ::BaseTexture"; }
};

struct TextureFactoryAnnotation final : das::ManagedStructureAnnotation<TextureFactory, false>
{
  TextureFactoryAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TextureFactory", ml)
  {
    cppName = " ::TextureFactory";
  }
};

struct SbufferAnnotation : das::ManagedStructureAnnotation<Sbuffer, false>
{
  SbufferAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Sbuffer", ml) { cppName = " ::Sbuffer"; }
};


static char dagorTexture3d_das[] =
#include "dagorTexture3d.das.inl"
  ;

namespace bind_dascript
{
class DagorTexture3DModule final : public das::Module
{
public:
  DagorTexture3DModule() : das::Module("DagorTexture3D")
  {
    das::ModuleLibrary lib(this);

    addEnumeration(das::make_smart<EnumerationD3DResourceType>());

    addAnnotation(das::make_smart<D3DRESIDAnnotation>(lib));
    addAnnotation(das::make_smart<TextureInfoAnnotation>(lib));
    addAnnotation(das::make_smart<BaseTextureAnnotation>(lib));
    addAnnotation(das::make_smart<SbufferAnnotation>(lib));
    addAnnotation(das::make_smart<TextureFactoryAnnotation>(lib));

    // ensure that resid is actually zero-inited, in case anyone decides to
    // change that -- required for the type to work as a value inside das
    G_ASSERT(D3DRESID{} == D3DRESID{0u});

    das::addFunctionBasic<D3DRESID>(*this, lib);
    das::addExtern<DAS_BIND_FUN(make_D3DRESID)>(*this, lib, "D3DRESID", das::SideEffects::none, "bind_dascript::D3DRESID");
    das::addExtern<DAS_BIND_FUN(make_uint_D3DRESID)>(*this, lib, "D3DRESID", das::SideEffects::none,
      "bind_dascript::make_uint_D3DRESID");

    das::addExtern<DAS_BIND_FUN(acquire_managed_tex)>(*this, lib, "acquire_managed_tex", das::SideEffects::modifyExternal,
      "::acquire_managed_tex");
    das::addExtern<DAS_BIND_FUN(release_managed_tex)>(*this, lib, "release_managed_tex", das::SideEffects::modifyExternal,
      "::release_managed_tex");
    das::addExtern<DAS_BIND_FUN(add_managed_texture)>(*this, lib, "add_managed_texture", das::SideEffects::modifyExternal,
      "::add_managed_texture");
    das::addExtern<DAS_BIND_FUN(get_managed_texture_id)>(*this, lib, "get_managed_texture_id", das::SideEffects::accessExternal,
      "::get_managed_texture_id");

#define BIND_METHOD(METHOD, NAME, SIDE_EFFECT)                                                           \
  {                                                                                                      \
    using method = DAS_CALL_MEMBER(METHOD);                                                              \
    das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, NAME, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(METHOD)); \
  }
    BIND_METHOD(BaseTexture::getinfo, "getinfo", das::SideEffects::modifyArgument)


#define BIND_FUNCTION(function, sinonim, side_effect) \
  das::addExtern<DAS_BIND_FUN(function)>(*this, lib, sinonim, side_effect, "bind_dascript::" #function);

    BIND_FUNCTION(sbuffer_update_data, "__builtin_updateData", das::SideEffects::modifyArgumentAndExternal)

#undef BIND_FUNCTION
#undef BIND_METHOD

    das::addCtorAndUsing<TextureInfo>(*this, lib, "TextureInfo", " ::TextureInfo");

    compileBuiltinModule("dagorTexture3d.das", (unsigned char *)dagorTexture3d_das, sizeof(dagorTexture3d_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/dagorTexture3d.h>\n";
    tw << "#include <ecs/render/resPtr.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorTexture3DModule, bind_dascript);
