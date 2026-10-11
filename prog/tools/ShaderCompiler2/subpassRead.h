// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class RenderPassObject;
class RenderPassGroup;

namespace semantic
{
struct SubpassReadResolution
{
  int readPos = -1;
  int targetIdx = -1;
  bool multisampled = false;
  bool emulated = false; // SRV in the read window instead of an input attachment

  bool operator==(const SubpassReadResolution &) const = default;
};
} // namespace semantic

struct RenderPassOptIn
{
  const RenderPassObject *pass = nullptr;
  int subpassIdx = -1;
  const RenderPassGroup *group = nullptr;

  bool operator==(const RenderPassOptIn &) const = default;
  explicit operator bool() const { return pass || group; }
  static RenderPassOptIn ofPass(const RenderPassObject *p, int subpass_idx) { return {p, subpass_idx, nullptr}; }
  static RenderPassOptIn ofGroup(const RenderPassGroup *g) { return {nullptr, -1, g}; }
  bool readsUnderAllowSplit() const;
  int readCount() const;
  int readWindowBase() const;
  const char *targetAtRegister(int reg) const;
};
