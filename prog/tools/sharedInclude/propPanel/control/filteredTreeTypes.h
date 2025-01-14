//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_common.h> // TLeafHandle
#include <drv/3d/dag_resId.h>
#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

namespace PropPanel
{

struct TTreeNode
{
  TTreeNode(const char *cap, TEXTUREID icon, void *data, TTreeNode *in_parent) :
    nodes(midmem), name(cap), icon(icon), isExpand(false), textColor(0, 0, 0), userData(data), item(nullptr), parent(in_parent)
  {}

  ~TTreeNode() { clear_all_ptr_items(nodes); }

  TTreeNode *getFirstChild() const { return nodes.empty() ? nullptr : nodes[0]; }

  String name;
  TEXTUREID icon;
  bool isExpand;
  E3DCOLOR textColor;
  void *userData;
  TLeafHandle item;
  Tab<String> childNames;

  TTreeNode *parent;
  Tab<TTreeNode *> nodes;
};

typedef bool (*TTreeNodeFilterFunc)(void *param, TTreeNode &node);

} // namespace PropPanel