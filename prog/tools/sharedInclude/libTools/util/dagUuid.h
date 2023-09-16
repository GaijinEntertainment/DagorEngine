//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>


/// 128-bit Universally unique ID.
/// @ingroup EditorCore
/// @ingroup Misc
struct DagUUID
{
  /// Array for storing UUID data.
  unsigned uuid[4];

  /// Clear UUID (set all bitts to 0).
  inline void clear() { memset(uuid, 0, sizeof(uuid)); }

  /// Test for equality with specified UUID.
  /// @param[in] id - reference to UUID to test
  /// @return @b true if UUID equals specified UUID, @b false in other case
  inline bool equal(const DagUUID &id) { return !memcmp(this, &id, sizeof(id)); }

  /// Generate UUID. The function uses Windows SDK's UuidCreate().
  /// @param[out] out - created UUID
  static void generate(DagUUID &out);
};
