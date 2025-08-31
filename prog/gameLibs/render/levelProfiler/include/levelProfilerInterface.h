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

enum TableColumn
{
  COLUMN_NONE = -1, // Invalid column indicator
  COL_NAME,         // Texture/resource name
  COL_FORMAT,       // Texture format
  COL_WIDTH,        // Texture width
  COL_HEIGHT,       // Texture height
  COL_MIPS,         // Mipmap count
  COL_MEM_SIZE,     // Memory size
  COL_TEX_USAGE     // Texture Usage information
};

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

class IDataCollector
{
public:
  virtual ~IDataCollector() = default;
  virtual void collect() = 0;
  virtual void clear() = 0;
};

class IFilter
{
public:
  virtual ~IFilter() = default;
  virtual bool pass(const void *item) const = 0;
  virtual void reset() = 0;
};

class IProfilerModule
{
public:
  virtual ~IProfilerModule() = default;
  virtual void init() = 0;
  virtual void shutdown() = 0;
  virtual void drawUI() = 0;

  virtual ICopyProvider *getCopyProvider() { return nullptr; }
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

class ILevelProfiler
{
public:
  virtual ~ILevelProfiler() = default;

  virtual void init() = 0;
  virtual void shutdown() = 0;

  virtual void drawUI() = 0;

  virtual void collectData() = 0;
  virtual void clearData() = 0;

  virtual void addTab(const char *name, IProfilerModule *module_ptr) = 0;
  virtual int getTabCount() const = 0;
  virtual ProfilerTab *getTab(int index) = 0;
  virtual void renameTab(int index, const char *new_name) = 0;

  static ILevelProfiler *getInstance();
};

} // namespace levelprofiler