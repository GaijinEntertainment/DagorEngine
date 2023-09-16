//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
