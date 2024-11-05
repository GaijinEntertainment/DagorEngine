// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/internal/performQuery.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector_map.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <ecs/core/utility/ecsBlkUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <regex>
#include <string>

EA_DISABLE_VC_WARNING(4189) // "local variable is initialized but not referenced" - msvc15 can't in lambda captures
using namespace console;

template <class To, class From>
inline void combine_hash(To &to, From b)
{
  to ^= To(To(b) + To(0x9e3779b9) + To(to << 6) + To(to >> 2));
}; // this is boost implementation, supposed to be good

template <typename F>
static inline void all_entities_query(const F &fn)
{
  ecs::ComponentDesc eidComp{ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};
  ecs::NamedQueryDesc desc{
    "all_entities_query",
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(&eidComp, 1),
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(),
  };
  ecs::QueryId qid = g_entity_mgr->createQuery(desc);

  ecs::perform_query(g_entity_mgr, qid, [&fn](const ecs::QueryView &qv) {
    for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      fn(qv.getComponentRO<ecs::EntityId>(0, it));
  });
  g_entity_mgr->destroyQuery(qid);
}

static inline ecs::EntityId query_selected_eid()
{
  ecs::EntityId selectedEid;
  ecs::ComponentDesc eidComp{ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};
  ecs::ComponentDesc selectedComp{ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()};
  ecs::NamedQueryDesc desc{
    "daeditor_selected_entity_query",
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(&eidComp, 1),
    dag::ConstSpan<ecs::ComponentDesc>(&selectedComp, 1),
    dag::ConstSpan<ecs::ComponentDesc>(),
  };
  ecs::QueryId qid = g_entity_mgr->createQuery(desc);

  ecs::perform_query(g_entity_mgr, qid, [&selectedEid](const ecs::QueryView &qv) {
    for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      selectedEid = qv.getComponentRO<ecs::EntityId>(0, it);
  });
  g_entity_mgr->destroyQuery(qid);
  return selectedEid;
}

#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
#define ECS_HAS_PERF_MARKERS 1
namespace ecs
{
extern bool events_perf_markers;
};
#endif
#if DAGOR_DBGLEVEL > 0 // ComponentDesc.nameStr does not exists otherwise

// returns parents from latest to earliest (deepest)
static void flatten_parents(const ecs::Template *root, eastl::vector<uint32_t> &result)
{
  dag::ConstSpan<uint32_t> parents = root->getParents();
  for (const uint32_t p : parents)
  {
    if (eastl::find(result.begin(), result.end(), p) == result.end())
    {
      result.push_back(p);
      flatten_parents(g_entity_mgr->getTemplateDB().getTemplateById(p), result);
    }
  }
}

static void merge_components(ecs::ComponentsMap &high_priority_source, const ecs::ComponentsMap &low_priority_arg)
{
  for (auto &c : low_priority_arg)
  {
    if (high_priority_source[c.first].isNull())
      high_priority_source[c.first] = c.second;
  }
}

static ecs::ComponentsMap gather_all_components(const char *template_name)
{
  ecs::ComponentsMap result;
  if (!template_name)
  {
    console::print_d("Template name is null");
    return result;
  }

  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(template_name);
  if (!templ)
  {
    console::print_d("Template '%s' not found", template_name);
    return result;
  }

  merge_components(result, templ->getComponentsMap());
  eastl::vector<uint32_t> parents;
  flatten_parents(templ, parents);
  for (const uint32_t p : parents)
  {
    auto pr = g_entity_mgr->getTemplateDB().getTemplateById(p);
    if (pr != nullptr)
      merge_components(result, pr->getComponentsMap());
  }
  return result;
}

static eastl::vector_set<ecs::component_t> gather_all_tracked(const ecs::Template *templ)
{
  eastl::vector_set<ecs::component_t> result;

  ecs::Template::ComponentsSet templTracked = templ->trackedSet();
  for (auto &c : templTracked)
    result.insert(c);

  eastl::vector<uint32_t> parents;
  flatten_parents(templ, parents);
  for (const uint32_t p : parents)
  {
    auto pr = g_entity_mgr->getTemplateDB().getTemplateById(p);
    if (pr != nullptr)
    {
      ecs::Template::ComponentsSet parentTracked = pr->trackedSet();
      for (auto &c : parentTracked)
        result.insert(c);
    }
  }
  return result;
}

/// replace 'file_name.das_l123_c21' with 'file_name.das:123:21' for easier seraching in the editor
static std::string to_searchable_querry_name(std::string qname)
{
  std::regex rexp("_l(\\d+)_c(\\d+)");
  std::smatch matches;
  if (std::regex_search(qname, matches, rexp))
  {
    int line = atoi(matches[1].str().c_str());
    int character = atoi(matches[2].str().c_str());
    std::string lineInfoStr = ":" + std::to_string(line) + ":" + std::to_string(character);
    return std::regex_replace(qname, rexp, lineInfoStr);
  }
  rexp = std::regex("das_l(\\d+)");
  if (std::regex_search(qname, matches, rexp))
  {
    int line = atoi(matches[1].str().c_str());
    std::string lineInfoStr = "das:" + std::to_string(line);
    return std::regex_replace(qname, rexp, lineInfoStr);
  }
  return qname;
}

static bool check_query_processes_template(const ecs::BaseQueryDesc *query, const ecs::ComponentsMap &templ_comps)
{
  auto checkComp = [&](dag::ConstSpan<ecs::ComponentDesc> system_comp_list, bool include) {
    for (const ecs::ComponentDesc &desc : system_comp_list)
    {
      if ((desc.flags & ecs::CDF_OPTIONAL) != 0 || desc.name == ECS_HASH("eid").hash)
        continue;
      bool componentFound = false;
      for (auto tcomp : templ_comps)
      {
        if (desc.name == tcomp.first)
        {
          componentFound = true;
          if (!include)
            return false;
          break;
        }
      }
      if (!componentFound && include)
        return false;
    }
    return true;
  };

  return checkComp(query->componentsRO, true) && checkComp(query->componentsRW, true) && checkComp(query->componentsRQ, true) &&
         checkComp(query->componentsNO, false);
}

static void does_system_process_template(const char *system_name, const char *template_name)
{
  const ecs::EntitySystemDesc *system = nullptr;
  auto systems = g_entity_mgr->getSystems();
  for (const ecs::EntitySystemDesc *s : systems)
  {
    if (strcmp(s->name, system_name) == 0)
    {
      system = s;
      break;
    }
  }
  if (!system)
  {
    console::print_d("System '%s' not found", system_name);
    return;
  }

  const ecs::ComponentsMap compMap = gather_all_components(template_name);
  if (compMap.empty())
    return;

  auto checkComp = [&](dag::ConstSpan<ecs::ComponentDesc> system_comp_list, bool include, bool check_type) {
    bool hasProblems = false;
    for (const ecs::ComponentDesc &desc : system_comp_list)
    {
      if ((desc.flags & ecs::CDF_OPTIONAL) != 0 || desc.name == ECS_HASH("eid").hash)
        continue;
      bool componentFound = false;
      for (auto tcomp : compMap)
      {
        if (desc.name == tcomp.first)
        {
          componentFound = true;
          const char *name = desc.nameStr;
          name = name ? name : g_entity_mgr->getTemplateDB().getComponentName(tcomp.first);
          if (!include)
          {
            console::print_d("Problem: \n\t'%s' is REQUIRE_NOT but is present in '%s'", name, template_name);
            hasProblems = true;
            break;
          }
          if (check_type && tcomp.second.getUserType() != desc.type && tcomp.second.getUserType() > 0)
          {
            console::print_d("Problem: \n\t'%s'`s type is different to the one the system requires", name);
            hasProblems = true;
            break;
          }
        }
      }
      if (!componentFound && include)
      {
        console::print_d("Problem: \n\t'%s' not found in the template", desc.nameStr);
        hasProblems = true;
      }
    }
    return !hasProblems;
  };
  bool ro_result = checkComp(system->componentsRO, true, true);
  bool rw_result = checkComp(system->componentsRW, true, true);
  bool rq_result = checkComp(system->componentsRQ, true, false);
  bool no_result = checkComp(system->componentsNO, false, true);
  if (ro_result && rw_result && rq_result && no_result)
    console::print_d("Positive! \n\t'%s' processes entities created from template '%s'", system_name, template_name);
}

