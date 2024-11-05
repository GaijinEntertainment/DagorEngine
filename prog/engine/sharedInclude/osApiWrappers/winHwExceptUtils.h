// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace WinHwExceptionUtils
{
const char *getExceptionTypeString(uint32_t code);
bool checkException(uint32_t code);

void parseExceptionInfo(EXCEPTION_POINTERS *eptr, const char *title, char *text_buffer, int text_buffer_size,
  const char **call_stack_ptr);

void setupCrashDumpFileName();
} // namespace WinHwExceptionUtils
