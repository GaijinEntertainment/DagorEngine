//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasContextLockGuard.h>
#include <dasModules/dasEvent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/component.h>
#include <daECS/core/sharedComponent.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/utility/createInstantiated.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <perfMon/dag_statDrv.h>
#include <daScript/src/builtin/module_builtin_rtti.h>

namespace das
{
template <>
struct SimPolicy<ecs::EntityId>
{
  static __forceinline auto to(vec4f a) { return das::cast<ecs::EntityId>::to(a); }
  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(ecs::EntityId a, ecs::EntityId b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(ecs::EntityId a, ecs::EntityId b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(ecs::EntityId a, Context &, LineInfo *) { return !a; }
};

template <>
struct WrapType<ecs::EntityId>
{
  enum
  {
    value = true
  };
  typedef ecs::entity_id_t type;
  typedef ecs::entity_id_t rettype;
};
template <>
struct WrapRetType<ecs::EntityId>
{
  typedef ecs::entity_id_t type;
};
class EntityId_WrapArg : public ecs::EntityId
{
public:
  EntityId_WrapArg(ecs::entity_id_t t) : ecs::EntityId(t) {}
};

template <>
struct WrapArgType<ecs::EntityId>
{
  typedef EntityId_WrapArg type;
};

template <>
struct SimPolicy<ecs::ChildComponent>
{
  static __forceinline const ecs::ChildComponent &to(vec4f a) { return *(const ecs::ChildComponent *)v_extract_ptr(v_cast_vec4i(a)); }
  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(ecs::ChildComponent &a, ecs::ChildComponent &b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(ecs::ChildComponent &a, ecs::ChildComponent &b, Context &, LineInfo *) { return a != b; }
};
}; // namespace das
#define MAKE_ECS_TYPES                                             \
  MAKE_ECS_TYPE(Tag, ecs::Tag)                                     \
  MAKE_ECS_TYPE(EntityManager, ecs::EntityManager)                 \
  MAKE_ECS_TYPE(EntityComponentRef, ecs::EntityComponentRef)       \
  MAKE_ECS_TYPE(ComponentsInitializer, ecs::ComponentsInitializer) \
  MAKE_ECS_TYPE(ComponentsMap, ecs::ComponentsMap)                 \
  MAKE_ECS_TYPE(Array, ecs::Array)                                 \
  MAKE_ECS_TYPE(Object, ecs::Object)                               \
  MAKE_ECS_TYPE(ChildComponent, ecs::ChildComponent)               \
  MAKE_ECS_TYPE(Template, ecs::Template)                           \
  MAKE_ECS_TYPE(SharedArray, ecs::SharedComponent<ecs::Array>)     \
  MAKE_ECS_TYPE(SharedObject, ecs::SharedComponent<ecs::Object>)

#define MAKE_ECS_TYPE MAKE_EXTERNAL_TYPE_FACTORY
MAKE_ECS_TYPES
#undef MAKE_ECS_TYPE

#define DECL_LIST_TYPE(lt, t)              \
  MAKE_EXTERNAL_TYPE_FACTORY(lt, ecs::lt); \
  MAKE_EXTERNAL_TYPE_FACTORY(Shared##lt, ecs::SharedComponent<ecs::lt>);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

namespace bind_dascript
{

inline const ecs::EntityManager &getEntityManager() { return *g_entity_mgr; }
inline ecs::EntityManager &getEntityManagerRW() { return *g_entity_mgr; }

template <typename T>
struct DasTypeAlias
{
  typedef T alias;
};
template <>
struct DasTypeAlias<Point2>
{
  typedef das::float2 alias;
};
template <>
struct DasTypeAlias<Point3>
{
  typedef das::float3 alias;
};
template <>
struct DasTypeAlias<Point4>
{
  typedef das::float4 alias;
};
template <>
struct DasTypeAlias<IPoint2>
{
  typedef das::int2 alias;
};
template <>
struct DasTypeAlias<IPoint3>
{
  typedef das::int3 alias;
};
template <>
struct DasTypeAlias<IPoint4>
{
  typedef das::int4 alias;
};
template <>
struct DasTypeAlias<TMatrix>
{
  typedef das::float3x4 alias;
};
template <>
struct DasTypeAlias<ecs::string>
{
  typedef das::string alias;
};
template <typename T>
struct DasTypeParamAlias
{
  typedef typename DasTypeAlias<T>::alias param_alias;
  typedef typename DasTypeAlias<T>::alias cparam_alias;
};

template <typename T>
struct DasTypeParamRefAlias
{
  typedef typename DasTypeAlias<T>::alias &param_alias;
  typedef const typename DasTypeAlias<T>::alias &cparam_alias;
};
template <>
struct DasTypeParamAlias<TMatrix> : DasTypeParamRefAlias<TMatrix>
{};
template <>
struct DasTypeParamAlias<ecs::string> : DasTypeParamRefAlias<ecs::string>
{};
template <>
struct DasTypeParamAlias<ecs::Object> : DasTypeParamRefAlias<ecs::Object>
{};
template <>
struct DasTypeParamAlias<ecs::Array> : DasTypeParamRefAlias<ecs::Array>
{};
#define DECL_LIST_TYPE(lt, t)                                       \
  template <>                                                       \
  struct DasTypeParamAlias<ecs::lt> : DasTypeParamRefAlias<ecs::lt> \
  {};
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

// all typedef are inside bind_dascript namespace
typedef ecs::string ecs_string;
typedef ecs::Object ecs_object;
typedef ecs::Array ecs_array;
typedef ecs::EntityId Eid;

typedef int8_t int8;
typedef int16_t int16;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint;
typedef uint64_t uint64;

#define DECL_LIST_TYPE(lt, t) typedef ecs::lt ecs_##lt;
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#define ECS_BASE_TYPE_LIST \
  TYPE(int8)               \
  TYPE(int16)              \
  TYPE(int)                \
  TYPE(int64)              \
  TYPE(uint8)              \
  TYPE(uint16)             \
  TYPE(uint)               \
  TYPE(uint64)             \
  TYPE(E3DCOLOR)           \
  TYPE(float)              \
  TYPE(bool)               \
  TYPE(Point2)             \
  TYPE(Point3)             \
  TYPE(Point4)             \
  TYPE(IPoint2)            \
  TYPE(IPoint3)            \
  TYPE(IPoint4)            \
  TYPE(TMatrix)            \
  TYPE(ecs_string)         \
  TYPE(Eid)

#define ECS_LIST_TYPE_LIST \
  TYPE(ecs_object)         \
  TYPE(ecs_array)          \
  TYPE(ecs_IntList)        \
  TYPE(ecs_UInt8List)      \
  TYPE(ecs_UInt16List)     \
  TYPE(ecs_UInt32List)     \
  TYPE(ecs_StringList)     \
  TYPE(ecs_EidList)        \
  TYPE(ecs_FloatList)      \
  TYPE(ecs_Point2List)     \
  TYPE(ecs_Point3List)     \
  TYPE(ecs_Point4List)     \
  TYPE(ecs_IPoint2List)    \
  TYPE(ecs_IPoint3List)    \
  TYPE(ecs_IPoint4List)    \
  TYPE(ecs_BoolList)       \
  TYPE(ecs_TMatrixList)    \
  TYPE(ecs_ColorList)      \
  TYPE(ecs_Int8List)       \
  TYPE(ecs_Int16List)      \
  TYPE(ecs_Int64List)      \
  TYPE(ecs_UInt64List)

typedef void (*das_context_log_cb)(das::Context *, const das::LineInfo *at, das::LogLevel level, const char *message);

extern das_context_log_cb global_context_log_cb;

das::StackAllocator &get_shared_stack();

struct EsContextBase
{
  enum
  {
    MAGIC_NOMEM = 0xBADC0402,
    MAGIC_HASMEM = 0xBADC0401
  };
  const uint32_t ctxMagic = MAGIC_NOMEM;
  void *userData = nullptr;

  EsContextBase(void *user_data) : userData(user_data) {}
};

struct EsContext final : EsContextBase, das::Context
{
  ecs::EntityManager *mgr = (bool)g_entity_mgr ? g_entity_mgr.get() : (ecs::EntityManager *)nullptr;
#if DAGOR_DBGLEVEL > 0
  int touchedByWkId = -1;
  int touchedAtFrame = -1;
  // Pointing to esData->functionPtr->name.
  // Used only for errors diagnostics and quite volatile so raw pointer should be fine (don't bother with copy)
  const char *touchedByES = nullptr;
#endif

#if KEEP_HEAP_FROM_LAST_RUN
  void tryRestartAndLock() override
  {
    if (insideContext == 0 && !persistent)
    {
      restart();
      restartHeaps();
    }
    lock();
  }
#else
  virtual uint32_t unlock() override
  {
    const uint32_t res = das::Context::unlock();
    if (insideContext == 0 && !persistent)
      restartHeaps(); // clear heap on end of call
    return res;
  }
#endif
  void useGlobalVariablesMem()
  {
    const_cast<uint32_t &>(ctxMagic) = MAGIC_HASMEM;
  } // this should not happen in production, but just in case..
  EsContext(uint32_t stackSize) : EsContextBase(nullptr), das::Context(stackSize) {}
  EsContext(const Context &ctx, uint32_t category_, bool reset_user_data, void *new_user_data = nullptr) :
    EsContextBase(reset_user_data ? new_user_data : ((EsContext &)ctx).userData), das::Context(ctx, category_)
  {
    EsContext &esCtx = (EsContext &)ctx;
    mgr = esCtx.mgr;
  }
  virtual void to_out(const das::LineInfo *at, const char *message) override
  {
    if (global_context_log_cb)
    {
      global_context_log_cb(this, at, das::LogLevel::info, message);
      return;
    }
#if DAGOR_DBGLEVEL > 0
    if (at && at->fileInfo)
    {
      debug("%s:%d: %s", at->fileInfo->name.c_str(), at->line, message);
      return;
    }
#endif
    debug("%s", message);
  }
  virtual void to_err(const das::LineInfo *at, const char *message) override
  {
    if (global_context_log_cb)
    {
      global_context_log_cb(this, at, das::LogLevel::error, message);
      return;
    }
#if DAGOR_DBGLEVEL > 0
    debug("%s", getStackWalk(at, false, false).c_str());
    if (at && at->fileInfo)
    {
      logerr("%s:%d: %s", at->fileInfo->name.c_str(), at->line, message);
      return;
    }
#endif
    logerr("%s", message);
  }
};


inline EsContext *safe_cast_es_context(das::Context *ctx)
{
  EsContext *esCtx = static_cast<EsContext *>(ctx);
  if (esCtx->ctxMagic == EsContext::MAGIC_NOMEM || esCtx->ctxMagic == EsContext::MAGIC_HASMEM)
    return esCtx;
  return nullptr;
}

inline EsContext *cast_es_context(das::Context *ctx)
{
  EsContext *esCtx = static_cast<EsContext *>(ctx);
  G_ASSERT(esCtx->ctxMagic == EsContext::MAGIC_NOMEM || esCtx->ctxMagic == EsContext::MAGIC_HASMEM);
  return esCtx;
}

inline EsContext &cast_es_context(das::Context &ctx)
{
  EsContext &esCtx = static_cast<EsContext &>(ctx);
  G_ASSERT(esCtx.ctxMagic == EsContext::MAGIC_NOMEM || esCtx.ctxMagic == EsContext::MAGIC_HASMEM);
  return esCtx;
}

extern bool load_das_script(const char *fname);
extern bool load_das_script_debugger(const char *fname);
extern bool load_das_script_with_debugcode(const char *fname);
inline bool load_das(const char *fname) { return load_das_script(fname); }
inline bool load_das_debugger(const char *fname) { return load_das_script_debugger(fname); }
inline bool load_das_with_debugcode(const char *fname) { return load_das_script_with_debugcode(fname); }
#if DAGOR_DBGLEVEL > 0
extern bool reload_das_debug_script(const char *fname, bool debug);
inline bool reload_das_debug(const char *fname, bool debug) { return reload_das_debug_script(fname, debug); }

extern bool find_loaded_das_script(const eastl::function<bool(const char *, das::Context &, das::smart_ptr<das::Program>)> &cb);
inline bool find_loaded_das(const das::TBlock<bool, const char *, das::Context &, das::smart_ptr<das::Program>> &block,
  das::Context *ctx, das::LineInfoArg *at)
{
  vec4f arg[3];
  bool found = false;
  ctx->invokeEx(
    block, arg, nullptr,
    [&](das::SimNode *code) {
      find_loaded_das_script([&](const char *classData, das::Context &context, das::ProgramPtr program) {
        arg[0] = das::cast<const char *>::from(classData);
        arg[1] = das::cast<das::Context *>::from(&context);
        arg[2] = das::cast<das::smart_ptr<das::Program>>::from(program);
        return code->evalBool(*ctx);
      });
    },
    at);
  return found;
}
#endif

uint32_t das_ecs_systems_count();
uint32_t das_ecs_aot_systems_count();
uint32_t das_load_time_ms();
int64_t das_memory_usage();
uint32_t link_aot_errors_count();
int get_das_compile_errors_count();
bool get_globally_enabled_aot();

bool get_underlying_ecs_type(const das::TypeDecl &info, das::string &str, das::Type &type, bool module_name = true);
bool get_underlying_ecs_type_with_string(const das::TypeDecl &info, das::string &str, das::Type &type, bool module_name = true);
void make_string_outer_ns(das::string &typeName);

void enable_thread_safe_das_ctx_region(bool on);

__forceinline ecs::event_type_t ecs_hash(const char *str) { return ECS_HASH_SLOW(str ? str : "").hash; }

template <bool sync, typename TInit>
inline ecs::EntityId do_create_entity(const char *template_name, TInit &&block_or_init, das::Context *ctx, das::LineInfoArg *at)
{
  if (DAGOR_UNLIKELY(!template_name))
    ctx->throw_error_at(at, "attempt to create entity with empty name");
  ecs::ComponentsInitializer initTmp, *pInit = &initTmp;
  if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(block_or_init)>, das::TBlock<void, ecs::ComponentsInitializer>>)
  {
    vec4f arg = das::cast<char *>::from((char *)pInit);
    ctx->invoke(block_or_init, &arg, nullptr, at);
  }
  else
    pInit = &block_or_init;
  if (sync)
    return g_entity_mgr->createEntitySync(template_name, eastl::move(*pInit));
  else
    return g_entity_mgr->createEntityAsync(template_name, eastl::move(*pInit));
}
inline ecs::EntityId createEntityWithComp(const char *template_name, const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context, das::LineInfoArg *at)
{
  return do_create_entity<false>(template_name, block, context, at);
}
inline ecs::EntityId createEntitySyncWithComp(const char *template_name, const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context, das::LineInfoArg *at)
{
  return do_create_entity<true>(template_name, block, context, at);
}

inline ecs::EntityId createInstantiatedEntitySyncWithComp(const char *template_name,
  const das::TBlock<void, ecs::ComponentsInitializer> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);

  return ecs::createInstantiatedEntitySync(*g_entity_mgr, template_name, eastl::move(init));
}

inline ecs::EntityId _builtin_create_entity2(const char *template_name, ecs::ComponentsInitializer &init, das::Context *ctx,
  das::LineInfoArg *at)
{
  return do_create_entity<false>(template_name, init, ctx, at);
}
inline ecs::EntityId _builtin_create_entity_sync2(const char *template_name, ecs::ComponentsInitializer &init, das::Context *ctx,
  das::LineInfoArg *at)
{
  return do_create_entity<true>(template_name, init, ctx, at);
}
inline ecs::EntityId _builtin_create_instantiated_entity_sync(const char *template_name, ecs::ComponentsInitializer &init)
{
  return ecs::createInstantiatedEntitySync(*g_entity_mgr, template_name, eastl::move(init));
}

template <typename F>
inline ecs::EntityId do_entity_fn_with_init_and_lambda(ecs::EntityId eid, const char *template_name,
  const das::TLambda<void, const ecs::EntityId> &lambda, ecs::ComponentsInitializer &&init, das::Context *context,
  das::LineInfoArg *line_info, F entfn)
{
  das::SimFunction **fnMnh = (das::SimFunction **)lambda.capture;
  if (DAGOR_UNLIKELY(!fnMnh))
    context->throw_error_at(line_info, "invoke null lambda");
  das::SimFunction *simFunc = *fnMnh;
  if (DAGOR_UNLIKELY(!simFunc))
    context->throw_error_at(line_info, "invoke null function");
  if (DAGOR_UNLIKELY(!template_name))
    context->throw_error_at(line_info, "attempt to create entity with empty name");
  auto pCapture = lambda.capture;
  ContextLockGuard lockg(*context);
  return entfn(eid, template_name, eastl::move(init), [=, lockg = eastl::move(lockg)](ecs::EntityId eid) {
    vec4f argI[2];
    argI[0] = das::cast<void *>::from(pCapture);
    argI[1] = das::cast<ecs::EntityId>::from(eid);
    if (!context->ownStack)
    {
      das::SharedStackGuard guard(*context, get_shared_stack());
      (void)context->call(simFunc, argI, 0);
    }
    else
      (void)context->call(simFunc, argI, 0);
  });
}

template <typename F>
inline ecs::EntityId do_entity_fn_with_init_and_lambda(ecs::EntityId eid, const char *template_name,
  const das::TLambda<void, const ecs::EntityId> &lambda, const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context, das::LineInfoArg *line_info, F entfn)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, line_info);
  return do_entity_fn_with_init_and_lambda(eid, template_name, lambda, eastl::move(init), context, line_info, entfn);
}


inline ecs::EntityId _builtin_create_entity_lambda(const char *template_name, const das::TLambda<void, const ecs::EntityId> &lambda,
  const das::TBlock<void, ecs::ComponentsInitializer> &block, das::Context *context, das::LineInfoArg *line_info)
{
  return do_entity_fn_with_init_and_lambda(ecs::INVALID_ENTITY_ID, template_name, lambda, block, context, line_info,
    [](ecs::EntityId, const char *tname, ecs::ComponentsInitializer &&cinit, ecs::create_entity_async_cb_t &&cb) {
      return g_entity_mgr->createEntityAsync(tname, eastl::move(cinit), ecs::ComponentsMap(), eastl::move(cb));
    });
}

inline ecs::EntityId reCreateEntityFromWithComp(ecs::EntityId eid, const char *template_name,
  const das::TBlock<void, ecs::ComponentsInitializer> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);
  return g_entity_mgr->reCreateEntityFromAsync(eid, template_name, eastl::move(init));
}

