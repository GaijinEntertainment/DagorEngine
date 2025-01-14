// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <util/dag_string.h>
#include <dasModules/dasEvent.h>
#include <dasModules/dasScriptsLoader.h>
#include <dasModules/dasCreatingTemplate.h>
#include <dasModules/aotEcs.h>
#include <ecs/scripts/dasEs.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/internal/ecsCounterRestorer.h>
#include <daECS/core/internal/performQuery.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string_view.h>
#include <EASTL/array.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_files.h>
#include <daScript/daScript.h>
#include <daScript/simulate/bin_serializer.h>
#include <daECS/core/coreEvents.h>
#include <daScript/misc/smart_ptr.h>
#include <daScript/misc/sysos.h>
#include <ecs/scripts/dascripts.h>
#include "das_es.h"

#include <memory/dag_dbgMem.h>
#include <memory/dag_memStat.h>
#include <osApiWrappers/dag_threads.h>
#include <contentUpdater/version.h>
#include <util/dag_threadPool.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#define ES_TO_QUERY     1
#define SHARED_COMP_ARG "shared_comp"

ECS_REGISTER_EVENT(EventDaScriptReloaded);

extern void require_project_specific_debugger_modules();

namespace bind_dascript
{
das_context_log_cb global_context_log_cb;

extern ecs::event_flags_t get_dasevent_cast_flags(ecs::event_type_t type);
extern bool enableSerialization;
extern bool serializationReading;
extern bool suppressSerialization;
extern eastl::string deserializationFileName;
extern eastl::string serializationFileName;
extern das::unique_ptr<das::SerializationStorage> initSerializerStorage;
extern das::unique_ptr<das::SerializationStorage> initDeserializerStorage;
extern das::unique_ptr<das::AstSerializer> initSerializer;
extern das::unique_ptr<das::AstSerializer> initDeserializer;
uint32_t g_serializationVersion = 0; // TODO: extract to settings

struct LoadedScript;

static das::daScriptEnvironment *mainThreadBound = nullptr;
static das::daScriptEnvironment *debuggerEnvironment = nullptr;
static AotMode globally_aot_mode = AotMode::AOT;
HotReload globally_hot_reload = HotReload::ENABLED;
static LogAotErrors globally_log_aot_errors = LogAotErrors::YES;
static int globally_load_threads_num = 1;
static bool globally_loading_in_queue = false;
static eastl::string globally_thread_init_script;
static ResolveECS globally_resolve_ecs_on_load = ResolveECS::YES;
static int verboseExceptions = 4;
static das::vector<eastl::string> loadingQueue;
#if DAGOR_DBGLEVEL > 0
static eastl::vector_set<ecs::hash_str_t> loadingQueueHash;
#endif
static FixedBlockAllocator esDescsAllocator(sizeof(EsDesc), 1024);
static FixedBlockAllocator esQueryDescsAllocator(sizeof(EsQueryDesc), 1024);
void EsDescDeleter::operator()(EsDesc *p)
{
  if (p)
  {
    eastl::destroy_at(p);
    esDescsAllocator.freeOneBlock(p);
  }
}
void EsQueryDescDeleter::operator()(EsQueryDesc *p)
{
  if (p)
  {
    eastl::destroy_at(p);
    esQueryDescsAllocator.freeOneBlock(p);
  }
}
typedef OAHashNameMap<false> DebugArgStrings;
bool das_is_in_init_phase = false;
static eastl::vector_map<eastl::string_view, eastl::string_view, eastl::less<eastl::string_view>, EASTLAllocatorType,
  eastl::fixed_vector<eastl::pair<eastl::string_view, eastl::string_view>, 2, true>>
  das_struct_aliases; // Note: static strings assumed

#if DAGOR_DBGLEVEL > 0
static bool in_thread_safe_das_ctx_region = false;
void enable_thread_safe_das_ctx_region(bool on) { in_thread_safe_das_ctx_region = on; }
#else
void enable_thread_safe_das_ctx_region(bool) {}
#endif

static void das_es_on_event_generic(const ecs::Event &evt, const ecs::QueryView &__restrict qv);
static void das_es_on_das_event(const ecs::Event &evt, const ecs::QueryView &__restrict qv);
static void das_es_on_event_generic_empty(const ecs::Event &evt, const ecs::QueryView &__restrict qv);
static void das_es_on_event_das_event_empty(const ecs::Event &evt, const ecs::QueryView &__restrict qv);
static void das_es_on_update(const ecs::UpdateStageInfo &info, const ecs::QueryView &__restrict qv);
static void das_es_on_update_empty(const ecs::UpdateStageInfo &info, const ecs::QueryView &__restrict qv);

static bool alwaysUseMainThreadEnv = true;

bool get_always_use_main_thread_env() { return alwaysUseMainThreadEnv; }

void set_always_use_main_thread_env(bool value) { alwaysUseMainThreadEnv = value; }

struct RAIIDasEnvBound
{
  das::daScriptEnvironment *savedBound = nullptr;
  RAIIDasEnvBound()
  {
    if (alwaysUseMainThreadEnv)
    {
      G_ASSERT(mainThreadBound);
      if (!is_main_thread())
      {
        savedBound = das::daScriptEnvironment::bound; // Preserve old value for nested ESes
        das::daScriptEnvironment::bound = mainThreadBound;
      }
    }
  };
  ~RAIIDasEnvBound()
  {
    if (alwaysUseMainThreadEnv && !is_main_thread())
      das::daScriptEnvironment::bound = savedBound;
  };
};

struct RAIIDasTempEnv
{
  das::daScriptEnvironment *initialOwned;
  das::daScriptEnvironment *initialBound;
  RAIIDasTempEnv(das::daScriptEnvironment *owned = nullptr, das::daScriptEnvironment *bound = nullptr)
  {
    initialOwned = das::daScriptEnvironment::owned;
    initialBound = das::daScriptEnvironment::bound;
    das::daScriptEnvironment::owned = owned;
    das::daScriptEnvironment::bound = bound;
  }
  ~RAIIDasTempEnv()
  {
    das::daScriptEnvironment::owned = initialOwned;
    das::daScriptEnvironment::bound = initialBound;
  }
};

struct RAIIContextMgr
{
  ecs::EntityManager *prev;
  EsContext *ctx;
  RAIIContextMgr(EsContext *ctx_, ecs::EntityManager *mgr)
  {
    ctx = ctx_;
    prev = ctx->mgr;
    ctx->mgr = mgr;
  }
  ~RAIIContextMgr() { ctx->mgr = prev; }
};

struct UpdateStageInfoActAnnotation final : das::ManagedStructureAnnotation<ecs::UpdateStageInfoAct, false>
{
  UpdateStageInfoActAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("UpdateStageInfoAct", ml)
  {
    cppName = " ::ecs::UpdateStageInfoAct";
    addField<DAS_BIND_MANAGED_FIELD(dt)>("dt");
    addField<DAS_BIND_MANAGED_FIELD(curTime)>("curTime");
  }
};

struct UpdateStageInfoRenderDebugAnnotation final : das::ManagedStructureAnnotation<ecs::UpdateStageInfoRenderDebug, false>
{
  UpdateStageInfoRenderDebugAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("UpdateStageInfoRenderDebug", ml)
  {
    cppName = " ::ecs::UpdateStageInfoRenderDebug";
  }
};


struct QueryViewAnnotation final : das::ManagedStructureAnnotation<ecs::QueryView, false>
{
  QueryViewAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("QueryView", ml) { cppName = " ::ecs::QueryView"; }
};

} // namespace bind_dascript

MAKE_TYPE_FACTORY(UpdateStageInfoAct, ecs::UpdateStageInfoAct)
MAKE_TYPE_FACTORY(UpdateStageInfoRenderDebug, ecs::UpdateStageInfoRenderDebug)
MAKE_TYPE_FACTORY(QueryView, ecs::QueryView)

namespace bind_dascript
{
inline bool component_compare(const ecs::ComponentDesc &a, const ecs::ComponentDesc &b) { return a.name < b.name; }

inline bool generic_event_name(const das::string &name) { return name == "ecs::Event"; }
inline bool valid_event_name(const das::TypeDecl &info, const das::string &name)
{
  return generic_event_name(name) || info.baseType == das::Type::tStructure;
}

inline bool valid_stage_name(const das::string &name)
{
  return name == "ecs::UpdateStageInfoAct" || name == "ecs::UpdateStageInfoRenderDebug";
}
inline bool valid_es_param_name(const das::TypeDecl &info, const das::string &name)
{
  return valid_stage_name(name) || valid_event_name(info, name);
}

template <class Container>
static inline void components_from_list(const das::AnnotationArgumentList &args, const char *name, Container &comps,
  DebugArgStrings &argStrings)
{
  eastl::fixed_vector<ecs::ComponentDesc, 4, true> components;
  G_UNUSED(argStrings);
  constexpr ecs::component_t auto_type_t = ecs::ComponentTypeInfo<ecs::auto_type>::type;
  for (const auto &arg : args)
    if (arg.type == das::Type::tString && arg.name == name)
    {
      auto argNameHash = ECS_HASH_SLOW(arg.sValue.c_str());
#if DAGOR_DBGLEVEL > 0
      argNameHash.str = argStrings.getName(argStrings.addNameId(argNameHash.str));
#endif
      components.emplace_back(argNameHash, auto_type_t);
    }
  eastl::sort(components.begin(), components.end(), component_compare);
  comps.insert(comps.end(), components.begin(), components.end());
}

static inline void events_from_list(const das::AnnotationArgumentList &args, const char *name, ecs::EventSet &evtMask)
{
  for (const auto &arg : args)
    if (arg.type == das::Type::tString && arg.name == name)
      evtMask.insert(ecs::event_type_t(ECS_HASH_SLOW(arg.sValue.c_str()).hash));
}

static inline void tags_from_list(const das::AnnotationArgumentList &args, const char *name, das::string &str)
{
  for (const auto &arg : args)
    if (arg.type == das::Type::tString && arg.name == name)
      str.append_sprintf("%s%s", str.length() > 0 ? "," : "", arg.sValue.c_str());
}
} // namespace bind_dascript

das::Context *get_clone_context(das::Context *ctx, uint32_t category) { return new bind_dascript::EsContext(*ctx, category, false); }
das::Context *get_clone_context(das::Context *ctx, uint32_t category, void *user_data)
{
  return new bind_dascript::EsContext(*ctx, category, true, user_data);
}

das::Context *get_context(int stack_size) { return new bind_dascript::EsContext(stack_size); }

