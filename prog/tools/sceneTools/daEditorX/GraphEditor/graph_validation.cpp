// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_validation.h"

#include <graphEditor/graph_data.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

namespace
{

using PinTypeList = eastl::fixed_vector<PinType, 4, true>;

// Implicit conversion table mirroring GE_conversions in stubNodesSettings.js. A pair {A, B}
// declares "type A can be implicitly converted to type B". Identity is handled directly in
// is_convertible() so it does not need to be listed here.
constexpr eastl::pair<PinType, PinType> CONVERSIONS[] = {
  {PinType::Int, PinType::Float},
  {PinType::Float, PinType::Int},
  {PinType::Float, PinType::Float2},
  {PinType::Float, PinType::Float3},
  {PinType::Float, PinType::Float4},
  {PinType::Float4, PinType::Float3},
  {PinType::Float4, PinType::Float2},
  {PinType::Float3, PinType::Float4},
  {PinType::Float3, PinType::Float2},
  {PinType::Float2, PinType::Float4},
  {PinType::Float2, PinType::Float3},
};

bool is_convertible(PinType from, PinType to)
{
  if (from == to)
  {
    return true;
  }
  for (const auto &c : CONVERSIONS)
  {
    if (c.first == from && c.second == to)
    {
      return true;
    }
  }
  return false;
}

// Remove from `inout_types` any type that cannot be reached from any type in `const_types`
// via the implicit conversion table (identity counts). Returns true if anything was removed.
// Mirrors typeConv.removeInconvertableTypes in nodeUtils.js.
bool remove_inconvertible_types(const PinTypeList &const_types, PinTypeList &inout_types)
{
  bool changed = false;
  for (int i = (int)inout_types.size() - 1; i >= 0; --i)
  {
    const PinType t = inout_types[i];
    bool keep = false;
    for (PinType c : const_types)
    {
      if (is_convertible(c, t))
      {
        keep = true;
        break;
      }
    }
    if (!keep)
    {
      inout_types.erase(inout_types.begin() + i);
      changed = true;
    }
  }
  return changed;
}

// Strict equality variant: removes any type from `inout_types` that is not present in
// `const_types`. Used for typeGroup synchronization (no implicit conversions).
bool remove_non_equal_types(const PinTypeList &const_types, PinTypeList &inout_types)
{
  bool changed = false;
  for (int i = (int)inout_types.size() - 1; i >= 0; --i)
  {
    const PinType t = inout_types[i];
    bool keep = false;
    for (PinType c : const_types)
    {
      if (c == t)
      {
        keep = true;
        break;
      }
    }
    if (!keep)
    {
      inout_types.erase(inout_types.begin() + i);
      changed = true;
    }
  }
  return changed;
}

// True if any type on the `from` side reaches any type on the `to` side.
bool any_convertible(const PinTypeList &from, const PinTypeList &to)
{
  for (PinType f : from)
  {
    for (PinType t : to)
    {
      if (is_convertible(f, t))
      {
        return true;
      }
    }
  }
  return false;
}

bool is_role_pair_compatible(const GraphData::Pin &p1, const GraphData::Pin &p2)
{
  // Mirrors graphEditor.js:1855: at least one is "any", or roles differ.
  const bool inOutOk = (p1.role == PinRole::Any || p2.role == PinRole::Any || p1.role != p2.role);
  if (!inOutOk)
  {
    return false;
  }

  // Ctrl pin pairing rule (graphEditor.js:1859-1866): if exactly one side is Ctrl, the other
  // must be an Out pin whose declared types include ctrl_t.
  const bool p1Ctrl = (p1.role == PinRole::Ctrl);
  const bool p2Ctrl = (p2.role == PinRole::Ctrl);
  if (p1Ctrl != p2Ctrl)
  {
    const GraphData::Pin &outPin = p1Ctrl ? p2 : p1;
    if (outPin.role != PinRole::Out)
    {
      return false;
    }
    bool hasCtrlT = false;
    for (PinType t : outPin.types)
    {
      if (t == PinType::CtrlT)
      {
        hasCtrlT = true;
        break;
      }
    }
    if (!hasCtrlT)
    {
      return false;
    }
  }
  return true;
}

// View of an edge in node-index space (not node-id space) so the cycle / type passes can index
// flat arrays sized by gd.nodes.size(). The candidate edge sits at the end of the vector with
// the same shape as real edges.
struct EdgeView
{
  int idxA = -1;
  int pinA = -1;
  int idxB = -1;
  int pinB = -1;
};

// Returns true if the proposed graph (gd.edges + the candidate at edgeViews.back()) contains
// a cycle that violates a non-allowLoop node, after the JS-style terminal-elimination pass.
// `excluded` is a pre-built mask matching edgeViews; entries set to true are skipped (used for
// duplicate / single-connect handling). Follows graphEditor.js:1383 testForLoops, but the elimination
// runs until nothing changes, where the JS stops at 255 rounds.
bool detect_cycle(const GraphData &gd, const eastl::vector<EdgeView> &edge_views, eastl::vector<uint8_t> excluded)
{
  const int n = (int)gd.nodes.size();
  eastl::vector<int> inCnt(n, 0);
  eastl::vector<int> outCnt(n, 0);
  eastl::vector<int> anyCnt(n, 0);
  eastl::vector<uint8_t> terminal(n, 0);

  auto bumpRole = [&](PinRole role, int idx, int delta) {
    switch (role)
    {
      case PinRole::In: inCnt[idx] += delta; break;
      case PinRole::Out: outCnt[idx] += delta; break;
      case PinRole::Any: anyCnt[idx] += delta; break;
      case PinRole::Ctrl: break; // Ctrl pins do not participate in the cycle metric.
    }
  };

  for (size_t i = 0; i < edge_views.size(); ++i)
  {
    if (excluded[i])
    {
      continue;
    }
    const EdgeView &e = edge_views[i];
    const PinRole rA = gd.nodes[e.idxA].pins[e.pinA].role;
    const PinRole rB = gd.nodes[e.idxB].pins[e.pinB].role;
    bumpRole(rA, e.idxA, +1);
    bumpRole(rB, e.idxB, +1);
  }

  bool changed = false;
  do
  {
    changed = false;
    for (int i = 0; i < n; ++i)
    {
      const int sum = anyCnt[i] + (inCnt[i] ? 1 : 0) + (outCnt[i] ? 1 : 0);
      if (sum == 1)
      {
        terminal[i] = 1;
      }
    }

    for (size_t i = 0; i < edge_views.size(); ++i)
    {
      if (excluded[i])
      {
        continue;
      }
      const EdgeView &e = edge_views[i];
      // An edge of a node marked in an earlier round is excluded already, so this sees this round's marks only.
      if (terminal[e.idxA] || terminal[e.idxB])
      {
        excluded[i] = 1;
        changed = true;
        const PinRole rA = gd.nodes[e.idxA].pins[e.pinA].role;
        const PinRole rB = gd.nodes[e.idxB].pins[e.pinB].role;
        bumpRole(rA, e.idxA, -1);
        bumpRole(rB, e.idxB, -1);
      }
    }
  } while (changed);

  for (int i = 0; i < n; ++i)
  {
    if (gd.nodes[i].allowLoop)
    {
      continue;
    }
    const int sum = anyCnt[i] + (inCnt[i] ? 1 : 0) + (outCnt[i] ? 1 : 0);
    if (sum > 1)
    {
      return true;
    }
  }
  return false;
}

// Iterative type-narrowing across the graph (proposed edges included). Returns true if every
// pin still has at least one candidate type after convergence (or the 400-iteration cap).
// Mirrors graphEditor.js:1487 checkTypeCorrect.
bool check_type_correct(const GraphData &gd, const eastl::vector<EdgeView> &edge_views, const eastl::vector<uint8_t> &excluded)
{
  const int n = (int)gd.nodes.size();

  // Per-node, per-pin scratch state: candidate type set seeded from descriptor's declared types,
  // and a "connected" flag the propagation loop sets when an edge touches the pin.
  struct PinState
  {
    PinTypeList candidates;
    uint8_t connected = 0;
  };
  eastl::vector<eastl::vector<PinState>> state(n);
  for (int i = 0; i < n; ++i)
  {
    const auto &pins = gd.nodes[i].pins;
    state[i].resize(pins.size());
    for (size_t j = 0; j < pins.size(); ++j)
    {
      state[i][j].candidates.assign(pins[j].types.begin(), pins[j].types.end());
    }
  }

  for (int iter = 0; iter < 400; ++iter)
  {
    bool changed = false;

    // Phase 1: per-edge convertibility narrowing (graphEditor.js:1502-1530).
    for (size_t k = 0; k < edge_views.size(); ++k)
    {
      if (excluded[k])
      {
        continue;
      }
      const EdgeView &e = edge_views[k];
      const GraphData::Pin &declaredA = gd.nodes[e.idxA].pins[e.pinA];
      const GraphData::Pin &declaredB = gd.nodes[e.idxB].pins[e.pinB];

      // Skip edges where either endpoint is unresolved -- pin hidden, or no declared types
      // in the descriptor (descriptor missing for the node). These pins are not user-visible
      // and shouldn't gate validation of the candidate. Mirrors the implicit JS behavior
      // where elems without a desc are filtered out of the loop. We still mark connected so
      // typeGroup logic stays consistent if those siblings appear elsewhere.
      PinState &fromState = state[e.idxA][e.pinA];
      PinState &toState = state[e.idxB][e.pinB];
      fromState.connected = 1;
      toState.connected = 1;
      if (declaredA.hidden || declaredB.hidden || declaredA.types.empty() || declaredB.types.empty())
      {
        continue;
      }

      const PinRole rA = declaredA.role;
      const PinRole rB = declaredB.role;

      if (rA == PinRole::Out || rB == PinRole::In)
      {
        if (remove_inconvertible_types(fromState.candidates, toState.candidates))
        {
          changed = true;
        }
        if (toState.candidates.empty())
        {
          return false;
        }
      }
      if (rA == PinRole::In || rB == PinRole::Out)
      {
        if (remove_inconvertible_types(toState.candidates, fromState.candidates))
        {
          changed = true;
        }
        if (fromState.candidates.empty())
        {
          return false;
        }
      }
    }

    // Phase 2: typeGroup synchronization (graphEditor.js:1532-1569). Pins in the same group on
    // the same node must agree; the constraint set is the union of the OTHER connected non-Out
    // group members' candidates, falling back to the pin's own current candidates when none
    // such sibling exists.
    for (int i = 0; i < n; ++i)
    {
      const auto &pins = gd.nodes[i].pins;
      for (size_t j = 0; j < pins.size(); ++j)
      {
        const eastl::string &group = pins[j].typeGroup;
        if (group.empty())
        {
          continue;
        }
        PinTypeList allInTypes;
        bool found = false;
        for (size_t kp = 0; kp < pins.size(); ++kp)
        {
          if (kp == j)
          {
            continue;
          }
          if (pins[kp].typeGroup != group || pins[kp].role == PinRole::Out)
          {
            continue;
          }
          if (!state[i][kp].connected)
          {
            continue;
          }
          for (PinType t : state[i][kp].candidates)
          {
            allInTypes.push_back(t);
          }
          found = true;
        }
        if (!found)
        {
          allInTypes.assign(state[i][j].candidates.begin(), state[i][j].candidates.end());
        }
        if (remove_non_equal_types(allInTypes, state[i][j].candidates))
        {
          changed = true;
        }
        if (state[i][j].candidates.empty())
        {
          return false;
        }
      }
    }

    if (!changed)
    {
      return true;
    }
  }
  // Exhausted the iteration cap: treat as a non-converging configuration -> reject. Matches
  // the JS behavior of returning false when iteration > 400.
  return false;
}

} // namespace

