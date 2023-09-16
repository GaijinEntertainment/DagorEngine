#pragma once


namespace dabfg
{

enum class AutoResTypeNameId : uint32_t
{
  Invalid = ~0u
};

struct AutoResolutionData
{
  AutoResTypeNameId id;
  float multiplier;
};

} // namespace dabfg
