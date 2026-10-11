// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "resource_paths.hpp"

#include <generic/dag_span.h>
#include <util/dag_string.h>

struct CreateSubgraphChoice
{
  String name; // sanitized
  int rootIndex = 0;
  String finalPath; // <root dir><name>.subgraph.blk
};

// Trimmed, then every character outside [0-9A-Za-z_] becomes '_'.
String sanitize_subgraph_name(const char *name);

// Modal, so between ImGui frames only. False on Cancel.
bool prompt_create_subgraph(dag::ConstSpan<SubgraphRoot> roots, const char *proposed_name, CreateSubgraphChoice &out);