inline ecs::EntityId _builtin_recreate_entity_lambda(ecs::EntityId eid, const char *template_name,
  const das::TLambda<void, const ecs::EntityId> &lambda, const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context, das::LineInfoArg *line_info)
{
  return do_entity_fn_with_init_and_lambda(eid, template_name, lambda, block, context, line_info,
    [](ecs::EntityId eid, const char *tname, ecs::ComponentsInitializer &&cinit, ecs::create_entity_async_cb_t &&cb) {
      return g_entity_mgr->reCreateEntityFromAsync(eid, tname, eastl::move(cinit), ecs::ComponentsMap(), eastl::move(cb));
    });
}

inline char *_builtin_add_sub_template_name(const ecs::EntityId eid, const char *add_name_str, das::Context *context,
  das::LineInfoArg *at)
{
  const char *addNameStr = add_name_str ? add_name_str : "";
  const ECS_DEFAULT_RECREATE_STR_T &str = add_sub_template_name(eid, addNameStr);
  return context->allocateString(str.c_str(), uint32_t(str.length()), at);
}

inline char *_builtin_remove_sub_template_name(const ecs::EntityId eid, const char *remove_name_str, das::Context *context,
  das::LineInfoArg *at)
{
  const char *removeNameStr = remove_name_str ? remove_name_str : "";
  const ECS_DEFAULT_RECREATE_STR_T &str = remove_sub_template_name(eid, removeNameStr);
  return context->allocateString(str.c_str(), uint32_t(str.length()), at);
}

