//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

class Sbuffer;
typedef Sbuffer Ibuffer;
typedef Sbuffer Vbuffer;
class GlobalVertexDataSrc;
typedef int VDECL;


class BBoxTreeRenderElement
{
public:
  static constexpr int FLAG_ATEST = 0x00000001;

  union
  {
    struct
    {
      Vbuffer *vBuf; // TODO: VB related stuff should be shared to save memory.
      Ibuffer *iBuf;
      VDECL vDecl;
      int stride;
    };

    struct // Used during linking.
    {
      GlobalVertexDataSrc *vDataSrc;
    };
  };

  BBoxTreeRenderElement() : faceList_(midmem) { flags = 0; };

public:
  Tab<unsigned short int> faceList_;
  unsigned int flags;
};


class BBoxTreeLeaf : public BBox3
{
public:
  BBoxTreeLeaf() : relemList(midmem) {}

public:
  Tab<BBoxTreeRenderElement> relemList;
};


class BBoxTreeNode : public BBox3
{
public:
  static constexpr int CHILDREN_PER_NODE = 4;

  BBoxTreeNode() { memset(childArray_, 0, sizeof(childArray_)); }

  void recursiveDelete(unsigned int current_depth, unsigned int tree_depth);

public:
  BBox3 *childArray_[CHILDREN_PER_NODE];

protected:
  ~BBoxTreeNode() {}
};
