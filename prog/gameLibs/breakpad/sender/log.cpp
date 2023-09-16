#include "log.h"

#include <cstdio>


namespace breakpad
{

Log log("bpreport.log");

void Log::move(const std::string &new_path)
{
  if (new_path == path)
    return;

  flush();
  close();

  open(new_path.c_str(), std::ios::out | std::ios::trunc);
  {
    std::ifstream src(path.c_str());
    *this << src.rdbuf();
  }
  std::remove(path.c_str());

  path = new_path;
}

} // namespace breakpad
