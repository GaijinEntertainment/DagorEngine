// Copyright (C) Gaijin Games KFT.  All rights reserved.

#undef NDEBUG
#define NDEBUG 1
#include "android/dagor_native_app_glue.c"
#include "cpu-features.c"

void android_native_app_destroy(struct android_app *app) { android_app_destroy(app); }
