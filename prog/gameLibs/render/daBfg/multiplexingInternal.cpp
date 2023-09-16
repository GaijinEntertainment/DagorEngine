#include <multiplexingInternal.h>


namespace dabfg
{

intermediate::MultiplexingIndex multiplexing_index_to_ir(multiplexing::Index frontMultiplexingIndex, multiplexing::Extents extents)
{
  return static_cast<intermediate::MultiplexingIndex>(
    ((frontMultiplexingIndex.viewport) * extents.superSamples + frontMultiplexingIndex.superSample) * extents.subSamples +
    frontMultiplexingIndex.subSample);
}

uint32_t multiplexing_extents_to_ir(multiplexing::Extents extents)
{
  multiplexing::Index lastIndex{extents.superSamples - 1, extents.subSamples - 1, extents.viewports - 1};
  return eastl::to_underlying(multiplexing_index_to_ir(lastIndex, extents)) + 1;
}

multiplexing::Index multiplexing_index_from_ir(intermediate::MultiplexingIndex backMultiplexingIndex, multiplexing::Extents extents)
{
  return multiplexing::Index{backMultiplexingIndex / extents.subSamples % extents.superSamples,
    backMultiplexingIndex % extents.subSamples,
    backMultiplexingIndex / (extents.superSamples * extents.subSamples) % extents.viewports};
}

bool multiplexing::mode_has_flag(Mode mode, Mode flag)
{
  return (eastl::to_underlying(mode) & eastl::to_underlying(flag)) == eastl::to_underlying(flag);
}

bool multiplexing::operator==(const Extents &fst, const Extents &snd)
{
  return fst.superSamples == snd.superSamples && fst.subSamples == snd.subSamples && fst.viewports == snd.viewports;
}

bool multiplexing::operator==(const Index &fst, const Index &snd)
{
  return fst.superSample == snd.superSample && fst.subSample == snd.subSample && fst.viewport == snd.viewport;
}

bool multiplexing::index_inside_extents(Index idx, Extents extents)
{
  return idx.superSample < extents.superSamples && idx.subSample < extents.subSamples && idx.viewport < extents.viewports;
}

auto multiplexing::next_index(Index idx, Extents extents) -> Index
{

  if (++idx.superSample < extents.superSamples)
    return idx;
  idx.superSample = 0;

  if (++idx.subSample < extents.subSamples)
    return idx;
  idx.subSample = 0;

  ++idx.viewport;
  return idx;
}

auto multiplexing::extents_for_node(Mode mode, Extents extents) -> Extents
{
  uint32_t superSampleCount = mode_has_flag(mode, Mode::SuperSampling) ? extents.superSamples : 1;
  uint32_t subSampleCount = mode_has_flag(mode, Mode::SubSampling) ? extents.subSamples : 1;
  uint32_t viewportCount = mode_has_flag(mode, Mode::Viewport) ? extents.viewports : 1;

  return Extents{superSampleCount, subSampleCount, viewportCount};
}

auto multiplexing::clamp(Index idx, Extents extents) -> Index
{
  using eastl::min;
  return Index{min(idx.superSample, extents.superSamples - 1), min(idx.subSample, extents.subSamples - 1),
    min(idx.viewport, extents.viewports - 1)};
}

bool multiplexing::less_multiplexed(Mode fst, Mode snd)
{
  return (eastl::to_underlying(fst) & eastl::to_underlying(snd)) == eastl::to_underlying(fst) && fst != snd;
}

} // namespace dabfg