bool validate_new_edge(const GraphData &gd, int elem_a, int pin_a, int elem_b, int pin_b)
{
  // Self-link short-circuit (the cycle test would also catch this, but rejecting up front is
  // cheap and clearer than letting the loop detector fire).
  if (elem_a == elem_b)
  {
    return false;
  }

  // Build node-id -> vector-index map. Linear-scan-once: O(n) construction, O(1) lookup.
  eastl::hash_map<int, int> idToIdx;
  idToIdx.reserve(gd.nodes.size());
  for (size_t i = 0; i < gd.nodes.size(); ++i)
  {
    idToIdx.insert({gd.nodes[i].id, (int)i});
  }

  auto itA = idToIdx.find(elem_a);
  auto itB = idToIdx.find(elem_b);
  if (itA == idToIdx.end() || itB == idToIdx.end())
  {
    return false;
  }
  const int idxA = itA->second;
  const int idxB = itB->second;

  if (pin_a < 0 || pin_a >= (int)gd.nodes[idxA].pins.size())
  {
    return false;
  }
  if (pin_b < 0 || pin_b >= (int)gd.nodes[idxB].pins.size())
  {
    return false;
  }

  const GraphData::Pin &pinA = gd.nodes[idxA].pins[pin_a];
  const GraphData::Pin &pinB = gd.nodes[idxB].pins[pin_b];

  if (pinA.hidden || pinB.hidden)
  {
    return false;
  }

  // Section dividers are not pins (graphEditor.js isValidConnection rejects them the same way).
  if (pinA.separator || pinB.separator)
  {
    return false;
  }

  if (!is_role_pair_compatible(pinA, pinB))
  {
    return false;
  }

  // Build the edge view list: existing edges first (in node-index space), then the candidate.
  eastl::vector<EdgeView> edgeViews;
  edgeViews.reserve(gd.edges.size() + 1);
  eastl::vector<uint8_t> excluded;
  excluded.reserve(gd.edges.size() + 1);
  // Exactly one entry per gd.edges slot, even for edges that take no part in the analysis: the
  // hide passes below iterate gd.edges and index `excluded` with the same subscript, so dropping
  // an entry would shift every later index -- masking the wrong edge, and writing past the end
  // once more than one edge is dropped.
  for (const GraphData::Edge &e : gd.edges)
  {
    auto ai = idToIdx.find(e.elemA);
    auto bi = idToIdx.find(e.elemB);
    // Dangling (an endpoint node is gone; should not happen on a well-formed graph, but defending
    // is cheap) or muted. A mute means the edge carries no data, so it must not narrow a pin's
    // types or close a cycle either. Excluded entries keep their -1 placeholder indices and are
    // never dereferenced: detect_cycle and check_type_correct both test the mask first.
    const bool inert = (ai == idToIdx.end() || bi == idToIdx.end() || e.muted);
    EdgeView v;
    if (ai != idToIdx.end() && bi != idToIdx.end())
    {
      v.idxA = ai->second;
      v.pinA = e.pinA;
      v.idxB = bi->second;
      v.pinB = e.pinB;
    }
    edgeViews.push_back(v);
    excluded.push_back(inert ? 1 : 0);
  }
  EdgeView candidate;
  candidate.idxA = idxA;
  candidate.pinA = pin_a;
  candidate.idxB = idxB;
  candidate.pinB = pin_b;
  edgeViews.push_back(candidate);
  excluded.push_back(0);
  const size_t candidateIdx = edgeViews.size() - 1;

  // Apply the JS-style "hide" passes to the existing edges (the candidate itself is never hidden):
  //
  // hideSameConnection (graphEditor.js:1245): if any existing edge already wires the same pin
  // pair (forward or reverse), exclude it from analysis. Lets the candidate replace duplicates.
  //
  // hideSingleConnections (graphEditor.js:1328): if either endpoint pin is singleConnect, any
  // existing edge using that pin is excluded. Lets the candidate replace the prior occupant of
  // a single-connect pin.
  for (size_t i = 0; i < gd.edges.size(); ++i)
  {
    const GraphData::Edge &e = gd.edges[i];
    const bool sameForward = (e.elemA == elem_a && e.pinA == pin_a && e.elemB == elem_b && e.pinB == pin_b);
    const bool sameReverse = (e.elemA == elem_b && e.pinA == pin_b && e.elemB == elem_a && e.pinB == pin_a);
    if (sameForward || sameReverse)
    {
      excluded[i] = 1;
    }
  }
  auto hideSingleOn = [&](int elem_id, int pin_idx, bool single) {
    if (!single)
    {
      return;
    }
    for (size_t i = 0; i < gd.edges.size(); ++i)
    {
      if (excluded[i])
      {
        continue;
      }
      const GraphData::Edge &e = gd.edges[i];
      if ((e.elemA == elem_id && e.pinA == pin_idx) || (e.elemB == elem_id && e.pinB == pin_idx))
      {
        excluded[i] = 1;
      }
    }
  };
  hideSingleOn(elem_a, pin_a, pinA.singleConnect);
  hideSingleOn(elem_b, pin_b, pinB.singleConnect);
  (void)candidateIdx; // candidate stays active; index retained for future debugging.

  if (detect_cycle(gd, edgeViews, excluded))
  {
    return false;
  }
  if (!check_type_correct(gd, edgeViews, excluded))
  {
    return false;
  }
  return true;
}