static const eastl::vector<ecs::component_type_t> blk_types = {ecs::ComponentTypeInfo<int>::type,
  ecs::ComponentTypeInfo<uint16_t>::type, ecs::ComponentTypeInfo<ecs::string>::type, ecs::ComponentTypeInfo<float>::type,
  ecs::ComponentTypeInfo<Point2>::type, ecs::ComponentTypeInfo<Point3>::type, ecs::ComponentTypeInfo<Point4>::type,
  ecs::ComponentTypeInfo<IPoint2>::type, ecs::ComponentTypeInfo<IPoint3>::type, ecs::ComponentTypeInfo<bool>::type,
  ecs::ComponentTypeInfo<TMatrix>::type, ecs::ComponentTypeInfo<E3DCOLOR>::type, ecs::ComponentTypeInfo<int64_t>::type,
  ecs::ComponentTypeInfo<ecs::Object>::type, ecs::ComponentTypeInfo<ecs::Array>::type, ecs::ComponentTypeInfo<ecs::List<int>>::type,
  ecs::ComponentTypeInfo<ecs::List<uint16_t>>::type, ecs::ComponentTypeInfo<ecs::List<ecs::string>>::type,
  ecs::ComponentTypeInfo<ecs::List<float>>::type, ecs::ComponentTypeInfo<ecs::List<Point2>>::type,
  ecs::ComponentTypeInfo<ecs::List<Point3>>::type, ecs::ComponentTypeInfo<ecs::List<Point4>>::type,
  ecs::ComponentTypeInfo<ecs::List<IPoint2>>::type, ecs::ComponentTypeInfo<ecs::List<IPoint3>>::type,
  ecs::ComponentTypeInfo<ecs::List<bool>>::type, ecs::ComponentTypeInfo<ecs::List<TMatrix>>::type,
  ecs::ComponentTypeInfo<ecs::List<E3DCOLOR>>::type, ecs::ComponentTypeInfo<ecs::List<int64_t>>::type};

eastl::string component_to_string(const char *name, const ecs::EntityComponentRef &comp)
{
  if (eastl::find(blk_types.begin(), blk_types.end(), comp.getUserType()) != blk_types.end())
  {
    DataBlock blk;
    ecs::component_to_blk_param(name, comp, &blk);
    DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 1 << 10);
    cwr.seekto(0);
    cwr.resize(0);
    blk.saveToTextStream(cwr);
    cwr.seekto(cwr.size() - 2);
    cwr.write("\0", 1);
    return eastl::string((const char *)cwr.data());
  }
  if (comp.template is<ecs::EntityId>())
    return eastl::string(name) + ":eid=" + eastl::to_string((ecs::entity_id_t)(comp.template get<ecs::EntityId>()));
  if (comp.template is<ecs::Tag>())
    return eastl::string("Tag:") + name;

  ecs::type_index_t typeIdx = ecs::find_component_type_index(comp.getUserType());
  const char *typeName = g_entity_mgr->getComponentTypes().getTypeNameById(typeIdx);
  if (typeName != nullptr)
    return eastl::string("[") + typeName + "]";
  else
    return "unknown_typename(" + eastl::to_string(typeIdx) + ")";
}

static bool check_system_uses_component(const ecs::BaseQueryDesc query_desc, ecs::component_t comp, bool rw_only = false)
{
  bool pass = false;
  for (auto &c : query_desc.componentsRW)
  {
    if (c.name == comp)
    {
      pass = true;
      break;
    }
  }
  if (!rw_only)
  {
    for (auto &c : query_desc.componentsRO)
    {
      if (c.name == comp)
      {
        pass = true;
        break;
      }
    }
  }
  return pass;
}

static void report_diff_result(const char *lhs_name, const char *rhs_name, eastl::vector<eastl::string> lhs_reports,
  eastl::vector<eastl::string> rhs_reports, eastl::vector<eastl::string> value_reports, int total_lhs_comps)
{
  auto sortStr = [](const eastl::string &a, const eastl::string &b) { return strcmp(a.c_str(), b.c_str()) < 0; };
  if (lhs_reports.size() > 0)
  {
    eastl::sort(lhs_reports.begin(), lhs_reports.end(), sortStr);
    console::print_d("Extra field in %s:", lhs_name);
    for (auto &str : lhs_reports)
    {
      console::print_d("\t%s", str.c_str());
    }
  }
  if (rhs_reports.size() > 0)
  {
    eastl::sort(rhs_reports.begin(), rhs_reports.end(), sortStr);
    console::print_d("Extra field in %s:", rhs_name);
    for (auto &str : rhs_reports)
    {
      console::print_d("\t%s", str.c_str());
    }
  }
  console::print_d(""); // flushing for better formatting
  int overlappingComponents = total_lhs_comps - lhs_reports.size();
  if (value_reports.size() > 0)
  {
    eastl::sort(value_reports.begin(), value_reports.end(), sortStr);
    for (auto &str : value_reports)
    {
      console::print_d("\t%s", str.c_str());
    }
  }
  console::print_d(""); // flushing for better formatting
  console::print_d("Extra components %d vs %d. Overlap: %d. Value mismatch: %d.", lhs_reports.size(), rhs_reports.size(),
    overlappingComponents, value_reports.size());
}

