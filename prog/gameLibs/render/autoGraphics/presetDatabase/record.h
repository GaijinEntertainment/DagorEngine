// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>

namespace auto_graphics::auto_quality_preset::detail
{

struct PresetDatabaseRecord
{
  struct Key
  {
    int cpuScore = 0;
    int gpuScore = 0;
    int vramMb = 0;

    bool valid() const { return cpuScore > 0 && gpuScore > 0 && vramMb > 0; }

    bool operator<(const Key &user_key) const
    {
      constexpr float CPU_SCORE_TOLERANCE = 1.1f;
      constexpr float GPU_SCORE_TOLERANCE = 1.1f;
      // user_key.<cpu|gpu>Score assumes to be always > 0, use max for unexpected case
      const float cpuScoreScale = static_cast<float>(cpuScore) / static_cast<float>(eastl::max(user_key.cpuScore, 1));
      const float gpuScoreScale = static_cast<float>(gpuScore) / static_cast<float>(eastl::max(user_key.gpuScore, 1));
      return cpuScoreScale <= CPU_SCORE_TOLERANCE && gpuScoreScale <= GPU_SCORE_TOLERANCE && vramMb <= user_key.vramMb;
    }
  };

  struct Value
  {
    int presetId = INVALID_PRESET_ID;
    IPoint2 resolution = IPoint2(0, 0);

    static constexpr int INVALID_PRESET_ID = -1;

    bool valid() const { return presetId != INVALID_PRESET_ID && resolution.x > 0 && resolution.y > 0; }

    bool isBetterByResolution(const Value &other) const
    {
      const int thisResArea = resolution.x * resolution.y;
      const int otherResArea = other.resolution.x * other.resolution.y;
      if (thisResArea > otherResArea)
        return true;
      // prefer higher preset if resolutions are same
      if (thisResArea == otherResArea)
        return presetId > other.presetId;
      return false;
    }

    bool isBetterByGraphics(const Value &other) const
    {
      if (presetId > other.presetId)
        return true;
      // prefer higher resolution if presets are same
      if (presetId == other.presetId)
      {
        const int thisResArea = resolution.x * resolution.y;
        const int otherResArea = other.resolution.x * other.resolution.y;
        return thisResArea > otherResArea;
      }
      return false;
    }
  };

  Key key;
  Value value;

  PresetDatabaseRecord(const DataBlock &blk, const DataBlock &preset_dict) :
    key{
      .cpuScore = blk.getInt("cpuScore", 0),
      .gpuScore = blk.getInt("gpuScore", 0),
      .vramMb = blk.getInt("vram", 0),
    },
    value{
      .presetId = preset_dict.getInt(blk.getStr("preset", ""), Value::INVALID_PRESET_ID),
      .resolution = blk.getIPoint2("resolution", IPoint2(0, 0)),
    }
  {}

  bool valid() const { return key.valid() && value.valid(); }
};

} // namespace auto_graphics::auto_quality_preset::detail