//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DagorAsset;

class IEntityCollisionState
{
public:
  static constexpr unsigned HUID = 0xABB48677u; // IEntityCollisionState

  virtual void setCollisionFlag(bool flag) = 0;
  virtual bool hasCollision() = 0;
  virtual int getAssetNameId() = 0;
  virtual DagorAsset *getAsset() = 0;
};
