//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_lowLatency.h>
#include <ecs/input/hidEventRouter.h>

class LatencyInputEventListener : public ecs::IGenHidEventHandler
{
public:
  LatencyInputEventListener();
  ~LatencyInputEventListener();

  LatencyInputEventListener(const LatencyInputEventListener &) = delete;
  LatencyInputEventListener &operator=(const LatencyInputEventListener &) = delete;

  bool ehIsActive() const override;
  bool gmehMove(const Context &, float dx, float dy) override;
  bool gmehButtonDown(const Context &, int btn_idx) override;
  bool gmehButtonUp(const Context &, int btn_idx) override;
  bool gmehWheel(const Context &, int) override;
  bool gkehButtonDown(const Context &, int btn_idx, bool repeat, wchar_t) override;
  bool gkehButtonUp(const Context &, int btn_idx) override;

private:
  void register_handler();
  void unregister_handler();

  bool registered = false;
};
