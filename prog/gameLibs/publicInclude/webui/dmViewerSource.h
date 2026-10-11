//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace dm
{
struct DataBlockRWHelper;
struct ObjectDescriptor;
struct CollisionDmObject;
} // namespace dm

namespace webui
{
struct IDmViewerSource
{
  virtual const dm::DataBlockRWHelper &getBlkHelper() const = 0;
  virtual dm::CollisionDmObject getCollisionDmObject(const dm::ObjectDescriptor &object) const = 0;
};
} // namespace webui
