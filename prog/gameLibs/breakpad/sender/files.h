// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