static void diff_systems(const ecs::ComponentsMap lhsComps, const ecs::ComponentsMap rhsComps, const char *lhsName,
  const char *rhsName, eastl::vector<std::string> filter_comps)
{
  eastl::vector<std::string> leftOnlyQueries;
  eastl::vector<std::string> rightOnlyQueries;
  eastl::vector<std::string> commonQueries;
  eastl::vector<ecs::component_t> filterCompIds;
  for (auto &cmp : filter_comps)
    filterCompIds.push_back(ECS_HASH_SLOW(cmp.c_str()).hash);

  uint32_t queryCount = g_entity_mgr->getQueriesCount();
  for (uint32_t i = 0; i < queryCount; i++)
  {
    ecs::QueryId qid = g_entity_mgr->getQuery(i);
    const ecs::BaseQueryDesc qd = g_entity_mgr->getQueryDesc(qid);
    bool passesCmpFilters = true;
    for (auto &cmp : filterCompIds)
      passesCmpFilters = passesCmpFilters && check_system_uses_component(qd, cmp, false);
    if (!passesCmpFilters)
      continue;
    bool lhsPass = check_query_processes_template(&qd, lhsComps);
    bool rhsPass = check_query_processes_template(&qd, rhsComps);
    const char *qRawName = g_entity_mgr->getQueryName(qid);
    std::string qName = to_searchable_querry_name(qRawName ? std::string(qRawName) : "-no_name-");
    if (lhsPass && rhsPass)
      commonQueries.push_back(qName);
    else if (lhsPass)
      leftOnlyQueries.push_back(qName);
    else if (rhsPass)
      rightOnlyQueries.push_back(qName);
  }
  console::print_d("%d Queries that process ONLY %s:", leftOnlyQueries.size(), lhsName);
  for (auto &str : leftOnlyQueries)
  {
    console::print_d("\t%s", str.c_str());
  }
  console::print_d("%d Queries that process ONLY %s:", rightOnlyQueries.size(), rhsName);
  for (auto &str : rightOnlyQueries)
  {
    console::print_d("\t%s", str.c_str());
  }
  console::print_d("%d Queries that process BOTH:", commonQueries.size());
  for (auto &str : commonQueries)
  {
    console::print_d("\t%s", str.c_str());
  }
  console::print_d("----------------------------------------------------------");
}

static void diff_systems_for_eids(int lhs_i_eid, int rhs_i_eid, const char *filter1, const char *filter2)
{
  ecs::EntityId lhsEid = ecs::EntityId(lhs_i_eid);
  ecs::EntityId rhsEid = ecs::EntityId(rhs_i_eid);

  const char *lhsTemplName = g_entity_mgr->getEntityTemplateName(lhsEid);
  const char *rhsTemplName = g_entity_mgr->getEntityTemplateName(rhsEid);

  const ecs::ComponentsMap lhsComps = gather_all_components(lhsTemplName);
  const ecs::ComponentsMap rhsComps = gather_all_components(rhsTemplName);

  eastl::vector<std::string> filters;
  if (filter1)
    filters.push_back(filter1);
  if (filter2)
    filters.push_back(filter2);

  diff_systems(lhsComps, rhsComps, lhsTemplName, rhsTemplName, filters);
}

static void diff_templates(const char *lhs, const char *rhs)
{
  const ecs::ComponentsMap lhsComps = gather_all_components(lhs);
  const ecs::ComponentsMap rhsComps = gather_all_components(rhs);
  eastl::vector<eastl::string> lhsCompReports;
  eastl::vector<eastl::string> rhsCompReports;

  auto findMissingComps = [&](const ecs::ComponentsMap &lhsMaps, const ecs::ComponentsMap &rhsMaps,
                            eastl::vector<eastl::string> &reportTo) {
    for (auto lhsTcomp : lhsMaps)
    {
      bool exists = false;
      for (auto rhsTcomp : rhsMaps)
      {
        if (lhsTcomp.first == rhsTcomp.first)
        {
          exists = true;
          break;
        }
      }
      if (!exists)
      {
        const char *name = g_entity_mgr->getTemplateDB().getComponentName(lhsTcomp.first);
        reportTo.push_back(name);
      }
    }
  };

  findMissingComps(lhsComps, rhsComps, lhsCompReports);
  findMissingComps(rhsComps, lhsComps, rhsCompReports);

  eastl::vector<eastl::string> mismatchReports;
  for (auto lhsTcomp : lhsComps)
  {
    for (auto rhsTcomp : rhsComps)
    {
      if (lhsTcomp.first == rhsTcomp.first)
      {
        const char *name = g_entity_mgr->getTemplateDB().getComponentName(lhsTcomp.first);
        eastl::string lhsValue = component_to_string(name, lhsTcomp.second.getEntityComponentRef());
        eastl::string rhsValue = component_to_string(name, rhsTcomp.second.getEntityComponentRef());
        if (lhsValue != rhsValue)
        {
          mismatchReports.push_back("\t" + lhsValue + "\tVS\t" + rhsValue);
        }
        break;
      }
    }
  }

  report_diff_result(lhs, rhs, lhsCompReports, rhsCompReports, mismatchReports, lhsComps.size());
  diff_systems(lhsComps, rhsComps, lhs, rhs, eastl::vector<std::string>());
}

static void serialize_entity(int i_eid, const char *file_name)
{
  ecs::EntityId eid = ecs::EntityId(i_eid);
  eastl::vector<eastl::string> res;

  auto lhsIt = g_entity_mgr->getComponentsIterator(eid);
  const char *prev = nullptr;
  while (lhsIt)
  {
    if (prev != nullptr && strcmp(prev, (*lhsIt).first) == 0)
    {
      ++lhsIt;
      continue;
    }
    prev = (*lhsIt).first;
    res.push_back(component_to_string((*lhsIt).first, (*lhsIt).second));
    ++lhsIt;
  }
  const char *templName = g_entity_mgr->getEntityTemplateName(eid);
  debug("Components of <%d>(%s)", eid, templName);
  eastl::sort(res.begin(), res.end());
  String output(20 * res.size(), "");
  eastl::string prevStr = "";
  for (auto &str : res)
  {
    if (str == prevStr)
      continue;
    prevStr = str;
    debug(str.c_str());
    output += (str + "\n").c_str();
  }
  file_ptr_t h = ::df_open(file_name, DF_WRITE | DF_CREATE);
  if (!h)
    return;

  LFileGeneralSaveCB cb(h);
  cb.tryWrite(output.c_str(), output.size());
  ::df_close(h);
}

static void diff_entities(int lhs_i_eid, int rhs_i_eid)
{
  ecs::EntityId lhsEid = ecs::EntityId(lhs_i_eid);
  ecs::EntityId rhsEid = ecs::EntityId(rhs_i_eid);

  const char *lhsTemplName = g_entity_mgr->getEntityTemplateName(lhsEid);
  const char *rhsTemplName = g_entity_mgr->getEntityTemplateName(rhsEid);

  eastl::vector<eastl::string> lhsCompReports;
  eastl::vector<eastl::string> rhsCompReports;
  eastl::vector<eastl::string> valueMismatchReports;

  auto lhsIt = g_entity_mgr->getComponentsIterator(lhsEid);
  int lhsCompCount = 0;
  const char *prev = nullptr;
  while (lhsIt)
  {
    if (prev != nullptr && strcmp(prev, (*lhsIt).first) == 0)
    {
      ++lhsIt;
      continue;
    }
    prev = (*lhsIt).first;
    lhsCompCount++;
    bool exists = false;
    auto rhsIt = g_entity_mgr->getComponentsIterator(rhsEid);
    while (rhsIt)
    {
      if (strcmp((*lhsIt).first, (*rhsIt).first) == 0)
      {
        exists = true;
        eastl::string lhsValue = component_to_string((*lhsIt).first, (*lhsIt).second);
        eastl::string rhsValue = component_to_string((*rhsIt).first, (*rhsIt).second);
        if (lhsValue != rhsValue)
        {
          valueMismatchReports.push_back("\t" + lhsValue + "\tVS\t" + rhsValue);
        }
        break;
      }
      ++rhsIt;
    }
    if (!exists)
    {
      lhsCompReports.push_back(eastl::string((*lhsIt).first) + ": " + component_to_string((*lhsIt).first, (*lhsIt).second));
    }
    ++lhsIt;
  }

  auto rhsIt = g_entity_mgr->getComponentsIterator(rhsEid);
  while (rhsIt)
  {
    bool exists = false;
    auto lhsIt = g_entity_mgr->getComponentsIterator(lhsEid);
    while (lhsIt)
    {
      if (strcmp((*lhsIt).first, (*rhsIt).first) == 0)
      {
        exists = true;
        break;
      }
      ++lhsIt;
    }
    if (!exists)
    {
      rhsCompReports.push_back(eastl::string((*rhsIt).first) + ": " + component_to_string((*rhsIt).first, (*rhsIt).second));
    }
    ++rhsIt;
  }
  debug("Compare <%d>(%s) with <%d>(%s)", lhs_i_eid, lhsTemplName, rhs_i_eid, rhsTemplName);
  report_diff_result(lhsTemplName, rhsTemplName, lhsCompReports, rhsCompReports, valueMismatchReports, lhsCompCount);

  const ecs::ComponentsMap lhsComps = gather_all_components(lhsTemplName);
  const ecs::ComponentsMap rhsComps = gather_all_components(rhsTemplName);
  diff_systems(lhsComps, rhsComps, lhsTemplName, rhsTemplName, eastl::vector<std::string>());
}

