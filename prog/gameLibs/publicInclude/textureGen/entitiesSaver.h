//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>


struct TextureGenEntity
{
  Point3 worldPos;
  Point3 forward;
  Point3 up;
  int nameIndex;
  int placeType;
  int seed;
  Point3 scale;
};

class ITextureGenEntitiesSaver
{
public:
  virtual bool onEntitiesGenerated(const Tab<TextureGenEntity> &entities, const char *blk_file_name,
    const Tab<String> &entity_names) = 0;
};
