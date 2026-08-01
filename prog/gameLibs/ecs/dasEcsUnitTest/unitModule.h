// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix4.h>
#include <daECS/core/componentType.h>

// 64-byte pod, the das float4x4 ecs type; declared here so AOT-compiled das
// tests (this header is emitted into them via aotRequire) see it too
ECS_DECLARE_TYPE(TMatrix4);

extern bool ignore_log_errors;
extern void set_test_value(const char *k, int v);
extern int get_test_value(const char *k); //-1 if missing
extern double make_nan_double();
extern double make_neg_zero_double();

void ignore_log_errors_(const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at);