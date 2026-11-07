// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_rendInstRes.h>

namespace levelprofiler
{

class ICopyProvider;
class ILpTableExportSource;
class LevelProfilerUI;

enum class TextureColumn : int
{
  NAME = 0,
  FORMAT,
  QUALITY,
  WIDTH,
  HEIGHT,
  MIPS,
  MEM_SIZE,
  USAGE,
  BASE_COUNT // Number of static columns - must always be last
};

enum class RiColumn : int
{
  NAME = 0,
  COUNT_ON_MAP,
  BSPHERE_RAD,
  BBOX_RAD,
  LOD_COUNT,
  PHYS_TRIS,
  TRACE_TRIS,
  BASE_COUNT // Number of static columns - must always be last
};

using ColumnIndex = int;
inline constexpr ColumnIndex operator*(TextureColumn c) { return static_cast<ColumnIndex>(c); }
inline constexpr ColumnIndex operator*(RiColumn c) { return static_cast<ColumnIndex>(c); }

static_assert(static_cast<int>(TextureColumn::BASE_COUNT) == static_cast<int>(TextureColumn::USAGE) + 1,
  "TextureColumn::BASE_COUNT must follow last data column");
static_assert(static_cast<int>(RiColumn::BASE_COUNT) == static_cast<int>(RiColumn::TRACE_TRIS) + 1,
  "RiColumn::BASE_COUNT must follow last data column");

using ProfilerString = eastl::string;
using TextureID = unsigned int;

// Tracks resource reference counts
struct TextureUsage
{
  int total = 0;  // Total references across all assets
  int unique = 0; // Number of unique assets referencing
  int shared = 0; // Number of shared assets referencing

  TextureUsage() = default;
  TextureUsage(int t, int u) : total(t), unique(u), shared(t - u) {}
};

// A texture used by one asset is unique, not shared, so the shared-usage range starts at two.
static constexpr int SHARED_TEXTURE_USAGE_MIN = 2;

class IDataCollector
{
public:
  virtual ~IDataCollector() = default;
  virtual void collect() = 0;
  virtual void clear() = 0;
  // False while the source data is not safe to read; the profiler then skips the whole collect.
  // A collector whose walk outlives collect() also checks it in continueCollect().
  virtual bool canCollect() const { return true; }

  // A collector whose walk outlives collect() overrides these; the defaults describe a synchronous
  // one, already done when collect() returns. Driving, pausing and progress go through here, so
  // those paths never name a specific collector.
  virtual void continueCollect() {}
  virtual bool isCollecting() const { return false; }
  virtual bool isPaused() const { return false; }
  virtual void pauseCollection() {}
  virtual void resumeCollection() {}
  virtual float getCollectProgress() const { return 1.0f; }
};

class IProfilerModule
{
public:
  virtual ~IProfilerModule() = default;
  // init() only wires the module up. It runs at renderer creation, before any level is loaded,
  // so collecting here would just snapshot an empty world; the Collect Data button collects.
  virtual void init() {}
  virtual void shutdown() {}
  virtual void drawUI() = 0;

  // Called on the tab modules when the data they display changed: once when collect replaces it,
  // again when an incremental walk finishes filling it in, so it has to be idempotent. A data
  // module does its own post-collect work inside collect() and is not dispatched here. Unlike
  // init() it must keep what the user set up (filters, selection) that still resolves.
  virtual void onDataCollected() {}

  virtual ICopyProvider *getCopyProvider() { return nullptr; }

  // The table that the Export button writes; nullptr when the tab has none.
  virtual ILpTableExportSource *getExportSource() { return nullptr; }
};

struct ProfilerTab
{
  ProfilerString name;
  IProfilerModule *module;

  ProfilerTab(const char *tab_name, IProfilerModule *module_ptr) : name(tab_name), module(module_ptr) {}
};

enum class ExportFormat
{
  CSV_File = 0,      // CSV file format
  Markdown_Table = 1 // Markdown table format
};

} // namespace levelprofiler