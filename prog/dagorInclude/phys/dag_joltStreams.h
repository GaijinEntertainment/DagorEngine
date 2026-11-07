//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <generic/dag_span.h>
#include <string.h>

template <class Bytes>
class JoltByteStreamOut final : public JPH::StreamOut
{
public:
  explicit JoltByteStreamOut(Bytes &bytes_) : bytes(bytes_) {}
  void WriteBytes(const void *data, size_t size) override
  {
    const JPH::uint8 *src = static_cast<const JPH::uint8 *>(data);
    bytes.insert(bytes.end(), src, src + size);
  }
  bool IsFailed() const override { return false; }

private:
  Bytes &bytes;
};

class JoltByteStreamIn final : public JPH::StreamIn
{
public:
  explicit JoltByteStreamIn(dag::ConstSpan<JPH::uint8> bytes) : at(bytes.data()), end(bytes.data() + bytes.size()) {}
  void ReadBytes(void *data, size_t size) override
  {
    failed = failed || size > size_t(end - at);
    if (failed)
      memset(data, 0, size);
    else
    {
      memcpy(data, at, size);
      at += size;
    }
  }
  bool IsEOF() const override { return failed; }
  bool IsFailed() const override { return failed; }

private:
  const JPH::uint8 *at, *end;
  bool failed = false;
};
