//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>

class SqModules;

namespace force_feedback
{
void bind_sq(SqModules *moduleMgr);

namespace rumble
{
void sqfade(const char *name, const Sqrat::Table &desc);
void sqtimed(const char *name, const Sqrat::Table &desc);
void sqfunc(const char *name, const Sqrat::Table &desc);
} // namespace rumble
}; // namespace force_feedback
