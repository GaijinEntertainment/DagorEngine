/*
 * Dagor Engine 3 - Game Libraries
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef DAGOR_GAMELIBS_BREAKPAD_SENDER_FILES_H_
#define DAGOR_GAMELIBS_BREAKPAD_SENDER_FILES_H_
#pragma once

#include <string>

namespace breakpad
{

struct Configuration;

namespace files
{

namespace path
{
#if _TARGET_PC_WIN
const char separator = '\\';
#else
const char separator = '/';
#endif

std::string normalize(const std::string &path);
} // namespace path

std::string compress(const std::string &path);
void prepare(Configuration &cfg);
void postprocess(Configuration &cfg, const std::string &guid);

} // namespace files
} // namespace breakpad

#endif // DAGOR_GAMELIBS_BREAKPAD_SENDER_FILES_H_
