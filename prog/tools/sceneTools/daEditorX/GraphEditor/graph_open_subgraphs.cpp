// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_open_subgraphs.h"

#include "graph_document.h"

#include <graphEditor/graph_subgraph_expand.h>
#include <ioSys/dag_dataBlock.h>

#include <EASTL/shared_ptr.h>

namespace
{
const OpenSubgraphDoc *find_doc(dag::ConstSpan<OpenSubgraphDoc> docs, int tab_id)
{
  for (const OpenSubgraphDoc &d : docs)
  {
    if (d.tabId == tab_id)
    {
      return &d;
    }
  }
  return nullptr;
}

// Erases the entries of tabs gone from docs, or that `same` no longer accepts; true when `dropped` is for one of them.
template <class Map, class Same, class Dropped>
bool prune_entries(Map &map, dag::ConstSpan<OpenSubgraphDoc> docs, Same same, Dropped dropped)
{
  bool changed = false;
  for (auto it = map.begin(); it != map.end();)
  {
    const OpenSubgraphDoc *doc = find_doc(docs, it->first);
    if (doc && same(*doc, it->second))
    {
      ++it;
      continue;
    }
    changed = dropped(it->second) || changed;
    it = map.erase(it);
  }
  return changed;
}
} // namespace

bool OpenSubgraphs::publish(dag::ConstSpan<OpenSubgraphDoc> docs, SubgraphCache &cache)
{
  // Removals first: a window that closed and another one opened on the same file in one tick share the path.
  bool changed = prune_entries(
    byTabId, docs, [](const OpenSubgraphDoc &d, const Published &p) { return d.path == p.path; },
    [&cache](const Published &p) {
      cache.removeOpen(p.path.c_str());
      return p.modified;
    });

  for (const OpenSubgraphDoc &d : docs)
  {
    const uint64_t revision = d.doc->getStructureRevision();
    const uint64_t positions = d.doc->getPositionsStamp();
    const bool modified = d.doc->isModified();
    auto it = byTabId.find(d.tabId);
    const bool known = it != byTabId.end();
    if (known && it->second.revision == revision && it->second.positions == positions)
    {
      it->second.modified = modified; // a save moves no revision
      continue;
    }
    // A move alone changes no compile, only where Explode places the nodes.
    if (!known || it->second.revision != revision)
    {
      changed = changed || modified || (known && it->second.modified);
    }
    // Nodes and edges only: all the splice reads.
    eastl::shared_ptr<GraphData> body = eastl::make_shared<GraphData>();
    body->nodes = d.doc->getGraphData().nodes;
    body->edges = d.doc->getGraphData().edges;
    cache.publishOpen(d.path.c_str(), eastl::move(body));
    byTabId[d.tabId] = Published{d.path, revision, positions, modified};
  }
  return changed;
}

void OpenSubgraphs::refreshInterfaces(dag::ConstSpan<OpenSubgraphDoc> docs)
{
  bool changed = prune_entries(
    interfaceByTabId, docs, [](const OpenSubgraphDoc &d, const Interface &i) { return d.stem == i.stem; },
    [](const Interface &i) { return i.valid; });

  for (const OpenSubgraphDoc &d : docs)
  {
    Interface &iface = interfaceByTabId[d.tabId];
    const uint64_t revision = d.doc->getStructureRevision();
    const uint64_t positions = d.doc->getPositionsStamp();
    if (iface.stem == d.stem && iface.revision == revision && iface.positions == positions)
    {
      continue;
    }
    iface.stem = d.stem;
    iface.revision = revision;
    iface.positions = positions;
    DataBlock desc;
    eastl::vector<SubgraphDescriptorMessage> messages; // the save reports them, not every edit
    const bool valid = synthesize_subgraph_descriptor(d.doc->getGraphData(), d.stem.c_str(), d.path.c_str(), "", desc, messages);
    eastl::vector<GraphData::Pin> pins;
    if (valid)
    {
      descriptor_pins(desc, pins);
    }
    if (iface.valid == valid && same_instance_pins(pins, iface.pins))
    {
      continue;
    }
    iface.pins = eastl::move(pins);
    iface.valid = valid;
    changed = true;
  }
  if (changed)
  {
    ++interfaceGeneration;
  }
}

const eastl::vector<GraphData::Pin> *OpenSubgraphs::findInterface(const char *stem) const
{
  for (const auto &kv : interfaceByTabId)
  {
    if (kv.second.valid && kv.second.stem == stem)
    {
      return &kv.second.pins;
    }
  }
  return nullptr;
}
