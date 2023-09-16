/*
 * Dagor Engine 3 - Game Libraries
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef DAGOR_GAMELIBS_BREAKPAD_SENDER_UPLOAD_H_
#define DAGOR_GAMELIBS_BREAKPAD_SENDER_UPLOAD_H_
#pragma once

#pragma once

#include <string>

namespace breakpad
{

struct Configuration;

bool upload(const Configuration &cfg, const std::string &url, std::string &response);

} // namespace breakpad

#endif // DAGOR_GAMELIBS_BREAKPAD_SENDER_UPLOAD_H_
