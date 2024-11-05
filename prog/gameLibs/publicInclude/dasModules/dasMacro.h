//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{
template <uint32_t ArgN = 1, bool OnlyFastCall = false>
struct BakeHashFunctionAnnotation : das::TransformFunctionAnnotation
{
  BakeHashFunctionAnnotation() : das::TransformFunctionAnnotation("bake_hash_" + das::to_string(ArgN)) {}

  das::ExpressionPtr transformCall(das::ExprCallFunc *call, das::string &err) override
  {
    auto arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (!starg->getValue().empty())
      {
        const char *text = starg->text.c_str();
        uint32_t hv = ecs_str_hash(text);
        auto hconst = das::make_smart<das::ExprConstUInt>(arg->at, hv);
        hconst->generated = true;
        hconst->type = das::make_smart<das::TypeDecl>(das::Type::tUInt);
        hconst->type->constant = true;
        auto newCall = das::static_pointer_cast<das::ExprCallFunc>(call->clone());
        newCall->arguments.insert(newCall->arguments.begin() + ArgN + 1, hconst);
        if (call->func->fromGeneric)
          newCall->name = call->func->fromGeneric->name;
        return newCall;
      }
      err.append("key argument can't be an empty string");
    }
    else if (OnlyFastCall)
    {
      err.append(
        "key argument must be a constant string. Pass precalculated hash for non-constant strings xxx(..., str, ecs_hash(str))");
    }
    return nullptr;
  }
  bool apply(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  }
  bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  }
};

__forceinline void fast_component_aot_prefix(das::TextWriter &tw, const das::string &arg_name, das::string type_name)
{
  auto program = das::daScriptEnvironment::bound->g_Program;
  tw << "#ifndef _component_" << arg_name << program->thisNamespace << "\n";
  tw << "#define _component_" << arg_name << program->thisNamespace << "\n";
  tw << "namespace __components {\n";
  tw << "static constexpr ecs::component_t " << arg_name << "_get_type(){return ecs::ComponentTypeInfo<" << type_name
     << ">::type; }\n";
  tw << "static ecs::LTComponentList " << arg_name << "_component(ECS_HASH(\"" << arg_name << "\"), ";
  tw << arg_name << "_get_type(), \"\", \"\", 0);\n";
  tw << "}\n";
  tw << "#endif\n";
}

template <uint32_t ArgN, uint32_t ArgTotal>
struct EcsGetOrFunctionAnnotation : das::TransformFunctionAnnotation
{
  EcsGetOrFunctionAnnotation() : das::TransformFunctionAnnotation("ecs_get_or_" + eastl::to_string(ArgN))
  {
    G_STATIC_ASSERT(ArgN > 0 && ArgN + 1 < ArgTotal);
  }

  das::ExpressionPtr transformCall(das::ExprCallFunc *, das::string &) override { return nullptr; }

  eastl::string aotName(das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return call->func->getAotBasicName();
      das::Type dasType;
      das::string typeName;
      if (get_underlying_ecs_type_with_string(*call->func->result, typeName, dasType))
      {
        das::string cppTypeName;
        const bool isStringResult = call->func->result->baseType == das::tString;
        if (isStringResult)
          cppTypeName = " char *";
        else
          cppTypeName = das::describeCppType(call->func->result->firstType, das::CpptSubstitureRef::no, das::CpptSkipRef::yes,
            das::CpptSkipConst::no);
        make_string_outer_ns(cppTypeName);
        make_string_outer_ns(typeName);
        return das::string(das::string::CtorSprintf(), "/* %s */ EcsToDas< %s%s, %s%s>::get(g_entity_mgr->getNullableFast<%s>",
          starg->text.c_str(), cppTypeName.c_str(), isStringResult ? "&" : " *", typeName.c_str(), isStringResult ? "" : " *",
          typeName.c_str());
      }
    }
    return call->func->getAotBasicName();
  }

  void aotPrefix(das::TextWriter &tw, das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return;
      das::Type dasType;
      das::string typeName;
      if (!get_underlying_ecs_type_with_string(*call->func->result, typeName, dasType))
        return;
      make_string_outer_ns(typeName);
      fast_component_aot_prefix(tw, starg->text, typeName);
    }
  }

  das::string aotArgumentSuffix(das::ExprCallFunc *call, int argIndex) override
  {
    if (argIndex == ArgN - 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type_with_string(*call->func->result, typeName, dasType))
          return "";
        return das::string(das::string::CtorSprintf(), ", __components::%s_component.getCidx() )/* ", starg->text.c_str());
      }
    }
    if (argIndex == ArgN + 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->func->result, typeName, dasType))
          return "";
        return " */";
      }
    }
    return "";
  }
};


template <uint32_t ArgN, uint32_t ArgResult, bool Optional>
struct EcsSetAnnotation : das::TransformFunctionAnnotation
{
  das::string resultTypeName;
  EcsSetAnnotation(const char *type_name) :
    TransformFunctionAnnotation(das::string(das::string::CtorSprintf(), "ecs_set_%d_%d", ArgN, Optional)), resultTypeName(type_name)
  {
    G_STATIC_ASSERT(ArgN > 0 && ArgN + 1 < ArgResult);
  }

  das::ExpressionPtr transformCall(das::ExprCallFunc *, das::string &) override { return nullptr; }

