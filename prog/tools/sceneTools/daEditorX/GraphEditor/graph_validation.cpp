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
// duplicate / single-connect handling). Mirrors graphEditor.js:1383 testForLoops.
bool detect_cycle(const GraphData &gd, const eastl::vector<EdgeView> &edge_views, eastl::vector<uint8_t> excluded)
{
  const int n = (int)gd.nodes.size();
  eastl::vector<uint8_t> inCnt(n, 0);
  eastl::vector<uint8_t> outCnt(n, 0);
  eastl::vector<uint8_t> anyCnt(n, 0);
  eastl::vector<uint8_t> terminalOrder(n, 0);

  auto bumpRole = [&](PinRole role, int idx, int delta) {
    switch (role)
    {
      case PinRole::In: inCnt[idx] = (uint8_t)(inCnt[idx] + delta); break;
      case PinRole::Out: outCnt[idx] = (uint8_t)(outCnt[idx] + delta); break;
      case PinRole::Any: anyCnt[idx] = (uint8_t)(anyCnt[idx] + delta); break;
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

  uint8_t order = 1;
  bool changed = false;
  do
  {
    changed = false;
    for (int i = 0; i < n; ++i)
    {
      if (terminalOrder[i] != 0)
      {
        continue;
      }
      const int sum = anyCnt[i] + (inCnt[i] ? 1 : 0) + (outCnt[i] ? 1 : 0);
      if (sum == 1)
      {
        terminalOrder[i] = order;
      }
    }

    for (size_t i = 0; i < edge_views.size(); ++i)
    {
      if (excluded[i])
      {
        continue;
      }
      const EdgeView &e = edge_views[i];
      if (terminalOrder[e.idxA] == order || terminalOrder[e.idxB] == order)
      {
        excluded[i] = 1;
        changed = true;
        const PinRole rA = gd.nodes[e.idxA].pins[e.pinA].role;
        const PinRole rB = gd.nodes[e.idxB].pins[e.pinB].role;
        bumpRole(rA, e.idxA, -1);
        bumpRole(rB, e.idxB, -1);
      }
    }

    if (order == 254)
    {
      break;
    }
    ++order;
  } while (changed);

  for (int i = 0; i < n; ++i)
  {
    if (terminalOrder[i] != 0 || gd.nodes[i].allowLoop)
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

  if (!is_role_pair_compatible(pinA, pinB))
  {
    return false;
  }

  // Build the edge view list: existing edges first (in node-index space), then the candidate.
  eastl::vector<EdgeView> edgeViews;
  edgeViews.reserve(gd.edges.size() + 1);
  eastl::vector<uint8_t> excluded;
  excluded.reserve(gd.edges.size() + 1);
  for (const GraphData::Edge &e : gd.edges)
  {
    auto ai = idToIdx.find(e.elemA);
    auto bi = idToIdx.find(e.elemB);
    if (ai == idToIdx.end() || bi == idToIdx.end())
    {
      // Dangling edge (refers to a missing node); skip from analysis. Should not happen on a
      // well-formed graph but defending here is cheap.
      continue;
    }
    EdgeView v;
    v.idxA = ai->second;
    v.pinA = e.pinA;
    v.idxB = bi->second;
    v.pinB = e.pinB;
    edgeViews.push_back(v);
    excluded.push_back(0);
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
