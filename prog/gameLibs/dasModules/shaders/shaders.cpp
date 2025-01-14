// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasShaders.h>
#include <daScript/ast/ast_policy_types.h>


DAS_BASE_BIND_ENUM(SHVT, SHVT, INT, REAL, COLOR4, TEXTURE, BUFFER);

struct ShaderVarAnnotation : das::ManagedStructureAnnotation<ShaderVar, false>
{
  ShaderVarAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShaderVar", ml)
  {
    cppName = " ::ShaderVar";
    addProperty<DAS_BIND_MANAGED_PROP(get_var_id)>("varId", "get_var_id");
  }
};

struct ShaderMaterialAnnotation : das::ManagedStructureAnnotation<ShaderMaterial, false>
{
  ShaderMaterialAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShaderMaterial", ml)
  {
    cppName = " ::ShaderMaterial";
  }
};

struct ShaderMaterialPtrAnnotation : das::ManagedStructureAnnotation<ShaderMaterialPtr, false>
{
  ShaderMaterialPtrAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShaderMaterialPtr", ml)
  {
    cppName = " ::ShaderMaterialPtr";
  }

  bool canBePlacedInContainer() const override { return true; }
};

namespace bind_dascript
{
using namespace ShaderGlobal;

class DagorShaders final : public das::Module
{
public:
  DagorShaders() : das::Module("DagorShaders")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorResPtr"), true);
    addEnumeration(das::make_smart<EnumerationSHVT>());
    addAnnotation(das::make_smart<ShaderVarAnnotation>(lib));
    addAnnotation(das::make_smart<ShaderMaterialAnnotation>(lib));
    addAnnotation(das::make_smart<ShaderMaterialPtrAnnotation>(lib));