bool can_pins_ever_connect(const GraphData::Pin &source, const GraphData::Pin &candidate)
{
  // The gates validate_new_edge opens with, minus everything that needs the graph.
  if (source.hidden || candidate.hidden || source.separator || candidate.separator)
  {
    return false;
  }
  if (!is_role_pair_compatible(source, candidate))
  {
    return false;
  }

  // The whole-graph pass skips undeclared pins rather than failing them; stay permissive too.
  if (source.types.empty() || candidate.types.empty())
  {
    return true;
  }

  // check_type_correct's phase 1 narrows both ways, each guarded by its own role test, and an Any
  // source is the `from` side against an In candidate. Picking one direction off source.role alone
  // refuses exactly those pairs, so mirror the two tests rather than the shape.
  const bool sourceToCandidate = source.role == PinRole::Out || candidate.role == PinRole::In;
  const bool candidateToSource = source.role == PinRole::In || candidate.role == PinRole::Out;
  if (sourceToCandidate && !any_convertible(source.types, candidate.types))
  {
    return false;
  }
  if (candidateToSource && !any_convertible(candidate.types, source.types))
  {
    return false;
  }
  // Neither side constrains the other (Any against Any), which the whole-graph pass also leaves be.
  return true;
}

namespace
{
// The far end of a wire the splice at (anchor_node, anchor_pin) could take over, or false: this edge
// is not on that pin, anchor_edge_id narrows it out, or its far end is one the renderer never
// submitted -- there is no pin there to wire, nor to draw a preview wire to. Kept beside its two
// callers because it is the rule that decides whether the gesture is offered at all.
bool splice_wire_far_end(const GraphData &gd, const GraphData::Edge &edge, int anchor_node, int anchor_pin, int anchor_edge_id,
  SplicePin &out)
{
  if (anchor_edge_id >= 0 && edge.id != anchor_edge_id)
  {
    return false;
  }

  int oppositeNode = -1;
  int oppositePin = -1;
  if (!edge_opposite_end(edge, anchor_node, anchor_pin, oppositeNode, oppositePin) || !is_pin_reachable(gd, oppositeNode, oppositePin))
  {
    return false;
  }

  out = SplicePin{oppositeNode, oppositePin, edge.id, edge.muted};
  return true;
}
} // namespace

