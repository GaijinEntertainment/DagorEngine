// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
