//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace darg
{

// Host-provided scope wrapping the async daScript worker compile. open() is
// called on the cpujobs worker at the top of DasLoadAndCompileJob::doJob()
// (returns an opaque host handle); close() runs on the worker on every doJob()
// return path. Lets a host establish whatever thread-local state its compile-
// time macros need on the worker thread - e.g. skyquake, which thread-pins its
// ECS EntityManager, scopes a valid one here so the compile does not assert off
// the pinned thread. Pure void* keeps daRg host/daScript-free; null in dargbox
// / non-host -> no-op.
void set_das_async_compile_scope_hooks(void *(*open)(), void (*close)(void *));

} // namespace darg
