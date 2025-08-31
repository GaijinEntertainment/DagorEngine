// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "templateFilters.h"
#include <daECS/net/component_replication_filter.h>
#include <daECS/net/compBlacklist.h>
#include <daECS/core/entityManager.h>
#include <ioSys/dag_dataBlock.h>

void read_component_filters(const DataBlock &blk, ComponentToFilterAndPath &to)
{
  const char *path = blk.resolveFilename();
  if (!path)
    path = "";
  const bool allowOverride = blk.getBool("_override", false);
  for (int pi = 0, pe = blk.paramCount(); pi < pe; ++pi)
  {
    if (blk.getParamType(pi) != DataBlock::TYPE_STRING)
      continue;
    eastl::string componentName = blk.getParamName(pi);
    const char *filterName = blk.getStr(pi);
    auto cIt = to.find(componentName);
    if (cIt != to.end() && !allowOverride && cIt->second.first != filterName)
    {
      logerr("replication filter <%s> for component <%s> - is already set in %s to different filter %s", filterName,
        componentName.c_str(), cIt->second.second.c_str(), cIt->second.first.c_str());
    }
    else
      to[eastl::move(componentName)] = eastl::pair<eastl::string, eastl::string>(filterName, path);
  }
}

void apply_component_filters(const ecs::EntityManager &mgr, const ComponentToFilterAndPath &flist)
{
  for (auto &f : flist)
  {
    const char *componentName = f.first.c_str();
    const char *filterName = f.second.first.c_str();
    ecs::component_t component = ECS_HASH_SLOW(componentName).hash;
#if DAGOR_DBGLEVEL > 0
    const char *path = f.second.second.c_str();
    ecs::component_index_t cidx = mgr.getDataComponents().findComponentId(component);
    if (cidx == ecs::INVALID_COMPONENT_INDEX)
    {
      if (mgr.getTemplateDB().getComponentType(component) == 0)
        logerr("component <%s|0x%X> does not exist in templateDB %s and so won't be filtered with %s", componentName, component, path,
          filterName);
    }
#endif
    net::replicate_component_filter_index_t fit = net::find_component_filter(filterName);
    if (fit == net::invalid_filter_id)
      logerr("can not set replication filter <%s> for component <%s> - no such filter", filterName, componentName);
    else
    {
      net::replicate_component_filter_index_t cfit = net::find_component_filter_for_component(mgr, component);
      if (cfit == fit)
        continue;
      if (cfit != net::replicate_everywhere_filter_id)
        logerr("replication filter <%s> for component <%s> - is already set to different filter %d", filterName, componentName,
          net::get_component_filter_name(cfit));
      debug("set replication filter <%s> for component <%s>", filterName, componentName);
      net::set_component_filter(mgr, component, fit);
    }
  }
}

void read_replicated_component_client_modify_blacklist(const DataBlock &blk, ComponentToFilterAndPath &to)
{
  const char *path = blk.resolveFilename();
  if (!path)
    path = "";
  for (int bi = 0, be = blk.blockCount(); bi < be; ++bi)
  {
    to.emplace(blk.getBlock(bi)->getBlockName(), eastl::pair<eastl::string, eastl::string>("", path));
  }
}

void apply_replicated_component_client_modify_blacklist(const ecs::EntityManager &mgr, const ComponentToFilterAndPath &flist)
{
  G_UNUSED(mgr);
  for (auto &f : flist)
  {
    const char *componentName = f.first.c_str();
    ecs::component_t component = ECS_HASH_SLOW(componentName).hash;
#if DAGOR_DBGLEVEL > 0
    ecs::component_index_t cidx = mgr.getDataComponents().findComponentId(component);
    if ((cidx == ecs::INVALID_COMPONENT_INDEX) && (mgr.getTemplateDB().getComponentType(component) == 0))
      logerr("component <%s|0x%X> does not exist in templateDB %s and so won't "
             "be added to replicated component client modify blacklist",
        componentName, component, f.second.second.c_str());
    else
#endif
      net::replicated_component_client_modify_blacklist_add(component);
  }
}