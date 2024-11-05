//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_shaders.h"

// You can get shadervar IDs automatically
// For example:
// In global scope
// shadervars::Node gi_quality("gi_quality");
// In function
// ShaderGlobal::set_int(gi_quality.shadervarId, WSAO);

namespace shadervars
{

inline struct Node *&head();

struct Node
{
  Node *next = nullptr;
  const char *shadervarName = nullptr;
  int shadervarId = -1;
  Node(const char *name) : shadervarName(name)
  {
    next = head();
    head() = this;
  }
};

template <typename T>
Node *head_node_ptr = nullptr;

inline Node *&head() { return head_node_ptr<void>; }

inline void resolve_shadervars()
{
  for (Node *node = head(); node; node = node->next)
    node->shadervarId = get_shader_variable_id(node->shadervarName, true);
}

} // namespace shadervars