namespace bind_dascript
{

void BaseEsDesc::resolveUnresolvedOutOfLine()
{
  if (!g_entity_mgr)
    return;
  resolved = true;
  for (uint32_t c = 0, e = dataCompCount(); c != e; ++c)
  {
    uint32_t a = argIndices[c];
    if (stride[a]) // already resolved
      continue;
    ecs::type_index_t tpidx = g_entity_mgr->getComponentTypes().findType(components[c].type);
    if (tpidx == ecs::INVALID_COMPONENT_TYPE_INDEX) // this could happen here, if and only if data is optional
    {
      resolved = false;
      continue;
    }
    ecs::ComponentType tpInfo = g_entity_mgr->getComponentTypes().getTypeInfo(tpidx);
    flags[a] |= (tpInfo.flags & (ecs::COMPONENT_TYPE_BOXED | ecs::COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE)) ? BOXED_SRC : 0;
    stride[a] = tpInfo.size;
  }
}

void make_string_outer_ns(das::string &typeName)
{
  const int firstNonWhiteSpace = typeName.find_first_not_of(" ");
  const int firstNS = typeName.find("::", firstNonWhiteSpace);
  if (firstNS != firstNonWhiteSpace && firstNS >= 0)
    typeName = " ::" + typeName;
}

static bool get_underlying_ecs_type(das::Type type, das::string &str)
{
  switch (type)
  {
    case das::tString: str = "ecs::string"; return true;
    case das::tBool: str = "bool"; return true;
    case das::tInt64: str = "int64_t"; return true;
    case das::tInt8: str = "int8_t"; return true;
    case das::tInt16: str = "int16_t"; return true;
    case das::tUInt64: str = "uint64_t"; return true;
    case das::tUInt8: str = "uint8_t"; return true;
    case das::tUInt16: str = "uint16_t"; return true;
    case das::tInt: str = "int"; return true;
    case das::tInt2: str = "IPoint2"; return true;
    case das::tInt3: str = "IPoint3"; return true;
    case das::tInt4: str = "IPoint4"; return true;
    case das::tUInt: str = "uint32_t"; return true;
    // case tUInt2:        return sizeof(uint2);
    // case tUInt3:        return sizeof(uint3);
    // case tUInt4:        return sizeof(uint4);
    case das::tFloat: str = "float"; return true;
    case das::tFloat2: str = "Point2"; return true;
    case das::tFloat3: str = "Point3"; return true;
    case das::tFloat4: str = "Point4"; return true;
    case das::tDouble: str = "double"; return true;
    default: return false;
  }
}

bool get_underlying_ecs_type(const das::TypeDecl &info, das::string &str, das::Type &type, bool module_name)
{
  type = info.baseType;
  if (info.baseType == das::Type::tHandle)
  {
    if (!info.annotation)
      return false;
    auto it = das_struct_aliases.find(info.annotation->name.c_str());
    if (it == das_struct_aliases.end())
    {
      if (!info.annotation->cppName.empty())
      {
        const char *cppName = info.annotation->cppName.c_str();
        while (*cppName == ' ' || *cppName == ':')
          cppName++;
        str = cppName;
      }
      else
      {
        str = (module_name && info.annotation->module && info.annotation->module->name != "$"
                 ? info.annotation->module->name + "::" + info.annotation->name
                 : info.annotation->name);
      }
    }
    else
      str = it->second;
    return true;
  }
  else if (info.baseType == das::Type::tStructure)
  {
    if (!info.structType)
      return false;
    str =
      module_name && info.structType->module ? info.structType->module->name + "::" + info.structType->name : info.structType->name;
    return true;
  }
  else if (info.baseType == das::Type::tPointer)
  {
    if (!info.firstType)
      return false;
    return get_underlying_ecs_type(*info.firstType, str, type);
  }
  else if (info.baseType == das::Type::tEnumeration)
  {
    if (!info.enumType)
      return false;
    if (info.enumType->external)
    {
      const char *cppName = info.annotation->cppName.c_str();
      while (*cppName == ' ' || *cppName == ':')
        cppName++;
      str = cppName;
    }
    else
    {
      str = module_name && info.enumType->module ? info.enumType->module->name + "::" + info.enumType->name : info.enumType->name;
    }
    return true;
  }
  else if (info.alias == "vec4f")
  {
    str = "vec4f";
    return true;
  }
  else
    return get_underlying_ecs_type(info.baseType, str);
}

bool get_underlying_ecs_type_with_string(const das::TypeDecl &info, das::string &str, das::Type &type, bool module_name)
{
  if (info.baseType == das::tString)
  {
    str = "ecs::string";
    type = das::tString;
    return true;
  }
  return get_underlying_ecs_type(*info.firstType.get(), str, type, module_name);
}

static bool get_default_ecs_value(das::Type type, das::TextWriter &str, vec4f init, int index)
{
  switch (type)
  {
    // case das::tString:   str << "\"" << das::cast<char*>::to(init) << "\""; return true;
    case das::tBool: str << "\tbool __def_comp" << index << " = " << das::cast<bool>::to(init) << ";\n"; return true;
    case das::tInt64: str << "\tint64_t __def_comp" << index << " = " << das::cast<int64_t>::to(init) << "ll;\n"; return true;
    case das::tUInt64: str << "\tuint64_t __def_comp" << index << " = " << das::cast<uint64_t>::to(init) << "ull;\n"; return true;
    case das::tInt: str << "\tint32_t __def_comp" << index << " = " << das::cast<int>::to(init) << ";\n"; return true;
    case das::tInt2:
      str << "\tauto __def_comp" << index << " = "
          << "das::int2(" << das::cast<das::int2>::to(init) << ");\n";
      return true;
    case das::tInt3:
      str << "\t\tauto __def_comp" << index << " = "
          << "das::int3(" << das::cast<das::int3>::to(init) << ");\n";
      return true;
    case das::tInt4:
      str << "\tauto __def_comp" << index << " = "
          << "das::int4(" << das::cast<das::int4>::to(init) << ");\n";
      return true;
    case das::tUInt: str << "\tuint32_t __def_comp" << index << " = " << das::cast<uint32_t>::to(init) << "u;\n"; return true;
    // case tUInt2:      return sizeof(uint2);
    // case tUInt3:      return sizeof(uint3);
    // case tUInt4:      return sizeof(uint4);
    case das::tFloat:
      str << "\tfloat __def_comp" << index << " = " << das::to_cpp_float(das::cast<float>::to(init)) << ";\n";
      return true;
    case das::tFloat2:
      str << "\tauto __def_comp" << index << " = "
          << "das::float2(" << das::cast<das::float2>::to(init) << ");\n";
      return true;
    case das::tFloat3:
      str << "\tauto __def_comp" << index << " = "
          << "das::float3(" << das::cast<das::float3>::to(init) << ");\n";
      return true;
    case das::tFloat4:
      str << "\tauto __def_comp" << index << " = "
          << "das::float4(" << das::cast<das::float4>::to(init) << ");\n";
      return true;
    case das::tDouble:
      str << "\tdouble __def_comp" << index << " = " << das::to_cpp_double(das::cast<double>::to(init)) << ";\n";
      return true;
    default: return false;
  }
}

static bool get_default_ecs_value(das::TypeDecl &info, das::TextWriter &str, das::ExpressionPtr initExpr, vec4f init, int index)
{
  if (initExpr->rtti_isConstant() && initExpr->rtti_isStringConstant())
  {
    str << "\tauto __def_comp" << index << " = \"" << das::static_pointer_cast<das::ExprConstString>(initExpr)->text << "\";\n";
    return true;
  }
  if (info.baseType == das::Type::tHandle)
  {
    if (!info.annotation)
      return false;
    if (info.isRefType())
      return false;
    vec4i initi = v_cast_vec4i(init);
    str << "\t" << info.annotation->cppName << " __def_comp" << index << " = ";
    str << "das::cast<" << info.annotation->cppName << ">::to(v_cast_vec4f(v_make_vec4i(int(0x" << das::HEX
        << uint32_t(v_extract_xi(initi)) << "), int(0x" << uint32_t(v_extract_yi(initi)) << "), int(0x"
        << uint32_t(v_extract_zi(initi)) << "), int(0x" << uint32_t(v_extract_wi(initi)) << ")" << das::DEC << ")));\n";
    return true;
  }
  else if (info.baseType == das::Type::tStructure)
  {
    return false;
  }
  else if (info.baseType == das::Type::tPointer)
  {
    if (!info.firstType)
      return false;
    das::Type dasType;
    das::string typeName;
    if (!get_underlying_ecs_type(info, typeName, dasType))
      return false;
    make_string_outer_ns(typeName);
    str << "\t" << typeName << "__def_comp" << index << " = nullptr;\n";
    return true;
  }
  else if (info.baseType == das::Type::tEnumeration)
  {
    if (!info.enumType)
      return false;
    das::string p = info.enumType->find(v_extract_xi(v_cast_vec4i(init)), "");
    if (p == "")
      return false;
    str << "\tauto __def_comp" << index << " = " << info.enumType->name << "::" << p << ";\n";
    return true;
  }
  else
    return get_default_ecs_value(info.baseType, str, init, index);
}

static bool is_entity_manager(const das::string &type_name) { return type_name == "ecs::EntityManager"; }

static bool is_entity_manager(const das::TypeDecl &info)
{
  das::string typeName;
  das::Type dasType;
  return get_underlying_ecs_type(info, typeName, dasType) && is_entity_manager(typeName);
}

template <class Container>
bool build_components_table(const char *name, BaseEsDesc &desc, Container &comps, bool can_use_access,
  const das::vector<das::VariablePtr> &arguments, uint32_t start_arg, das::string &err, DebugArgStrings &argStrings)
{
  G_UNUSED(argStrings);
  enum
  {
    RW = 0,
    RO = 1
  };
  eastl::fixed_vector<ecs::ComponentDesc, 4, true> components[2]; // rw, ro
  uint16_t argNo = 0;
  vec4f *__restrict defArg = (vec4f *)alloca(arguments.size() * sizeof(vec4f));            // more than enough
  int32_t *__restrict compIndices = (int32_t *)alloca(arguments.size() * sizeof(int32_t)); // more than enough
  uint8_t *__restrict flags = (uint8_t *)alloca(arguments.size() * sizeof(uint8_t));       // more than enough
  uint32_t optCount = 0;
  das::string typeName;
  das::string cerr;
  for (auto argI = arguments.begin() + start_arg, argE = arguments.end(); argI != argE; ++argI)
  {
    const auto &arg = *argI;
    das::Type dasType;
    if (!get_underlying_ecs_type(*arg->type, typeName, dasType))
    {
      const bool isSoaArray = arg->type->baseType == das::Type::tArray && ends_with(arg->type->alias, "_SOA");
      // skip error for soa arrays in completion mode (only for lsp plugin)
      if (!das::is_in_completion() || !isSoaArray)
      {
        err.append_sprintf("daScript: es<%s> arg:%d of type <%s> %s can not be used in ES (invalid type) %d %s\n", name, argNo,
          arg->type->describe().c_str(), arg->name.c_str(), das::is_in_completion(), arg->type->alias.c_str());
        continue;
      }
    }
    if (is_entity_manager(typeName))
    {
      // TODO: verify type, rw, etc
      if (argI != arguments.begin() + start_arg)
      {
        err.append_sprintf("daScript: es<%s> arg:%d of type <%s> %s can be placed only as %d-th argument\n", name, argNo,
          arg->type->describe().c_str(), arg->name.c_str(), start_arg + 1);
        continue;
      }
      desc.useManager = true;
      continue;
    }
    vec4f def = v_zero();
    const bool isPointerDest = arg->type->isPointer() || arg->type->isRef();
    const bool isROByType = arg->type->isConst() || !isPointerDest;
    const bool onlyReading = !arg->access_ref;
    bool isRO = isROByType || (onlyReading && !can_use_access);
    if ((onlyReading && !can_use_access) && !isROByType)
      isRO = true; // forcibly set soa fields as ro
// automatically set ro flag for soa fields
#if DAGOR_DBGLEVEL > 0
    if (arg->isAccessUnused() && !arg->marked_used)
    {
      logerr("%s: component <%s> is not used in es<%s> \n", arg->at.describe().c_str(), arg->name.c_str(), name);
    }
    else if ((onlyReading && !can_use_access) && !isROByType)
    {
      logerr("%s: es<%s> arg #%d<%s> of type  <%s> is RW by it's type, but code is not writing to it\n", arg->at.describe().c_str(),
        name, argNo, arg->name.c_str(), arg->type->describe().c_str());
    }
#endif
    const bool isTypeOptional = arg->type->isPointer();
    bool isOptional = isTypeOptional;
    if (arg->init)
    {
      if (arg->init->rtti_isConstant())
      {
        if (!arg->init->rtti_isStringConstant())
          def = das::static_pointer_cast<das::ExprConst>(arg->init)->value;
        else
        {
          desc.defStrings.emplace_back(das::static_pointer_cast<das::ExprConstString>(arg->init)->text.c_str());
          def = das::cast<char *>::from(desc.defStrings.back().c_str());
        }
        isOptional = true;
      }
      else if (!arg->init->type->isRefType() && arg->init->noSideEffects && arg->init->rtti_isVar() &&
               das::static_pointer_cast<das::ExprVar>(arg->init)->isGlobalVariable() &&
               das::static_pointer_cast<das::ExprVar>(arg->init)->variable->type->constant)
      {
        if (arg->type->ref)
        {
          err.append_sprintf("%s: Unsupported arg type. Default value for %s is ref type \n", name, arg->name.c_str());
        }
        else
        {
          auto var = das::static_pointer_cast<das::ExprVar>(arg->init)->variable;
          if (var->init->rtti_isConstant())
          {
            if (!var->init->rtti_isStringConstant())
              def = das::static_pointer_cast<das::ExprConst>(var->init)->value;
            else
            {
              desc.defStrings.emplace_back(das::static_pointer_cast<das::ExprConstString>(var->init)->text.c_str());
              def = das::cast<char *>::from(desc.defStrings.back().c_str());
            }
            isOptional = true;
          }
          else
          {
            das::Context ctx;
            auto node = var->init->simulate(ctx);
            ctx.restart();
            vec4f result = ctx.evalWithCatch(node);
            if (ctx.getException())
              err.append_sprintf("%s: default value for %s failed to simulate\n", name, arg->name.c_str());
            else
              def = result;
            isOptional = true;
          }
        }
      }
      else
        err.append_sprintf("%s: default value for %s is not a constant\n", name, arg->name.c_str());
      if (!isRO)
        err.append_sprintf("%s: default value for RW variable %s is not allowed\n", name, arg->name.c_str());
    }
    if (dasType == das::tString)
    {
      if (!isRO || isPointerDest)
        err.append_sprintf("%s:string arguments should be only const values, and %s is %s\n", name, arg->name.c_str(),
          arg->type->describe().c_str());
    }
    auto &compUsed = components[isRO ? RO : RW];
    const bool isSharedComponent = arg->annotation.getBoolOption(SHARED_COMP_ARG);
    if (isSharedComponent)
    {
      if (dasType == das::tString)
        err.append_sprintf("%s:[%s]string arguments should be das_string, and %s is [%s] string. Use das_string.\n", name,
          SHARED_COMP_ARG, arg->name.c_str(), SHARED_COMP_ARG);
      if (arg->init)
        err.append_sprintf("%s:[%s] argument shouldn't be default initialized, and %s is [%s].\n", name, SHARED_COMP_ARG,
          arg->name.c_str(), SHARED_COMP_ARG);
      if (!isRO)
        err.append_sprintf("%s:[%s] arguments can't be RW, and %s is [%s].\n", name, SHARED_COMP_ARG, arg->name.c_str(),
          SHARED_COMP_ARG);
      bool qualifiedEcs = false;
      switch (ECS_HASH_SLOW(typeName.c_str()).hash)
      {
#define DECL_LIST_TYPE(lt, t) case ecs::ComponentTypeInfo<ecs::lt>::type:
        ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
        case ecs::ComponentTypeInfo<ecs::Object>::type:
        case ecs::ComponentTypeInfo<ecs::Array>::type:
        case ecs::ComponentTypeInfo<ecs::string>::type: qualifiedEcs = true; break;
      }
      typeName =
        eastl::string(eastl::string::CtorSprintf(), "ecs::SharedComponent<%s%s>", qualifiedEcs ? " ::" : "", typeName.c_str());
    }
    const ecs::component_type_t ecsType = ECS_HASH_SLOW(typeName.c_str()).hash;
    ecs::HashedConstString argNameHash = ECS_HASH_SLOW(arg->name.c_str());
#if DAGOR_DBGLEVEL > 0
    argNameHash.str = argStrings.getName(argStrings.addNameId(argNameHash.str));
#endif
    ecs::ComponentDesc newDesc(argNameHash, ecsType, isOptional ? ecs::CDF_OPTIONAL : 0);
    auto insertIdx = eastl::lower_bound(compUsed.begin(), compUsed.end(), newDesc, component_compare);
    auto newIdx = compUsed.emplace(insertIdx, newDesc);
    const int compIndex = int(newIdx - compUsed.begin()) + 1;
    for (int k = 0; k < argNo; ++k)
    {
      if ((compIndices[k] < 0) == isRO)
      {
        auto wasIdx = isRO ? -compIndices[k] : compIndices[k];
        if (wasIdx >= compIndex) // shift right comps
          compIndices[k] = isRO ? -(wasIdx + 1) : wasIdx + 1;
      }
    }
    compIndices[argNo] = isRO ? -compIndex : compIndex;
    flags[argNo] = dasType == das::tString ? STRING_DEST : (isPointerDest ? POINTER_DEST : 0);
    // flags[argNo] |= isSharedComponent ? SHARED_COMPONENT : 0;
    defArg[argNo] = def;
    if (isOptional)
      optCount++;
    argNo++;
  }
  if (!err.empty())
    return false;
  comps.assign(components[RW].begin(), components[RW].end());
  desc.ends[BaseEsDesc::RW_END] = comps.size();
  comps.insert(comps.end(), components[RO].begin(), components[RO].end());
  desc.ends[BaseEsDesc::RO_END] = comps.size();
  desc.stride = eastl::make_unique<uint16_t[]>(argNo);
  desc.argIndices = eastl::make_unique<uint16_t[]>(argNo);
  desc.flags = eastl::make_unique<uint8_t[]>(argNo);
  if (optCount)
    desc.def = eastl::make_unique<vec4f[]>(argNo);
  desc.combinedFlags = optCount ? HAS_OPTIONAL : 0;
  for (int i = 0; i < argNo; ++i)
  {
    desc.stride[i] = 0;
    const uint32_t argIndex = (compIndices[i] < 0 ? (int)desc.compCount(BaseEsDesc::RW_END) - compIndices[i] : compIndices[i]) - 1;
    // debug("%d: %d", i, argIndex);
    desc.argIndices[argIndex] = i;
    desc.flags[i] = flags[i];
    desc.combinedFlags |= flags[i];
    if (desc.def)
      desc.def[i] = defArg[i];
  }
  return true;
}

inline bool entity_system_is_das(ecs::EntitySystemDesc *desc)
{
  return desc->isDynamic() && desc->getUserData() &&
         (desc->getOps().onUpdate == &das_es_on_update_empty || desc->getOps().onUpdate == &das_es_on_update ||
           desc->getOps().onEvent == das_es_on_event_generic || desc->getOps().onEvent == das_es_on_event_generic_empty ||
           desc->getOps().onEvent == das_es_on_event_das_event_empty || desc->getOps().onEvent == das_es_on_das_event); // otherwise it
                                                                                                                        // is not ours
}
static void das_es_desc_deleter(ecs::EntitySystemDesc *desc);

uint32_t ESModuleGroupData::es_resolve_function_ptrs(EsContext *ctx, dag::VectorSet<ecs::EntitySystemDesc *> &systems, const char *fn,
  uint64_t load_start_time, AotMode aot_mode, AotModeIsRequired aot_mode_is_required, DasEcsStatistics &stats)
{
  uint32_t cnt = 0;
  int totalSystems = 0;
  int aotSystems = 0;
  for (auto &i : unresolvedEs)
  {
    i.first->context = ctx;
    // debug(" <%s>", i.second.c_str());
    bool isUnique = false;
    i.first->functionPtr = ctx->findFunction(i.second.c_str(), isUnique);
    if (!i.first->functionPtr)
    {
      logerr("function <%s> not found", i.second.c_str());
      continue;
    }
    if (!isUnique)
    {
      logerr("function <%s> is not unique, ambiguity", i.second.c_str());
      continue;
    }

    // ctor puts system to the list
    const bool isDasEvent = i.first->typeInfo != nullptr;
    totalSystems++;
    aotSystems += i.first->functionPtr->aot ? 1 : 0;
#if DAGOR_DBGLEVEL > 1 || DAECS_EXTENSIVE_CHECKS // we can't get logs from consoles in release
    if (!i.first->functionPtr->aot && aot_mode_is_required == AotModeIsRequired::YES)
      debug("daScript: register_es %s<%s>%s, with 0x%X update mask and %d%s events before<%s> after<%s> tags=<%s> , tracked "
            "components=<%s> "
            "%d components",
        i.first->functionPtr->aot ? "AOT:" : "INTERPRET:", i.second.c_str(), fn ? fn : "", i.first->stageMask, i.first->evtMask.size(),
        isDasEvent ? " scripted" : " core", i.first->beforeList.c_str(), i.first->afterList.c_str(), i.first->tagsList.c_str(),
        i.first->trackedList.c_str(), i.first->base.components.size());
#endif
    auto es = new ecs::EntitySystemDesc(i.first->functionPtr->name, fn,
      ecs::EntitySystemOps(
        i.first->stageMask == 0 ? nullptr : (i.first->base.components.size() ? das_es_on_update : das_es_on_update_empty),
        i.first->evtMask.empty() && i.first->trackedList.length() == 0
          ? nullptr
          : (i.first->base.components.size() ? (isDasEvent ? das_es_on_das_event : das_es_on_event_generic)
                                             : (isDasEvent ? das_es_on_event_das_event_empty : das_es_on_event_generic_empty))),
      i.first->base.getSlice(BaseEsDesc::RW_END), i.first->base.getSlice(BaseEsDesc::RO_END),
      i.first->base.getSlice(BaseEsDesc::RQ_END), i.first->base.getSlice(BaseEsDesc::NO_END), eastl::move(i.first->evtMask),
      i.first->stageMask, i.first->tagsList.length() > 0 ? i.first->tagsList.c_str() : nullptr,
      i.first->trackedList.length() > 0 ? i.first->trackedList.c_str() : nullptr,
      i.first->beforeList.length() > 0 ? i.first->beforeList.c_str() : nullptr,
      i.first->afterList.length() > 0 ? i.first->afterList.c_str() : nullptr, i.first.get(), i.first->minQuant, true,
      das_es_desc_deleter);
    i.first.release();
    systems.insert(es);
    cnt++;
  }
  if (fn)
    debug("daScript: %s resolved in %d ms, %d interpreted es, %d aot es%s", fn, profile_time_usec(load_start_time) / 1000,
      totalSystems - aotSystems, aotSystems, aot_mode == AotMode::NO_AOT ? " [no_aot]" : "");
  stats.systemsCount += totalSystems;
  stats.aotSystemsCount += (aot_mode_is_required == AotModeIsRequired::NO) ? totalSystems : aotSystems;
  if (totalSystems > 0 && aot_mode == AotMode::NO_AOT && aot_mode_is_required == AotModeIsRequired::YES)
  {
    logerr("<%s>: has disabled AOT when it should not. Remove `options no_aot` or put ES's into _debug.das/_console.das scripts.", fn);
  }
  unresolvedEs.clear();
  if (cnt && !das_is_in_init_phase && g_entity_mgr)
    g_entity_mgr->resetEsOrder();
  return cnt;
}

static bool build_es(BaseEsDesc &esDesc, const char *debug_name, const das::vector<das::VariablePtr> &arguments,
  const das::AnnotationArgumentList &args, uint32_t start_arg, das::string &err, bool can_use_access, DebugArgStrings &argStrings)
{
  eastl::fixed_vector<ecs::ComponentDesc, 16, true> components;
  bool use_access = can_use_access && args.getBoolOption("trust_access", false);
  build_components_table(debug_name, esDesc, components, use_access, arguments, start_arg, err, argStrings);
  if (!err.empty())
    return false;
  components_from_list(args, "REQUIRE", components, argStrings);
  esDesc.ends[BaseEsDesc::RQ_END] = components.size();
  components_from_list(args, "REQUIRE_NOT", components, argStrings);
  esDesc.ends[BaseEsDesc::NO_END] = components.size();
  esDesc.components.assign(components.begin(), components.end());
  esDesc.resolveUnresolved(); // try to resolve asap
  return true;
};

template <typename T = const char *>
using str_hash_set = ska::flat_hash_set<T, eastl::hash<T>, eastl::str_equal_to<T>>;
static const str_hash_set<> supportedSystemArgs = {"REQUIRE", "REQUIRE_NOT", "on_appear", "on_disappear", "on_event", "tag", "track",
  "before", "after", "no_order", "trust_access", "parallel_for"};
static const str_hash_set<> supportedQueryArgs = {"REQUIRE", "REQUIRE_NOT", "trust_access"};

struct EsFunctionAnnotation final : das::FunctionAnnotation
{
  enum class EsFunctionKind
  {
    System,
    Query,
  };
  EsFunctionAnnotation() : FunctionAnnotation("es") {}
  ESModuleGroupData *getGroupData(das::ModuleGroup &group) const
  {
    // skip in completion mode to avoid memory leaks
    if (das::is_in_completion())
      return nullptr;
    if (auto data = group.getUserData("es"))
    {
      return (ESModuleGroupData *)data;
    }
    auto esData = new ESModuleGroupData(*(new DebugArgStrings), g_entity_mgr.get()); // this can only happen in AOT
    group.setUserData(esData);
    return esData;
  }
  __forceinline bool checkArgumentNames(const das::AnnotationArgumentList &args, EsFunctionKind func_kind, das::string &err)
  {
    auto &supportedArgs = func_kind == EsFunctionKind::System ? supportedSystemArgs : supportedQueryArgs;
    for (const das::AnnotationArgument &arg : args)
      if (supportedArgs.find(arg.name.c_str()) == supportedArgs.end())
      {
#if DAGOR_DBGLEVEL > 0 // in release keep it forward compatible with new arguments
        err = "unsupported annotation argument: " + arg.name;
        return false;
#else
        G_UNUSED(err);
        logerr("unsupported annotation argument: %s", arg.name.c_str());
#endif
      }
    return true;
  }
  bool verifyArguments(const das::vector<das::VariablePtr> &arguments, das::string &err)
  {
    G_UNUSED(arguments);
#if DAGOR_DBGLEVEL > 0
    for (const auto &arg : arguments)
    {
      if (!arg->aka.empty() && arg->aka.find(arg->name) == das::string::npos)
        err.sprintf("argument alias '%s' should contain argument name '%s', for example 'hero_%s'\n", arg->aka.c_str(),
          arg->name.c_str(), arg->name.c_str());
    }
#endif
    return err.empty();
  };
  virtual bool apply(das::ExprBlock *block, das::ModuleGroup &, const das::AnnotationArgumentList &args, das::string &err) override
  {
    for (const das::VariablePtr &arg : block->arguments)
      if (arg->type->isTag)
      {
        // We have unresolved tag argument ($a) here,
        // so any verification is invalid
        return true;
      }

    if (!verifyArguments(block->arguments, err))
      return false;
    if (block->arguments.empty() && args.find("REQUIRE", das::Type::tString) == nullptr &&
        args.find("REQUIRE_NOT", das::Type::tString) == nullptr)
    {
      err = "block needs arguments or require/require_not annotations";
      return false;
    }
    if (!checkArgumentNames(args, EsFunctionKind::Query, err))
      return false;
    // to be uncommented when function can start skipping block interface and would generate lambda access directly
    // block->aotSkipMakeBlock = true;
    // block->aotDoNotSkipAnnotationData = true;

    for (auto argI = block->arguments.begin(), argE = block->arguments.end(); argI != argE; ++argI)
    {
      if (!(*argI)->type->constant && (*argI)->name == "eid" && (*argI)->type->isHandle() &&
          (*argI)->type->annotation->module->name == "ecs" && (*argI)->type->annotation->name == "EntityId")
      {
        err.append_sprintf("arg <%s> of type <%s> shouldn't be RW value, make this argument constant", (*argI)->name.c_str(),
          (*argI)->type->getMangledName().c_str());
        return false;
      }
      if ((*argI)->type->isPointer() && (*argI)->type->isConst() && !(*argI)->type->firstType->isConst())
      {
        err.append_sprintf("%s: Argument '%s : %s' which is constant pointer should point to constant data."
                           "\nAdd const before question mark (for example 'int const?')",
          (*argI)->at.describe().c_str(), (*argI)->name.c_str(), (*argI)->type->describe().c_str());
        return false;
      }
    }

#if DAGOR_DBGLEVEL > 0
    for (auto argI = block->arguments.begin(), argE = block->arguments.end(); argI != argE; ++argI)
    {
      if (!(*argI)->type->constant && !(*argI)->type->isRefOrPointer())
        logerr("%s: block arg <%s> of type <%s> is var (RWR) but it's change won't affected the actual data. Probably you forgot &",
          block->at.describe().c_str(), (*argI)->name.c_str(), (*argI)->type->getMangledName().c_str());
    }
#endif
    return true;
  }
  virtual bool apply(const das::FunctionPtr &func, das::ModuleGroup &mg_, const das::AnnotationArgumentList &args,
    das::string &err) override
  {
    for (const das::VariablePtr &arg : func->arguments)
      if (arg->type->isTag)
      {
        // We have unresolved tag argument ($a) here,
        // so any verification is invalid
        return true;
      }

    if (!verifyArguments(func->arguments, err))
      return false;
    ESModuleGroupData &mg = *getGroupData(mg_); // -V522
    if (func->arguments.empty())
    {
      err = "es function needs arguments";
      return false;
    }
    if (!checkArgumentNames(args, EsFunctionKind::System, err))
      return false;
    das::lock_guard<das::recursive_mutex> guard(bind_dascript::DasScripts<LoadedScript, EsContext>::mutex);
    if (!das::is_in_completion())
    {
      ecs::EntitySystemDesc *desc =
        ecs::find_if_systems([&func](ecs::EntitySystemDesc *desc) { return strcmp(desc->name, func->name.c_str()) == 0; });
      if (desc)
      {
        if (!entity_system_is_das(desc) || ((EsDesc *)desc->getUserData())->hashedScriptName != mg.hashedScriptName)
        {
          err = "es <" + func->name + "> is already registered in other file";
          if (desc->getModuleName())
            err.append_sprintf(" <%s>", desc->getModuleName());
          return false;
        }
      }
    }
    das::Type dasType;
    das::string stageName;
    if ((!get_underlying_ecs_type(*func->arguments[0]->type, stageName, dasType)) || !func->arguments[0]->type->isRef() ||
        !valid_es_param_name(*func->arguments[0]->type, stageName))
    {
      if (!get_underlying_ecs_type(*func->arguments[0]->type, stageName, dasType))
        err = "first argument has to be _reference_ to UpdateStageInfoAct, UpdateStageInfoRenderDebug or Event! and type is unknown";
      else if (!func->arguments[0]->type->isRef())
        err = "first argument has to be _reference_ to UpdateStageInfoAct, UpdateStageInfoRenderDebug or Event! it is" + stageName;
      else
        err = "first argument has to be UpdateStageInfoAct, UpdateStageInfoRenderDebug or Event, and it is " + stageName;
      return false;
    }
    else
    {
      // We allow to pass no const events to gather some data from event listeners
      func->arguments[0]->type->constant = valid_stage_name(stageName);
      func->arguments[0]->marked_used = 1; // mark first (evt|info) as used to avoid "unused function argument" error
    }
    uint32_t start_arg = 1;
    for (auto argI = func->arguments.begin() + start_arg, argE = func->arguments.end(); argI != argE; ++argI)
    {
      if (!(*argI)->type->constant && (*argI)->name == "eid" && (*argI)->type->isHandle() &&
          (*argI)->type->annotation->module->name == "ecs" && (*argI)->type->annotation->name == "EntityId")
      {
        err.append_sprintf("arg <%s> of type <%s> shouldn't be RW value, make this argument constant", (*argI)->name.c_str(),
          (*argI)->type->getMangledName().c_str());
        return false;
      }
      if (!(*argI)->type->constant && get_underlying_ecs_type(*(*argI)->type, stageName, dasType) && dasType == das::tString)
      {
        debug("%s: string args are currently RO, use das_string type if you need rw", func->name.c_str());
        (*argI)->type->constant = 1;
      }
      if ((*argI)->type->isPointer() && (*argI)->type->isConst() && !(*argI)->type->firstType->isConst())
      {
        logerr("%s: Argument '%s : %s' which is constant pointer should point to constant data."
               "\nAdd const before question mark (for example 'int const?')",
          (*argI)->at.describe(), (*argI)->name.c_str(), (*argI)->type->describe().c_str());
        (*argI)->type->firstType->constant = true;
      }
#if DAGOR_DBGLEVEL > 0
      if (!(*argI)->type->constant && !(*argI)->type->isRefOrPointer())
        logerr("%s: arg <%s> of type <%s> is var (RW) but it's change won't affected the actual data. Probably you forgot &",
          func->name.c_str(), (*argI)->name.c_str(), (*argI)->type->getMangledName().c_str());
#endif
    }
    func->exports = true;

#if ES_TO_QUERY
    get_underlying_ecs_type(*func->arguments[0]->type, stageName, dasType);
    das::string typeName;
    const bool hasManager = func->arguments.size() > start_arg && is_entity_manager(*func->arguments[start_arg]->type);
    // if (valid_stage_name(stageName))
    if (func->arguments.size() > (hasManager ? 2 : 1)) //!!
    {
      // we could transforming function into query-call here
      // def funName(info:ecs::UpdateStageInfoAct) // note, no more arguments
      //    ecs::query() <| $( clone of function arguments, but first )
      //        clone of function body
      // but that would be at least one call on everything, and is not suitable for Event

      // better way
      //  def funName(info:ecs::UpdateStageInfoAct; var qv:QueryView) // note, no more arguments
      //     ecs::process_view(qv) <| $( clone of function arguments, but first )
      //         clone of function body
      auto cle = das::make_smart<das::ExprCall>(func->at, "ecs::process_view");
      cle->generated = true;
      auto blk = das::make_smart<das::ExprBlock>();
      blk->at = func->at;
      blk->isClosure = true;
      bool first = true;
      das::TextWriter tw;
      for (auto &arg : func->arguments)
      {
        if (!first)
        {
          auto carg = arg->clone();
          blk->arguments.push_back(carg);
          if (carg->init)
            tw << *carg->init;
        }
        else
        {
          first = false;
        }
      }
      blk->returnType = das::make_smart<das::TypeDecl>(das::Type::tVoid);
      auto ann = das::make_smart<das::AnnotationDeclaration>();
      ann->annotation = this;
      ann->arguments = args;
      ann->arguments.push_back(das::AnnotationArgument("fn_name", func->name));
      blk->annotations.push_back(ann);
      blk->list.push_back(func->body->clone());
      // to be uncommented when function can start skipping block interface and would generate lambda access directly
      // blk->aotSkipMakeBlock = true;

      auto mkb = das::make_smart<das::ExprMakeBlock>(func->at, blk);
      mkb->generated = true;
      auto vinfo = das::make_smart<das::ExprVar>(func->at, "qv");
      vinfo->generated = true;

      cle->arguments.push_back(vinfo);
      cle->arguments.push_back(mkb);
      cle->arguments.push_back(das::make_smart<das::ExprConstUInt64>(das::hash_blockz64((const uint8_t *)tw.str().c_str())));
      auto cleb = das::make_smart<das::ExprBlock>();
      cleb->generated = true;
      cleb->at = func->body ? func->body->at : func->at;
      cleb->list.push_back(cle);
      func->body = cleb;
      func->arguments.resize(1);

      auto pVar = das::make_smart<das::Variable>();
      pVar->generated = true;
      pVar->at = func->at;
      pVar->name = "qv";
      pVar->type = mg_.makeHandleType("ecs::QueryView");
      pVar->type->constant = false;
      pVar->type->removeConstant = true;

      func->arguments.push_back(pVar);
    }
#endif
    return true;
  };