void splice_ends(const GraphData &gd, int anchor_node, int anchor_pin, int anchor_edge_id, SpliceEnds &out)
{
  out.feed = SplicePin{anchor_node, anchor_pin};
  out.sinks.clear();

  const GraphData::Pin *const anchor = find_pin(gd, anchor_node, anchor_pin);
  if (!anchor)
  {
    return;
  }
  const bool anchorIsOut = anchor->role == PinRole::Out;

  for (const GraphData::Edge &edge : gd.edges)
  {
    SplicePin farEnd;
    if (!splice_wire_far_end(gd, edge, anchor_node, anchor_pin, anchor_edge_id, farEnd))
    {
      continue;
    }
    if (anchorIsOut)
    {
      out.sinks.push_back(farEnd);
      continue;
    }
    // An In anchor names one wire: its driver feeds, and the anchor itself is the consumer.
    out.feed = SplicePin{farEnd.node, farEnd.pin};
    out.sinks.push_back(SplicePin{anchor_node, anchor_pin, farEnd.edgeId, farEnd.muted});
    return;
  }
}

bool has_splice_target(const GraphData &gd, int anchor_node, int anchor_pin, int anchor_edge_id)
{
  // The rule splice_ends builds its ends from, asked one edge at a time and stopped at the first
  // hit: the hint bar and the menu rows want the bool alone, every frame. The anchor's role does not
  // enter it -- either way a sink exists exactly when one of its wires has a far end to reach.
  if (!find_pin(gd, anchor_node, anchor_pin))
  {
    return false;
  }

  for (const GraphData::Edge &edge : gd.edges)
  {
    SplicePin farEnd;
    if (splice_wire_far_end(gd, edge, anchor_node, anchor_pin, anchor_edge_id, farEnd))
    {
      return true;
    }
  }
  return false;
}

