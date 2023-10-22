#pragma once
namespace da_profiler
{

enum MessageType : uint16_t
{
  Connected,
  Disconnect,
  SetSettings,
  StartInfiniteCapture,
  StopInfiniteCapture,
  CancelInfiniteCapture,
  Capture,
  TurnSampling,
  CancelProfiling,
  Heartbeat,
  PluginCommand,
};

};