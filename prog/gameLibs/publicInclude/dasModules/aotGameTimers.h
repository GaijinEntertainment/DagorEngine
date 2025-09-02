//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daGame/timers.h>
#include <dasModules/dasContextLockGuard.h>
#include <dasModules/dasScriptsLoader.h>
#include <dasModules/dasSharedStack.h>

namespace bind_dascript
{
inline game::timer_handle_t game_timer_set(const das::Lambda &callback, float delay, das::Context *context,
  das::LineInfoArg *line_info)
{
  das::SimFunction **fnMnh = (das::SimFunction **)callback.capture;
  if (DAGOR_UNLIKELY(!fnMnh))
    context->throw_error_at(line_info, "invoke null lambda");
  das::SimFunction *simFunc = *fnMnh;
  if (DAGOR_UNLIKELY(!simFunc))
    context->throw_error_at(line_info, "invoke null function");
  auto pCapture = callback.capture;
  ContextLockGuard lockg{*context};

  auto wrapper = [pCapture, simFunc, context, lockg = eastl::move(lockg)]() {
    vec4f argI[1];
    argI[0] = das::cast<void *>::from(pCapture);
    if (!context->ownStack)
    {
      das::SharedFramememStackGuard guard(*context);
      (void)context->call(simFunc, argI, 0);
    }
    else
      (void)context->call(simFunc, argI, 0);
  };

  game::Timer timer;
  game::g_timers_mgr->setTimer(timer, eastl::move(wrapper), delay);

  return timer ? timer.release() : game::INVALID_TIMER_HANDLE;
}

inline void game_timer_clear(game::timer_handle_t &handle)
{
  if (handle != game::INVALID_TIMER_HANDLE)
  {
    game::g_timers_mgr->clearTimer(handle);
    handle = game::INVALID_TIMER_HANDLE;
  }
}
} // namespace bind_dascript
