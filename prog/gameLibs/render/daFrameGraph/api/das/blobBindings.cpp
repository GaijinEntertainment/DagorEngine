// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphModule.h"

#include <EASTL/shared_ptr.h>

#include <daScript/include/daScript/simulate/aot_builtin_ast.h>
#include <daScript/src/builtin/module_builtin_ast.h>
#include <daScript/ast/ast_typedecl.h>

#include <runtime/runtime.h>


extern void try_register_builtins(const das::ModuleLibrary &lib);

namespace bind_dascript
{

vec4f getTypeInfo(das::Context &, das::SimNode_CallBase *call, vec4f *)
{
  das::TypeInfo *ti = call->types[0];
  return das::cast<das::TypeInfo *>::from(ti);
}

void setBlobDescription(dafg::ResourceData &res, const char *mangled_name, int size, int align, das::TypeInfo *type_info,
  das::TFunc<void, void *> ctor, das::TFunc<void, void *> dtor, das::TFunc<void, void *, const void *> copy, das::Context *ctx)
{
  G_ASSERT(align > 0 && size > 0);

  dafg::detail::RTTI rtti{static_cast<size_t>(size), static_cast<size_t>(align)};

  if (ctor && dtor && copy)
  {
    rtti.ctor = [ctx, ctor, type_info](void *p) {
      const auto call = [ctx, ctor, p]() { das::das_invoke_function<void>::invoke<void *>(ctx, nullptr, ctor, p); };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    };
    rtti.dtor = [ctx, dtor](void *p) {
      ctx->removeGcRoot(p);
      const auto call = [ctx, dtor, p]() { das::das_invoke_function<void>::invoke<void *>(ctx, nullptr, dtor, p); };
      callDasFunction(ctx, call);
    };
    rtti.copy = [ctx, copy, type_info](void *p, const void *from) {
      const auto call = [ctx, copy, p, from]() {
        das::das_invoke_function<void>::invoke<void *, const void *>(ctx, nullptr, copy, p, from);
      };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    };
  }
  else if (type_info->isPod())
  {
    rtti.ctor = [ctx, size, type_info](void *p) {
      ::memset(p, 0, size);
      ctx->addGcRoot(p, type_info);
    };
    rtti.dtor = [ctx](void *p) { ctx->removeGcRoot(p); };
    rtti.copy = [ctx, size, type_info](void *p, const void *from) {
      ::memcpy(p, from, size);
      ctx->addGcRoot(p, type_info);
    };
  }
  else if (type_info->isRawPod())
  {
    rtti.ctor = [size](void *p) { ::memset(p, 0, size); };
    rtti.dtor = [](void *) {};
    rtti.copy = [size](void *p, const void *from) { ::memcpy(p, from, size); };
  }
  else
  {
    // We check for POD-ness on das side, so this should never happen.
    G_ASSERTF(false, "Impossible situation!");
  }

  const auto tag = dafg::Runtime::get().getTypeDb().registerForeignType(mangled_name, eastl::move(rtti));
  res.creationInfo = dafg::BlobDescription{tag};
}

void overrideBlobCtor(dafg::ResourceData &res, das::TypeInfo *type_info, das::TLambda<void, void *> ctor, das::Context *ctx)
{
  G_ASSERT_RETURN(ctor, );

  eastl::get<dafg::BlobDescription>(res.creationInfo).ctorOverride = eastl::make_unique<dafg::BlobDescription::CtorT>(
    eastl::move([ctx, ctor = das::GcRootLambda(eastl::move(ctor), ctx), type_info](void *p) {
      const auto call = [ctx, &ctor, p]() { das::das_invoke_lambda<void>::invoke<void *>(ctx, nullptr, ctor, p); };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    }));
}

dafg::BlobView getBlobView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history)
{
  G_ASSERT(provider);
  auto &storage = history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<dafg::BlobView>(it->second);
  else
    return {};
}

void *getBlobViewData(dafg::BlobView view, const char *type_mangled_name)
{
  // TODO: maybe we can return a pointer to a zero-initialized page here to try and avoid a crash?
  G_ASSERTF_RETURN(view.typeTag == dafg::Runtime::get().getTypeDb().tagForForeign(type_mangled_name), nullptr,
    "daFG: Detected a blob type mismatch when accessing a blob from das! Things WILL crash!");
  return view.data;
}

void markWithTag(dafg::ResourceRequest &request, const char *mangled_name)
{
  request.subtypeTag = dafg::Runtime::get().getTypeDb().tagForForeign(mangled_name);
}

void useRequestTagForBinding(const dafg::ResourceRequest &request, dafg::Binding &binding)
{
  binding.projectedTag = request.subtypeTag;
}

void DaFgCoreModule::addBlobBindings(das::ModuleLibrary &lib)
{
  try_register_builtins(lib);

  das::addInterop<&getTypeInfo, das::TypeInfo *, vec4f>(*this, lib, //
    "getTypeInfo", das::SideEffects::none, "bind_dascript::getTypeInfo")
    ->args({"type"});

  das::addExtern<DAS_BIND_FUN(bind_dascript::setBlobDescription)>(*this, lib, //
    "setDescription", das::SideEffects::accessExternal, "bind_dascript::setBlobDescription");

  das::addExtern<DAS_BIND_FUN(bind_dascript::overrideBlobCtor)>(*this, lib, //
    "overrideCtor", das::SideEffects::accessExternal, "bind_dascript::overrideBlobCtor");

  das::addExtern<DAS_BIND_FUN(bind_dascript::getBlobView)>(*this, lib, //
    "getBlobView", das::SideEffects::accessExternal, "bind_dascript::getBlobView");

  das::addExtern<DAS_BIND_FUN(bind_dascript::getBlobViewData)>(*this, lib, //
    "getData", das::SideEffects::accessExternal, "bind_dascript::getBlobViewData");

  das::addExtern<DAS_BIND_FUN(bind_dascript::markWithTag)>(*this, lib, //
    "markWithTag", das::SideEffects::modifyArgumentAndExternal, "bind_dascript::markWithTag");

  das::addExtern<DAS_BIND_FUN(bind_dascript::useRequestTagForBinding)>(*this, lib, //
    "useRequestTagForBinding", das::SideEffects::modifyArgumentAndExternal, "bind_dascript::useRequestTagForBinding");
}

} // namespace bind_dascript
