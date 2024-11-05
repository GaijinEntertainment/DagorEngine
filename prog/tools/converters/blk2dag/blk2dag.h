// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"

class Node;
class MaterialData;


typedef std::map<std::string, std::string> StrMap;

class Blk2Dag
{
  bool maxPrecision;


  void loadVertexData(DataBlock &blk_in, Node *node);
  void loadFaceData(DataBlock &blk_in, Node *node);
  void loadNormalVertData(DataBlock &blk, Node *node);
  void loadNormalFaceData(DataBlock &blk, Node *node);
  void loadChannelData(DataBlock &blk, Node *node, int uv_channel);
  void loadUVData(DataBlock &blk, Node *node);

  void loadTexture(DataBlock *blk, MaterialData *mdata);

  void loadScriptData(DataBlock &blk_in, Node *node);
  MaterialData *loadMaterialData(DataBlock &blk, int &index);

  void getParamsForMaterialClass(const char *name, std::vector<std::string> &params);

  void fillDataFromConfig(MaterialData *mdata);

  bool loadConfig();

public:
  bool work(const char *input_filename, const char *output_filename);

  Blk2Dag();
  virtual ~Blk2Dag();
};
