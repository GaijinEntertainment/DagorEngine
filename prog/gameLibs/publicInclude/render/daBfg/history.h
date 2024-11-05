//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace dabfg
{

/**
 * \brief History policy for a resource. Must always be specified on creation.
 */
enum class History : uint8_t
{
  /// Resource does not need history, requesting it will result in an error message.
  No,

  /**
   * History is required and should be an all-zero image on the first frame.
   * This might be slow, so should only be used when necessary.
   * NOTE: for CPU-resources, this simply default-constructs them
   */
  ClearZeroOnFirstFrame,

  /**
   * History is required and it doesn't matter what it contains on the first frame
   * This is the preferred variant as it is faster, but it might cause
   * artifacts and trash value propagation for cumulative type algorithms.
   * NOTE: for CPU-resources, this simply default-constructs them
   */
  DiscardOnFirstFrame
};

} // namespace dabfg
