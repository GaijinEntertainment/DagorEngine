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
  NameResolver(const InternalRegistry &reg) : registry{reg} {}

  // Names for which their resolved name changed
  struct NameResolutionChanged : public IdIndexedFlags<ResNameId, framemem_allocator>,
                                 public IdIndexedFlags<AutoResTypeNameId, framemem_allocator>
  {
    template <class T>
    void resize(size_t count, bool value)
    {
      IdIndexedFlags<T, framemem_allocator>::resize(count, value);
    }

    template <class T>
    void set(T nameId, bool value)
    {
      IdIndexedFlags<T, framemem_allocator>::set(nameId, value);
    }

    template <class T>
    bool operator[](T nameId) const
    {
      return IdIndexedFlags<T, framemem_allocator>::operator[](nameId);
    }
  };

  using NodesChanged = IdIndexedFlags<NodeNameId, framemem_allocator>;

  struct Changes
  {
    NameResolutionChanged nameResolutionChanged;
    NodesChanged nodesWithChangedRequests;
  };

  Changes update(const NodesChanged &node_changed);

  template <class T>
  T resolve(T name_id) const;

  // Returns the list of unresolved resource ids that were requested by node node_id
  // but resulted in a single resolved res_id request. Note that this does not
  // account for history requests.
  eastl::span<ResNameId const> unresolve(NodeNameId node_id, ResNameId res_id) const;
  eastl::span<ResNameId const> historyUnresolve(NodeNameId node_id, ResNameId res_id) const;

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
  using PrevResolvedNameForAutoResType = IdIndexedMapping<AutoResTypeNameId, AutoResTypeNameId, framemem_allocator>;

  ResNameId getPrevResolvedNameForResource(ResNameId res_id) const;
  void updateMapping();
  void updateInverseMapping(NodesChanged &out_nodes_with_changed_requests,
    const PrevResolvedNameForResource &prev_resolved_name_for_resource, const NodesChanged &node_changed,
    const NameResolutionChanged &name_resolution_changed);

  const InternalRegistry &registry;
  IdNameResolver<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> resolver;


  using ResolvedToUnresolved = dag::FixedVectorMap<ResNameId, dag::RelocatableFixedVector<ResNameId, 4>, 16>;
  // Mapping (node, resolved res id) -> unresolved res Ids
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseMapping;
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseHistoryMapping;
};

} // namespace dafg