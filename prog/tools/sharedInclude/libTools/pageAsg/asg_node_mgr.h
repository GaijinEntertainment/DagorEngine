//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/pageAsg/asg_decl.h>

// provider of node indexing - knows full list of nodes and can resolve indices by name
class IAsgGraphNodeManager
{
public:
  virtual AnimGraphState *getState(int state_id) = 0;
  virtual int findState(const char *name) = 0;
  virtual int getNodeId(int state_id) = 0;
};