  virtual bool finalize(const das::FunctionPtr &func, das::ModuleGroup &mg_, const das::AnnotationArgumentList &args,
    const das::AnnotationArgumentList &progArgs, das::string &err) override
  {
    if (das::is_in_completion())
      return true;
    das::lock_guard<das::recursive_mutex> guard(bind_dascript::DasScripts<LoadedScript, EsContext>::mutex);
    ESModuleGroupData &mg = *getGroupData(mg_); // -V522
    EsDescUP esDesc(new (esDescsAllocator.allocateOneBlock(), _NEW_INPLACE) EsDesc());
    esDesc->hashedScriptName = mg.hashedScriptName;
    const das::vector<das::VariablePtr> *arguments_ = &func->arguments;
    int startId = 1;

    const bool hasManager = func->arguments.size() > 1 && is_entity_manager(*func->arguments[1]->type);
#if ES_TO_QUERY
    if (func->arguments.size() > (hasManager ? 2 : 1))
    {
      G_ASSERT(func->body->rtti_isBlock());
      auto cleb = das::static_pointer_cast<das::ExprBlock>(func->body);

      G_ASSERT(cleb->list[0]->rtti_isCall());
      auto cle = das::static_pointer_cast<das::ExprCall>(cleb->list[0]);

      G_ASSERT(cle->arguments[1]->rtti_isMakeBlock());
      auto mkBlock = das::static_pointer_cast<das::ExprMakeBlock>(cle->arguments[1]);

      G_ASSERT(mkBlock->block->rtti_isBlock());
      auto block = das::static_pointer_cast<das::ExprBlock>(mkBlock->block);
      arguments_ = &block->arguments;
      startId = 0;
    }
#endif
    const das::vector<das::VariablePtr> &arguments = *arguments_;

    if (!build_es(esDesc->base, func->name.c_str(), arguments, args, startId, err, progArgs.getBoolOption("can_trust_access", true),
          mg.argStrings))
    {
      err.append_sprintf("can't add es <%s>", func->name.c_str());
      return false;
    }
    events_from_list(args, "on_event", esDesc->evtMask);
    if (args.getBoolOption("on_appear"))
    {
      esDesc->evtMask.insert(ecs::EventEntityCreated::staticType());
      esDesc->evtMask.insert(ecs::EventComponentsAppear::staticType());
    }
    if (args.getBoolOption("on_disappear"))
    {
      esDesc->evtMask.insert(ecs::EventEntityDestroyed::staticType());
      esDesc->evtMask.insert(ecs::EventComponentsDisappear::staticType());
    }
    tags_from_list(args, "tag", esDesc->tagsList);
    tags_from_list(args, "before", esDesc->beforeList);
    tags_from_list(args, "after", esDesc->afterList);
    if (args.getBoolOption("no_order", false))
    {
      if (esDesc->beforeList.length() > 0 || esDesc->afterList.length() > 0)
      {
        err.append_sprintf("system has both before/after and no_order arg list. It can't be not ordered and ordered in same time",
          func->name.c_str());
        return false;
      }
      esDesc->beforeList = "*"; // special ignored order
      esDesc->afterList = "";
    }
    esDesc->minQuant = args.getIntOption("parallel_for", 0);

    tags_from_list(args, "track", esDesc->trackedList);
    das::Type dasType;
    das::string stageName;
    get_underlying_ecs_type(*func->arguments[0]->type, stageName, dasType, /*module_name*/ false);
    G_ASSERT(valid_es_param_name(*func->arguments[0]->type, stageName));
    if (valid_stage_name(stageName))
    {
      if (stageName == "ecs::UpdateStageInfoRenderDebug")
        esDesc->stageMask = (1 << ecs::US_RENDER_DEBUG);
      else
      {
        G_ASSERTF(stageName == "ecs::UpdateStageInfoAct", "Undefined stageMask for <%@>", stageName.c_str());
        esDesc->stageMask = (1 << ecs::US_ACT);
      }
      if (esDesc->trackedList.length() > 1)
      {
        logerr("System <%s> is tracking <%s> component changes, while is Update System!", func->name.c_str(),
          esDesc->trackedList.c_str());
        esDesc->trackedList.clear();
      }
    }
    else
    {
      constexpr ecs::event_type_t changeEventType = ecs::EventComponentChanged::staticType();
      if (esDesc->trackedList.length() <= 1 && esDesc->evtMask.count(changeEventType))
      {
        logerr("es event handler <%s> has no tracked components list, and can't subscribe for %s", func->name.c_str(),
          ecs::EventComponentChanged::staticName());
        // return false;//we should return false, but there are many of such ES now
      }
      if (generic_event_name(stageName))
      {
        if (esDesc->trackedList.empty() && esDesc->evtMask.empty())
        {
          err.append_sprintf("can't add generic es event handler <%s> as it misses any on_event annotation", func->name.c_str());
          return false;
        }
      }
      else if (!esDesc->evtMask.empty() || !esDesc->trackedList.empty())
      {
        err.append_sprintf("can't add typed es event handler <%s> with on_event/track annotation", func->name.c_str());
        return false;
      }
      else if (!func->arguments[0]->type->isStructure() || !func->arguments[0]->type->structType->annotations.size())
      {
        err.append_sprintf("%s: Only Event, or struct annotated as [event] or [cpp_event] can now be handled as event, and not <%s>",
          func->name.c_str(),
          func->arguments[0]->type->describe(das::TypeDecl::DescribeExtra::no, das::TypeDecl::DescribeContracts::no).c_str());
        return false;
      }
      else
      {
        eastl::string str;
        bool dasEvent = true;
        auto &structAnnotations = func->arguments[0]->type->structType->annotations;
        if (structAnnotations.end() != eastl::find_if(structAnnotations.begin(), structAnnotations.end(),
                                         [](auto &i) { return i->annotation->describe() == "event"; }))
          str = (stageName.length() < 2 || stageName[0] != ':') ? stageName : &stageName[2];
        else
        {
          auto cppEventIt = eastl::find_if(structAnnotations.begin(), structAnnotations.end(),
            [](auto &i) { return i->annotation->describe() == "cpp_event"; });
          if (structAnnotations.end() == cppEventIt)
          {
            err.append_sprintf("%s: Only struct annotated as [event] or [cpp_event] can now be valid event.", func->name.c_str());
            return false;
          }
          else
          {
            auto cppNameIt = (*cppEventIt)->arguments.find("name", das::Type::tString);
            if (cppNameIt)
              str = cppNameIt->sValue;
            else
            {
              das::Type dasType;
              get_underlying_ecs_type(*func->arguments[0]->type, str, dasType, false);
            }
          }
          dasEvent = false;
        }
        esDesc->evtMask.insert(ecs::event_type_t(ECS_HASH_SLOW(str.c_str()).hash));
        esDesc->typeInfo = dasEvent && mg.helper ? mg.helper->makeTypeInfo(nullptr, func->arguments[0]->type) : nullptr;
      }
    }
    mg.unresolvedEs.emplace_back(eastl::move(esDesc), func->name);
    return true;
  }
  virtual bool finalize(das::ExprBlock *block, das::ModuleGroup &mg_, const das::AnnotationArgumentList &args,
    const das::AnnotationArgumentList &progArgs, das::string &err) override
  {
    if (das::is_in_completion())
      return true;
    das::lock_guard<das::recursive_mutex> guard(bind_dascript::DasScripts<LoadedScript, EsContext>::mutex);
    ESModuleGroupData &mg = *getGroupData(mg_); // -V522
    das::string blockName;
    blockName.append_sprintf("query_%s_l%d", block->at.fileInfo->name.c_str(), block->at.line);
    EsQueryDescUP desc(new (esQueryDescsAllocator.allocateOneBlock(), _NEW_INPLACE) EsQueryDesc());
    if (!build_es(desc->base, blockName.c_str(), block->arguments, args, 0, err, progArgs.getBoolOption("can_trust_access", true),
          mg.argStrings))
    {
      err.append_sprintf("can't add es query <%s>", blockName.c_str());
      return false;
    }
    auto mangledName = block->getMangledName(true, true);
    for (auto &ann : block->annotations)
      mangledName += ann->getMangledName();
    G_ASSERTF(block->inFunction, "query not called from function");
    mangledName += block->inFunction->getMangledName();

    block->annotationDataSid = das::hash_blockz64((uint8_t *)mangledName.c_str());
    block->annotationData = uintptr_t(desc.get()); // todo: add indirection to control pointers
    auto program = das::daScriptEnvironment::bound->g_Program;
    const bool promoteToBuiltin = program->promoteToBuiltin;
    desc->shared = promoteToBuiltin;
    mg.unresolvedQueries.emplace_back(QueryData{blockName, eastl::move(desc)});
    return true;
  }
};


void LoadedScript::unload()
{
  if (!systems.empty())
  {
    auto systemsCopy = eastl::move(systems);
    for (auto system : systemsCopy)
    {
      remove_system_from_list(system);
      if (system->getUserData())
        system->freeIfDynamic();
    }
    if (!das_is_in_init_phase && g_entity_mgr)
      g_entity_mgr->resetEsOrder();
  }
  queries.clear();

  bind_dascript::DasLoadedScript<EsContext>::unload();
}

static bool contains_all_tags(const ecs::TagsSet &src, const ecs::TagsSet &filter_data)
{
  for (unsigned tag : src)
    if (filter_data.find(tag) == filter_data.end())
      return false;
  return true;
}

static struct Scripts final : public bind_dascript::DasScripts<LoadedScript, EsContext>
{
  using Super = bind_dascript::DasScripts<LoadedScript, EsContext>;

