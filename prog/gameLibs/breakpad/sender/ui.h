/*
 * Dagor Engine 3 - Game Libraries
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef DAGOR_GAMELIBS_BREAKPAD_SENDER_UI_H_
#define DAGOR_GAMELIBS_BREAKPAD_SENDER_UI_H_
#pragma once

#include "configuration.h"

#include <string>

namespace breakpad
{

namespace ui
{
enum Response
{
  Close = 0,
  Send = 1,
  Restart = 2
};

Response query(breakpad::Configuration &cfg);
Response notify(breakpad::Configuration &cfg, const std::string &msg, bool success);

} // namespace ui
} // namespace breakpad

#endif // DAGOR_GAMELIBS_BREAKPAD_SENDER_UI_H_
