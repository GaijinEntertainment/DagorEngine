//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_log.h>

class TextureGenLogger
{
protected:
  int startLevel;

public:
  TextureGenLogger() : startLevel(LOGLEVEL_WARN) {}
  virtual ~TextureGenLogger() {}

  void setStartLevel(int level) { startLevel = level; }

  virtual void log(int level, const String &s)
  {
    if (level <= startLevel)
      logmessage(level, "%s", s.str());
  }
};
