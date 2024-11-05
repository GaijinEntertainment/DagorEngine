// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class StaticGeometryNode;

class ICsgManager
{
public:
  // no merge now
  //  virtual StaticGeometryNode *merge(StaticGeometryNode *brush1, StaticGeometryNode *brush2, bool only_shape)=0;
  virtual StaticGeometryNode *subtract(StaticGeometryNode *brush1, StaticGeometryNode *brush2) = 0;
  virtual StaticGeometryNode *hollow(StaticGeometryNode *brush, real thickness) = 0;
};

extern ICsgManager *createCsgWrapper();
