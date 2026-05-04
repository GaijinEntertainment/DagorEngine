//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <util/dag_strUtil.h>

namespace sndsys
{
typedef int sound_handle_t;
enum : sound_handle_t
{
  INVALID_SOUND_HANDLE = 0
};

enum PoolType
{
  POOLTYPE_EVENT = 0,
  POOLTYPE_STREAM,
  POOLTYPE_OCCLUSION_BLOB,
  POOLTYPE_TYPES_COUNT
};

struct Handle
{
  Handle() = default;
  explicit Handle(sound_handle_t h) : handle(h) {}
  void reset() { handle = INVALID_SOUND_HANDLE; }
  explicit operator sound_handle_t() const { return handle; }
  explicit operator bool() const { return handle != INVALID_SOUND_HANDLE; }

protected:
  sndsys::sound_handle_t handle = INVALID_SOUND_HANDLE;
};

struct EventHandle final : public Handle
{
  EventHandle() = default;
  EventHandle(sound_handle_t h) : Handle(h) {}
  operator sound_handle_t() const { return handle; }
  bool operator==(const EventHandle &rhs) const { return handle == rhs.handle; }
  bool operator!=(const EventHandle &rhs) const { return handle != rhs.handle; }
};

struct StreamHandle final : public Handle
{
  StreamHandle() = default;
  explicit StreamHandle(sound_handle_t h) : Handle(h) {}
  bool operator==(const StreamHandle &rhs) const { return handle == rhs.handle; }
  bool operator!=(const StreamHandle &rhs) const { return handle != rhs.handle; }
};

struct OcclusionBlobHandle final : public Handle
{
  OcclusionBlobHandle() = default;
  explicit OcclusionBlobHandle(sound_handle_t h) : Handle(h) {}
  bool operator==(const OcclusionBlobHandle &rhs) const { return handle == rhs.handle; }
  bool operator!=(const OcclusionBlobHandle &rhs) const { return handle != rhs.handle; }
};

struct FMODGUID
{
  uint64_t bits[2] = {0};

  const char *hex(char *dst, size_t len) const { return data_to_str_hex_buf(dst, len, (const char *)bits, sizeof(bits)); }

  bool operator==(const FMODGUID &other) const { return bits[0] == other.bits[0] && bits[1] == other.bits[1]; }
  bool operator!=(const FMODGUID &other) const { return bits[0] != other.bits[0] || bits[1] != other.bits[1]; }
};
} // namespace sndsys
