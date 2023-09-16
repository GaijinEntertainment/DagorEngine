//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <libTools/util/iLogWriter.h>


class ConsoleLogWriter : public ILogWriter
{
  bool err;

public:
  ConsoleLogWriter() : err(false) {}

  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum);
  virtual bool hasErrors() const { return err; }

  virtual void startLog() {}
  virtual void endLog() {}
};