  Cont<das::shared_ptr<das::DebugInfoHelper>> helper;
  Cont<das::shared_ptr<DebugArgStrings>> argStrings;

  eastl::vector<QueryData> sharedQueries;

  // thread safe data
  DasEcsStatistics statistics;

  void done() override
  {
    Super::done();
    helper.done();
    argStrings.done();
  }

  virtual void processModuleGroupUserData(const das::string &fname, das::ModuleGroup &libGroup) override
  {
    if (!helper.value)
      helper.value = das::make_shared<das::DebugInfoHelper>();
    if (!argStrings.value)
      argStrings.value = das::make_shared<DebugArgStrings>();

    ESModuleGroupData *mgd = new ESModuleGroupData(*argStrings.value.get(), g_entity_mgr.get());
    mgd->hashedScriptName = ECS_HASH_SLOW(fname.c_str()).hash;
    mgd->helper = helper.value.get();
    libGroup.setUserData(mgd);
  }

  int getPendingSystemsNum(das::ModuleGroup &group) const override
  {
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    return (int)mgd->unresolvedEs.size();
  }

  int getPendingQueriesNum(das::ModuleGroup &group) const override
  {
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    return (int)mgd->unresolvedQueries.size();
  }

  void storeSharedQueries(das::ModuleGroup &group) override
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    for (auto *it = mgd->unresolvedQueries.begin(); it != mgd->unresolvedQueries.end();)
    {
      if ((*it).desc->shared)
      {
        sharedQueries.emplace_back(eastl::move(*it));
        it = mgd->unresolvedQueries.erase(it);
      }
      else
        ++it;
    }
  }
  bool postProcessModuleGroupUserData(const das::string &fname, das::ModuleGroup &group) override
  {
    if (!g_entity_mgr) // aot compiler doesn't init g_entity_mgr
      return true;
    const ecs::TagsSet &filterTags = g_entity_mgr->getTemplateDB().info().filterTags;
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
    if (mgd->templates.empty())
      return true;
    ecs::TemplateRefs trefs(*g_entity_mgr);
    trefs.addComponentNames(mgd->trefs);
    for (const CreatingTemplate &templateData : mgd->templates)
    {
      if (templateData.filterTags.empty() || contains_all_tags(templateData.filterTags, filterTags))
      {
        ecs::ComponentsMap comps;
        ecs::Template::component_set tracked = templateData.tracked;
        ecs::Template::component_set replicated = templateData.replicated;
        ecs::Template::component_set ignored = templateData.ignored;
        for (const auto &comp : templateData.map)
        {
          auto compTags = templateData.compTags.find(comp.first);
          if (compTags == templateData.compTags.end() || contains_all_tags(compTags->second, filterTags))
          {
            comps[comp.first] = comp.second;
          }
          else
          {
            tracked.erase(comp.first);
            replicated.erase(comp.first);
            ignored.erase(comp.first);
          }
        }
        ecs::Template::ParentsList parents = templateData.parents;

        trefs.emplace(ecs::Template(templateData.name.c_str(), eastl::move(comps), eastl::move(tracked), eastl::move(replicated),
                        eastl::move(ignored), false),
          eastl::move(parents));
      }
    }
    if (g_entity_mgr && !g_entity_mgr->updateTemplates(trefs, true, ECS_HASH_SLOW(fname.c_str()).hash,
                          [&](const char *n, ecs::EntityManager::UpdateTemplateResult r) {
                            switch (r)
                            {
                              case ecs::EntityManager::UpdateTemplateResult::InvalidName: logerr("%s wrong name\n", n); break;
                              case ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities:
                                logerr("%s has entities and cant be removed\n", n);
                                break;
                              case ecs::EntityManager::UpdateTemplateResult::DifferentTag:
                                logerr("%s is registered with different tag and can't be updated\n", n);
                                break;
                              default: break;
                            }
                          }))
      return false;
    return true;
  }

  virtual bool keepModuleGroupUserData(const das::string &fname, const das::ModuleGroup &group) const
  {
    G_UNUSED(fname);
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
    return !mgd->templates.empty();
  }

  virtual bool processLoadedScript(LoadedScript &script, const das::string &fname, uint64_t load_start_time, das::ModuleGroup &group,
    AotMode aot_mode, AotModeIsRequired aot_mode_is_required) override
  {
    if (script.postProcessed)
      return true;
    script.postProcessed = true;
    ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
    G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
    G_UNUSED(fname);

    storeSharedQueries(group);
    script.queries = eastl::move(mgd->unresolvedQueries);

    if (g_entity_mgr)
    {
      for (QueryData &queryData : script.queries)
      {
        queryData.desc->base.resolveUnresolved(); // try to resolve asap
        queryData.desc->query = g_entity_mgr->createQuery(ecs::NamedQueryDesc(queryData.name.c_str(),
          queryData.desc->base.getSlice(BaseEsDesc::RW_END), queryData.desc->base.getSlice(BaseEsDesc::RO_END),
          queryData.desc->base.getSlice(BaseEsDesc::RQ_END), queryData.desc->base.getSlice(BaseEsDesc::NO_END)));
      }

      for (QueryData &queryData : sharedQueries)
      {
        if (ecs::ecs_query_handle_t(queryData.desc->query) == ecs::INVALID_QUERY_HANDLE_VAL)
        {
          queryData.desc->base.resolveUnresolved(); // try to resolve asap
          queryData.desc->query = g_entity_mgr->createQuery(ecs::NamedQueryDesc(queryData.name.c_str(),
            queryData.desc->base.getSlice(BaseEsDesc::RW_END), queryData.desc->base.getSlice(BaseEsDesc::RO_END),
            queryData.desc->base.getSlice(BaseEsDesc::RQ_END), queryData.desc->base.getSlice(BaseEsDesc::NO_END)));
        }
      }
    }

    const char *debugFn = nullptr;
#if DAGOR_DBGLEVEL > 0
    if (script.fn.empty())
    {
      script.fn = fname.c_str();
      debugFn = script.fn.c_str();
    }
#endif
    mgd->es_resolve_function_ptrs(script.ctx.get(), script.systems, debugFn, load_start_time, aot_mode, aot_mode_is_required,
      statistics);

    const bool hasMtSystems = eastl::find_if(script.systems.begin(), script.systems.end(),
                                [](ecs::EntitySystemDesc *desc) { return desc->getQuant() > 0; }) != script.systems.end();

    if (hasMtSystems)
    {
      for (int idx = 0, n = script.ctx->getTotalVariables(); idx < n; ++idx)
      {
        const das::VarInfo *globalVar = script.ctx->getVariableInfo(idx);
        if (globalVar && !globalVar->isConst())
        {
          logerr("daScript: Global variable '%s' in script <%s>."
                 "This script contains multithreaded systems and shouldn't have global variables",
            globalVar->name, fname.c_str());
          return false;
        }
      }
    }

    if (!script.ctx->persistent)
    {
      const uint64_t heapBytes = script.ctx->heap->bytesAllocated();
      const uint64_t stringHeapBytes = script.ctx->stringHeap->bytesAllocated();
      if (heapBytes > 0)
        logerr("daScript: script <%s> allocated %@ bytes for global variables", fname.c_str(), heapBytes);
      if (stringHeapBytes > 0)
      {
        das::string strings = "";
        script.ctx->stringHeap->forEachString([&](const char *str) {
          if (strings.length() < 250)
            strings.append_sprintf("%s\"%s\"", strings.empty() ? "" : ", ", str);
        });
        logerr("daScript: script <%s> allocated %@ bytes for global strings. Allocated strings: %s", fname.c_str(), stringHeapBytes,
          strings.c_str());
      }
      if (heapBytes > 0 || stringHeapBytes > 0)
        script.ctx->useGlobalVariablesMem();
    }
    return true;
  }

  void unloadAllScripts(UnloadDebugAgents unload_debug_agents) override
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    Super::unloadAllScripts(unload_debug_agents);
    sharedQueries.clear();

    helper.value.reset();
    helper.clear();

    argStrings.value.reset();
    argStrings.clear();
  }
  size_t dumpMemory()
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    size_t globals = 0, stack = 0, code = 0, codeTotal = 0, debugInfo = 0, debugInfoTotal = 0, constStrings = 0, constStringsTotal = 0,
           heap = 0, heapTotal = 0, string = 0, stringTotal = 0, shared = 0, unique = 0;
    for (const auto &sc : scripts)
    {
      auto &s = sc.second;
      // globals += s.ctx->globalsSize;//currently is protected!
      globals = 0;
      stack += s.ctx->stack.size();
      code += s.ctx->code->bytesAllocated();
      codeTotal += s.ctx->code->totalAlignedMemoryAllocated();
      constStrings += s.ctx->constStringHeap->bytesAllocated();
      constStringsTotal += s.ctx->constStringHeap->totalAlignedMemoryAllocated();
      debugInfo += s.ctx->debugInfo->bytesAllocated();
      debugInfoTotal += s.ctx->debugInfo->totalAlignedMemoryAllocated();
      heap += s.ctx->heap->bytesAllocated();
      heapTotal += s.ctx->heap->totalAlignedMemoryAllocated();
      string += s.ctx->stringHeap->bytesAllocated();
      stringTotal += s.ctx->stringHeap->totalAlignedMemoryAllocated();
      shared += s.ctx->getSharedMemorySize();
      unique += s.ctx->getUniqueMemorySize();
    }
    size_t total = stack + globals + codeTotal + constStringsTotal + debugInfoTotal + heapTotal + stringTotal;
    size_t waste = constStringsTotal - constStrings + debugInfoTotal - debugInfo + heapTotal - heap + stringTotal - string;
    G_UNUSED(total);
    G_UNUSED(waste);
    debug("daScript: memory usage total = %d, waste=%d\n"
          "globals=%d, stack=%d, code=%d(%d), constStr=%d(%d) debugInfo=%d(%d),heap=%d(%d),string=%d(%d),"
          "shared=%d, unique = %d",
      total, waste, globals, stack, code, codeTotal, constStrings, constStringsTotal, debugInfo, debugInfoTotal, heap, heapTotal,
      string, stringTotal, shared, unique);
    return total;
  }
} scripts;


static void das_es_desc_deleter(ecs::EntitySystemDesc *desc)
{
  G_ASSERT_RETURN(entity_system_is_das(desc), );
  EsDesc *esData = (EsDesc *)desc->getUserData();
  eastl::destroy_at(esData);
  esDescsAllocator.freeOneBlock(esData);
  desc->setUserData(nullptr);
  for (auto &s : scripts.scripts)
  {
    auto it = s.second.systems.find(desc);
    if (it != s.second.systems.end())
    {
      s.second.systems.erase(it);
      break;
    }
  }
}

static void unload_all_das_es()
{
  bool removed = false;
  ecs::remove_if_systems([&removed](ecs::EntitySystemDesc *system) {
    if (!entity_system_is_das(system))
      return false;
    removed = true;
    return true;
  });
  if (removed && g_entity_mgr)
    g_entity_mgr->resetEsOrder();
}

void shutdown_systems()
{
  unload_all_das_es(); // we do that to reduce amount of reset_es_order
  scripts.unloadAllScripts(UnloadDebugAgents::YES);
  G_VERIFY(esDescsAllocator.clear() == 0);
  G_VERIFY(esQueryDescsAllocator.clear() == 0);
  das::Module::Shutdown();


#if DAS_SMART_PTR_TRACKER
  if (das::g_smart_ptr_total > 0)
    debug("smart pointers leaked: %@", uint64_t(das::g_smart_ptr_total));

#if DAS_SMART_PTR_ID
  debug("leaked ids:");
  for (auto it : das::ptr_ref_count::ref_count_ids)
    debug("0x%X", it);
#endif
#endif

  if (debuggerEnvironment)
  {
    RAIIDasTempEnv env(debuggerEnvironment, debuggerEnvironment);
    das::Module::Shutdown();
  }
}

void collect_heap(float mem_scale_threshold, ValidateDasGC validate)
{
  if (globally_loading_in_queue)
    return;

  TIME_PROFILE(collect_das_garbage);

  G_ASSERT(mem_scale_threshold > 0.);
  das::unique_lock<das::recursive_mutex> lock(scripts.mutex, std::defer_lock);
  if (!lock.try_lock()) // in case of hot-reload
    return;
  const int now = get_time_msec();
  for (auto &[path, data] : scripts.scriptsMemory)
  {
    if (data.nextGcMsec > now)
      continue;
    const auto scriptIt = scripts.scripts.find(path);
    if (scriptIt == scripts.scripts.end())
    {
      logerr("das: can't find script '%s' to perform gc", path.c_str());
      continue;
    }
    const LoadedScript &script = scriptIt->second;
    const uint64_t heapSize = script.ctx->heap->bytesAllocated();
    const uint64_t stringsHeapSize = script.ctx->stringHeap->bytesAllocated();
    const float heapThreshold = data.heapSizeThreshold;
    const float stringsHeapThreshold = data.stringHeapSizeThreshold;
    bool collectHeap = (float)heapSize > heapThreshold;
    const bool collectStrings = (float)stringsHeapSize > stringsHeapThreshold;
    if (collectHeap || collectStrings)
    {
      if (collectHeap)
        debug("%s: heap size reached threshold %ld / %ld", path.c_str(), heapSize, heapThreshold);
      if (collectStrings)
        debug("%s: strings heap size reached threshold %ld / %ld", path.c_str(), stringsHeapSize, stringsHeapThreshold);
      // script.ctx->heap->report();
      // script.ctx->stringHeap->report();
      const int64_t ref = profile_ref_ticks();
      script.ctx->collectHeap(nullptr, collectStrings, validate == ValidateDasGC::YES);

      const uint64_t newHeapSize = script.ctx->heap->bytesAllocated();
      const uint64_t newStringsHeapSize = script.ctx->stringHeap->bytesAllocated();

      debug("%s: gc executed in %d ms, heap size: %d, strings heap: %d", path.c_str(), profile_time_usec(ref) / 1000.f, newHeapSize,
        newStringsHeapSize);

      // we always collect heap, because we can't collect only strings heap
      data.heapSizeThreshold =
        eastl::max(eastl::max((uint64_t)script.ctx->heap->getInitialSize(), newHeapSize), heapSize * mem_scale_threshold);
      if (collectStrings)
        data.stringHeapSizeThreshold = eastl::max(eastl::max((uint64_t)script.ctx->stringHeap->getInitialSize(), newStringsHeapSize),
          stringsHeapSize * mem_scale_threshold);
      data.nextGcMsec = now + 5000; // + 5sec
    }
  }
}

bool load_das_script(const char *name, const char *program_text)
{
  auto access = das::make_smart<DagFileAccess>(scripts.getFileAccess(), globally_hot_reload);
  das::string fname = das::string("inplace:") + name;
  access->setFileInfo(fname, das::make_unique<das::TextFileInfo>(program_text, strlen(program_text), false));
  return scripts.loadScript(fname, access, globally_aot_mode, globally_resolve_ecs_on_load, globally_log_aot_errors);
}

