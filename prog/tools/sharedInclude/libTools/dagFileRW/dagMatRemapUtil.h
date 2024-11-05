// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <ioSys/dag_dataBlock.h>

struct DagData
{
  struct Material
  {
    SimpleString name;
    DagMater mater;
    SimpleString classname, script;
    int start, end;
  };
  struct Block
  {
    int tag, start, end;
    void *data;
  };
  struct Node
  {
    struct NodeBlock
    {
      DagNodeData data;
      SimpleString name;
      TMatrix nodeTm;
      SimpleString script;
      SmallTab<unsigned short, TmpmemAlloc> mater;
      Block block;

      int blockType;
    };
    Tab<Node> children;
    Tab<NodeBlock> blocks;

    Node() : children(tmpmem), blocks(tmpmem) {}
  };

  Tab<Material> matlist;
  Tab<String> texlist;
  Tab<Block> blocks;
  Tab<Node> nodes;
  int texStart = 0, texEnd = 0;
  DagData() : texlist(tmpmem_ptr()), matlist(tmpmem_ptr()), blocks(tmpmem), nodes(tmpmem) {}
};

DataBlock make_dag_one_mat_blk(const DagData &data, int mat_id, bool put_all_params = false);
DataBlock make_dag_mats_blk(const DagData &data);
int remove_unused_textures(DagData &data);
bool load_scene(const char *fname, DagData &data, bool read_nodes = false);
bool write_dag(const char *dest, const DagData &data);
