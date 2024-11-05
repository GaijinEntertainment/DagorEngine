// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"

#include <stdlib.h>
#include <unistd.h>

namespace folders
{
namespace internal
{
extern String game_name;

String get_gamedata_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String gameData;
  gameData.printf(260, "%s/%s/%s", getenv("HOME"), "Documents", game_name);
  return gameData;
}
} // namespace internal
} // namespace folders