inline char *_builtin_add_sub_template_name_str(const char *from_templ_name, const char *add_name_str, das::Context *context,
  das::LineInfoArg *at)
{
  const char *fromTemplName = from_templ_name ? from_templ_name : "";
  const char *addNameStr = add_name_str ? add_name_str : "";
  const ECS_DEFAULT_RECREATE_STR_T &str = add_sub_template_name(fromTemplName, addNameStr);
  return context->allocateString(str.c_str(), uint32_t(str.length()), at);
}

inline char *_builtin_remove_sub_template_name_str(const char *from_templ_name, const char *remove_name_str, das::Context *context,
  das::LineInfoArg *at)
{
  const char *fromTemplName = from_templ_name ? from_templ_name : "";
  const char *removeNameStr = remove_name_str ? remove_name_str : "";
  const ECS_DEFAULT_RECREATE_STR_T &str = remove_sub_template_name(fromTemplName, removeNameStr);
  return context->allocateString(str.c_str(), uint32_t(str.length()), at);
}

inline ecs::EntityId _builtin_add_sub_template(ecs::EntityId eid, const char *add_name_str,
  const das::TBlock<void, ecs::ComponentsInitializer> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);
  return add_sub_template_async(eid, add_name_str, eastl::move(init));
}

inline ecs::EntityId _builtin_add_sub_template_lambda(ecs::EntityId eid, const char *template_name,
  const das::TLambda<void, const ecs::EntityId> &lambda, const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context, das::LineInfoArg *line_info)
{
  return do_entity_fn_with_init_and_lambda(eid, template_name, lambda, block, context, line_info,
    [](ecs::EntityId eid, const char *tname, ecs::ComponentsInitializer &&cinit, ecs::create_entity_async_cb_t &&cb) {
      return add_sub_template_async(eid, tname, eastl::move(cinit), eastl::move(cb));
    });
}

