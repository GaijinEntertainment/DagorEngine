// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

extern bool ignore_log_errors;
extern void set_test_value(const char *k, int v);
extern int get_test_value(const char *k); //-1 if missing

void ignore_log_errors_(const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at);