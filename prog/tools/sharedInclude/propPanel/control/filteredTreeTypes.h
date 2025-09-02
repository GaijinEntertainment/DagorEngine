//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_common.h> // TLeafHandle
#include <propPanel/propPanel.h>
#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

namespace PropPanel
{

struct TTreeNode
{
  TTreeNode(const char *cap, IconId icon, void *data, TTreeNode *in_parent) :
    nodes(midmem), name(cap), icon(icon), isExpand(false), textColor(0, 0, 0), userData(data), item(nullptr), parent(in_parent)
  {}

  ~TTreeNode() { clear_all_ptr_items(nodes); }

  TTreeNode *getFirstChild() const { return nodes.empty() ? nullptr : nodes[0]; }

  String name;
  IconId icon;
  // Note: isExpand might not be always in sync with the filtered tree's state. See updateUnfilteredExpansionStateFromFilteredTree.
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