inline ecs::EntityId _builtin_remove_sub_template(ecs::EntityId eid, const char *add_name_str,
  const das::TBlock<void, ecs::ComponentsInitializer> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);
  return remove_sub_template_async(eid, add_name_str, eastl::move(init));
}


inline void destroyEntity(ecs::EntityId id) { g_entity_mgr->destroyEntity(id); }
inline bool doesEntityExist(ecs::EntityId id) { return g_entity_mgr->doesEntityExist(id); }
inline bool isLoadingEntity(ecs::EntityId id) { return g_entity_mgr->isLoadingEntity(id); }
inline bool hasHint(ecs::EntityId eid, const char *key, uint32_t key_hash)
{
  return g_entity_mgr->has(eid, ecs::HashedConstString({key, key_hash}));
}
inline bool has(ecs::EntityId eid, const char *s) { return hasHint(eid, s, ECS_HASH_SLOW(s ? s : "").hash); }
inline ecs::template_t getEntityTemplateId(ecs::EntityId id) { return g_entity_mgr->getEntityTemplateId(id); }
inline const char *getEntityTemplateName(ecs::EntityId id) { return g_entity_mgr->getEntityTemplateName(id); }
inline const char *getEntityFutureTemplateName(ecs::EntityId id) { return g_entity_mgr->getEntityFutureTemplateName(id); }

