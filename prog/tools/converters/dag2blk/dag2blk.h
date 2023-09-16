// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "common.h"

class Node;
class MaterialData;

class Dag2Blk
{
  bool maxPrecision;


  std::vector<MaterialData *> mat_list;
  std::map<std::string, int> matind_map;

  void addNode(Node *node, DataBlock &blk_parent);
  void addMaterials(DataBlock &blk_parent);

public:
  bool work(const char *input_filename, const char *output_filename);

  Dag2Blk();
  virtual ~Dag2Blk();
};
