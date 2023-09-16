#pragma once
#include <dasModules/dasScriptsLoader.h>
#include <ecs/scripts/dasEs.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <daScript/simulate/bin_serializer.h>
#include <daScript/misc/smart_ptr.h>
#include <daScript/misc/sysos.h>
#include <ecs/scripts/dascripts.h>
#include <dasModules/dasCreatingTemplate.h>

struct DasEcsStatistics;

namespace bind_dascript
{
enum ComponentArgFlags
{
  BOXED_SRC = 0x01,
  POINTER_DEST = 0x02,
  STRING_DEST = 0x04,
  HAS_OPTIONAL = 0x08,
  // SHARED_COMPONENT = 0x10,
};

struct BaseEsDesc
{
  eastl::unique_ptr<uint16_t[]> argIndices;
  eastl::unique_ptr<uint16_t[]> stride;
  eastl::unique_ptr<uint8_t[]> flags;
  eastl::unique_ptr<vec4f[]> def;

  bool resolved = false;
  bool useManager = false;
  uint8_t combinedFlags = 0;

  eastl::vector<ecs::ComponentDesc> components; // unique_ptr[]? todo
  eastl::vector<SimpleString> defStrings;       // unique_ptr[]? todo:
  enum
  {
    RW_END,
    RO_END,
    RQ_END,
    NO_END,
    COUNT
  };
  uint16_t ends[COUNT] = {0, 0, 0, 0};
  uint16_t dataCompCount() const { return ends[RO_END]; }

  uint16_t compCount(uint32_t c) const
  {
    return ends[min(uint32_t(c), uint32_t(NO_END))] - (c == 0 ? 0 : ends[min(uint32_t(c), uint32_t(NO_END)) - 1]);
  }
  const ecs::ComponentDesc *compPtr(uint32_t c) const
  {
    return components.data() + (c == 0 ? 0 : ends[min(uint32_t(c), uint32_t(NO_END)) - 1]);
  }
  dag::ConstSpan<ecs::ComponentDesc> getSlice(uint32_t c) const
  {
    return make_span_const<ecs::ComponentDesc>(compPtr(c), (uint32_t)compCount(c));
  }
  bool isResolved() const { return resolved; }
  void resolveUnresolved()
  {
    if (!isResolved())
      resolveUnresolvedOutOfLine();
  }
  void resolveUnresolvedOutOfLine();
};

struct EsContext;
struct EsDesc
{
  das::SimFunction *functionPtr = nullptr;
  EsContext *context = nullptr;
  BaseEsDesc base;
  das::TypeInfo *typeInfo = nullptr;
  das::string tagsList, beforeList, afterList;
  das::string trackedList;
  // union
  //{
  ecs::EventSet evtMask;
  uint32_t stageMask = 0;
  //}
  uint32_t hashedScriptName = 0;
  int minQuant = 0;
};

struct EsQueryDesc
{
  ecs::QueryId query;
  BaseEsDesc base;
  bool shared = false;
  EsQueryDesc() = default;
  ~EsQueryDesc()
  {
    if ((bool)query && g_entity_mgr)
      g_entity_mgr->destroyQuery(query);
  }
  EsQueryDesc(EsQueryDesc &&a) : base(eastl::move(a.base)), query(a.query), shared(a.shared) { a.query = ecs::QueryId(); }
  EsQueryDesc &operator=(EsQueryDesc &&a)
  {
    shared = a.shared;
    base = eastl::move(a.base);
    query = a.query;
    a.query = ecs::QueryId();
    return *this;
  }
  EsQueryDesc(const EsQueryDesc &) = delete;
  EsQueryDesc &operator=(const EsQueryDesc &) = delete;
};

struct EsDescDeleter
{
  void operator()(EsDesc *p);
};
using EsDescUP = eastl::unique_ptr<EsDesc, EsDescDeleter>;
struct EsQueryDescDeleter
{
  void operator()(EsQueryDesc *p);
};
using EsQueryDescUP = eastl::unique_ptr<EsQueryDesc, EsQueryDescDeleter>;

typedef OAHashNameMap<false> DebugArgStrings;

struct QueryData
{
  eastl::string name;
  EsQueryDescUP desc;
};

struct ESModuleGroupData : das::ModuleGroupUserData
{
  DebugArgStrings &argStrings;
  ecs::TemplateRefs trefs; // contains only components
  das::vector<CreatingTemplate> templates;
  ESModuleGroupData(DebugArgStrings &str) : das::ModuleGroupUserData("es"), argStrings(str) {}
  eastl::vector<eastl::pair<EsDescUP, eastl::string>> unresolvedEs;
  eastl::vector<QueryData> unresolvedQueries;
  das::DebugInfoHelper *helper = nullptr;
  uint32_t hashedScriptName = 0;
  uint32_t es_resolve_function_ptrs(EsContext *ctx, eastl::set<ecs::EntitySystemDesc *> &systems, const char *fname,
    uint64_t load_start_time, AotMode aot_mode, AotModeIsRequired aot_mode_is_required, DasEcsStatistics &stats);
};

}; // namespace bind_dascript
