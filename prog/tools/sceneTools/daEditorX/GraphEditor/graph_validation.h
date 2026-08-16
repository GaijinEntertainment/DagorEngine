// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct GraphData;

// Returns true if the proposed edge (output side: elem_a/pin_a; input side: elem_b/pin_b
// OR vice versa, the validator handles either orientation) would be a valid addition to
// gd.edges. Pure: reads gd only and never mutates it.
//
// Mirrors the WebUI graph editor's graphEditor.js isValidConnection: pin role compatibility
// (in/out/any/ctrl with the ctrl_t pairing rule), duplicate / single-connect handling,
// terminal-elimination cycle detection, and iterative type narrowing across the graph
// (per-edge convertibility + typeGroup synchronization).
//
// Muted edges are treated as absent, so muting one frees the types it pinned and opens the loop it
// closed -- mute is meant to be as weak as delete. The cost is that unmuting is not re-checked and
// can therefore reinstate a cycle. Deliberately not guarded here: unmute must always succeed, and
// every edit recompiles, so compile_graph_to_blks reports the loop it left behind through its
// out_message instead of this validator refusing the edit.
//
// elem_a / elem_b are GraphData::Node ids (matching Node::id, not vector positions).
// pin_a / pin_b are indices into the corresponding node's pins[] vector (same convention as
// GraphData::Edge::pinA / pinB).
bool validate_new_edge(const GraphData &gd, int elem_a, int pin_a, int elem_b, int pin_b);
