// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// cleanup queue tags
// used to handle specific logic of object destruction, i.e. in-type ordering, special delays, etc
//
// separated from cleanup queue to avoid including it for every resource

namespace drv3d_vulkan
{

enum class CleanupTag
{
  DESTROY,
  DESTROY_TOP,
  DESTROY_BOTTOM
};

}
