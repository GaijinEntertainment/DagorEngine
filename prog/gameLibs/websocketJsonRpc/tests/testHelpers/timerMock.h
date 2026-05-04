// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocketJsonRpc/details/abstractionLayer.h>


namespace tests
{


class TimerMock final : public websocketjsonrpc::abstraction::Timer
{
public:
  using TickCount = websocketjsonrpc::TickCount;

private:
  TickCount getCurrentTickCount() const override
  {
    return currentTime ? currentTime : 1; // never return zero, it may break some tests
  }
  TickCount getTicksPerSecond() const override
  {
    return 10'000'000'000ULL; // some value greater than 2^32 and also not equal to 1000 (not milliseconds)
  }

public:
  int getCurrentTimeMs() const
  {
    return static_cast<int>(currentTime ? currentTime * 1000 / getTicksPerSecond() : 1); // never return zero, it may break some tests
  }

  void addTimeMs(int diff_ms) { currentTime += diff_ms * getTicksPerSecond() / 1000; }

  int getRelativeMs(int diff_ms) const { return static_cast<int>(currentTime * 1000 / getTicksPerSecond() + diff_ms); }

private:
  TickCount currentTime = 0;
};


} // namespace tests