static inline bool internal_load_das_script_sync(const char *fname, ResolveECS resolve_ecs)
{
  String tmpPath;
  return scripts.loadScript(dd_resolve_named_mount(tmpPath, fname) ? tmpPath.c_str() : fname,
    das::make_smart<DagFileAccess>(scripts.getFileAccess(), globally_hot_reload), globally_aot_mode, resolve_ecs,
    globally_log_aot_errors);
}

bool load_das_script(const char *fname)
{
  if (globally_loading_in_queue)
  {
    loadingQueue.push_back(fname);
#if DAGOR_DBGLEVEL > 0
    const uint32_t pathHash = ecs_str_hash(fname);
    if (loadingQueueHash.find(pathHash) != loadingQueueHash.end())
      logerr("das: file '%s' was loaded multiple times, probably load_folder() was called multiple times with same arguments", fname);
    else
      loadingQueueHash.insert(pathHash);
#endif
    return true;
  }

  return internal_load_das_script_sync(fname, globally_resolve_ecs_on_load);
}

bool load_das_script_debugger(const char *fname)
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
  RAIIDasTempEnv env(debuggerEnvironment, debuggerEnvironment);
  if (!debuggerEnvironment)
  {
    bind_dascript::pull_das();
    require_project_specific_debugger_modules();
  }
  const bool ok = internal_load_das_script_sync(fname, ResolveECS::YES);
  debuggerEnvironment = das::daScriptEnvironment::owned;
  return ok;
#else
  G_UNUSED(fname);
  return true; // just ignore
#endif
}

bool load_das_script_with_debugcode(const char *fname)
{
  const bool wasLoadDebugCode = scripts.loadDebugCode;
  scripts.loadDebugCode = true;
  const bool ok = internal_load_das_script_sync(fname, globally_resolve_ecs_on_load);
  scripts.loadDebugCode = wasLoadDebugCode;
  return ok;
}

struct ScopedMultipleScripts
{
  ScopedMultipleScripts() { start_multiple_scripts_loading(); }
  ~ScopedMultipleScripts() { end_multiple_scripts_loading(); }
  EA_NON_COPYABLE(ScopedMultipleScripts);
};

#if DAGOR_DBGLEVEL > 0
bool reload_das_debug_script(const char *fname, bool debug)
{
  const auto script = scripts.scripts.find_as(fname);
  if (script == scripts.scripts.end())
  {
    ::debug("dap: %s was not loaded, ignore this file", fname);
    return true; // it's ok, we just ignore files that were not loaded
  }
  if (script != scripts.scripts.end() && debug == script->second.hasDebugger)
  {
    ::debug("dap: %s already was loaded %s debugger", fname, debug ? "with" : "without");
    return true;
  }
  ScopedMultipleScripts scope;
  return scripts.loadScript(fname, das::make_smart<DagFileAccess>(scripts.getFileAccess(), globally_hot_reload), globally_aot_mode,
    ResolveECS::YES, globally_log_aot_errors, debug ? EnableDebugger::YES : EnableDebugger::NO);
}
bool find_loaded_das_script(const eastl::function<bool(const char *, das::Context &, das::smart_ptr<das::Program>)> &cb)
{
  for (auto &script : scripts.scripts)
    if (cb(script.first.c_str(), *script.second.ctx, script.second.program))
      return true;
  return false;
}
#endif

struct DascriptLoadJob final : public DaThread
{
  TInitDas init;
  volatile int &shared_counter;
  das::vector<das::string> &fileNames;
  das::daScriptEnvironment *environment = nullptr;
  bool enableStackFill = true;
  int count = 0;
  bool success = true;

  DascriptLoadJob(TInitDas init_, volatile int &cnt, das::vector<das::string> &file_names, bool enable_stack_fill) :
    DaThread("DasLoadThread", 256 << 10, 0, WORKER_THREADS_AFFINITY_MASK),
    init(init_),
    shared_counter(cnt),
    fileNames(file_names),
    enableStackFill(enable_stack_fill)
  {}

  void do_work(Scripts &local_scripts, das::vector<das::string> &file_names)
  {
    const bool wasStackFill = DagDbgMem::enable_stack_fill(enableStackFill);
    das::daScriptEnvironment::bound->g_resolve_annotations = false;
    if (!globally_thread_init_script.empty())
    {
      auto file_access = das::make_smart<DagFileAccess>(local_scripts.getFileAccess(), globally_hot_reload);
      success = local_scripts.loadScript(globally_thread_init_script, file_access, globally_aot_mode, ResolveECS::NO,
                  globally_log_aot_errors) &&
                success;
    }
    while (true)
    {
      int i = interlocked_increment(shared_counter);
      if (i >= file_names.size())
        break;
      auto file_access = das::make_smart<DagFileAccess>(local_scripts.getFileAccess(), globally_hot_reload);
      success =
        local_scripts.loadScript(file_names[i], file_access, globally_aot_mode, ResolveECS::NO, globally_log_aot_errors) && success;
      count++;
    }
    das::daScriptEnvironment::bound->g_resolve_annotations = true;
    DagDbgMem::enable_stack_fill(wasStackFill);
  }

  void execute() override
  {
    debug("dascript: queue: doJob started...");
    const uint64_t startTime = profile_ref_ticks();
    das::ReuseCacheGuard guard;
    init();
    debug("dascript: queue: job init in %@ ms", profile_time_usec(startTime) / 1000);

    Scripts &localScripts = scripts;
    do_work(localScripts, fileNames);

    {
      das::lock_guard<das::recursive_mutex> guard(scripts.mutex);
      localScripts.done();
    }

    G_ASSERT(das::daScriptEnvironment::owned == das::daScriptEnvironment::bound);
    G_ASSERT(!!das::daScriptEnvironment::owned);
    environment = das::daScriptEnvironment::owned;

    das::daScriptEnvironment::owned = nullptr;
    das::daScriptEnvironment::bound = nullptr;
    debug("dascript: queue: job loaded ok? %@ in %@ ms %@ files", success, profile_time_usec(startTime) / 1000, count);
  }
};

bool fill_stack_while_compile_das() { return !(bool)::dgs_get_argv("das-no-stack-fill", DAGOR_DBGLEVEL > 0 ? "y" : nullptr); }

static bool load_scripts_from_serialized_data()
{
  uint64_t size = 0;
  das::vector<das::string> filenames;
  bool ok = initDeserializer && initDeserializer->trySerialize([&](das::AstSerializer &ser) {
    ser << size;
    for (size_t i = 0; i < size; i++)
    {
      das::string name;
      ser << name;
      filenames.push_back(name);
    }
  });
  if (!ok)
  {
    logwarn("das: serialize: failed to read serialized data");
    return false;
  }
  for (const auto &name : filenames)
  {
    auto file_access = das::make_smart<DagFileAccess>(scripts.getFileAccess(), globally_hot_reload);
    ok = scripts.loadScript(name, file_access, globally_aot_mode, ResolveECS::NO, LogAotErrors::YES) && ok;
  }
  // clean up module memory
  for (auto &[name, script] : scripts.scripts)
  {
    if (enableSerialization && script.program && !script.program->options.getBoolOption("rtti", false) && !das::is_in_aot())
      script.program.reset();
    if (script.moduleGroup)
      script.moduleGroup->reset();
  }
  // collect created file infos that no module owns
  if (initDeserializer)
    initDeserializer->collectFileInfo(scripts.orphanFileInfos);
  enableSerialization = false;
  loadingQueue.clear(); // clear the queue
#if DAGOR_DBGLEVEL > 0
  loadingQueueHash.clear();
#endif
  return ok;
}

bool stop_loading_queue(TInitDas init)
{
  globally_loading_in_queue = false;

  if (enableSerialization && serializationReading)
  {
    const bool loaded = load_scripts_from_serialized_data();
    if (loaded)
    {
      debug("das: serialize: all scripts loaded from serialized data");
      if (bind_dascript::initSerializer)
        bind_dascript::initSerializer->failed = true; // disable serialization
      return loaded;
    }
    logwarn("das: serialize: failed to load scripts from serialized data. Fallback to regular loading");
    bind_dascript::serializationReading = false;
    bind_dascript::suppressSerialization = true;
  }
  else if (enableSerialization && !serializationReading)
    suppressSerialization = true; // do not write the data in serveral threads, it's written after compiling everything in execute()

  bool ok = true;
  das::vector<eastl::string> queue;
  loadingQueue.swap(queue);
  union
  {
    volatile int cnt = -1;
    char padding[128];
  } shcnt;
  const bool enableStackFill = fill_stack_while_compile_das();
  const int numJobs = clamp((int)queue.size(), 1, globally_load_threads_num);
  debug("dascript: queue: init %@ jobs", numJobs);
  dag::RelocatableFixedVector<DascriptLoadJob, 4, true> jobs;
  jobs.reserve(numJobs);
  for (int i = 0; i < numJobs; ++i)
  {
    DascriptLoadJob &job = jobs.emplace_back(init, shcnt.cnt, queue, enableStackFill);
    debug("dascript: queue: enqueue job %@", i);
    if (i != 0)
    {
      G_VERIFY(job.start());
    }
  }
  {
    das::ReuseCacheGuard guard;
    jobs[0].do_work(scripts, queue);
  }
  ok = jobs[0].success && ok;
  for (int i = 1, n = jobs.size(); i < n; ++i)
  {
    jobs[i].terminate(/*wait*/ true);
    ok = jobs[i].success && ok;
  }

  if (enableSerialization && !serializationReading && initSerializer)
  {
    uint64_t filesCount = 0;
    for (auto &fname : queue)
    {
      if (scripts.scripts[fname].program)
      {
        filesCount += 1;
      }
    }
    *initSerializer << filesCount;
    debug("das: serialize: Serializing %@ files", filesCount);
    for (auto &fname : queue)
    {
      auto &script = scripts.scripts[fname];
      if (script.program)
        *initSerializer << fname;
    }
    for (auto &fname : queue)
    {
      auto &script = scripts.scripts[fname];
      if (script.program)
      {
        debug("das: serialize: Serializing file %s", fname);
        initSerializer->serializeScript(script.program);
      }
    }
  }

  // clean up module memory
  for (auto &[fname, script] : scripts.scripts)
  {
    if (script.program)
    {
      if (!script.program->options.getBoolOption("rtti", false) && !das::is_in_aot())
        script.program.reset();
      script.moduleGroup->reset();
    }
  }

  {
    RAIIDasTempEnv env;
    for (int i = 1; i < jobs.size(); ++i)
    {
      das::daScriptEnvironment *jobEnv = jobs[i].environment;
      das::daScriptEnvironment::owned = jobEnv;
      das::daScriptEnvironment::bound = jobEnv;
      das::Module::CollectFileInfo(scripts.orphanFileInfos);
      das::Module::Shutdown();
    }
  }

  G_ASSERT(!globally_loading_in_queue);
  G_ASSERTF(loadingQueue.empty(), "das: loading queue should be empty, but contains %d files, for example '%s'", loadingQueue.size(),
    loadingQueue.begin()->c_str());
  debug("daScript: loaded %d scripts in %d jobs", queue.size(), numJobs);

#if DAGOR_DBGLEVEL > 0
  loadingQueueHash.clear();
#endif
  return ok;
}

static bool internal_load_entry_script(const char *fname)
{
  const bool res = scripts.loadScript(fname, das::make_smart<DagFileAccess>(scripts.getFileAccess(), globally_hot_reload),
    globally_aot_mode, ResolveECS::NO, globally_log_aot_errors);
  return res;
}

static uint64_t entryScriptStartTime;
static size_t entryScriptMemUsed;

void begin_loading_queue()
{
  entryScriptStartTime = profile_ref_ticks();
  scripts.statistics = {}; // reset das load stats from previous load
  entryScriptMemUsed = dagor_memory_stat::get_memory_allocated(true);
  G_ASSERT(!globally_loading_in_queue && loadingQueue.empty());
  globally_loading_in_queue = true;
}

bool load_entry_script(const char *entry_point_name, TInitDas init, LoadEntryScriptCtx ctx)
{
  begin_loading_queue();
  bool res = internal_load_entry_script(entry_point_name) && stop_loading_queue(init);
  end_loading_queue(ctx);
  return res;
}

static auto get_serialization_scripts_version() -> das::vector<int64_t>
{
  Tab<SimpleString> list;

  bind_dascript::DagFileAccess access{bind_dascript::HotReload::DISABLED};
  if (const DataBlock *pathsBlk = ::dgs_get_settings()->getBlockByName("game_das_serialization_scan_paths"))
  {
    dag::Vector<eastl::string> scanFolders;
    for (int i = 0, n = pathsBlk->paramCount(); i < n; i++)
    {
      const char *path = pathsBlk->getStr(i);
      const char *dirPath = path;
      String tmp_dir_path;
      if (dd_resolve_named_mount(tmp_dir_path, dirPath))
        dirPath = tmp_dir_path;
      if (!dirPath || !*dirPath)
      {
        logerr(
          "das: serialize: empty mount point '%s' in 'game_das_serialization_scan_paths'. Please fix it, set correct, not empty path",
          path);
        continue;
      }
      if (eastl::find(scanFolders.begin(), scanFolders.end(), dirPath) == scanFolders.end())
        scanFolders.push_back(eastl::string(dirPath));
    }
    bool useVromsSrc = false;
#if DAGOR_DBGLEVEL > 0
    useVromsSrc = ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("useAddonVromSrc", false);
#endif
    size_t prevSize = list.size();
    for (auto &dirPath : scanFolders)
    {
      debug("das: serialize: search das files in '%s' ...", dirPath);
      uint64_t startTime = profile_ref_ticks();
      find_files_in_folder(list, dirPath.c_str(), ".das", /*vromfs*/ true, /*realfs*/ useVromsSrc, /*subdirs*/ true);
      debug("das: serialize: scan complete, found %d files in %@ ms", list.size() - prevSize, profile_time_usec(startTime) / 1000);
      prevSize = list.size();
    }
  }
  else
  {
    logerr(
      "setting 'game_das_serialization_scan_paths' is not set, please add this block and list all mount points with Daslang files,"
      " for example\ngame_das_serialization_scan_paths { it:t=\"%danetlibs\"; it:t=\"%scripts\" ... }");
  }

  das::vector<das::pair<das::string, int64_t>> mts;
  for (auto &i : list)
  {
    das::string s{i.c_str()};
    mts.emplace_back(s, access.getFileMtime(i.c_str()));
  }

  das::vector<int64_t> mt;
  mt.reserve(mts.size());

  eastl::sort(mts.begin(), mts.end());

  // Map into mtime
  eastl::transform(mts.begin(), mts.end(), eastl::back_inserter(mt), [](const auto &pair) { return pair.second; });

  return mt;
}

static bool compare_loading_queues()
{
  if (!initDeserializer)
    return false;
  das::vector<das::string> savedQueue;
  if (!bind_dascript::initDeserializer->trySerialize([&](das::AstSerializer &ser) { ser << savedQueue; }))
  {
    logwarn("das: serialize: failed to read serialized data (files)");
    return false;
  }

  bool queuesMatch = bind_dascript::loadingQueue == savedQueue;

  if (queuesMatch) // also check mtimes in this case
  {
    das::vector<int64_t> savedMtimes;
    if (!bind_dascript::initDeserializer->trySerialize([&](das::AstSerializer &ser) { ser << savedMtimes; }))
    {
      logwarn("das: serialize: failed to read serialized data (mtimes)");
      return false;
    }
    auto loadingMtimes = get_serialization_scripts_version();
    return savedMtimes == loadingMtimes;
  }

  return queuesMatch;
}

static void write_serializer_version()
{
  if (!bind_dascript::initSerializer)
    return;
  uint32_t ptrsize = sizeof(void *), protocolVersion = bind_dascript::initSerializer->getVersion();
  debug("das: serialize: Versions differ: init from text files, writeback the result. Game version: %lu, protocol version: %lu, "
        "ptrsize: %@",
    g_serializationVersion, protocolVersion, ptrsize);
  *bind_dascript::initSerializer << g_serializationVersion << protocolVersion << ptrsize;
  *bind_dascript::initSerializer << bind_dascript::loadingQueue;
  {
    auto v = get_serialization_scripts_version();
    *bind_dascript::initSerializer << v;
  }
}

static void set_up_serializer_versions(uint32_t serialized_version)
{
  bind_dascript::serializationReading = serialized_version != -1 && g_serializationVersion == serialized_version;
  if (bind_dascript::serializationReading)
  {
    bind_dascript::serializationReading = compare_loading_queues();
  }

  if (bind_dascript::serializationReading)
    debug("das: serialize: Versions match: init from binary data");
  else
    write_serializer_version();
}

bool load_entry_script_with_serialization(const char *entry_point_name, TInitDas init, LoadEntryScriptCtx ctx,
  const uint32_t serialized_version)
{
  bool res = false;
  start_multiple_scripts_loading();
  begin_loading_queue();
  if (internal_load_entry_script(entry_point_name))
  {
    set_up_serializer_versions(serialized_version);
    if (serializationReading)
    {
      if (initDeserializer)
        initDeserializer->getCompiledModules();
    }
    else
    {
      if (initSerializer)
        initSerializer->getCompiledModules();
    }
    enableSerialization = true;
    res = stop_loading_queue(init);
    enableSerialization = false;
  }
  end_loading_queue(ctx);

  drop_multiple_scripts_loading(); // unset das_is_in_init_phase, es_reset_order calls in the end of function
  return res;
}

void end_loading_queue(LoadEntryScriptCtx ctx)
{
  scripts.done();
  scripts.cleanupMemoryUsage();
  if (initDeserializer)
    initDeserializer->moduleLibrary = nullptr; // Memory has already been reset
  scripts.statistics.loadTimeMs = profile_time_usec(entryScriptStartTime) / 1000;
  scripts.statistics.memoryUsage =
    int64_t(dagor_memory_stat::get_memory_allocated(true) - entryScriptMemUsed) + ctx.additionalMemoryUsage;
  dump_statistics();
}

