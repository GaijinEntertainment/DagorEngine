// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <frontend/internalRegistry.h>
#include <id/idNameResolver.h>
#include <memory/dag_framemem.h>


namespace dafg
{

struct FrontendRecompilationData;

// Encapsulates our now non-trivial name resolution logic, both
// lookup in parent namespaces and slot resolution.
class NameResolver
{
public:
  NameResolver(const InternalRegistry &reg, FrontendRecompilationData &recompData) :
    registry{reg}, frontendRecompilationData{recompData}
  {}

  void update();

  template <class T>
  T resolve(T name_id) const;

  // Returns the list of unresolved resource ids that were requested by node node_id
  // but resulted in a single resolved res_id request. Note that this does not
  // account for history requests.
  eastl::span<ResNameId const> unresolve(NodeNameId node_id, ResNameId res_id) const;

  // Iterate over all resolved ids and unresolved res id lists corresponding to
  // them that were requested by the node node_id. This iterates both history
  // and regular requests.
  template <class F>
  void iterateInverseMapping(NodeNameId node_id, F &&callback) const
  {
    for (const auto &[resolvedResId, unresolvedResIds] : inverseMapping[node_id])
      callback(false, resolvedResId, unresolvedResIds);
    for (const auto &[resolvedResId, unresolvedResIds] : inverseHistoryMapping[node_id])
      callback(true, resolvedResId, unresolvedResIds);
  }

private:
  using PrevResolvedNameForResource = IdIndexedMapping<ResNameId, ResNameId, framemem_allocator>;

  ResNameId getPrevResolvedNameForResource(ResNameId res_id) const;
  void updateMapping();
  void updateFrontendRecompilationData(const PrevResolvedNameForResource &prev_resolved_name_for_resource);
  void updateInverseMapping(const PrevResolvedNameForResource &prev_resolved_name_for_resource);

  const InternalRegistry &registry;
  FrontendRecompilationData &frontendRecompilationData;
  IdNameResolver<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> resolver;


  using ResolvedToUnresolved = dag::FixedVectorMap<ResNameId, dag::RelocatableFixedVector<ResNameId, 4>, 16>;
  // Mapping (node, resolved res id) -> unresolved res Ids
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseMapping;
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseHistoryMapping;
};

} // namespace dafg