    das::addExtern<DAS_BIND_FUN(my_get_var_type)>(*this, lib, "get_var_type", das::SideEffects::modifyExternal,
      "::bind_dascript::my_get_var_type");
    das::addExtern<DAS_BIND_FUN(get_shader_variable_id)>(*this, lib, "get_shader_variable_id", das::SideEffects::modifyExternal,
      "get_shader_variable_id")
      ->arg_init(/*is_optional*/ 1, das::make_smart<das::ExprConstBool>(false));
    das::addExtern<bool (*)(int, int), set_int>(*this, lib, "set_int", das::SideEffects::modifyExternal, "::ShaderGlobal::set_int");
    das::addExtern<bool (*)(int, const IPoint4 &), set_int4>(*this, lib, "set_int4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_int4");
    das::addExtern<bool (*)(int, const TMatrix4 &), set_float4x4>(*this, lib, "set_float4x4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_float4x4");
    das::addExtern<bool (*)(int, float), set_real>(*this, lib, "set_real", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_real");
    das::addExtern<bool (*)(int, D3DRESID), set_texture>(*this, lib, "set_texture", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_texture");
    das::addExtern<bool (*)(int, D3DRESID), set_buffer>(*this, lib, "set_buffer", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_buffer");
    das::addExtern<bool (*)(int, ManagedTexView), set_texture>(*this, lib, "set_texture", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_texture");
    das::addExtern<bool (*)(int, ManagedBufView), set_buffer>(*this, lib, "set_buffer", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_buffer");
    das::addExtern<DAS_BIND_FUN(set_color4)>(*this, lib, "set_color4", das::SideEffects::modifyExternal, "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_e3d)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_p4)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_p3)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_p3f)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_f4)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_f3)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");
    das::addExtern<DAS_BIND_FUN(set_color4_f2)>(*this, lib, "set_color4", das::SideEffects::modifyExternal,
      "::ShaderGlobal::set_color4");

    das::addExtern<int (*)(int), get_int>(*this, lib, "get_int", das::SideEffects::modifyExternal, "::ShaderGlobal::get_int");
    das::addExtern<float (*)(int), get_real>(*this, lib, "get_real", das::SideEffects::modifyExternal, "::ShaderGlobal::get_real");
    das::addExtern<Color4 (*)(int), get_color4, das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_color4",
      das::SideEffects::modifyExternal, "::ShaderGlobal::get_color4");
    das::addExtern<DAS_BIND_FUN(get_tex)>(*this, lib, "get_tex", das::SideEffects::modifyExternal, "::ShaderGlobal::get_tex");
    das::addExtern<DAS_BIND_FUN(get_buf)>(*this, lib, "get_buf", das::SideEffects::modifyExternal, "::ShaderGlobal::get_buf");
    das::addExtern<DAS_BIND_FUN(get_shader_global_time_phase)>(*this, lib, "get_shader_global_time_phase",
      das::SideEffects::accessExternal, "::get_shader_global_time_phase");
    das::addExtern<DAS_BIND_FUN(get_slot_by_name)>(*this, lib, "get_slot_by_name", das::SideEffects::modifyExternal,
      "::ShaderGlobal::get_slot_by_name");

    das::addExtern<DAS_BIND_FUN(ShaderGlobal::getBlockId)>(*this, lib, "getBlockId", das::SideEffects::accessExternal,
      "ShaderGlobal::getBlockId")
      ->args({"block_name", "layer"})
      ->arg_init(1, das::make_smart<das::ExprConstInt>(-1));

    das::addExtern<DAS_BIND_FUN(ShaderGlobal::setBlock)>(*this, lib, "setBlock", das::SideEffects::accessExternal,
      "ShaderGlobal::setBlock")
      ->args({"block_id", "layer", "invalidate_cached_stblk"})
      ->arg_init(2, das::make_smart<das::ExprConstBool>(true));

    das::addExtern<DAS_BIND_FUN(exec_with_shader_blocks_scope_reset)>(*this, lib, "exec_with_shader_blocks_scope_reset",
      das::SideEffects::modifyExternal, "::bind_dascript::exec_with_shader_blocks_scope_reset");

    das::addConstant<int>(*this, "LAYER_FRAME", ShaderGlobal::LAYER_FRAME);
    das::addConstant<int>(*this, "LAYER_SCENE", ShaderGlobal::LAYER_SCENE);
    das::addConstant<int>(*this, "LAYER_OBJECT", ShaderGlobal::LAYER_OBJECT);
    das::addConstant<int>(*this, "LAYER_GLOBAL_CONST", ShaderGlobal::LAYER_GLOBAL_CONST);

#define ADD_METHOD(name, side_effects)                         \
  using method_##name = DAS_CALL_MEMBER(ShaderMaterial::name); \
  das::addExtern<DAS_CALL_METHOD(method_##name)>(*this, lib, #name, side_effects, DAS_CALL_MEMBER_CPP(ShaderMaterial::name));

    ADD_METHOD(get_flags, das::SideEffects::none)
    ADD_METHOD(get_num_textures, das::SideEffects::none)
    ADD_METHOD(get_texture, das::SideEffects::none)
    ADD_METHOD(hasVariable, das::SideEffects::none)
    ADD_METHOD(getColor4Variable, das::SideEffects::modifyArgument)
    ADD_METHOD(getRealVariable, das::SideEffects::modifyArgument)
    ADD_METHOD(getIntVariable, das::SideEffects::modifyArgument)
    ADD_METHOD(getTextureVariable, das::SideEffects::modifyArgument)
    ADD_METHOD(set_flags, das::SideEffects::modifyArgument)
    ADD_METHOD(set_int_param, das::SideEffects::modifyArgument)
    ADD_METHOD(set_real_param, das::SideEffects::modifyArgument)
    ADD_METHOD(set_color4_param, das::SideEffects::modifyArgument)
    ADD_METHOD(set_texture_param, das::SideEffects::modifyArgument)
    ADD_METHOD(set_texture, das::SideEffects::modifyArgument)

#undef ADD_METHOD

    das::addExtern<DAS_BIND_FUN(ShaderMaterialPtr_get)>(*this, lib, "get", das::SideEffects::modifyArgument,
      "bind_dascript::ShaderMaterialPtr_get");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/dasShaders.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorShaders, bind_dascript);