static void who_can_access_component_of_template(const char *component_name, const char *template_name, bool rw_only = true)
{
  const ecs::ComponentsMap compMap = gather_all_components(template_name);
  bool templateHasComponent = false;
  ecs::component_t compNameId = 0;

  for (auto &c : compMap)
  {
    const char *name = g_entity_mgr->getTemplateDB().getComponentName(c.first);
    if (strcmp(name, component_name) == 0)
    {
      templateHasComponent = true;
      compNameId = c.first;
      break;
    }
  }

  if (!templateHasComponent)
  {
    console::print_d("Template %s does not contain component %s", template_name, component_name);
    return;
  }

  eastl::vector<const char *> outputSystems;

  auto systems = g_entity_mgr->getSystems();
  for (const ecs::EntitySystemDesc *s : systems)
  {
    for (auto &c : s->componentsRW)
    {
      if (c.name == compNameId)
      {
        if (check_query_processes_template(s, compMap))
          outputSystems.push_back(s->name);
        break;
      }
    }
    if (!rw_only)
    {
      for (auto &c : s->componentsRO)
      {
        if (c.name == compNameId)
        {
          if (check_query_processes_template(s, compMap))
            outputSystems.push_back(s->name);
          break;
        }
      }
    }
  }


  eastl::vector<std::string> outputQueries;

  uint32_t queryCount = g_entity_mgr->getQueriesCount();
  for (uint32_t i = 0; i < queryCount; i++)
  {
    ecs::QueryId qid = g_entity_mgr->getQuery(i);
    const ecs::BaseQueryDesc qd = g_entity_mgr->getQueryDesc(qid);
    for (auto &c : qd.componentsRW)
    {
      if (c.name == compNameId)
      {
        if (check_query_processes_template(&qd, compMap))
          outputQueries.push_back(to_searchable_querry_name(std::string(g_entity_mgr->getQueryName(qid))));
        break;
      }
    }
    if (!rw_only)
    {
      for (auto &c : qd.componentsRO)
      {
        if (c.name == compNameId)
        {
          if (check_query_processes_template(&qd, compMap))
            outputQueries.push_back(to_searchable_querry_name(std::string(g_entity_mgr->getQueryName(qid))));
          break;
        }
      }
    }
  }

  console::print_d(""); // formatting

  if (outputSystems.size() > 0)
    console::print_d("Systems:");
  for (auto name : outputSystems)
    console::print_d("\t%s", name);

  if (outputQueries.size() > 0)
    console::print_d("Queries:");
  for (auto name : outputQueries)
    console::print_d("\t%s", name.c_str());

  console::print_d("Systems: %d queries: %d.", outputSystems.size(), outputQueries.size());
}

static void who_modifies_tracked_components_of_template(const char *template_name)
{
  const ecs::ComponentsMap compMap = gather_all_components(template_name);

  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(template_name);
  if (!templ)
    return;
  eastl::vector_set<ecs::component_t> tracked = gather_all_tracked(templ);

  console::print_d(""); // formatting
  console::print_d("%d Tracked components:", tracked.size());
  for (auto &c : tracked)
  {
    const char *name = g_entity_mgr->getTemplateDB().getComponentName(c);
    console::print_d("\t%s", name);
  }

  auto componentsToStr = [](eastl::vector<ecs::component_type_t> trackedComps) {
    String result;
    for (auto &c : trackedComps)
    {
      const char *name = g_entity_mgr->getTemplateDB().getComponentName(c);
      if (result.size() > 0)
        result.append(String(0, ", %s", name));
      else
        result.append(String(0, "%s", name));
    }
    return result;
  };

  eastl::vector<String> outputSystems;
  auto systems = g_entity_mgr->getSystems();
  for (const ecs::EntitySystemDesc *s : systems)
  {
    eastl::vector<ecs::component_type_t> trackedComps;
    for (auto &c : s->componentsRW)
    {
      if (tracked.find(c.name) != tracked.end())
        trackedComps.push_back(c.name);
    }
    if (trackedComps.size() > 0)
    {
      if (!check_query_processes_template(s, compMap))
        continue;
      outputSystems.push_back(String(0, "%s\n%s", s->name, componentsToStr(trackedComps).c_str()));
    }
  }

  eastl::vector<String> outputQueries;
  uint32_t queryCount = g_entity_mgr->getQueriesCount();
  for (uint32_t i = 0; i < queryCount; i++)
  {
    ecs::QueryId qid = g_entity_mgr->getQuery(i);
    const ecs::BaseQueryDesc qd = g_entity_mgr->getQueryDesc(qid);
    eastl::vector<ecs::component_type_t> trackedComps;
    for (auto &c : qd.componentsRW)
    {
      if (tracked.find(c.name) != tracked.end())
        trackedComps.push_back(c.name);
    }
    if (trackedComps.size() > 0)
    {
      if (!check_query_processes_template(&qd, compMap))
        continue;
      auto queryName = to_searchable_querry_name(std::string(g_entity_mgr->getQueryName(qid)));
      outputQueries.push_back(String(0, "%s\n%s", queryName.c_str(), componentsToStr(trackedComps).c_str()));
    }
  }

  console::print_d(""); // formatting

  if (outputSystems.size() > 0)
    console::print_d("Systems:");
  for (auto name : outputSystems)
    console::print_d("\t%s", name);

  if (outputQueries.size() > 0)
    console::print_d("Queries:");
  for (auto name : outputQueries)
    console::print_d("\t%s", name.c_str());

  console::print_d("Systems: %d queries: %d.", outputSystems.size(), outputQueries.size());
}

