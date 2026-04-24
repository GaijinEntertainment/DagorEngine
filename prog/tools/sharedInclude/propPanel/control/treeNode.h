//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <propPanel/c_common.h> // TLeafHandle
#include <propPanel/propPanel.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

namespace PropPanel
{

class TreeNode final
{
public:
  enum Flags
  {
    CHECKBOX_VALUE = 1 << 0,
    CHECKBOX_ENABLED = 1 << 1,
    EXPANDED = 1 << 2,
    SELECTED = 1 << 3,
    FILTERED_IN = 1 << 4,
  };

  ~TreeNode() { clear_all_ptr_items(children); }

  int getChildIndex(const TreeNode *child) const
  {
    auto it = eastl::find(children.begin(), children.end(), child);
    if (it == children.end())
      return -1;
    else
      return it - children.begin();
  }

  TreeNode *getFirstChild() const { return children.empty() ? nullptr : children[0]; }

  TreeNode *getPrevSibling() const
  {
    const int i = parent->getChildIndex(this);
    G_ASSERT(i >= 0);
    if (i >= 1)
      return parent->children[i - 1];
    return nullptr;
  }

  TreeNode *getNextSibling() const
  {
    const int i = parent->getChildIndex(this);
    G_ASSERT(i >= 0);
    if ((i + 1) < parent->children.size())
      return parent->children[i + 1];
    return nullptr;
  }

  TreeNode *getFirstChildFiltered() const
  {
    for (auto *child : children)
    {
      if (child->getFlagValue(FILTERED_IN))
      {
        return child;
      }
    }

    return nullptr;
  }

  TreeNode *getPrevSiblingFiltered() const
  {
    int i = parent->getChildIndex(this);
    G_ASSERT(i >= 0);

    for (i -= 1; i >= 0; --i)
    {
      if (parent->children[i]->getFlagValue(FILTERED_IN))
      {
        return parent->children[i];
      }
    }

    return nullptr;
  }

  TreeNode *getNextSiblingFiltered() const
  {
    int i = parent->getChildIndex(this);
    G_ASSERT(i >= 0);

    for (i += 1; i < parent->children.size(); ++i)
    {
      if (parent->children[i]->getFlagValue(FILTERED_IN))
      {
        return parent->children[i];
      }
    }

    return nullptr;
  }

  [[nodiscard]] bool getFlagValue(Flags flag) const { return flags & flag; }
  void setFlagValue(Flags flag, bool value) { flags = (flags & ~flag) | (-static_cast<int32_t>(value) & flag); }

  const TreeNode *getParent() const { return parent; }

  int getChildCount() const { return children.size(); }
  const TreeNode &getChild(int idx) const { return *children[idx]; }

  int getChildCountFiltered() const
  {
    int i = 0;
    for (const TreeNode *child : children)
    {
      if (child->getFlagValue(FILTERED_IN))
      {
        ++i;
      }
    }

    return i;
  }

  const String &getTitle() const { return title; }
  IconId getIconId() const { return icon; }
  IconId getStateIconId() const { return stateIcon; }
  E3DCOLOR getTextColor() const { return textColor; }

  void *getUserData() const { return userData; }

private:
  friend class TreeControlStandalone;

  TreeNode *parent = nullptr;
  dag::Vector<TreeNode *> children;

  String title;
  void *userData = nullptr;
  IconId icon = IconId::Invalid;
  IconId stateIcon = IconId::Invalid;
  E3DCOLOR textColor = E3DCOLOR(0);
  uint32_t flags = Flags::FILTERED_IN;
};

} // namespace PropPanel
