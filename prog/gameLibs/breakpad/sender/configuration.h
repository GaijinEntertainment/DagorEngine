// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <map>
#include <string>
#include <ostream>
#include <stdio.h>
#include <stdint.h>
#include <vector>

namespace breakpad
{
namespace id
{

const std::string dump = "upload_file_minidump";
const std::string log = "upload_file_log";
inline std::string file(const std::string &path)
{
#if _TARGET_PC_WIN
  const char separator = '\\';
#else
  const char separator = '/';
#endif

  std::string fname = path.substr(path.rfind(separator) + 1);
  for (auto &c : fname)
    if (!isalnum(c))
      c = '_';

  return fname;
}

} // namespace id

struct Configuration
{
  Configuration(int argc, char **argv) { update(argc, argv); }

  void update(int, char **);
  bool isValid() const;
  void addComment(const std::string &k, const std::string &v);

  std::map<std::string, std::string> params;
  std::map<std::string, std::string> files;
  bool silent = false;
  bool haveDump = false;
  std::vector<std::string> urls = {"http://palvella.gaijin.net/submit"};

  std::string userAgent = "Breakpad 1.0";
  std::string product;
  std::string productTitle;
  time_t timestamp = 0;
  std::string systemId;
  std::string userId;
  std::string crashType;
  bool sendLogs = true;
  bool allowEmail = false;
  bool restartParent = false;
  std::vector<std::string> parent;

  struct Stats
  {
    std::string host = "client-stats.gaijin.net";
    uint16_t port = 20011;
    std::string env = "production";
  } stats;
}; // struct Configuration

std::ostream &operator<<(std::ostream &, const Configuration &);

} // namespace breakpad