static constexpr ecs::ComponentDesc iterate_through_all_entities_ecs_query_comps[] = {
  // start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}};
static ecs::CompileTimeQueryDesc iterate_through_all_entities_ecs_query_desc("iterate_through_all_entities_ecs_query", empty_span(),
  make_span(iterate_through_all_entities_ecs_query_comps + 0, 1) /*ro*/, empty_span(), empty_span());
template <typename Callable>
inline void iterate_through_all_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, iterate_through_all_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView &__restrict components) {
      auto comp = components.begin(), compE = components.end();
      G_ASSERT(comp != compE);
      do
      {
        function(ECS_RO_COMP(iterate_through_all_entities_ecs_query_comps, "eid", ecs::EntityId));

      } while (++comp != compE);
    });
}

static void search_for_eid_in_entities(int raw_eid)
{
  ecs::EntityId parsedEid = ecs::EntityId(raw_eid);
  int count = 0;
  iterate_through_all_entities_ecs_query([&](ecs::EntityId eid) {
    G_UNUSED(eid);
    count++;
  });
  debug("Total entities count is %d", count);
  int matchCount = 0;
  iterate_through_all_entities_ecs_query([&](ecs::EntityId eid) {
    auto compsIter = g_entity_mgr->getComponentsIterator(eid);
    const char *prev = nullptr;
    while (compsIter)
    {
      if (prev != nullptr && strcmp(prev, (*compsIter).first) == 0)
      {
        ++compsIter;
        continue;
      }
      prev = (*compsIter).first;
      if ((*compsIter).second.is<ecs::EntityId>())
      {
        if ((*compsIter).second.get<ecs::EntityId>() == parsedEid)
        {
          debug("Match: <%d>(%s):\t%s", eid, g_entity_mgr->getEntityTemplateName(eid), (*compsIter).first);
          matchCount++;
        }
      }
      ++compsIter;
    };
  });
  debug("Finished searching entities. Match count: %d", matchCount);
}

struct ParentStats
{
  ecs::ComponentsMap uniqueComps;
  eastl::vector<ecs::component_t> overwrites;
  int totalParents;
  int parents;
  int totalComps;
  int comps;
  int depth;
  int originIdx;
  uint32_t templId;

  ParentStats(int _depth, int _originIdx, uint32_t _templId) :
    totalParents(0), parents(0), totalComps(0), comps(0), depth(_depth), originIdx(_originIdx), templId(_templId){};
};

static void explore_template_routine(uint32_t templ_id, eastl::vector<ParentStats> &all_stats, int origin_idx, int depth = 0)
{
  int idx = all_stats.size();
  all_stats.emplace_back(depth, origin_idx, templ_id);
  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateById(templ_id);
  dag::ConstSpan<uint32_t> parents = templ->getParents();

  for (const uint32_t p : parents)
    explore_template_routine(p, all_stats, idx, depth + 1);

  ParentStats &stats = all_stats[idx];
  stats.parents = parents.size();
  stats.totalParents = parents.size();
  const ecs::ComponentsMap &thisComps = templ->getComponentsMap();
  stats.totalComps = thisComps.size();
  stats.comps = thisComps.size();
  if (parents.size() > 0)
  {
    for (int i = idx + 1; i < all_stats.size(); ++i)
    {
      if (all_stats[i].originIdx == idx)
      {
        stats.totalComps += all_stats[i].totalComps;
        stats.totalParents += all_stats[i].totalParents;
        for (auto &c : all_stats[i].uniqueComps)
        {
          stats.uniqueComps[c.first] = c.second;
        }
      }
      if (all_stats[i].depth <= depth) // we're in a parallel branch, this sub-tree is fully explored.
        break;
    }
  }
  for (auto &c : thisComps)
  {
    if (!stats.uniqueComps[c.first].isNull())
      stats.overwrites.push_back(c.first);
    stats.uniqueComps[c.first] = c.second;
  }
}

static void print_explored_template_routine(const eastl::vector<ParentStats> &all_stats, int idx)
{
  String indentation("\t");
  for (int i = 0; i < all_stats[idx].depth; ++i)
    indentation += "\t";
  String parentsInfo = String(0, "[P: %d / %d]", all_stats[idx].parents, all_stats[idx].totalParents);
  String componentInfo =
    all_stats[idx].totalComps == all_stats[idx].comps
      ? String(0, "[C:(%d)]", all_stats[idx].comps)
      : String(0, "[C: %d / (%d) / %d]", all_stats[idx].comps, all_stats[idx].uniqueComps.size(), all_stats[idx].totalComps);

  debug("%s%s%s %s", indentation, all_stats[idx].totalParents > 0 ? parentsInfo : String(""), componentInfo,
    g_entity_mgr->getTemplateDB().getTemplateById(all_stats[idx].templId)->getName());

  for (int i = idx + 1; i < all_stats.size(); ++i)
  {
    if (all_stats[i].originIdx == idx)
      print_explored_template_routine(all_stats, i);
    if (all_stats[i].depth <= all_stats[idx].depth) // we're in a parallel branch, this sub-tree is fully explored.
      break;
  }
}

static void explore_template_structure(const char *template_name)
{
  if (!template_name)
  {
    console::print_d("Template name is null");
    return;
  }

  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(template_name);
  if (!templ)
  {
    console::print_d("Template '%s' not found", template_name);
    return;
  }

  eastl::vector<ParentStats> allStats;
  explore_template_routine(g_entity_mgr->getTemplateDB().getTemplateIdByName(template_name), allStats, 0);
  debug("NB: This does not account for shared components.");
  debug("[P: this_parents / total_parents] [C: this_comps / (unique_comps_so_far) / total_comps] templateName");
  print_explored_template_routine(allStats, 0);
}

#endif

struct CountEcsObjects
{
  int objs = 0, objs_children = 0, arrays = 0, arrays_children = 0;
};
static void recursive_count(const ecs::Object &v, CountEcsObjects &c);
static void recursive_count(const ecs::Array &v, CountEcsObjects &c);

static void recursive_count(const ecs::Object &v, CountEcsObjects &c)
{
  c.objs++;
  c.objs_children += v.size();
  for (auto &i : v)
  {
    if (i.second.is<ecs::Object>())
      recursive_count(i.second.get<ecs::Object>(), c);
    if (i.second.is<ecs::Array>())
      recursive_count(i.second.get<ecs::Array>(), c);
  }
}

static void recursive_count(const ecs::Array &v, CountEcsObjects &c)
{
  c.arrays++;
  c.arrays_children += v.size();
  for (auto &i : v)
  {
    if (i.is<ecs::Object>())
      recursive_count(i.get<ecs::Object>(), c);
    if (i.is<ecs::Array>())
      recursive_count(i.get<ecs::Array>(), c);
  }
}

