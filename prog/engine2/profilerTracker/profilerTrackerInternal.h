#pragma once

#include <3d/dag_profilerTracker.h>

#include <EASTL/vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class DataBlock;

// This class is needed, because the hashing function has to support both a eastl::string and c
// string in a consistent way
struct StrCstrHash
{
  size_t operator()(const char *s) const
  {
    // implementation of eastl::hash<eastl::string>
    const unsigned char *p = (const unsigned char *)s; // To consider: limit p to at most
                                                       // 256 chars.
    unsigned int c, result = 2166136261U;              // We implement an FNV-like string hash.
    while ((c = *p++) != 0)                            // Using '!=' disables compiler warnings.
      result = (result * 16777619) ^ c;
    return (size_t)result;
  }

  size_t operator()(const eastl::string &s) const { return (*this)(s.c_str()); }
};

class Tracker
{
public:
  using TrackSet = ska::flat_hash_set<eastl::string, StrCstrHash>;
  Tracker(uint32_t history_size);

  void startFrame();
  void updateRecord(const eastl::string &group, const eastl::string &label, float value, uint32_t frame_offset);
  void updateRecord(const char *group, const char *label, float value, uint32_t frame_offset);
  void clear();

  struct PlotData
  {
    uint32_t skipped = 0;
    uint32_t size1 = 0;
    uint32_t size2 = 0;
    const float *values1 = nullptr;
    const float *values2 = nullptr;
  };

  PlotData getValues(const eastl::string &group, const eastl::string &label) const;

  const uint32_t getHistorySize() const { return historySize; }
  struct Record
  {
    eastl::string group;
    eastl::string label;
    eastl::vector<float> values;
    uint32_t valuesEnd;
    uint16_t skippedCount;
  };
  using SubgroupMap = ska::flat_hash_map<eastl::string, Record>;
  using Map = ska::flat_hash_map<eastl::string, SubgroupMap>;
  using HideMap = ska::flat_hash_map<eastl::string, bool>;

  const Map &getMap() const { return records; }
  const HideMap &getVisibleMap() const { return visibleGroups; }
  HideMap &getVisibleMap() { return visibleGroups; }
  bool isPaused() const { return paused; }
  void setPaused(bool p);

  void trackProfiler(eastl::string &&label, bool need_save = true);
  void untrackProfiler(const eastl::string &label);

  const TrackSet &getTrackedProfilers() const { return trackedProfilers; }

  bool save() const;
  bool load();

private:
  bool paused = false;
  uint32_t historySize;
  Map records;
  HideMap visibleGroups;
  TrackSet trackedProfilers;
};
