//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/detail/resourceType.h>


namespace dabfg
{

using TypeErasedCall = void (*)(void *);

struct BlobDescription
{
  ResourceSubtypeTag typeTag;
  size_t size;
  size_t alignment;
  TypeErasedCall activate;
  TypeErasedCall deactivate;
};

struct BlobView
{
  void *data = nullptr;
  ResourceSubtypeTag typeTag = ResourceSubtypeTag::Invalid;
};

} // namespace dabfg