static bool ecs_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  if (!g_entity_mgr)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("ecs", "dump_mem", 1, 1)
  {
    size_t memory = g_entity_mgr->dumpMemoryUsage();
    console::print_d("ecs memory usage is %d (see log for more info)", memory);
  }
  CONSOLE_CHECK_NAME("ecs", "unused_components", 1, 1)
  {
    eastl::vector_set<ecs::component_t> used;
    auto add = [&](auto &cd) {
      for (auto &c : cd)
        used.emplace(c.name);
    };
    for (uint32_t i = 0, e = g_entity_mgr->getQueriesCount(); i != e; ++i)
    {
      ecs::BaseQueryDesc q = g_entity_mgr->getQueryDesc(g_entity_mgr->getQuery(i));
      add(q.componentsRW);
      add(q.componentsRO);
      add(q.componentsRQ);
      add(q.componentsNO);
    }
    uint32_t templatesWithUnused = 0;
    eastl::vector_map<ecs::component_t, eastl::pair<eastl::string, uint32_t>> unusedCount;
    eastl::string info, missed;
    for (uint32_t i = 0, e = g_entity_mgr->getTemplateDB().size(); i != e; ++i)
    {
      auto *t = g_entity_mgr->getTemplateDB().getTemplateById(i);
      if (!t)
        continue;
      auto &components = t->getComponentsMap();
      missed.clear();
      for (auto it = components.begin(), end = components.end(); it != end; ++it)
      {
        ecs::component_t c = components.getHash(it);
        if (used.find(c) == used.end())
        {
          auto &u = unusedCount[c];
          u.second++;
          const char *cName = g_entity_mgr->getTemplateDB().getComponentName(it->first);
          if (u.first.empty())
            u.first = cName;
          missed += " " + u.first;
        }
      }
      if (missed.length())
      {
        info.append_sprintf("template <%s> has unused components:\n%s\n", t->getName(), missed.c_str());
        templatesWithUnused++;
      }
    }
    if (!unusedCount.empty())
    {
      console::print_d("%d total components never used anywhere, %d templates with unused components. see log for details",
        unusedCount.size(), templatesWithUnused);
      for (auto &c : unusedCount)
        debug("%s, used in %d templates", c.second.first.c_str(), c.second.second);
      debug("%d templates with missing components:\n%s", templatesWithUnused, info.c_str());
    }
  }
  CONSOLE_CHECK_NAME("ecs", "unused_queries", 1, 1)
  {
    eastl::vector_set<ecs::component_t> used;
    used.emplace(ECS_HASH("eid").hash);
    for (uint32_t i = 0, e = g_entity_mgr->getTemplateDB().size(); i != e; ++i)
    {
      auto *t = g_entity_mgr->getTemplateDB().getTemplateById(i);
      if (!t)
        continue;
      auto &components = t->getComponentsMap();
      for (auto it = components.begin(), end = components.end(); it != end; ++it)
        used.emplace(components.getHash(it));
    }
    eastl::string missing;
    auto check = [&](auto &cd) {
      for (auto &c : cd)
        if (!(c.flags & ecs::CDF_OPTIONAL) && used.find(c.name) == used.end())
#if DAGOR_DBGLEVEL > 0
          missing.append_sprintf(" %s(0x%X)", c.nameStr, c.name);
#else
          missing.append_sprintf(" 0x%X", c.name);
#endif
    };
    auto check2 = [&](auto &q) {
      check(q.componentsRW);
      check(q.componentsRO);
      check(q.componentsRQ);
    };
    uint32_t unusedES = 0;
    for (auto es : g_entity_mgr->getSystems())
    {
      missing.clear();
      if (es)
        check2(*es);
      if (!missing.empty())
      {
        debug("EntitySystem <%s> rely on components%s, which is not available in any template", es->name, missing.c_str());
        unusedES++;
      }
      // this system/query has zero chance to run
    }

    uint32_t unused = 0;
    for (uint32_t i = 0, e = g_entity_mgr->getQueriesCount(); i != e; ++i)
    {
      missing.clear();
      check2(g_entity_mgr->getQueryDesc(g_entity_mgr->getQuery(i)));
      if (!missing.empty())
      {
        debug("query <%s> rely on components%s, which is not available in any template",
          g_entity_mgr->getQueryName(g_entity_mgr->getQuery(i)), missing.c_str());
        unused++;
      }
      // this system/query has zero chance to run
    }
    console::print_d("%d total queries (%d systems) that could not be used anywhere, see log for details.", unused, unusedES);
  }
  CONSOLE_CHECK_NAME("ecs", "remove_template", 1, 2)
  {
    if (argc < 2)
      console::print("syntax is: ecs.remove_template template_name_to_remove");
    else
    {
      auto ret = g_entity_mgr->removeTemplate(argv[1]);
      console::print_d("'%s' template %sremoved%s.", argv[1],
        ret != ecs::EntityManager::RemoveTemplateResult::Removed ? "can not be " : "",
        ret == ecs::EntityManager::RemoveTemplateResult::HasEntities
          ? " as it used by existing entities"
          : (ret == ecs::EntityManager::RemoveTemplateResult::NotFound ? " as it is unknown" : ""));
    }
  }
  CONSOLE_CHECK_NAME("ecs", "defrag_templates", 1, 1)
  {
    int64_t reft = profile_ref_ticks();
    int ret = g_entity_mgr->defragTemplates();
    console::print_d("%d unused templates freed in %dus", ret, profile_time_usec(reft));
  }
  CONSOLE_CHECK_NAME("ecs", "defrag_archetypes", 1, 1)
  {
    debug_flush(true);
    int64_t reft = profile_ref_ticks();
    int ret = g_entity_mgr->defragArchetypes();
    console::print_d("%d unused archetypes freed in %dus", ret, profile_time_usec(reft));
  }
  CONSOLE_CHECK_NAME("ecs", "archetypes_sizes", 1, 2)
  {
    g_entity_mgr->dumpArchetypes(argc > 1 ? atoi(argv[1]) : 10);
    console::print("(see log for more info)");
  }
  CONSOLE_CHECK_NAME("ecs", "dump_arch", 2, 2)
  {
    if (g_entity_mgr->dumpArchetype(atoi(argv[1])))
      console::print("dumped, see log for more info");
    else
      console::print_d("no such archetype");
  }
  CONSOLE_CHECK_NAME("ecs", "dump_events", 1, 1)
  {
    g_entity_mgr->getEventsDb().dump();
    console::print("Events DB dumped to log.");
    for (auto s : g_entity_mgr->getSystems())
      for (auto ev : s->getEvSet())
        if (g_entity_mgr->getEventsDb().findEvent(ev) == g_entity_mgr->getEventsDb().invalid_event_id)
          logerr("EntitySystem <%s>, module <%s> is subscribed for event <0x%X>, which is not registered", s->name, s->getModuleName(),
            ev);
  }
  CONSOLE_CHECK_NAME("ecs", "multi_threading", 1, 2)
  {
    if (argc > 1)
      g_entity_mgr->setMaxUpdateJobs(to_int(argv[1]));
    else
      console::print("ecs.multi_threading <max_num_jobs> (will be limited to existent workers)");
  }
  CONSOLE_CHECK_NAME("ecs", "set_query_quant", 1, 3)
  {
    if (argc > 2)
      ecs::iterate_compile_time_queries([&](ecs::CompileTimeQueryDesc *d) {
        if (strcmp(d->name, argv[1]) != 0)
          return false;
        d->setQuant(atoi(argv[2]));
        return true;
      });
    else
      console::print("ecs.set_query_quant <query_name> <parallization_chunk>");
  }
  CONSOLE_CHECK_NAME("ecs", "update_component", 1, 3)
  {
    if (argc > 2)
    {
      ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(ECS_HASH_SLOW(argv[2]).hash);
      ecs::EntityComponentRef eref = g_entity_mgr->getComponentRefRW(ecs::EntityId(atoi(argv[1])), cidx);
      if (eref.isNull())
        return found;
      // call changeGen() implicitly via non const accessors methods
      else if (eref.is<ecs::Object>())
        eref.get<ecs::Object>().begin();
      else if (eref.is<ecs::Array>())
        eref.get<ecs::Array>().begin();
#define DECL_LIST_TYPE(lt, t) \
  if (eref.is<ecs::lt>())     \
    eref.get<ecs::lt>().begin();
      ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
    }
    else
      console::print("ecs.update_component <eid> <componentName>. Causes tracking change");
  }

  CONSOLE_CHECK_NAME("ecs", "dump_templates", 1, 1)
  {
    auto &templates = g_entity_mgr->getTemplateDB();
    dag::Vector<uint32_t> templOrder(templates.size());
    for (int i = 0, e = templOrder.size(); i < e; ++i)
      templOrder[i] = i;
    eastl::sort(templOrder.begin(), templOrder.end(), [&](uint32_t a, uint32_t b) {
      return strcmp(templates.data().begin()[a].getName(), templates.data().begin()[b].getName()) < 0;
    });
    size_t hash = 1;
    uint32_t cmpnts = 0, tracked = 0, repl = 0, ignored = 0;
    eastl::string str;
    for (auto ti : templOrder)
    {
      auto &t = templates.data().begin()[ti];
      auto thash = t.buildHash();
      combine_hash(hash, thash);
      str.append_sprintf("template <%s:%s>: hash=0x%X, %d components", t.getName(), t.getPath(), thash, t.componentsCount());
      cmpnts += t.componentsCount();
      tracked += t.trackedSet().size();
      repl += t.replicatedSet().size();
      ignored += t.ignoredSet().size();
      auto printList = [&](const char *n, const ecs::Template::ComponentsSet &s) {
        if (s.empty())
          return;
        str.append_sprintf(", %s(%d) : ", n, s.size());
        for (auto ti : s)
          str.append_sprintf(" %s", templates.getComponentName(ti));
      };
      printList("tracked", t.trackedSet());
      printList("replicated", t.replicatedSet());
      printList("ignored", t.ignoredSet());
      str.append_sprintf("\n");
    }
    console::print_d("%d total templates = %d(%d unique) tracked = %d repl = %d ignored = %d, hash 0x%X.\nSee log for more info.",
      templates.size(), cmpnts, templates.data().componentToName.size(), tracked, repl, ignored, hash);
    debug(str.c_str());
  }

  CONSOLE_CHECK_NAME("ecs", "dump_template", 1, 2)
  {
    if (argc > 1)
    {
      auto &templates = g_entity_mgr->getTemplateDB();
      const char *name = argv[1];
      int ti = templates.data().getTemplateIdByName(name);
      if (ti < 0)
      {
        const size_t len = strlen(name);
        for (int i = 0, e = templates.data().size(); i < e; ++i)
          if (strncmp(templates.data().begin()[i].getName(), name, len) == 0)
          {
            ti = i;
            break;
          }
      }
      if (ti >= 0)
      {
        auto &t = templates.data().begin()[ti];
        name = t.getName();
        auto thash = t.buildHash();
        eastl::string str;
        str.append_sprintf("template <%s:%s>: hash=0x%X,\ncomponents(%d) :", t.getName(), t.getPath(), thash, t.componentsCount());
        for (auto c : t.getComponentsMap())
          str.append_sprintf(" %s:0x%X", templates.data().getComponentName(c.first), templates.data().getComponentType(c.first));
        str.append_sprintf(")");
        auto printList = [&](const char *n, const ecs::Template::ComponentsSet &s) {
          if (s.empty())
            return;
          str.append_sprintf("\n%s(%d) : ", n, s.size());
          for (auto ti : s)
            str.append_sprintf(" %s", templates.getComponentName(ti));
        };
        printList("tracked", t.trackedSet());
        printList("replicated", t.replicatedSet());
        printList("ignored", t.ignoredSet());
        str.append_sprintf("\n");
        debug("template info:\n%s", str.c_str());
        console::print("template<%s> info dumped to log", name);
      }
      else
      {
        console::print_d("Unknown template <%s>", name);
      }
    }
    else
      console::print("usage: ecs.dump_template <template_name>");
  }

  CONSOLE_CHECK_NAME("ecs", "dump_systems", 1, 10)
  {
    eastl::fixed_vector<ecs::component_t, 10, true> comps;
    for (int argNo = 1; argNo < argc; ++argNo)
    {
      const ecs::component_t comp = ECS_HASH_SLOW(argv[argNo]).hash;
      if (g_entity_mgr->getDataComponents().findComponentId(comp) != ecs::INVALID_COMPONENT_INDEX)
        comps.push_back(comp);
      else
        console::print_d("Component '%s' not found", argv[argNo]);
    }

    auto isInList = [&](dag::ConstSpan<ecs::ComponentDesc> es_comps) {
      for (const auto &esComp : es_comps)
        if (eastl::find(comps.begin(), comps.end(), esComp.name) != comps.end())
          return true;
      return false;
    };

    auto shouldPrint = [&](const ecs::EntitySystemDesc *s) {
      return comps.empty() || isInList(s->componentsRO) || isInList(s->componentsRW);
    };

    int totalCount = 0;
    debug("dumping entity systems");
    for (auto s : g_entity_mgr->getSystems())
      if (shouldPrint(s))
      {
        ++totalCount;
        debug("%s", s->name);
      }
    console::print("%d ecs systems (see log for more info)", totalCount);
  }
  CONSOLE_CHECK_NAME("ecs", "dump_components", 1, 1)
  {
    auto &dc = g_entity_mgr->getDataComponents();
    int cnt = 0;
    for (; dc.getComponentTpById(cnt); ++cnt)
    {
      auto c = dc.getComponentById(cnt);
      debug("%d: %s of type 0x%X%s", cnt, dc.getComponentNameById(cnt), c.componentTypeName,
        c.flags & ecs::DataComponent::IS_COPY ? " copy" : "");
    }
    console::print("%d ecs components (see log for more info)", cnt);
  }

  CONSOLE_CHECK_NAME("ecs", "track_component_access", 2, 2)
  {
    ecs::component_t comp = ECS_HASH_SLOW(argv[1]).hash;
    ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(comp);
    if (cidx == ecs::INVALID_COMPONENT_INDEX)
      console::print_d("Component '%s' not found", argv[1]);
    else
      g_entity_mgr->startTrackComponent(comp);
  }

  CONSOLE_CHECK_NAME("ecs", "stop_track_component_access", 1, 1) { g_entity_mgr->stopTrackComponentsAndDump(); }

  CONSOLE_CHECK_NAME("ecs", "createEntityRaw", 1, 2)
  {
    if (argc < 2)
      console::print("Syntax: ecs.createEntityRaw <template_name>.");
    else
    {
      auto eid = g_entity_mgr->createEntitySync(argv[1]);
      if (eid)
        console::print_d("entityId=%d of template <%s> created.", (ecs::entity_id_t)(eid), argv[1]);
      else
        console::print_d("entityId of template <%s> could not be created.", argv[1]);
    }
  }

  /*CONSOLE_CHECK_NAME("ecs", "enable_validation", 1, 2)
  {
    if (argc>1)
      g_entity_mgr->enableQueriesValidation(to_bool(argv[1]));
    else
      console::print("will dis(en)able queries validation. syntax enable_validation off|on");
  }
  CONSOLE_CHECK_NAME("ecs", "system_info", 1, 3)
  {
    if (argc>1)
    {
      int id = g_entity_mgr->getSystemId(argv[1]);
      if (id<0)
        console::print_d("systems %s not found!", argv[1]);//todo print 'like'
      else
      {
        console::print_d("%s: isUsed = %s, entities: %d",
          g_entity_mgr->getSystemInfo(id)->name, g_entity_mgr->isUsed(id) ? "yes":"no",
  g_entity_mgr->getSystemInfoEntitiesQueried(id)); if (argc>2)
        {
          g_entity_mgr->setUsed(id, to_bool(argv[2]));
          console::print_d("isUsedNow = %s",g_entity_mgr->isUsed(id) ? "yes":"no");
        }
      }
    }
    else
    {
      console::print_d("systems %d", g_entity_mgr->getNumSystems());
      for (int id = 0; id < g_entity_mgr->getNumSystems(); ++id)
        console::print_d("%s: isUsed = %s, entities: %d",
         g_entity_mgr->getSystemInfo(id)->name, g_entity_mgr->isUsed(id) ? "yes":"no", g_entity_mgr->getSystemInfoEntitiesQueried(id));
    }
  }*/
  CONSOLE_CHECK_NAME("ecs", "print_filter_tags", 1, 1)
  {
    const ecs::TemplateDB &tplDb = g_entity_mgr->getTemplateDB();
    const ecs::TemplateDBInfo &tplDbinfo = tplDb.info();
    for (const auto &tag : tplDbinfo.filterTags)
      console::print_d("0x%x (%u)", tag, tag);
  }
  CONSOLE_CHECK_NAME("ecs", "biggest_object", 1, 1)
  {
    const ecs::Object *biggestObj = nullptr;
    const char *name = nullptr;
    int total_objects = 0, total_objects_children = 0;
    CountEcsObjects counters;
    all_entities_query([&](ecs::EntityId eid) {
      for (auto aiter = g_entity_mgr->getComponentsIterator(eid); aiter; ++aiter)
      {
        const auto &comp = (*aiter).second;

        ecs::component_flags_t flags = g_entity_mgr->getDataComponents().getComponentById(comp.getComponentId()).flags;
        if (comp.is<ecs::Object>())
        {
          const ecs::Object &obj = comp.get<ecs::Object>();
          recursive_count(obj, counters);
          total_objects++;
          total_objects_children += obj.size();
        }
        if (comp.is<ecs::Array>())
          recursive_count(comp.get<ecs::Array>(), counters);

        if (flags & ecs::DataComponent::IS_COPY)
          continue;
        if (comp.is<ecs::Object>())
        {
          const ecs::Object &obj = comp.get<ecs::Object>();
          if (!biggestObj || biggestObj->size() < obj.size())
          {
            biggestObj = &obj;
            name = (*aiter).first;
          }
        }
      }
    });
    console::print_d("there are %d objects with %d children total", total_objects, total_objects_children);
    console::print_d("in hetero containers there are %d children total, %d objects with %d children and %d arrays with %d children",
      counters.arrays_children + counters.objs_children, counters.objs, counters.objs_children, counters.arrays,
      counters.arrays_children);
    if (biggestObj)
    {
      console::print("biggest object size is %d component %s, dumping to log...", biggestObj->size(), name);
      for (auto &a : *biggestObj)
        debug("%s", a.first.c_str());
    }
  }
  CONSOLE_CHECK_NAME("ecs", "entity_histogram", 1, 2)
  {
    eastl::vector_map<const char *, int> hist;
    int totalEntities = 0;
    all_entities_query([&](ecs::EntityId eid) {
      hist.emplace(g_entity_mgr->getEntityTemplateName(eid), 0).first->second++;
      totalEntities++;
    });
    int ndump = argc > 1 ? atoi(argv[1]) : 20;
    using HistRec = eastl::pair<const char *, int>;
    eastl::sort(hist.begin(), hist.end(), [](const HistRec &a, const HistRec &b) { return b.second < a.second; });
    console::print_d("Dump histogram of total #%d entities of #%d distinct templates:", totalEntities, hist.size());
    for (int i = 0, n = ndump ? eastl::min<int>(hist.size(), ndump) : hist.size(); i < n; ++i)
      console::print_d("%4d %5d %s", i + 1, hist.begin()[i].second, hist.begin()[i].first);
  }