bool main_thread_post_load()
{
  ScopedMultipleScripts scope;
  const bool res = scripts.mainThreadDone();
  dump_statistics();

  mainThreadBound = das::daScriptEnvironment::bound;
  G_ASSERT(mainThreadBound);

  return res;
}

bool unload_es_script(const char *fname) { return scripts.unloadScript(fname, /*strict*/ true); }

struct QueryEmpty
{};

template <bool has_info, bool stoppable, class T, typename Fn>
static void es_run(const char *name, const das::Block &block, const das::LineInfoArg *line, BaseEsDesc &esData, EsContext &context,
  const T &info, const ecs::QueryView &__restrict qv, Fn &&fn)
{
  uint32_t nAttr = (uint32_t)esData.dataCompCount();
  vec4f *__restrict allArgs = (vec4f *)(alloca((esData.dataCompCount() + 1 + (esData.useManager ? 1 : 0)) * sizeof(vec4f)));
  vec4f *__restrict _args = allArgs + (has_info ? 1 : 0) + (esData.useManager ? 1 : 0);
  char *__restrict *__restrict data = (char *__restrict *__restrict)alloca(nAttr * sizeof(char *));
  if (has_info)
    *((void **)&allArgs[0]) = (void *)&info;
  if (esData.useManager)
    allArgs[has_info ? 1 : 0] = das::cast<ecs::EntityManager *>::from(&qv.manager());

  uint16_t *__restrict stride = esData.stride.get();
  uint8_t *__restrict flags = esData.flags.get();
  const uint8_t combinedFlags = esData.combinedFlags;
  for (uint32_t c = 0; c != nAttr; ++c)
  {
    // uint32_t a = esData.argIndices[c];
    // debug("c = %d arg = %d pointer = %p stride=%d flags=%d",
    //   c, esData.argIndices[c], (char *__restrict) qv.getComponentUntypedData(c), stride[a], flags[a]);
    data[esData.argIndices[c]] = (char *__restrict)qv.getComponentUntypedData(c);
  }
  if (uint32_t firstEntity = qv.begin())
    for (uint32_t c = 0; c != nAttr; ++c)
      data[c] += data[c] ? stride[c] * firstEntity : 0;

  if (combinedFlags & HAS_OPTIONAL)
    memcpy(_args, esData.def.get(), nAttr * sizeof(vec4f));

  esData.resolveUnresolved(); // todo:this can also be removed by re-registering es if fully resolved

#if DAECS_EXTENSIVE_CHECKS
  const ecs::component_index_t *cindices = qv.manager().queryComponents(qv.getQueryId());
  auto &dataComponents = qv.manager().getDataComponents();
  for (uint32_t c = 0, e = esData.dataCompCount(); c != e; ++c, ++cindices)
  {
    ecs::DataComponent dt = dataComponents.getComponentById(*cindices);
    G_ASSERTF(dt.componentTypeName == esData.components[c].type || !data[esData.argIndices[c]], "%d(%d): 0x%X != 0x%X && %p", c,
      esData.argIndices[c], dt.componentTypeName, esData.components[c].type, data[esData.argIndices[c]]);
  }
#endif
  // debug("context.insideContext = %d", context.insideContext);
  // debug_dump_stack();
  G_ASSERT(context.insideContext); // we should be already in locked context.
  // This assert makes running query outside of es NOT possible, if called directly from some exported script version
  // this should not be possible, i.e. context should be always locked on running this exported function.
  // so, if happen add tryRestartAndLock on top of stack of calling
  if ((combinedFlags & (~POINTER_DEST)) == 0)
  {
    fn(allArgs, [&](das::SimNode *code) {
      uint32_t nAttr = (uint32_t)esData.dataCompCount();
      vec4f *__restrict args = _args;
      for (uint32_t i = qv.getEntitiesCount(); i != 0; --i)
      {
        uint16_t *__restrict stridePtr = stride, *__restrict stridePtrE = stridePtr + nAttr;
        char *__restrict *__restrict dataPtr = data;
        uint8_t *__restrict flagsPtr = flags;
        vec4f *__restrict argsPtr = args;
        for (; stridePtr != stridePtrE; ++stridePtr, ++flagsPtr, ++dataPtr, ++argsPtr)
        {
          uint8_t flag = *flagsPtr;
          char *__restrict src = (flag & BOXED_SRC) ? *((char *__restrict *__restrict)*dataPtr) : *dataPtr;
          *((void *__restrict *__restrict)argsPtr) = src;
          if (!(flag & POINTER_DEST))
          {
            if (DAGOR_UNLIKELY(*stridePtr == 1))
              *argsPtr = v_cast_vec4f(v_splatsi(*(uint8_t *)src));
            else if (DAGOR_UNLIKELY(*stridePtr == 2))
              *argsPtr = v_cast_vec4f(v_splatsi(*(uint16_t *)src));
            else
              *argsPtr = v_ldu((float *)src);
          }
          *dataPtr += *stridePtr;
        }
        // debug("eid %d", v_extract_xi(v_cast_vec4i(args[0])));
        vec4f ret = code->eval(context);
        context.stopFlags = 0;
        if (DAGOR_UNLIKELY(context.getException()))
        {
          logerr("%s: exception <%s> during es <%s>", line ? line->describe() : "", context.getException(),
            block.info ? block.info->name : name);
          return;
        }
        if (stoppable && das::cast<bool>::to(ret)) // fixme: should be enum
          break;
      }
    });
  }
  else
  {
    fn(allArgs, [&](das::SimNode *code) {
      uint32_t nAttr = (uint32_t)esData.dataCompCount();
      vec4f *args = _args;
      for (uint32_t i = qv.getEntitiesCount(); i != 0; --i)
      {
        uint16_t *__restrict stridePtr = stride, *__restrict stridePtrE = stridePtr + nAttr;
        char *__restrict *__restrict dataPtr = data;
        uint8_t *__restrict flagsPtr = flags;
        vec4f *__restrict argsPtr = args;
        for (; stridePtr != stridePtrE; ++stridePtr, ++flagsPtr, ++dataPtr, ++argsPtr)
        {
          if (*dataPtr)
          {
            uint8_t flag = *flagsPtr;
            char *__restrict src = (flag & BOXED_SRC) ? *((char *__restrict *__restrict)*dataPtr) : *dataPtr;
            if (flag & POINTER_DEST) // todo: make optimal permutations as different es function, if there is no 'pointer dest' (remove
                                     // this condition)
              *((void **)&*argsPtr) = src;
            else if (flag & STRING_DEST)
              *((void const **)&*argsPtr) = ((ecs::string *)src)->c_str();
            else
            {
              if (DAGOR_UNLIKELY(*stridePtr == 1))
                *argsPtr = v_cast_vec4f(v_splatsi(*(uint8_t *)src));
              else if (DAGOR_UNLIKELY(*stridePtr == 2))
                *argsPtr = v_cast_vec4f(v_splatsi(*(uint16_t *)src));
              else
                *argsPtr = v_ldu((float *)src);
            }
            *dataPtr += *stridePtr;
          }
        }
        vec4f ret = code->eval(context);
        context.stopFlags = 0;
        if (DAGOR_UNLIKELY(context.getException()))
        {
          logerr("%s: exception <%s> during es <%s>", line ? line->describe() : "", context.getException(),
            block.info ? block.info->name : name);
          return;
        }
        if (stoppable && das::cast<bool>::to(ret)) // fixme: should be enum
          break;
      }
    });
  }
}

using DasWorkerContextsArray = eastl::array<eastl::unique_ptr<das::Context>, ecs::MAX_POSSIBLE_WORKERS_COUNT - 1>;
static inline DasWorkerContextsArray create_worker_contexts()
{
  DasWorkerContextsArray workerContexts;
  for (int i = 0, n = workerContexts.size(); i < n; ++i)
  {
    auto &ctx = workerContexts[i];
    das::Context *context = get_context(/*stack_size*/ 4 * 1024);
    context->name.sprintf("worker #%d", i);

    context->persistent = false;
    context->heap = das::make_smart<das::LinearHeapAllocator>();
    context->stringHeap = das::make_smart<das::LinearStringAllocator>();

    context->heap->setInitialSize(16 * 1024);
    context->stringHeap->setInitialSize(16 * 1024);

    ctx.reset(context);
  }

  return workerContexts;
}

static EsContext *get_worker_context(const ecs::QueryView &qv, const EsDesc *esData)
{
  G_FAST_ASSERT(qv.getUserData() == esData);
  static DasWorkerContextsArray workerContexts = create_worker_contexts();

  uint32_t workerId = qv.getWorkerId();
  bool isWorkerThread = esData->minQuant > 0 && workerId != 0;

  if (DAGOR_UNLIKELY(isWorkerThread))
  {
    uint32_t contextId = workerId - 1u;
    G_ASSERTF(!esData->context->thisHelper, "helper should be null here, otherwise remove copying of thisHelper from makeWorkerFor");
    EsContext *context = (EsContext *)workerContexts[contextId].get();
    context->makeWorkerFor(*esData->context);
    context->mgr = &qv.manager();
    return context;
  }

#if DAGOR_DBGLEVEL > 0
  if (in_thread_safe_das_ctx_region)
  {
    int tpwid = threadpool::get_current_worker_id();
    int curFrame = dagor_frame_no();
    if (esData->context->touchedByWkId != tpwid)
    {
      if (DAGOR_UNLIKELY(esData->context->touchedAtFrame == curFrame))
      {
        static eastl::vector_set<uint32_t> reported_violation_eses;
        if (reported_violation_eses.insert(str_hash_fnv1(esData->functionPtr->name)).second)
          logerr("Violation of thread safe access to das context of %s: %s | %s\n"
                 "Move offending ESes to different modules",
            esData->context->name.c_str(), esData->functionPtr->name, esData->context->touchedByES);
      }
      esData->context->touchedByWkId = tpwid;
    }
    esData->context->touchedAtFrame = curFrame;
    esData->context->touchedByES = esData->functionPtr->name;
  }
#endif

  return esData->context;
}

template <typename F>
static inline void run_es_query_lambda(const ecs::QueryView &qv, const EsDesc *esData, const F &lambda)
{
  EsContext *context = get_worker_context(qv, esData);
  RAIIContextMgr newMgr(context, &qv.manager());
  context->tryRestartAndLock();

  auto ctxLambda = [&lambda, context]() { lambda(context); };
  bool result = false;
  ecs::NestedQueryRestorer nestedQuery(context->mgr);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  auto runLambdaWithCatch = [&]() {
#endif
    if (!context->ownStack)
    {
      das::SharedStackGuard guard(*context, get_shared_stack());
      result = context->runWithCatch(ctxLambda);
    }
    else
      result = context->runWithCatch(ctxLambda);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  };
  if (get_globally_enabled_aot() && !esData->functionPtr->aot)
  {
    TIME_PROFILE_DEV(das_interpret);
    runLambdaWithCatch();
  }
  else
    runLambdaWithCatch();
#endif

  if (DAGOR_UNLIKELY(!result))
  {
    nestedQuery.restore(context->mgr);
    if (verboseExceptions > 0)
    {
      --verboseExceptions;
      context->stackWalk(&context->exceptionAt, false, false);
    }
    logerr("%s: unhandled exception <%s> during es <%s>", context->exceptionAt.describe().c_str(), context->getException(),
      esData->functionPtr->name);
  }

  context->unlock();
}


static void das_es_on_update(const ecs::UpdateStageInfo &info, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );

  auto lambda = [&info, &qv, esData](EsContext *context) {
    RAIIDasEnvBound bound;
#if ES_TO_QUERY
    vec4f args[2];
    args[0] = das::cast<const ecs::UpdateStageInfo *>::from(&info);
    args[1] = das::cast<const ecs::QueryView *>::from(&qv);
    context->call(esData->functionPtr, args, 0);
#else
    es_run<true, false>(esData->functionPtr->name, esData->base, *context, info, qv,
      [&](vec4f *__restrict all_args, auto &&lambda) { context->callEx(esData->functionPtr, all_args, nullptr, 0, lambda); });
#endif
  };
  run_es_query_lambda(qv, esData, lambda);
}

static void das_es_on_event_generic(const ecs::Event &evt, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );

  auto lambda = [&evt, &qv, esData](EsContext *context) {
    RAIIDasEnvBound bound;
#if ES_TO_QUERY
    vec4f args[2];
    args[0] = das::cast<const ecs::Event *>::from(&evt);
    args[1] = das::cast<const ecs::QueryView *>::from(&qv);
    context->call(esData->functionPtr, args, 0);
#else
    es_run<true, false>(esData->functionPtr->name, esData->base, *context, evt, qv,
      [&](vec4f *__restrict all_args, auto &&lambda) { context->callEx(esData->functionPtr, all_args, nullptr, 0, lambda); });
#endif
  };
  run_es_query_lambda(qv, esData, lambda);
}

static void das_es_on_das_event(const ecs::Event &evt, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );
  // debug("size is %d evt DataSize %d", esData->typeInfo->structType->size, data_size(evt));

  char *stackVar = (char *)&evt;
  auto lambda = [&qv, esData, stackVar](EsContext *context) {
    RAIIDasEnvBound bound;
#if ES_TO_QUERY
    vec4f args[2];
    args[0] = das::cast<char *>::from(stackVar);
    args[1] = das::cast<const ecs::QueryView *>::from(&qv);
    context->call(esData->functionPtr, args, 0);
#else
    es_run<true, false>(esData->functionPtr->name, esData->base, *context, *stackVar, qv,
      [&](vec4f *__restrict all_args, auto &&lambda) { context->callEx(esData->functionPtr, all_args, nullptr, 0, lambda); });
#endif
  };
  run_es_query_lambda(qv, esData, lambda);
}

static void das_es_on_event_generic_empty(const ecs::Event &evt, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );

  vec4f args[2];
  args[0] = das::cast<const char *>::from((const char *)&evt);
  if (esData->base.useManager)
    args[1] = das::cast<ecs::EntityManager &>::from(qv.manager());
  auto lambda = [&args, esData](EsContext *context) {
    RAIIDasEnvBound bound;
    context->call(esData->functionPtr, args, 0);
  };
  run_es_query_lambda(qv, esData, lambda);
}

static void das_es_on_event_das_event_empty(const ecs::Event &evt, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );

  char *stackVar = (char *)&evt;
  vec4f args[2];
  args[0] = das::cast<char *>::from(stackVar);
  if (esData->base.useManager)
    args[1] = das::cast<ecs::EntityManager &>::from(qv.manager());
  auto lambda = [&args, esData](EsContext *context) {
    RAIIDasEnvBound bound;
    context->call(esData->functionPtr, args, 0);
  };
  run_es_query_lambda(qv, esData, lambda);
}

void process_view(const ecs::QueryView &qv, const das::Block &block, const uint64_t /*init_args_hash*/, das::Context *das_context,
  das::LineInfoArg *line)
{
  EsDesc *esData = (EsDesc *)qv.getUserData(); // todo: put name in EsDesc

  EsContext *context = get_worker_context(qv, esData);
  RAIIContextMgr newMgr(context, &qv.manager());

  if (!context->ownStack)
  {
    das::SharedStackGuard guard(*context, get_shared_stack());
    if (!das_context->ownStack)
      es_run<false, false>("query_name", block, line, esData->base, *context, QueryEmpty(), qv,
        [&](vec4f *__restrict all_args, auto &&lambda) {
          RAIIDasEnvBound bound;
          das::SharedStackGuard guard(*das_context, get_shared_stack());
          das_context->invokeEx(block, all_args, nullptr, lambda, line);
        });
    else
      es_run<false, false>("query_name", block, line, esData->base, *context, QueryEmpty(), qv,
        [&](vec4f *__restrict all_args, auto &&lambda) {
          RAIIDasEnvBound bound;
          das_context->invokeEx(block, all_args, nullptr, lambda, line);
        });
  }
  else
  {
    if (!das_context->ownStack)
      es_run<false, false>("query_name", block, line, esData->base, *context, QueryEmpty(), qv,
        [&](vec4f *__restrict all_args, auto &&lambda) {
          RAIIDasEnvBound bound;
          das::SharedStackGuard guard(*das_context, get_shared_stack());
          das_context->invokeEx(block, all_args, nullptr, lambda, line);
        });
    else
      es_run<false, false>("query_name", block, line, esData->base, *context, QueryEmpty(), qv,
        [&](vec4f *__restrict all_args, auto &&lambda) {
          RAIIDasEnvBound bound;
          das_context->invokeEx(block, all_args, nullptr, lambda, line);
        });
  }
}

static void das_es_on_update_empty(const ecs::UpdateStageInfo &info, const ecs::QueryView &__restrict qv)
{
  EsDesc *esData = (EsDesc *)qv.getUserData();
  G_ASSERT_RETURN(esData && esData->functionPtr, );

  vec4f args[2];
  args[0] = das::cast<const ecs::UpdateStageInfo *>::from(&info);
  if (esData->base.useManager)
    args[1] = das::cast<ecs::EntityManager &>::from(qv.manager());

  auto lambda = [&args, esData](EsContext *context) {
    RAIIDasEnvBound bound;
    context->call(esData->functionPtr, args, 0);
  };
  run_es_query_lambda(qv, esData, lambda);
}