  eastl::string aotName(das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return call->func->getAotBasicName();
      das::Type dasType;
      das::string typeName;
      if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
        return call->func->getAotBasicName();
      return das::string(das::string::CtorSprintf(), "/* %s */ bind_dascript::entitySetFast%s%s", starg->text.c_str(),
        Optional ? "Optional" : "", resultTypeName.c_str());
    }
    return call->func->getAotBasicName();
  }

  void aotPrefix(das::TextWriter &tw, das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return;
      das::Type dasType;
      das::string typeName;
      if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
        return;
      make_string_outer_ns(typeName);
      fast_component_aot_prefix(tw, starg->text, typeName);
    }
  }
  das::string aotArgumentSuffix(das::ExprCallFunc *call, int argIndex) override
  {
    if (argIndex == ArgN - 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return das::string(das::string::CtorSprintf(), ", __components::%s_component.getInfo() /* ", starg->text.c_str(),
          starg->text.c_str());
      }
    }
    if (argIndex == ArgN + 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return " */";
      }
    }
    if (!Optional && argIndex == ArgResult)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return das::string(das::string::CtorSprintf(), ", &__components::%s_component", starg->text.c_str());
      }
    }
    return "";
  }
};


template <uint32_t ArgN, uint32_t ArgResult>
struct EcsInitSetAnnotation : das::TransformFunctionAnnotation
{
  das::string resultTypeName;
  EcsInitSetAnnotation(const char *type_name) :
    TransformFunctionAnnotation("ecs_init_" + eastl::to_string(ArgN)), resultTypeName(type_name)
  {
    G_STATIC_ASSERT(ArgN > 0 && ArgN + 1 < ArgResult);
  }

  das::ExpressionPtr transformCall(das::ExprCallFunc *, das::string &) override { return nullptr; }

  eastl::string aotName(das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return call->func->getAotBasicName();
      das::Type dasType;
      das::string typeName;
      if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
        return call->func->getAotBasicName();
      return das::string(das::string::CtorSprintf(), "/* %s */ bind_dascript::fastSetInit%s", starg->text.c_str(),
        resultTypeName.c_str());
    }
    return call->func->getAotBasicName();
  }

  void aotPrefix(das::TextWriter &tw, das::ExprCallFunc *call) override
  {
    auto &&arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (starg->text.length() == 0)
        return;
      das::Type dasType;
      das::string typeName;
      if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
        return;
      make_string_outer_ns(typeName);
      fast_component_aot_prefix(tw, starg->text, typeName);
    }
  }
  das::string aotArgumentSuffix(das::ExprCallFunc *call, int argIndex) override
  {
    if (argIndex == ArgN - 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return das::string(das::string::CtorSprintf(),
          ", __components::%s_component.getName(), __components::%s_component.getInfo() /* ", starg->text.c_str(),
          starg->text.c_str());
      }
    }
    if (argIndex == ArgN + 1)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return " */";
      }
    }
    if (argIndex == ArgResult)
    {
      auto &&arg = call->arguments[ArgN];
      if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
      {
        auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
        if (starg->text.length() == 0)
          return "";
        das::Type dasType;
        das::string typeName;
        if (!get_underlying_ecs_type(*call->arguments[ArgResult]->type, typeName, dasType))
          return "";
        return das::string(das::string::CtorSprintf(), ", __components::%s_component.getNameStr()", starg->text.c_str());
      }
    }
    return "";
  }
};


template <uint32_t ArgN = 1>
struct ConstStringArgCallFunctionAnnotation : das::FunctionAnnotation
{
  ConstStringArgCallFunctionAnnotation() : das::FunctionAnnotation("const_string_arg_" + eastl::to_string(ArgN)) {}

  bool apply(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  }
  bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  }
  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override { return true; }
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  }

  bool verifyCall(das::ExprCallFunc *call, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &err) override
  {
    auto arg = call->arguments[ArgN];
    if (arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant())
    {
      auto starg = das::static_pointer_cast<das::ExprConstString>(arg);
      if (!starg->getValue().empty())
        return true;
    }

    err.append_sprintf("argument #%d should be const non-empty string", ArgN);
    return false;
  }
};

struct LogFmtHashFunctionAnnotation : das::TransformFunctionAnnotation
{
  LogFmtHashFunctionAnnotation() : das::TransformFunctionAnnotation("log_fmt_hash") {}

  das::ExpressionPtr transformCall(das::ExprCallFunc *call, das::string &err) override
  {
    auto arg = call->arguments[0];
    // for string builder collect all const parts of the string and calculate hash based on that
    if (strcmp(arg->__rtti, "ExprStringBuilder") == 0)
    {
      auto builder = das::static_pointer_cast<das::ExprStringBuilder>(arg);
      uint32_t hash = 0;
      for (auto e : builder->elements)
      {
        if (e->type && e->type->isString() && e->type->isConst() && e->rtti_isConstant())
        {
          auto starg = das::static_pointer_cast<das::ExprConstString>(e);
          hash = str_hash_fnv1<32>(starg->text.c_str(), hash);
        }
      }
      auto exprHash = das::make_smart<das::ExprConstUInt>(arg->at, hash);
      exprHash->generated = true;
      auto newCall = das::static_pointer_cast<das::ExprCallFunc>(call->clone());
      if (call->func->fromGeneric)
        newCall->name = call->func->fromGeneric->name;
      // insert our calcualted hash param after the format string, so hinted version of the log gets called
      newCall->arguments.insert(newCall->arguments.begin() + 1, exprHash);
      return newCall;
    }

    // const strings can get hashes later from itself, no need for precalculation
    // and if it's a non const string but not a string builder - we can't precalc anything either
    if (arg->type && arg->type->isString())
      return nullptr;

    err.append("Log function called without ExprStringBuilder or String");
    return nullptr;
  }
  bool apply(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  }
  bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  }
};

__forceinline das::AnnotationDeclarationPtr annotation_declaration(const das::AnnotationPtr &ann)
{
  auto decl = das::make_smart<das::AnnotationDeclaration>();
  decl->annotation = ann;
  return decl;
}
} // namespace bind_dascript
