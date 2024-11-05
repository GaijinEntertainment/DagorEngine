// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animPdlMgr.h"
static AnimV20::GenericAnimStatesGraph *makeGraphFromRes(const char * /*graphname*/) { return NULL; }

AnimV20::GenericAnimStatesGraph *(*AnimV20::makeGraphFromRes)(const char *graphname) = &::makeGraphFromRes;