inline uint32_t getRegExpInheritedFlags(const ecs::EntityManager &manager, const ecs::Template *tpl, const char *name)
{
  return tpl->getRegExpInheritedFlags(ECS_HASH_SLOW(name ? name : "").hash, manager.getTemplateDB().data());
}

inline ecs::EntityId getSingletonEntityHint(const char *key, uint32_t key_hash)
{
  return g_entity_mgr->getSingletonEntity(ecs::HashedConstString({key, key_hash}));
}
inline ecs::EntityId getSingletonEntity(const char *s) { return g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(s ? s : "")); }
inline bool is_entity_mgr_exists() { return (bool)g_entity_mgr; }

template <typename T>
const ecs::EntityComponentRef getEntityComponentRefList(const T &a, uint32_t idx)
{
  return a.getEntityComponentRef((size_t)idx);
}

template <typename T>
void list_ctor(const das::TBlock<void, T> &block, das::Context *context, das::LineInfoArg *at)
{
  bind_dascript::EsContext *ctx = cast_es_context(context);
  T list(ctx->mgr);
  vec4f arg = das::cast<T *>::from((T *)&list);
  context->invoke(block, &arg, nullptr, at);
}

inline ecs::EntityComponentRef cloneEntityComponentRef(const ecs::EntityComponentRef &a) { return a; }

