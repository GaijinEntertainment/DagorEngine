//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>

class IObjectCreator
{
public:
  TMatrix matrix;

  virtual ~IObjectCreator() {}

  IObjectCreator() : matrix(TMatrix::IDENT), stateFinished(false), stateOk(false) {}

  inline bool isFinished() { return stateFinished; }
  inline bool isOk() { return stateOk; }

  virtual bool handleMouseMove(int, int, bool, int, int, bool = true) { return false; }
  virtual bool handleMouseLBPress(int, int, bool, int, int) { return false; }
  virtual bool handleMouseLBRelease(int, int, bool, int, int) { return false; }
  virtual bool handleMouseRBPress(int, int, bool, int, int) { return false; }

  virtual void render() {}

protected:
  bool stateFinished;
  bool stateOk;
};