void queryEs(const das::Block &block, das::Context *das_context, das::LineInfoArg *line)
{
  auto *closure = (das::SimNode_ClosureBlock *)block.body;
  EsQueryDesc *esData = (EsQueryDesc *)closure->annotationData;
  if (!esData)
  {
    das_context->throw_error_at(line, "invalid (non-ES) block");
    return;
  }
  struct
  {
    const das::Block *block;
    EsQueryDesc *esData;
    das::LineInfoArg *line;
    bool failed;
  } queryInfo = {&block, esData, line, false};
  EsContext *context = cast_es_context(das_context);

  ecs::perform_query(
    context->mgr, queryInfo.esData->query,
    [context, &queryInfo](const ecs::QueryView &qv) {
      ecs::NestedQueryRestorer nestedQuery(context->mgr);
      if (!context->runWithCatch([&]() {
            es_run<false, false>("query_name", *queryInfo.block, queryInfo.line, queryInfo.esData->base, *context, QueryEmpty(), qv,
              [&](vec4f *__restrict all_args, auto &&lambda) {
                context->invokeEx(*queryInfo.block, all_args, nullptr, lambda, queryInfo.line);
              });
          }))
      {
        nestedQuery.restore(context->mgr);
        queryInfo.failed = true;
        if (verboseExceptions > 0)
        {
          --verboseExceptions;
          context->stackWalk(&context->exceptionAt, false, false);
        }
        logerr("%s: unhandled exception <%s> during query <%s>", context->exceptionAt.describe().c_str(), context->getException(),
          context->mgr->getQueryName(queryInfo.esData->query));
      }
    },
    (void *)queryInfo.esData);

  if (queryInfo.failed)
    context->rethrow();
}

void queryEsMgr(ecs::EntityManager &mgr, const das::Block &block, das::Context *das_context, das::LineInfoArg *line)
{
  EsContext *context = (EsContext *)das_context;
  RAIIContextMgr newMgr(context, &mgr);
  queryEs(block, das_context, line);
}

bool findQueryEs(const das::Block &block, das::Context *das_context, das::LineInfoArg *line)
{
  auto *closure = (das::SimNode_ClosureBlock *)block.body;
  EsQueryDesc *esData = (EsQueryDesc *)closure->annotationData;
  if (!esData)
  {
    das_context->throw_error_at(line, "invalid (non-ES) block");
    return false;
  }
  struct
  {
    const das::Block *block;
    EsQueryDesc *esData;
    das::LineInfoArg *line;
    bool failed;
  } queryInfo = {&block, esData, line, false};
  EsContext *context = cast_es_context(das_context);

  const bool res = ecs::perform_query(
                     context->mgr, queryInfo.esData->query,
                     [context, &queryInfo](const ecs::QueryView &qv) {
                       ecs::NestedQueryRestorer nestedQuery(context->mgr);
                       ecs::QueryCbResult shouldContinue = ecs::QueryCbResult::Continue;
                       if (!context->runWithCatch([&]() {
                             es_run<false, true>("find_query_name", *queryInfo.block, queryInfo.line, queryInfo.esData->base, *context,
                               QueryEmpty(), qv, [&](vec4f *__restrict all_args, auto &&lambda) {
                                 // fixme: Aot is not working for enums now, so we return bool
                                 // shouldContinue = (ecs::QueryCbResult)(das::cast<int>::to(context->invokeEx(*queryInfo.block,
                                 // all_args, nullptr, lambda)));
                                 shouldContinue =
                                   das::cast<bool>::to(context->invokeEx(*queryInfo.block, all_args, nullptr, lambda, queryInfo.line))
                                     ? ecs::QueryCbResult::Stop
                                     : ecs::QueryCbResult::Continue;
                               });
                           }))
                       {
                         nestedQuery.restore(context->mgr);
                         queryInfo.failed = true;
                         if (verboseExceptions > 0)
                         {
                           --verboseExceptions;
                           context->stackWalk(&context->exceptionAt, false, false);
                         }
                         logerr("%s: unhandled exception <%s> during query <%s>", context->exceptionAt.describe().c_str(),
                           context->getException(), context->mgr->getQueryName(queryInfo.esData->query)); // TODO: switch off query?
                       }
                       return shouldContinue;
                     },
                     (void *)queryInfo.esData) == ecs::QueryCbResult::Stop;
  if (queryInfo.failed)
    context->rethrow();
  return res;
}

bool findQueryEsMgr(ecs::EntityManager &mgr, const das::Block &block, das::Context *das_context, das::LineInfoArg *line)
{
  EsContext *context = (EsContext *)das_context;
  RAIIContextMgr newMgr(context, &mgr);
  return findQueryEs(block, das_context, line);
}

bool queryEsEid(ecs::EntityId eid, const das::Block &block, das::Context *das_context, das::LineInfoArg *line)
{
  auto *closure = (das::SimNode_ClosureBlock *)block.body;
  EsQueryDesc *esData = (EsQueryDesc *)closure->annotationData;
  if (!esData)
  {
    das_context->throw_error_at(line, "invalid block");
    return false;
  }
  struct
  {
    const das::Block *block;
    EsQueryDesc *esData;
    das::LineInfoArg *line;
    bool failed;
  } queryInfo = {&block, esData, line, false};
  EsContext *context = cast_es_context(das_context);

  const bool res = ecs::perform_query(
    context->mgr, eid, esData->query,
    [context, &queryInfo](const ecs::QueryView &qv) {
#if DAECS_EXTENSIVE_CHECKS // for performance reasons, eidQuery in release build won't catch exceptions. If ever causes issues - change
                           // that to always catch.
      ecs::NestedQueryRestorer nestedQuery(context->mgr);
      if (!context->runWithCatch([&]() {
#endif
            es_run<false, false>("query_name", *queryInfo.block, queryInfo.line, queryInfo.esData->base, *context, QueryEmpty(), qv,
              [&](vec4f *__restrict all_args, auto &&lambda) {
                context->invokeEx(*queryInfo.block, all_args, nullptr, lambda, queryInfo.line);
              });
#if DAECS_EXTENSIVE_CHECKS
          }))
      {
        nestedQuery.restore(context->mgr);
        queryInfo.failed = true;
        if (verboseExceptions > 0)
        {
          --verboseExceptions;
          context->stackWalk(&context->exceptionAt, false, false);
        }
        logerr("%s: unhandled exception <%s> during eid query <%s>", context->exceptionAt.describe().c_str(), context->getException(),
          context->mgr->getQueryName(queryInfo.esData->query)); // TODO: switch off query?
      }
#endif
    },
    (void *)esData);
  if (queryInfo.failed)
    context->rethrow();
  return res;
}

bool queryEsEidMgr(ecs::EntityManager &mgr, ecs::EntityId eid, const das::Block &block, das::Context *das_context,
  das::LineInfoArg *line)
{
  EsContext *context = (EsContext *)das_context;
  RAIIContextMgr newMgr(context, &mgr);
  return queryEsEid(eid, block, das_context, line);
}

enum class EsType
{
  ES,
  Query,
  FindQuery,
  SingleEidQuery,
};

static das::string aotEsRunBlockName(das::ExprBlock *block, EsType esType)
{
  G_ASSERTF(block->inFunction, "query not called from function");
  const uint64_t functionHash = das::hash_blockz64((uint8_t *)block->inFunction->getMangledName().c_str());
  const uint64_t blockHash = das::hash_blockz64((uint8_t *)block->getMangledName().c_str());
  das::TextWriter nw;
  nw << (esType == EsType::ES ? "__process_view" : (esType == EsType::SingleEidQuery ? "__query_eid_es" : "__query_es"));
  for (auto &arg : block->arguments)
    nw << "_" << arg->name;
  nw << "_" << block->at.line << "_" << (functionHash ^ blockHash);
  return nw.str();
}

static bool aotEsRunBlock(das::TextWriter &ss, EsQueryDesc *desc, const bind_dascript::EsDesc *es_desc, das::ExprBlock *block,
  EsType esType)
{
  auto fnName = aotEsRunBlockName(block, esType);
  const bool hasReturn = esType == EsType::FindQuery || esType == EsType::SingleEidQuery;
  const bool isEmptySystem = desc->base.components.empty();
  ss << "template<typename BT>\n__forceinline " << (hasReturn ? "bool " : "void ") << fnName << " ( ";
  if (esType == EsType::SingleEidQuery)
  {
    ss << "ecs::EntityId eid, uint64_t annotationData";
  }
  else if (esType == EsType::ES)
  {
    ss << "const ::ecs::QueryView &__restrict qv";
  }
  else
  {
    ss << "uint64_t annotationData";
  }
  ss << ", const BT & block";
  if (esType == EsType::ES)
    ss << ", /* init_args_hash */uint64_t";
  ss << ", das::Context * __restrict __context, das::LineInfoArg * __restrict __line )\n{\n";
  if (esType != EsType::ES)
  {
    ss << "ecs::EntityManager *mgr = ((bind_dascript::EsContext *)__context)->mgr;";
    static_assert(offsetof(EsQueryDesc, query) == 0, "AOT assumes annotation data starts with QueryId. keep it that way");
    ss << (hasReturn ? "\treturn " : "\t");
    ss << "::ecs::perform_query(mgr, ";
    if (esType == EsType::SingleEidQuery)
      ss << "eid, ";
    ss << "*((::ecs::QueryId *)annotationData),[&](const ::ecs::QueryView& __restrict qv) DAS_AOT_INLINE_LAMBDA {\n";
  }
  das::StringBuilderWriter body;
  bool isUnicastSystem = es_desc && es_desc->stageMask == 0;
  if (isUnicastSystem)
  {
    for (ecs::event_type_t evtMask : es_desc->evtMask)
    {
      if (bind_dascript::get_dasevent_cast_flags(evtMask) != ecs::EVCAST_UNICAST)
      {
        isUnicastSystem = false;
        break;
      }
    }
  }
  const bool doLoop = !(esType == EsType::SingleEidQuery || isEmptySystem || isUnicastSystem);
  if (doLoop)
    body << "\tuint32_t i = qv.begin(), ie = qv.end();\n\tG_ASSERT(ie > i);\n\tdo {\n";
  else if (!isUnicastSystem || esType == EsType::SingleEidQuery)
    body << "\tconstexpr uint32_t i = 0;\n\tG_ASSERT(qv.end() == 1 && qv.begin() == 0);\n\t{\n";
  else
    body << "\tconstexpr uint32_t i = 0;\n\t{\n";

  body << "\t\t";
  if (esType == EsType::FindQuery)
    // ss << "if (ecs::QueryCbResult::Stop == (ecs::QueryCbResult)";//should be
    body << "if ((bool)"; // should be
  body << "block(\n";

  for (auto &arg : block->arguments)
  {
    const bool isPointer = arg->type->isPointer();
    const bool isImmutableString = arg->type->isString();

    das::Type dasType;
    das::string typeName;
    if (!get_underlying_ecs_type(*arg->type, typeName, dasType))
      return false; // todo: print reason why!
    if (dasType == das::Type::tHandle)
    {
      auto &pointeeType = (arg->type->baseType == das::Type::tPointer ? arg->type->firstType : arg->type);
      auto it = das_struct_aliases.find(pointeeType->annotation->name.c_str());
      if (it == das_struct_aliases.end())
        typeName = pointeeType->annotation->cppName.empty() ? pointeeType->annotation->name : pointeeType->annotation->cppName;
    }
    make_string_outer_ns(typeName);
    int argi = &arg - block->arguments.begin();
    int index = -1; // fixme
    for (int i = 0; i < block->arguments.size(); ++i)
      if (desc->base.argIndices[i] == argi)
      {
        index = i;
        break;
      }
    if (index < 0) // todo: print reason why!
      return false;
    const bool isRO = (index >= desc->base.compCount(BaseEsDesc::RW_END));
    const bool hasDefValue = (!isPointer && arg->init);
    if (isImmutableString && hasDefValue)
      typeName = "const char *";
    das::string cppTypeName =
      das::describeCppType(arg->type, das::CpptSubstitureRef::no, das::CpptSkipRef::yes, das::CpptSkipConst::yes);
    make_string_outer_ns(cppTypeName);
    const bool isSharedComponent = arg->annotation.getBoolOption(SHARED_COMP_ARG);
    const char *endParen = "))";
    if (!hasDefValue)
    {
      if (isImmutableString || isSharedComponent)
      {
        body << "\t\t\tEcsToDas<" << ((isRO && !isPointer && !isImmutableString) ? "const " : "") << cppTypeName
             << (isPointer ? "" : "&") << ", " << (isSharedComponent ? " ::ecs::SharedComponent<" : "") << typeName
             << (isSharedComponent ? ">" : "") << (isPointer ? (isRO ? " const * " : " * ") : "") << ">::get(";
      }
      else
      {
        body << "\t\t\t";
        endParen = ")";
      }
    }
    else
    {
      G_ASSERT(isRO);
      body << "\t\t\t__get_or" << index << "(";
    }

    body << "qv.getComponent" << (isRO ? "RO" : "RW") << ((isPointer || (isImmutableString && hasDefValue)) ? "Opt" : "")
         << ((hasDefValue && !isImmutableString) ? "Opt" : "");

    auto ptrTypeName = [](const das::string &ts) {
      size_t firstNonSpaceCharPos = (size_t)(ts[0] == ' '); // Faster then `ts.find_first_not_of(' ')`
      return ts.substr(0, ts.find(' ', firstNonSpaceCharPos));
    };

    if (isSharedComponent)
      body << "< ::ecs::SharedComponent<" << typeName << "> >";
    else if (isImmutableString)
      body << "< ::ecs::string >";
    else if (typeName == cppTypeName || (isPointer && typeName == ptrTypeName(cppTypeName)))
      body << "<" << typeName << ">";
    else if (!isPointer)
      body << "<" << typeName << ", " << cppTypeName << ">";
    else
      body << "<" << typeName << ", eastl::remove_pointer_t<" << cppTypeName << ">>";

    body << "(" << index << ", i";

    if (hasDefValue)
    {
      G_ASSERT(isRO);
      body << "), ";
      if (get_default_ecs_value(*arg->type, ss, arg->init, desc->base.def[argi], index))
      {
        ss << "\tauto __get_or" << index << " = [](const ";
        if (isImmutableString)
          ss << " ::ecs::string* ";
        else
          ss << cppTypeName << (isPointer ? (isRO ? " * const " : " * ") : "") << " *";
        ss << " value, ";
        ss << "const " << cppTypeName << ((isPointer || isImmutableString) ? "" : "&")
           << " default_value) DAS_AOT_INLINE_LAMBDA"
           //<< "-> " << cppTypeName << ((isPointer || isImmutableString) ? "" : "&") // doesn't work with gcc 9.1
           // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90333
           << "\n\t{\n\t\tif (value != nullptr)\n\t\t\treturn ";
        if (isImmutableString)
          ss << "(char *)get_immutable_string(*value);"; // FIXME: const correctness for strings
        else
          ss << "*value;";
        ss << "\n\t\treturn ";
        if (isImmutableString)
          ss << "(" << cppTypeName << ")";
        ss << " default_value;\n"
           << "\t};\n";
        body << "__def_comp" << index;
      }
      else
        return false;
      body << ")";
    }
    else
      body << endParen;
    if (&arg != &block->arguments.back())
      body << ", // " << arg->name << "\n";
    else
      body << "  // " << arg->name;
  }

  auto body_sz = body.tellp();
  ss.writeStr(body.c_str(), body_sz);

  //
  ss << "\n\t\t)";
  if (esType == EsType::FindQuery)
  {
    ss << ")\n";
    ss << "\t\treturn ::ecs::QueryCbResult::Stop;\n";
  }
  else
    ss << ";\n";
  if (doLoop)
    ss << "\t} while (++i != ie);\n";
  else
    ss << "\t}\n";
  if (esType == EsType::FindQuery)
    ss << "\treturn ::ecs::QueryCbResult::Continue;\n";
  if (esType != EsType::ES)
  {
    ss << "\t}, (void*)annotationData)";
    ss << (esType == EsType::FindQuery ? " == ::ecs::QueryCbResult::Stop;\n" : ";\n");
  }
  ss << "}\n\n";
  return true;
}

template <int ArgsOffset = 0>
struct EsRunFunctionAnnotation : das::FunctionAnnotation
{
  EsType esType;

  das::string get_name(EsType est)
  {
    if (est == EsType::FindQuery)
      return das::string(das::string::CtorSprintf(), "find_query_annotation_es_%d", ArgsOffset);
    if (est == EsType::Query)
      return das::string(das::string::CtorSprintf(), "query_annotation_es_%d", ArgsOffset);
    if (est == EsType::SingleEidQuery)
      return das::string(das::string::CtorSprintf(), "query_eid_annotation_es_%d", ArgsOffset);

    return das::string(das::string::CtorSprintf(), "process_view_annotation_es_%d", ArgsOffset);
  }