inline uint32_t castEid(ecs::EntityId e) { return ecs::entity_id_t(e); }
inline ecs::EntityId eidCast(uint32_t e) { return ecs::EntityId(ecs::entity_id_t(e)); }

inline ecs::EntityId createEntitySync(const char *template_name, das::Context *ctx, das::LineInfoArg *at)
{
  return do_create_entity<true>(template_name, ecs::ComponentsInitializer{}, ctx, at);
}
inline ecs::EntityId createEntity(const char *template_name, das::Context *ctx, das::LineInfoArg *at)
{
  return do_create_entity<false>(template_name, ecs::ComponentsInitializer{}, ctx, at);
}
inline ecs::EntityId reCreateEntityFrom(ecs::EntityId eid, const char *template_name)
{
  return g_entity_mgr->reCreateEntityFromAsync(eid, template_name);
}


void process_view(const ecs::QueryView &qv, const das::Block &block, uint64_t init_args_hash, das::Context *das_context,
  das::LineInfoArg *line);
void queryEs(const das::Block &block, das::Context *das_context, das::LineInfoArg *line);
bool queryEsEid(ecs::EntityId eid, const das::Block &block, das::Context *das_context, das::LineInfoArg *line);
bool findQueryEs(const das::Block &block, das::Context *das_context, das::LineInfoArg *line);

