// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphModule.h"

#include <EASTL/shared_ptr.h>

#include <api/das/typeInterop.h>
#include <daScript/include/daScript/simulate/aot_builtin_ast.h>
#include <daScript/src/builtin/module_builtin_ast.h>
#include <daScript/ast/ast_typedecl.h>

#include <render/daBfg/das/registerBlobType.h>


namespace bind_dascript
{

static TypeInterop typeInterop;
// NOTE: das script loading is multi-threaded, but daBfg runtime is not,
// so only some functions need to use the mutex.
static das::mutex typeInteropMutex;

vec4f getTypeInfo(das::Context &, das::SimNode_CallBase *call, vec4f *)
{
  das::TypeInfo *ti = call->types[0];
  return das::cast<das::TypeInfo *>::from(ti);
}

void setBlobDescription(dabfg::ResourceData &res, const char *mangled_name, int size, int align, das::TypeInfo *type_info,
  das::TFunc<void, void *> ctor, das::TFunc<void, void *> dtor, das::TFunc<void, void *, const void *> copy, das::Context *ctx)
{
  const auto tag = typeInterop.tryRegisterDasOnlyType(mangled_name);

  G_ASSERT(align > 0 && size > 0);

  // TODO: for C++ types, save their ctor/dtor/copy inside of typeInterop and
  // use those instead of going through das to improve perf.
  if (ctor && dtor && copy)
  {
    const auto callCtor = [ctx, ctor, type_info](void *p) {
      const auto call = [ctx, ctor, p]() { das::das_invoke_function<void>::invoke<void *>(ctx, nullptr, ctor, p); };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    };
    const auto callDtor = [ctx, dtor](void *p) {
      ctx->removeGcRoot(p);
      const auto call = [ctx, dtor, p]() { das::das_invoke_function<void>::invoke<void *>(ctx, nullptr, dtor, p); };
      callDasFunction(ctx, call);
    };
    const auto callCopy = [ctx, copy, type_info](void *p, const void *from) {
      const auto call = [ctx, copy, p, from]() {
        das::das_invoke_function<void>::invoke<void *, const void *>(ctx, nullptr, copy, p, from);
      };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else if (type_info->isPod())
  {
    const auto callCtor = [ctx, size, type_info](void *p) {
      ::memset(p, 0, size);
      ctx->addGcRoot(p, type_info);
    };
    const auto callDtor = [ctx](void *p) { ctx->removeGcRoot(p); };
    const auto callCopy = [ctx, size, type_info](void *p, const void *from) {
      ::memcpy(p, from, size);
      ctx->addGcRoot(p, type_info);
    };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else if (type_info->isRawPod())
  {
    const auto callCtor = [size](void *p) { ::memset(p, 0, size); };
    const auto callDtor = [](void *) {};
    const auto callCopy = [size](void *p, const void *from) { ::memcpy(p, from, size); };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else
  {
    // We check for POD-ness on das side, so this should never happen.
    G_ASSERTF(false, "Impossible situation!");
  }
}

void setBlobDescriptionDefValue(dabfg::ResourceData &res, const char *mangled_name, int size, int align, das::TypeInfo *type_info,
  das::TLambda<void, void *> ctor, das::TFunc<void, void *> dtor, das::TFunc<void, void *, const void *> copy, das::Context *ctx)
{
  const auto tag = typeInterop.tryRegisterDasOnlyType(mangled_name);

  G_ASSERT(align > 0 && size > 0);
  G_ASSERT_RETURN(ctor, );

  auto callCtor = [ctx, ctor = eastl::make_shared<das::GcRootLambda>(eastl::move(ctor), ctx), type_info](void *p) {
    const auto call = [ctx, &ctor, p]() { das::das_invoke_lambda<void>::invoke<void *>(ctx, nullptr, *ctor.get(), p); };
    callDasFunction(ctx, call);
    ctx->addGcRoot(p, type_info);
  };

  // TODO: for C++ types, save their dtor/copy inside of typeInterop and
  // use those instead of going through das to improve perf.
  if (dtor && copy)
  {
    const auto callDtor = [ctx, dtor](void *p) {
      ctx->removeGcRoot(p);
      const auto call = [ctx, dtor, p]() { das::das_invoke_function<void>::invoke<void *>(ctx, nullptr, dtor, p); };
      callDasFunction(ctx, call);
    };
    const auto callCopy = [ctx, copy, type_info](void *p, const void *from) {
      const auto call = [ctx, copy, p, from]() {
        das::das_invoke_function<void>::invoke<void *, const void *>(ctx, nullptr, copy, p, from);
      };
      callDasFunction(ctx, call);
      ctx->addGcRoot(p, type_info);
    };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else if (type_info->isPod())
  {
    const auto callDtor = [ctx](void *p) { ctx->removeGcRoot(p); };
    const auto callCopy = [ctx, size, type_info](void *p, const void *from) {
      ::memcpy(p, from, size);
      ctx->addGcRoot(p, type_info);
    };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else if (type_info->isRawPod())
  {
    const auto callDtor = [](void *) {};
    const auto callCopy = [size](void *p, const void *from) { ::memcpy(p, from, size); };

    res.creationInfo =
      dabfg::BlobDescription{tag, static_cast<size_t>(size), static_cast<size_t>(align), callCtor, callDtor, callCopy};
  }
  else
  {
    // We check for POD-ness on das side, so this should never happen.
    G_ASSERTF(false, "Impossible situation!");
  }
}

dabfg::BlobView getBlobView(const dabfg::ResourceProvider *provider, dabfg::ResNameId resId, bool history)
{
  G_ASSERT(provider);
  auto &storage = history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<dabfg::BlobView>(it->second);
  else
    return {};
}

void *getBlobViewData(dabfg::BlobView view, const char *type_mangled_name)
{
  // TODO: maybe we can return a pointer to a zero-initialized page here to try and avoid a crash?
  G_ASSERTF_RETURN(view.typeTag == typeInterop.tagFor(type_mangled_name), nullptr,
    "daBfg: Detected a blob type mismatch when accessing a blob from das! Things WILL crash!");
  return view.data;
}

void markWithTag(dabfg::ResourceRequest &request, const char *mangled_name) { request.subtypeTag = typeInterop.tagFor(mangled_name); }

void useRequestTagForBinding(const dabfg::ResourceRequest &request, dabfg::Binding &binding)
{
  binding.projectedTag = request.subtypeTag;
}

void DaBfgCoreModule::addBlobBindings(das::ModuleLibrary &lib)
{
  {
    das::lock_guard lock{typeInteropMutex};
    typeInterop.tryRegisterBuiltins(lib);
  }

  das::addInterop<&getTypeInfo, das::TypeInfo *, vec4f>(*this, lib, //
    "getTypeInfo", das::SideEffects::none, "bind_dascript::getTypeInfo")
    ->args({"type"});

  das::addExtern<DAS_BIND_FUN(bind_dascript::setBlobDescription)>(*this, lib, //
    "setDescription", das::SideEffects::accessExternal, "bind_dascript::setBlobDescription");

  das::addExtern<DAS_BIND_FUN(bind_dascript::setBlobDescriptionDefValue)>(*this, lib, //
    "setDescription", das::SideEffects::accessExternal, "bind_dascript::setBlobDescriptionDefValue");

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

void dabfg::detail::register_external_interop_type(const char *mangled_name, dabfg::ResourceSubtypeTag tag)
{
  ::das::lock_guard lock{bind_dascript::typeInteropMutex};
  bind_dascript::typeInterop.registerInteropType(mangled_name, tag);
}
