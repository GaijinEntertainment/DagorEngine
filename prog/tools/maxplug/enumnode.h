// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ilayer.h>

class ENodeCB
{
public:
  virtual ~ENodeCB() {}
  virtual int proc(INode *) = 0;
};

enum
{
  ECB_CONT,
  ECB_STOP,
  ECB_SKIP
};

int enum_nodes(INode *node, ENodeCB *cb);
void enum_layer_nodes(ILayer *layer, ENodeCB *cb);
void enum_nodes_by_node(INode *node, ENodeCB *cb);