template <typename T>
inline typename DasTypeParamAlias<T>::cparam_alias entity_get_type_impl(ecs::EntityId eid, ecs::HashedConstString key,
  eastl::true_type)
{
  return g_entity_mgr->get<T>(eid, key);
}
template <typename T>
inline typename DasTypeParamAlias<T>::cparam_alias entity_get_type_impl(ecs::EntityId eid, ecs::HashedConstString key,
  eastl::false_type)
{
  return *(const typename DasTypeAlias<T>::alias *)&g_entity_mgr->get<T>(eid, key);
}
template <typename T>
inline typename DasTypeParamAlias<T>::cparam_alias entity_get_type(ecs::EntityId eid, ecs::HashedConstString key)
{
  typedef typename eastl::type_select<ECS_MAYBE_VALUE(T), eastl::true_type, eastl::false_type>::type TP;
  return entity_get_type_impl<T>(eid, key, TP());
}

#define TYPE(type)                                                                                                                    \
  inline void entitySetOptionalHint##type(ecs::EntityId id, const char *key, uint32_t key_hash,                                       \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                                \
  {                                                                                                                                   \
    return g_entity_mgr->setOptional(id, ecs::HashedConstString({key, key_hash}), *(const type *)&to);                                \
  }                                                                                                                                   \
  inline void entitySetOptional##type(ecs::EntityId id, const char *s, typename DasTypeParamAlias<type>::cparam_alias to)             \
  {                                                                                                                                   \
    return entitySetOptionalHint##type(id, s, ECS_HASH_SLOW(s ? s : "").hash, to);                                                    \
  }                                                                                                                                   \
  inline void entitySetFastOptional##type(ecs::EntityId id, const ecs::FastGetInfo name,                                              \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                                \
  {                                                                                                                                   \
    return g_entity_mgr->setOptionalFast(id, name, *(const type *)&to);                                                               \
  }                                                                                                                                   \
  inline void entitySetHint##type(ecs::EntityId id, const char *key, uint32_t key_hash,                                               \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                                \
  {                                                                                                                                   \
    return g_entity_mgr->set(id, ecs::HashedConstString({key, key_hash}), *(const type *)&to);                                        \
  }                                                                                                                                   \
  inline void entitySet##type(ecs::EntityId id, const char *s, typename DasTypeParamAlias<type>::cparam_alias to)                     \
  {                                                                                                                                   \
    return entitySetHint##type(id, s, ECS_HASH_SLOW(s ? s : "").hash, to);                                                            \
  }                                                                                                                                   \
  inline void entitySetFast##type(ecs::EntityId id, const ecs::FastGetInfo name, typename DasTypeParamAlias<type>::cparam_alias to,   \
    const ecs::LTComponentList *list)                                                                                                 \
  {                                                                                                                                   \
    return g_entity_mgr->setFast(id, name, *(const type *)&to, list);                                                                 \
  }                                                                                                                                   \
  inline typename DasTypeParamAlias<type>::cparam_alias entityGetHint_##type(ecs::EntityId id, const char *key, uint32_t key_hash)    \
  {                                                                                                                                   \
    return entity_get_type<type>(id, ecs::HashedConstString({key, key_hash}));                                                        \
  }                                                                                                                                   \
  inline typename DasTypeParamAlias<type>::cparam_alias entityGet_##type(ecs::EntityId id, const char *s)                             \
  {                                                                                                                                   \
    return entityGetHint_##type(id, s, ECS_HASH_SLOW(s ? s : "").hash);                                                               \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *entityGetNullableHint_##type(ecs::EntityId id, const char *key, uint32_t key_hash) \
  {                                                                                                                                   \
    return (const typename DasTypeAlias<type>::alias *)g_entity_mgr->getNullable<type>(id, ecs::HashedConstString({key, key_hash}));  \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *entityGetNullable_##type(ecs::EntityId id, const char *s)                          \
  {                                                                                                                                   \
    return entityGetNullableHint_##type(id, s, ECS_HASH_SLOW(s ? s : "").hash);                                                       \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *entityGetNullableRWHint_##type(ecs::EntityId id, const char *key, uint32_t key_hash)     \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)g_entity_mgr->getNullableRW<type>(id, ecs::HashedConstString({key, key_hash}));      \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *entityGetNullableRW_##type(ecs::EntityId id, const char *s)                              \
  {                                                                                                                                   \
    return entityGetNullableRWHint_##type(id, s, ECS_HASH_SLOW(s ? s : "").hash);                                                     \
  }

ECS_BASE_TYPE_LIST
ECS_LIST_TYPE_LIST
#undef TYPE

inline void entitySetOptionalStrHint(ecs::EntityId eid, const char *key, uint32_t key_hash, const char *to)
{
  ecs::string *st = g_entity_mgr->getNullableRW<ecs::string>(eid, ecs::HashedConstString({key, key_hash}));
  if (st)
    *st = to ? to : "";
}
inline void entitySetOptionalStr(ecs::EntityId eid, const char *s, const char *to)
{
  entitySetOptionalStrHint(eid, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}

inline void entitySetStrHint(ecs::EntityId eid, const char *key, uint32_t key_hash, const char *to)
{
  ecs::string *st = g_entity_mgr->getNullableRW<ecs::string>(eid, ecs::HashedConstString({key, key_hash}));
  if (st)
    *st = to ? to : "";
  else
    logerr("eid = %d<%s> has no %s component to set to string", ecs::entity_id_t(eid), getEntityTemplateName(eid), key);
}
inline void entitySetStr(ecs::EntityId eid, const char *s, const char *to)
{
  entitySetStrHint(eid, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}

inline const char *entityGetStringHint(ecs::EntityId eid, const char *key, uint32_t key_hash, const char *default_value)
{
  ecs::string *st = g_entity_mgr->getNullableRW<ecs::string>(eid, ecs::HashedConstString({key, key_hash}));
  return st ? st->c_str() : default_value;
}
inline const char *entityGetString(ecs::EntityId eid, const char *s, const char *default_value)
{
  return entityGetStringHint(eid, s, ECS_HASH_SLOW(s ? s : "").hash, default_value);
}

inline void setEntityChildComponentHint(ecs::EntityId eid, const char *key, uint32_t key_hash, const ecs::ChildComponent &to)
{
  // Copy ChildComponent and pass it as rvalue to call the correct overload of setComponentInternal
  // (see EntityManager::setComponentInternal(EntityId eid, const HashedConstString name, ChildComponent &&a))
  // TODO: Perhaps that setComponentInternal should be reworked and these hacks will need to be removed.
  g_entity_mgr->set(eid, ecs::HashedConstString({key, key_hash}), ecs::ChildComponent(to));
}
inline void setEntityChildComponent(ecs::EntityId eid, const char *s, const ecs::ChildComponent &to)
{
  setEntityChildComponentHint(eid, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}

} // namespace bind_dascript

namespace das
{
template <class DasType, class EcsType>
struct EcsToDas
{
  template <typename T = EcsType>
  static auto &get(T &&v) // Both l and r-values
  {
    static_assert(!eastl::is_pointer_v<eastl::remove_reference_t<T>>); // Pointers should use it's own EcsToDas<> specialization
    static_assert(eastl::is_convertible_v<EcsType, DasType> || sizeof(DasType) == sizeof(EcsType));
    if constexpr (eastl::is_const_v<eastl::remove_reference_t<T>>)
      return (const DasType &)v;
    else
      return (DasType &)v;
  }
};

template <class DasType, class EcsType>
struct EcsToDas<DasType *, EcsType *>
{
  static DasType *get(const EcsType *v)
  {
    static_assert(eastl::is_convertible_v<EcsType, DasType> || sizeof(DasType) == sizeof(EcsType));
    return (DasType *)v;
  }
};

template <class DasType, class EcsType>
struct EcsToDas<const DasType *, const EcsType *> : EcsToDas<DasType *, EcsType *>
{};

template <class DasType, class EcsType>
struct EcsToDas<DasType *const, EcsType *const> : EcsToDas<DasType *, EcsType *>
{};

struct EcsToDasStr
{
  static char *get(const ecs::string &v) { return (char *)v.c_str(); }
  static char *get(const ecs::string *v, char *def_val) { return v ? (char *)v->c_str() : def_val; }
};

template <>
struct EcsToDas<char *&, ecs::string> : EcsToDasStr
{};

template <>
struct EcsToDas<char *const &, ecs::string> : EcsToDasStr
{};

template <class DasType, class EcsType>
struct EcsToDas<const DasType *, const ecs::SharedComponent<EcsType> *>
{
  static const DasType *get(const ecs::SharedComponent<EcsType> *v) { return v ? v->get() : nullptr; }
};

inline const char *get_immutable_string(const ecs::string &v) { return v.c_str(); }
inline const char *get_default_immutable_string(const ecs::string *v, const char *const def) { return v ? v->c_str() : def; }
} // namespace das

ECS_DECLARE_TYPE_ALIAS(das::float3x4, "TMatrix");