#if ECS_HAS_PERF_MARKERS
  CONSOLE_CHECK_NAME("ecs", "enableEventsMarkers", 1, 2)
  {
    ecs::events_perf_markers = argc < 2 ? !ecs::events_perf_markers : to_bool(argv[1]);
    console::print_d("events_perf_markers is %s now", ecs::events_perf_markers ? "on" : "off");
  }
#endif
#if DAGOR_DBGLEVEL > 0
  CONSOLE_CHECK_NAME("ecs", "does_system_process_template", 3, 3) { does_system_process_template(argv[1], argv[2]); }
  CONSOLE_CHECK_NAME("ecs", "does_system_process_entity", 3, 3)
  {
    ecs::EntityId eid = ecs::EntityId(atoi(argv[2]));
    does_system_process_template(argv[1], g_entity_mgr->getEntityTemplateName(eid));
  }
  CONSOLE_CHECK_NAME("ecs", "does_system_process_selected_entity", 2, 2)
  {
    ecs::EntityId selectedEid = query_selected_eid();
    if (selectedEid == ecs::INVALID_ENTITY_ID)
      console::print_d("No entity is selected");
    else
      does_system_process_template(argv[1], g_entity_mgr->getEntityTemplateName(selectedEid));
  }
  CONSOLE_CHECK_NAME("ecs", "diff_templates", 3, 3) { diff_templates(argv[1], argv[2]); }
  CONSOLE_CHECK_NAME("ecs", "diff_entities", 3, 3) { diff_entities(atoi(argv[1]), atoi(argv[2])); }
  CONSOLE_CHECK_NAME("ecs", "who_can_change_component_of_template", 3, 3) { who_can_access_component_of_template(argv[1], argv[2]); }
  CONSOLE_CHECK_NAME("ecs", "who_can_access_component_of_template", 3, 3)
  {
    who_can_access_component_of_template(argv[1], argv[2], false);
  }
  CONSOLE_CHECK_NAME("ecs", "diff_systems_for_eids", 3, 5)
  {
    diff_systems_for_eids(atoi(argv[1]), atoi(argv[2]), argc > 3 ? argv[3] : nullptr, argc > 4 ? argv[4] : nullptr);
  }
  CONSOLE_CHECK_NAME("ecs", "search_for_eid_in_entities", 2, 2) { search_for_eid_in_entities(atoi(argv[1])); }
  CONSOLE_CHECK_NAME("ecs", "who_modifies_tracked_components_of_template", 2, 2)
  {
    who_modifies_tracked_components_of_template(argv[1]);
  }
  CONSOLE_CHECK_NAME("ecs", "explore_template_structure", 2, 2) { explore_template_structure(argv[1]); }
  CONSOLE_CHECK_NAME("ecs", "serialize_entity", 3, 3) { serialize_entity(atoi(argv[1]), argv[2]); }
#endif
  return found;
}

REGISTER_CONSOLE_HANDLER(ecs_console_handler);

EA_RESTORE_VC_WARNING()