  EsRunFunctionAnnotation(EsType est) : das::FunctionAnnotation(get_name(est)), esType(est) {}
  virtual bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  virtual bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  virtual bool apply(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  };
  virtual bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &,
    const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  }
  virtual bool verifyCall(das::ExprCallFunc *call, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &err) override
  {
    const uint32_t blockArgIdx = ArgsOffset + (esType != EsType::ES && esType != EsType::SingleEidQuery ? 0 : 1);
    if (!call->arguments[blockArgIdx]->rtti_isMakeBlock())
    {
      err = "ES Query can only accept explicitly declare [es] block";
      return false;
    }
    auto mb = das::static_pointer_cast<das::ExprMakeBlock>(call->arguments[blockArgIdx]);
    das::ExprBlock *closure = (das::ExprBlock *)mb->block.get();
    auto esA =
      eastl::find_if(closure->annotations.begin(), closure->annotations.end(), [](auto &a) { return a->annotation->name == "es"; });
    if (esA == closure->annotations.end())
    {
      err = "Query should be only called on [es] annotated block";
      return false;
    }

    auto blockReturnType = call->arguments[blockArgIdx]->type->firstType;
    if (esType == EsType::FindQuery)
    {
      if (blockReturnType->baseType != das::tBool) // todo: when AOT for enum will be fixed, it should be enum
      {
        err = "return type of block in find_query should be bool (true - stop, false - continue), and it is " +
              blockReturnType->describe();
        return false;
      }
    }
    else if (!blockReturnType->isVoid())
    {
      err = "return type of block in query or es function should be void, and it is " + blockReturnType->describe();
      return false;
    }
    return true;
  }
  virtual das::string aotName(das::ExprCallFunc *call) override
  {
    const uint32_t blockArgIdx = ArgsOffset + (esType != EsType::ES && esType != EsType::SingleEidQuery ? 0 : 1);
    auto mb = das::static_pointer_cast<das::ExprMakeBlock>(call->arguments[blockArgIdx]);
    das::ExprBlock *closure = (das::ExprBlock *)mb->block.get();
    return aotEsRunBlockName(closure, esType);
  }
  const bind_dascript::EsDesc *getEsDesc(const das::AnnotationList &annotations) const
  {
    if (esType != EsType::ES)
      return nullptr;
    for (const das::smart_ptr<das::AnnotationDeclaration> &annotation : annotations)
    {
      if (annotation->annotation->name != "es")
        continue;
      for (const das::AnnotationArgument &argument : annotation->arguments)
      {
        if (argument.name != "fn_name")
          continue;
        const das::ModuleGroup *moduleGroup = das::daScriptEnvironment::bound->g_Program->thisModuleGroup;
        const ESModuleGroupData *gd = (ESModuleGroupData *)moduleGroup->getUserData("es");
        for (const auto &es : gd->unresolvedEs)
          if (es.second == argument.sValue)
            return es.first.get();
      }
    }
    G_ASSERT("Internal error: closure without system annotation");
    return nullptr;
  }
  virtual void aotPrefix(das::TextWriter &ss, das::ExprCallFunc *call) override
  {
    const uint32_t blockArgIdx = ArgsOffset + (esType != EsType::ES && esType != EsType::SingleEidQuery ? 0 : 1);
    auto mb = das::static_pointer_cast<das::ExprMakeBlock>(call->arguments[blockArgIdx]);
    das::ExprBlock *closure = (das::ExprBlock *)mb->block.get();
    EsQueryDesc *desc = (EsQueryDesc *)closure->annotationData;
    aotEsRunBlock(ss, desc, esType == EsType::ES ? getEsDesc(closure->annotations) : nullptr, closure, esType);
    closure->aotSkipMakeBlock = true;
    closure->aotDoNotSkipAnnotationData = esType != EsType::ES;
  }
  virtual das::ExpressionPtr transformCall(das::ExprCallFunc *call, das::string &err) override
  {
    const uint32_t blockArgIdx = ArgsOffset + (esType != EsType::ES && esType != EsType::SingleEidQuery ? 0 : 1);
    if (call->arguments.size() <= blockArgIdx || !call->arguments[blockArgIdx]->rtti_isMakeBlock())
    {
      err = "expecting make block argument";
      return nullptr;
    }
    auto mkb = das::static_pointer_cast<das::ExprMakeBlock>(call->arguments[blockArgIdx]);
    auto blk = das::static_pointer_cast<das::ExprBlock>(mkb->block);
    bool any = false;
    for (auto &arg : blk->arguments)
      if (!arg->can_shadow)
      {
        arg->can_shadow = true;
        any = true;
      }
    if (any)
      return call->clone();
    return nullptr;
  }
};

struct EntityManagerAnnotation final : das::ManagedStructureAnnotation<ecs::EntityManager, false>
{
  EntityManagerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntityManager", ml)
  {
    cppName = " ::ecs::EntityManager";
  }
};

uint32_t das_ecs_systems_count() { return scripts.statistics.systemsCount; }

uint32_t das_ecs_aot_systems_count() { return scripts.statistics.aotSystemsCount; }

uint32_t das_load_time_ms() { return scripts.statistics.loadTimeMs; }

int64_t das_memory_usage() { return scripts.statistics.memoryUsage; }

bool get_globally_enabled_aot() { return globally_aot_mode == AotMode::AOT; }

uint32_t link_aot_errors_count() { return scripts.linkAotErrorsCount; }

int get_das_compile_errors_count() { return scripts.compileErrorsCount; }

void ECS::addES(das::ModuleLibrary &lib)
{
  addAnnotation(das::make_smart<UpdateStageInfoActAnnotation>(lib));
  addAnnotation(das::make_smart<UpdateStageInfoRenderDebugAnnotation>(lib));
  addAnnotation(das::make_smart<QueryViewAnnotation>(lib));
  addAnnotation(das::make_smart<EntityManagerAnnotation>(lib));

  addAnnotation(das::make_smart<EsFunctionAnnotation>());
  auto queryEidFn =
    das::addExtern<DAS_BIND_FUN(queryEsEid)>(*this, lib, "query", das::SideEffects::modifyExternal, "bind_dascript::queryEsEid");
  auto queryEidFnMgr = das::addExtern<DAS_BIND_FUN(queryEsEidMgr)>(*this, lib, "query", das::SideEffects::modifyArgumentAndExternal,
    "bind_dascript::queryEsEidMgr");

  auto queryEsFn =
    das::addExtern<DAS_BIND_FUN(queryEs)>(*this, lib, "query", das::SideEffects::modifyExternal, "bind_dascript::queryEs");
  auto queryEsFnMgr = das::addExtern<DAS_BIND_FUN(queryEsMgr)>(*this, lib, "query", das::SideEffects::modifyArgumentAndExternal,
    "bind_dascript::queryEsMgr");
  auto findQueryEsFn = das::addExtern<DAS_BIND_FUN(findQueryEs)>(*this, lib, "find_query", das::SideEffects::modifyExternal,
    "bind_dascript::findQueryEs");
  auto findQueryEsFnMgr = das::addExtern<DAS_BIND_FUN(findQueryEsMgr)>(*this, lib, "find_query",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::findQueryEsMgr");
  {
    auto queryEidFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<0>>(EsType::SingleEidQuery);
    addAnnotation(queryEidFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEidFunctionAnnotation;
    queryEidFn->annotations.push_back(queryEsDecl);
  }
  {
    auto queryEidFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<1>>(EsType::SingleEidQuery);
    addAnnotation(queryEidFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEidFunctionAnnotation;
    queryEidFnMgr->annotations.push_back(queryEsDecl);
  }
  {
    auto queryEsFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<0>>(EsType::Query);
    addAnnotation(queryEsFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEsFunctionAnnotation;
    queryEsFn->annotations.push_back(queryEsDecl);
  }
  {
    auto queryEsFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<1>>(EsType::Query);
    addAnnotation(queryEsFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEsFunctionAnnotation;
    queryEsFnMgr->annotations.push_back(queryEsDecl);
  }
  {
    auto queryEsFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<0>>(EsType::FindQuery);
    addAnnotation(queryEsFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEsFunctionAnnotation;
    findQueryEsFn->annotations.push_back(queryEsDecl);
  }
  {
    auto queryEsFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<1>>(EsType::FindQuery);
    addAnnotation(queryEsFunctionAnnotation);
    auto queryEsDecl = das::make_smart<das::AnnotationDeclaration>();
    queryEsDecl->annotation = queryEsFunctionAnnotation;
    findQueryEsFnMgr->annotations.push_back(queryEsDecl);
  }

  auto processViewFn = das::addExtern<DAS_BIND_FUN(process_view)>(*this, lib, "process_view", das::SideEffects::modifyExternal,
    "bind_dascript::process_view");
  auto processViewFunctionAnnotation = das::make_smart<EsRunFunctionAnnotation<0>>(EsType::ES);
  addAnnotation(processViewFunctionAnnotation);
  auto processViewDecl = das::make_smart<das::AnnotationDeclaration>();
  processViewDecl->annotation = processViewFunctionAnnotation;
  processViewFn->annotations.push_back(processViewDecl);

  das::addExternTempRef<DAS_BIND_FUN(getEntityManager), das::SimNode_ExtFuncCallRef>(*this, lib, "getEntityManager",
    das::SideEffects::accessExternal, "bind_dascript::getEntityManager");
  das::addExternTempRef<DAS_BIND_FUN(getEntityManagerRW), das::SimNode_ExtFuncCallRef>(*this, lib, "getEntityManagerRW",
    das::SideEffects::accessExternal, "bind_dascript::getEntityManagerRW");

  das::addExtern<DAS_BIND_FUN(bind_dascript::get_globally_enabled_aot)>(*this, lib, "globally_enabled_aot",
    das::SideEffects::accessExternal, "bind_dascript::get_globally_enabled_aot");

  das::addExtern<DAS_BIND_FUN(bind_dascript::das_ecs_systems_count)>(*this, lib, "das_ecs_systems_count",
    das::SideEffects::accessExternal, "bind_dascript::das_ecs_systems_count");

  das::addExtern<DAS_BIND_FUN(bind_dascript::das_ecs_aot_systems_count)>(*this, lib, "das_ecs_aot_systems_count",
    das::SideEffects::accessExternal, "bind_dascript::das_ecs_aot_systems_count");

  das::addExtern<DAS_BIND_FUN(bind_dascript::das_load_time_ms)>(*this, lib, "das_load_time_ms", das::SideEffects::accessExternal,
    "bind_dascript::das_load_time_ms");

  das::addExtern<DAS_BIND_FUN(bind_dascript::das_memory_usage)>(*this, lib, "das_memory_usage", das::SideEffects::accessExternal,
    "bind_dascript::das_memory_usage");

  das::addExtern<DAS_BIND_FUN(bind_dascript::link_aot_errors_count)>(*this, lib, "link_aot_errors_count",
    das::SideEffects::accessExternal, "bind_dascript::link_aot_errors_count");
  das::addExtern<DAS_BIND_FUN(bind_dascript::get_das_compile_errors_count)>(*this, lib, "das_compile_errors_count",
    das::SideEffects::accessExternal, "bind_dascript::get_das_compile_errors_count");
}

ReloadResult reload_all_changed()
{
  if (das_is_in_init_phase)
    return NO_CHANGES;
  ScopedMultipleScripts scope;
  const bool fillStack = fill_stack_while_compile_das();
  const bool wasStackFill = DagDbgMem::enable_stack_fill(fillStack);
  ReloadResult res = scripts.reloadAllChanged(globally_aot_mode, globally_log_aot_errors);
  DagDbgMem::enable_stack_fill(wasStackFill);
  return res;
}

bool reload_all_scripts(const char *entry_point_name, TInitDas init)
{
  ScopedMultipleScripts scope;
  scripts.unloadAllScripts(UnloadDebugAgents::NO);
  return bind_dascript::load_entry_script(entry_point_name, init);
}

void set_das_aot(AotMode value) { globally_aot_mode = value; }

AotMode get_das_aot() { return globally_aot_mode; }

void set_das_log_aot_errors(LogAotErrors value) { globally_log_aot_errors = value; }

LogAotErrors get_das_log_aot_errors() { return globally_log_aot_errors; }

size_t dump_memory() { return scripts.dumpMemory(); }

void dump_statistics()
{
  debug("daScript: loaded %d systems, %d in AOT mode, in %u ms, %d threads, mem usage %dK", scripts.statistics.systemsCount,
    scripts.statistics.aotSystemsCount, scripts.statistics.loadTimeMs, globally_load_threads_num,
    scripts.statistics.memoryUsage / 1024);
  if (globally_aot_mode == AotMode::AOT && scripts.linkAotErrorsCount > 0u)
    logwarn("daScript: failed to link cpp aot. Errors count %d. To see errors add -config:debug/das_log_aot_errors:b=yes",
      scripts.linkAotErrorsCount);
}

void init_sandbox(const char *pak)
{
  scripts.sandboxModuleFileAccess.dasProject = pak ? pak : "";
  scripts.sandboxModuleFileAccess.hotReloadMode = globally_hot_reload;
}
void set_sandbox(bool is_sandbox) { scripts.setSandboxMode(is_sandbox); }
void set_game_mode(bool is_active)
{
  G_ASSERTF(!is_active || is_active != scripts.storeTemporaryScripts, "multiple game modes is not supported");
  scripts.storeTemporaryScripts = is_active;
  scripts.unloadTemporaryScript();
}

void unload_all_das_scripts_without_debug_agents()
{
  ScopedMultipleScripts scope;
  scripts.unloadAllScripts(UnloadDebugAgents::NO);
}

void das_load_ecs_templates()
{
  scripts.postProcessModuleGroups(); // currently module groups contains only templates
}
} // namespace bind_dascript
DAG_DECLARE_RELOCATABLE(bind_dascript::DascriptLoadJob);

static void pull()
{
  das::daScriptEnvironment::ensure();
  das::daScriptEnvironment::bound->das_def_tab_size = 2; // our coding style requires indenting of 2
  if (das::Module::require("ecs"))
    return;
  NEED_MODULE(Module_BuiltIn);
  NEED_MODULE(Module_Math);
  NEED_MODULE(Module_Strings);
  NEED_MODULE(DagorMath)
  NEED_MODULE(DagorDataBlock)
  NEED_MODULE(DagorSystem)
  NEED_MODULE(Module_Rtti)
  NEED_MODULE(Module_Ast)
  NEED_MODULE(ECS)
}

void bind_dascript::set_das_root(const char *root_path) { das::setDasRoot(root_path); }

void bind_dascript::free_serializer_buffer(bool success)
{
  const bool needWriteSerData = success && bind_dascript::initSerializer && !bind_dascript::initSerializer->failed &&
                                !bind_dascript::serializationFileName.empty();

  bind_dascript::initSerializerStorage.reset();
  bind_dascript::initDeserializerStorage.reset();
  bind_dascript::initSerializer.reset();
  bind_dascript::initDeserializer.reset();

  const auto &newFileName = bind_dascript::serializationFileName;
  if (needWriteSerData)
  {
    const auto &filename = bind_dascript::deserializationFileName;
    const bool erased = dd_erase(filename.c_str());
    const bool renamed = dd_rename(newFileName.c_str(), filename.c_str());
    if (!renamed)
      logwarn("das: serialize: failed to save '%s' (erased? %@)", filename.c_str(), erased);
    else
      debug("das: serialize: save to '%s'", filename.c_str());
  }
  else
  {
    dd_erase(newFileName.c_str());
    debug("das: serialize: dropped '%s' (no changes)", newFileName.c_str());
  }
}

void bind_dascript::set_command_line_arguments(int argc, char *argv[]) { das::setCommandLineArguments(argc, argv); }

void bind_dascript::pull_das() { pull(); }

int bind_dascript::set_load_threads_num(int load_threads_num)
{
  const int maxWorkers = threadpool::get_num_workers();
  globally_load_threads_num = eastl::max(1, eastl::min(maxWorkers, load_threads_num));
  debug("daScript: load scripts in %@ threads. Required: %@, max workers: %@", globally_load_threads_num, load_threads_num,
    maxWorkers);
  return globally_load_threads_num;
}

int bind_dascript::get_load_threads_num() { return globally_load_threads_num; }

void bind_dascript::set_thread_init_script(const char *file_name)
{
  debug("daScript: thread init script: '%s'", file_name);
  globally_thread_init_script = file_name ? file_name : "";
}

void bind_dascript::set_resolve_ecs_on_load(ResolveECS resolve_ecs) { globally_resolve_ecs_on_load = resolve_ecs; }

bind_dascript::ResolveECS bind_dascript::get_resolve_ecs_on_load() { return globally_resolve_ecs_on_load; }

void bind_dascript::set_loading_profiling_enabled(bool enabled) { scripts.profileLoading = enabled; }


void bind_dascript::init_das(AotMode enable_aot, HotReload allow_hot_reload, LogAotErrors log_aot_errors)
{
  das_struct_aliases["float3x4"] = "TMatrix";
  das_struct_aliases["float4x4"] = "TMatrix4";
  das_struct_aliases["das_string"] = "ecs::string";
  globally_aot_mode = enable_aot;
  globally_hot_reload = allow_hot_reload;
  globally_log_aot_errors = log_aot_errors;
  debug("daScript: init %s mode, %s hot reload", enable_aot == AotMode::AOT ? "AOT" : "INTERPRET",
    allow_hot_reload == HotReload::ENABLED ? "with" : "without");
}

void bind_dascript::init_scripts(HotReload hot_reload_mode, LoadDebugCode load_debug_code, const char *pak)
{
  scripts.moduleFileAccess.dasProject = pak ? pak : "";
  scripts.moduleFileAccess.hotReloadMode = hot_reload_mode;
  scripts.init();
  scripts.loadDebugCode = load_debug_code == LoadDebugCode::YES;
  debug("daScript: load debug code = %s", load_debug_code == LoadDebugCode::YES ? "yes" : "no");
}

void bind_dascript::init_systems(AotMode enable_aot, HotReload allow_hot_reload, LoadDebugCode load_debug_code,
  LogAotErrors log_aot_errors, const char *pak)
{
  pull_das();
  init_das(enable_aot, allow_hot_reload, log_aot_errors);
  init_scripts(allow_hot_reload, load_debug_code, pak);

  if (is_main_thread())
  { // init mainThreadBound for any standalone application
    mainThreadBound = das::daScriptEnvironment::bound;
    G_ASSERT(mainThreadBound);
  }
}

void bind_dascript::start_multiple_scripts_loading()
{
  G_ASSERT(!das_is_in_init_phase);
  das_is_in_init_phase = true;
}

void bind_dascript::drop_multiple_scripts_loading() { das_is_in_init_phase = false; }

void bind_dascript::end_multiple_scripts_loading()
{
  G_ASSERT(das_is_in_init_phase);
  das_is_in_init_phase = false;
  if (g_entity_mgr)
    g_entity_mgr->resetEsOrder();
}

void bind_dascript::load_event_files(bool do_load) { scripts.loadEvents = do_load; }

#if !HAS_DANET
ecs::event_flags_t bind_dascript::get_dasevent_cast_flags(ecs::event_type_t) { return ecs::EVCAST_UNKNOWN; }
void bind_dascript::ECS::addEvents(das::ModuleLibrary &) {}
#endif
