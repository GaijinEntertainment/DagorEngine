//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_treeBitmap.h>

namespace mkbindump
{
inline void save_tree_bitmap(BinDumpSaveCB &cwr, const TreeBitmapNode *node)
{
  const int finalMipSize = 16;

  cwr.writeInt8e(node->fillColor);
  cwr.writeInt8e(node->subnodesOrMapdata ? 1 : 0);
  if (!node->subnodesOrMapdata)
    return;

  if (node->fillColor & TreeBitmapNode::HAS_SUBNODES)
  {
    TreeBitmapNode *subnodes = (TreeBitmapNode *)node->subnodesOrMapdata;
    int subnodeDimensions = 1 << ((node->fillColor & 0x7f) + 1);
    for (int y = 0; y < subnodeDimensions; ++y)
      for (int x = 0; x < subnodeDimensions; ++x)
        save_tree_bitmap(cwr, &subnodes[y * subnodeDimensions + x]);
  }
  else
  {
    if (node->fillColor & TreeBitmapNode::MAPDATA_1BIT)
      cwr.writeRaw(node->subnodesOrMapdata, finalMipSize * finalMipSize / 8 + 2);
    else if (node->fillColor & TreeBitmapNode::MAPDATA_2BIT)
      cwr.writeRaw(node->subnodesOrMapdata, finalMipSize * finalMipSize / 4 + 4);
    else
      cwr.writeRaw(node->subnodesOrMapdata, finalMipSize * finalMipSize);
  }
}
}; // namespace mkbindump
