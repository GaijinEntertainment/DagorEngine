//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

class DataBlock;

struct HeightmapColorGradient
{
  struct Key
  {
    float height = 0.0f;
    E3DCOLOR color = E3DCOLOR(0);
  };

  void load(const DataBlock &blk, int anchor_name_id);
  void save(DataBlock &blk) const;

  void sortByHeight();

  String name;
  dag::Vector<Key> keys;
};

struct HeightmapColorGradients
{
  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;

  void sortByName();

  int getIndexByName(const char *name) const;
  HeightmapColorGradient *getByName(const char *name);
  const HeightmapColorGradient *getByName(const char *name) const;

  dag::Vector<HeightmapColorGradient> gradients;
};
