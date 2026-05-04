//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace shader_assert
{
void readback_all();
void reset_all();

// @TODO: remove, legacy
inline void init() {}
inline void close() {}
inline void readback() { readback_all(); }
inline void bind(int) {}
} // namespace shader_assert
