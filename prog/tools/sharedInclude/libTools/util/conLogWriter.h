//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/iLogWriter.h>


class ConsoleLogWriter : public ILogWriter
{
  bool err;

public:
  ConsoleLogWriter() : err(false) {}

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override;
  bool hasErrors() const override { return err; }

  void startLog() override {}
  void endLog() override {}
};
