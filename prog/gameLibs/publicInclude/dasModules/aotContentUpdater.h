//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <contentUpdater/embeddedUpdater.h>
#include <contentUpdater/fsUtils.h>
#include <folders/folders.h>


namespace bind_dascript
{
inline const char *get_state_text(das::Context *context)
{
  const eastl::string str{embeddedupdater::get_state_text()};
  return context->stringHeap->allocateString(str.c_str(), str.length());
}

inline const char *get_loc_text(const char *loc, das::Context *context)
{
  const eastl::string str{embeddedupdater::get_loc_text(loc)};
  return context->stringHeap->allocateString(str.c_str(), str.length());
}

inline const char *get_lang(das::Context *context)
{
  const eastl::string str{embeddedupdater::get_lang()};
  return context->stringHeap->allocateString(str.c_str(), str.length());
}


inline const char *get_error_text(das::Context *context)
{
  const eastl::string str{embeddedupdater::get_error_text()};
  return context->stringHeap->allocateString(str.c_str(), str.length());
}


inline const char *get_fail_text(das::Context *context)
{
  const eastl::string str{embeddedupdater::get_fail_text()};
  return context->stringHeap->allocateString(str.c_str(), str.length());
}


inline const char *get_abs_path(const char *fname, das::Context *context)
{
  const eastl::string str = updater::fs::join_path({folders::get_game_dir().c_str(), fname ? fname : ""});
  return context->stringHeap->allocateString(str.c_str(), str.length());
}
} // namespace bind_dascript
