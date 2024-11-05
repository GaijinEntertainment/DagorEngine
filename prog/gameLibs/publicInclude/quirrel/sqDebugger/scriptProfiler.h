//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
#include <util/dag_string.h>

class SqModules;


namespace scriptprofile
{
#if DAGOR_DBGLEVEL > 0
void initProfile(HSQUIRRELVM vm);
bool getProfileResult(String &res);
bool isStarted();
void start(HSQUIRRELVM vm);
void stop(HSQUIRRELVM vm);
void onEnterFunction(HSQUIRRELVM vm, const HSQOBJECT &func);
void onLeaveFunction(HSQUIRRELVM vm);
void register_profiler_module(HSQUIRRELVM vm, SqModules *module_mgr);
void shutdown(HSQUIRRELVM vm);

#else
inline void initProfile(HSQUIRRELVM) {}
inline bool getProfileResult(String &) { return false; }
inline bool isStarted() { return false; }
inline void start(HSQUIRRELVM) {}
inline void stop(HSQUIRRELVM) {}
inline void onEnterFunction(HSQUIRRELVM, const HSQOBJECT &) {}
inline void onLeaveFunction(HSQUIRRELVM) {}
inline void register_profiler_module(HSQUIRRELVM, SqModules *);
inline void shutdown(HSQUIRRELVM) {}
#endif
} // namespace scriptprofile
