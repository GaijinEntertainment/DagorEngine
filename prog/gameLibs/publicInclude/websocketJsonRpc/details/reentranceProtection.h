//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace websocketjsonrpc::details
{

// This class is designed to be used in a single thread only.
class ReentranceProtection final
{
public:
  class CaptureGuard final
  {
  public:
    explicit CaptureGuard(ReentranceProtection &protection_) : protection(protection_), captured(!protection.captured)
    {
      if (captured)
      {
        protection.captured = true;
      }
    }

    // deny copy and move semantics
    CaptureGuard(const CaptureGuard &) = delete;
    CaptureGuard(CaptureGuard &&) = delete;
    CaptureGuard &operator=(const CaptureGuard &) = delete;
    CaptureGuard &operator=(CaptureGuard &&) = delete;

    ~CaptureGuard()
    {
      if (captured)
      {
        protection.captured = false;
      }
    }

    bool isCaptured() const { return captured; }

  private:
    ReentranceProtection &protection;
    const bool captured = false;
  };

public:
  constexpr ReentranceProtection() = default;

  // deny copy and move semantics
  ReentranceProtection(const ReentranceProtection &) = delete;
  ReentranceProtection(ReentranceProtection &&) = delete;
  ReentranceProtection &operator=(const ReentranceProtection &) = delete;
  ReentranceProtection &operator=(ReentranceProtection &&) = delete;

  ~ReentranceProtection() = default;

  CaptureGuard capture() { return CaptureGuard(*this); }

private:
  bool captured = false;
};


} // namespace websocketjsonrpc::details
