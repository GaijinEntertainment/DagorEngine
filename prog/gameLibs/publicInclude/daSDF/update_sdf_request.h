//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// SYNC_BEST can only return all_ok
// SYNC_AVAILABLE can return all_ok and some_ready
// ASYNC_REQUEST_BEST_AVAILABLE can return all_ok, NOT_READY and some_ready
// ASYNC_REQUEST_BEST_ONLY can return all_ok or NOT_READY only
enum class UpdateSDFQualityRequest
{
  SYNC,
  ASYNC_REQUEST_PRECACHE,
  ASYNC_REQUEST_BEST_ONLY
};

// all_ok - all were loaded. the only possible result for SYNC_BEST
// some_ready - some were loaded in required quality, some not. We can render, but we have to re-render later
// not_ready - there are missing objects, we can't render at all

enum class UpdateSDFQualityStatus
{
  ALL_OK,
  NOT_READY
};
