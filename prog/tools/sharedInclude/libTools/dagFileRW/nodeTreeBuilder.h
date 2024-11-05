// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_geomTree.h>

namespace mkbindump
{
class BinDumpSaveCB;
}
class IGenSave;
class Node;


class GeomNodeTreeBuilder
{
  struct TreeNode
  {
    mat44f tm, wtm;
    PatchableTab<TreeNode> child;
    PatchablePtr<TreeNode> parent;
    PatchablePtr<const char> name;
  };
  PatchableTab<TreeNode> nodes;
  int lastValidWtmIndex;
  unsigned invalidTmOfs : 20, lastUnimportantCount : 12;
  SmallTab<char, MidmemAlloc> data;

public:
  bool loadFromDag(const char *filename);

  void buildFromDagNodes(Node *n, const char *unimportant_nodes_prefix = NULL);

  void save(mkbindump::BinDumpSaveCB &cb, bool compr, bool allow_oodle);

  void saveDump(IGenSave &cb, unsigned target_code, bool compr, bool allow_oodle);

protected:
  struct NodeBuildData
  {
    int childId, childNum, parentId, nameOfs;

    NodeBuildData(int ci, int cn, int pi, int no) : childId(ci), childNum(cn), parentId(pi), nameOfs(no) {}
  };
};