bool can_node_splice(const GraphData &gd, const SpliceEnds &ends, const GraphData::Node &candidate)
{
  const GraphData::Pin *const feed = find_pin(gd, ends.feed.node, ends.feed.pin);
  if (!feed)
  {
    return false;
  }

  // Resolved above the loop: find_pin scans gd.nodes, and the picker runs this once per descriptor
  // in the library.
  eastl::fixed_vector<const GraphData::Pin *, 8, true> sinkPins;
  for (const SplicePin &sink : ends.sinks)
  {
    if (const GraphData::Pin *const sinkDesc = find_pin(gd, sink.node, sink.pin))
    {
      sinkPins.push_back(sinkDesc);
    }
  }

  bool takesFeed = false;
  bool passesOn = sinkPins.empty();
  for (const GraphData::Pin &pin : candidate.pins)
  {
    takesFeed = takesFeed || can_pins_ever_connect(*feed, pin);
    // Role tested as the commit does, so nothing is admitted that its consumer loop would skip.
    if (pin.role == PinRole::Out)
    {
      for (const GraphData::Pin *const sinkDesc : sinkPins)
      {
        passesOn = passesOn || can_pins_ever_connect(*sinkDesc, pin);
      }
    }
    if (takesFeed && passesOn)
    {
      return true;
    }
  }
  return false;
}

int find_connectable_pin(const GraphData &gd, int source_node_id, int source_pin, int candidate_node_id)
{
  const GraphData::Node *const candidate = find_node_by_id(gd, candidate_node_id);
  if (!candidate)
  {
    return -1;
  }

  // No can_pins_ever_connect pre-gate: it only repeats what the validator decides next, over the
  // handful of pins one node has, and two copies of the rule are two things to keep in step.
  for (int i = 0; i < static_cast<int>(candidate->pins.size()); ++i)
  {
    // The validator normalizes either orientation itself.
    if (validate_new_edge(gd, source_node_id, source_pin, candidate_node_id, i))
    {
      return i;
    }
  }
  return -1;
}
