// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "multiplexingInternal.h"


namespace dafg
{

intermediate::MultiplexingIndex multiplexing_index_to_ir(multiplexing::Index frontMultiplexingIndex, multiplexing::Extents extents)
{
  return static_cast<intermediate::MultiplexingIndex>(
    (((frontMultiplexingIndex.subCamera) * extents.viewports + frontMultiplexingIndex.viewport) * extents.superSamples +
      frontMultiplexingIndex.superSample) *
      extents.subSamples +
    frontMultiplexingIndex.subSample);
}

uint32_t multiplexing_extents_to_ir(multiplexing::Extents extents)
{
  return extents.superSamples * extents.subSamples * extents.viewports * extents.subCameras;
}

multiplexing::Index multiplexing_index_from_ir(intermediate::MultiplexingIndex backMultiplexingIndex, multiplexing::Extents extents)
{
  const uint32_t subSuperExt = extents.subSamples * extents.superSamples;

  const uint32_t subSample = backMultiplexingIndex % extents.subSamples;
  const uint32_t superSample = backMultiplexingIndex / extents.subSamples % extents.superSamples;
  const uint32_t viewport = backMultiplexingIndex / subSuperExt % extents.viewports;
  const uint32_t subCamera = backMultiplexingIndex / (subSuperExt * extents.viewports);

  return multiplexing::Index{superSample, subSample, viewport, subCamera};
}

bool multiplexing::mode_has_flag(Mode mode, Mode flag)
{
  return (eastl::to_underlying(mode) & eastl::to_underlying(flag)) == eastl::to_underlying(flag);
}

bool multiplexing::index_inside_extents(Index idx, Extents extents)
{
  return idx.superSample < extents.superSamples && idx.subSample < extents.subSamples && idx.viewport < extents.viewports &&
         idx.subCamera < extents.subCameras;
}

auto multiplexing::next_index(Index idx, Extents extents) -> Index
{

  if (++idx.superSample < extents.superSamples)
    return idx;
  idx.superSample = 0;

  if (++idx.subSample < extents.subSamples)
    return idx;
  idx.subSample = 0;

  if (++idx.viewport < extents.viewports)
    return idx;
  idx.viewport = 0;

  ++idx.subCamera;
  return idx;
}

auto multiplexing::extents_for_node(Mode mode, Extents extents) -> Extents
{
  uint32_t superSampleCount = mode_has_flag(mode, Mode::SuperSampling) ? extents.superSamples : 1;
  uint32_t subSampleCount = mode_has_flag(mode, Mode::SubSampling) ? extents.subSamples : 1;
  uint32_t viewportCount = mode_has_flag(mode, Mode::Viewport) ? extents.viewports : 1;
  uint32_t subCamerasCount = mode_has_flag(mode, Mode::CameraInCamera) ? extents.subCameras : 1;

  return Extents{superSampleCount, subSampleCount, viewportCount, subCamerasCount};
}

auto multiplexing::clamp(Index idx, Extents extents) -> Index
{
  using eastl::min;
  return Index{min(idx.superSample, extents.superSamples - 1), min(idx.subSample, extents.subSamples - 1),
    min(idx.viewport, extents.viewports - 1), min(idx.subCamera, extents.subCameras - 1)};
}

auto multiplexing::clamp_and_wrap(Index idx, Extents extents) -> Index
{
  return Index{idx.superSample % extents.superSamples, idx.subSample % extents.subSamples,
    eastl::min(idx.viewport, extents.viewports - 1), eastl::min(idx.subCamera, extents.subCameras - 1)};
}

bool multiplexing::less_multiplexed(Mode fst, Mode snd, Extents extents)
{
  // If not multiplexing on that, implicitly allow requesting less multiplexed resources
  uint32_t first = eastl::to_underlying(fst);
  uint32_t second = eastl::to_underlying(snd);
  if (extents.subSamples == 1)
    first |= eastl::to_underlying(Mode::SubSampling);
  if (extents.superSamples == 1)
    first |= eastl::to_underlying(Mode::SuperSampling);
  if (extents.viewports == 1)
    first |= eastl::to_underlying(Mode::Viewport);
  if (extents.subCameras == 1)
    first |= eastl::to_underlying(Mode::CameraInCamera);
  return (first & second) == first && first != second;
}

} // namespace dafg
