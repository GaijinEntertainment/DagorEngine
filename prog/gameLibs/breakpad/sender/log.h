// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assert.h>
#include <fstream>
#include <string>

namespace breakpad
{

class Log : public std::ofstream
{
public:
  Log(const std::string &dst) : std::ofstream(dst.c_str(), std::ios::out | std::ios::trunc), path(dst) {}

  void move(const std::string &path);
  std::string getPath() { return this->path; }

private:
  std::string path;
}; // class Log

extern Log log;

} // namespace breakpad
