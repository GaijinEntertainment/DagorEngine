// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresCommon.h>

#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>
#include <id/idIndexedMapping.h>

#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorMap.h>
#include <drv/3d/dag_consts.h>


namespace dafg::visualization
{

enum class HeapIndex : int
{
};

struct Lifetime
{
  int firstNodeIdx = eastl::numeric_limits<int>::max();
  int lastNodeIdx = eastl::numeric_limits<int>::min();

  bool empty() const { return firstNodeIdx > lastNodeIdx; }
  void record(int nodeIdx)
  {
    firstNodeIdx = eastl::min(firstNodeIdx, nodeIdx);
    lastNodeIdx = eastl::max(lastNodeIdx, nodeIdx);
  }
};

struct FrontendResourceInfo
{
  const char *name = nullptr;
  Lifetime physicalLifetime;
  Lifetime logicalLifetime;
  bool createdByRename = false;
  bool consumedByRename = false;
  dag::VectorMap<int, ResourceUsage> usageTimeline;
};

struct Placement
{
  int heapIndex = -1;
  int offset = 0;
  int size = 0;
};

struct IRResourceInfo
{
  static constexpr int FRAME_COUNT = 2;
  eastl::array<Placement, FRAME_COUNT> placements;

  Lifetime physicalLifetime;
  Lifetime logicalLifetime;

  eastl::string name;
  dag::Vector<FrontendResourceInfo> frontendResources;
  eastl::fixed_vector<eastl::pair<int, ResourceBarrier>, 32> barrierEvents;
};

struct ResourcePlacementEntry
{
  ResNameId id;
  int frame;
  int heap;
  int offset;
  int size;
  bool is_cpu = false;
};

struct ResourceBarrierEntry
{
  ResNameId res_id;
  int res_frame;
  int exec_time;
  int exec_frame;
  ResourceBarrier barrier;
};

struct ColumnLayout
{
  struct Column
  {
    int firstNodeIdx;
    int lastNodeIdx;
    eastl::string displayName;
    eastl::string indexLabel;
    float widthMultiplier = 1.0f;
  };

  dag::Vector<Column> columns;
  dag::Vector<float> columnStartX;                             // cumulative X offset per column
  IdIndexedMapping<intermediate::NodeIndex, int> nodeToColumn; // maps NodeIndex -> column index
  float totalWidth = 0.f;

  int columnCount() const { return (int)columns.size(); }

  float columnCenterX(int col) const
  {
    float halfW = columns[col].widthMultiplier * 0.5f;
    return columnStartX[col] + halfW;
  }

  float columnLeftEdge(int col) const { return columnStartX[col]; }

  float columnRightEdge(int col) const { return columnStartX[col] + columns[col].widthMultiplier; }

  float nodeXCenter(intermediate::NodeIndex nodeIdx) const { return columnCenterX(nodeToColumn[nodeIdx]); }

  float nodeXFraction(intermediate::NodeIndex nodeIdx, float offset) const
  {
    int col = nodeToColumn[nodeIdx];
    return columnCenterX(col) + offset * columns[col].widthMultiplier;
  }

  int nodeIdxAtX(float x) const
  {
    for (int col = 0; col < columnCount(); ++col)
    {
      if (x < columnStartX[col] + columns[col].widthMultiplier)
        return columns[col].firstNodeIdx;
    }
    return columns.empty() ? 0 : columns.back().lastNodeIdx;
  }

  void buildCompact(const IdIndexedMapping<intermediate::NodeIndex, eastl::string> &nodeNames,
    const dag::Vector<bool> &interestingNodes)
  {
    using NodeIndex = intermediate::NodeIndex;

    columns.clear();
    nodeToColumn.clear();
    columnStartX.clear();
    totalWidth = 0.f;

    const int numNodes = (int)nodeNames.size();
    if (numNodes == 0)
      return;
    nodeToColumn.resize(numNodes, -1);

    dag::Vector<NodeIndex, framemem_allocator> occupiedNodes;
    for (NodeIndex ni : nodeNames.keys())
      if (!nodeNames[ni].empty())
        occupiedNodes.push_back(ni);

    if (occupiedNodes.empty())
      return;

    int groupStart = 0;
    for (int i = 0; i <= (int)occupiedNodes.size(); ++i)
    {
      bool endGroup = (i == (int)occupiedNodes.size()) || (i > groupStart && interestingNodes[eastl::to_underlying(occupiedNodes[i])]);
      if (!endGroup)
        continue;

      const int groupEnd = i - 1;
      const NodeIndex firstIdx = occupiedNodes[groupStart];
      const NodeIndex lastIdx = occupiedNodes[groupEnd];
      Column col;
      col.firstNodeIdx = eastl::to_underlying(firstIdx);
      col.lastNodeIdx = eastl::to_underlying(lastIdx);

      if (groupStart == groupEnd)
      {
        col.displayName = nodeNames[firstIdx];
        col.indexLabel = eastl::to_string(groupStart);
        col.widthMultiplier = 1.0f;
      }
      else
      {
        const auto &first = nodeNames[firstIdx];
        size_t prefixLen = first.size();
        for (int j = groupStart + 1; j <= groupEnd; ++j)
        {
          const auto &name = nodeNames[occupiedNodes[j]];
          prefixLen = eastl::min(prefixLen, name.size());
          for (size_t k = 0; k < prefixLen; ++k)
            if (first[k] != name[k])
            {
              prefixLen = k;
              break;
            }
        }
        col.displayName = first.substr(0, prefixLen);
        col.displayName += "...";
        col.indexLabel = eastl::to_string(groupStart) + ".." + eastl::to_string(groupEnd);
        col.widthMultiplier = 2.0f;
      }

      int colIdx = (int)columns.size();
      for (int j = groupStart; j <= groupEnd; ++j)
        nodeToColumn[occupiedNodes[j]] = colIdx;
      columns.push_back(eastl::move(col));

      groupStart = i;
    }

    float x = -0.5f;
    for (auto &col : columns)
    {
      columnStartX.push_back(x);
      x += col.widthMultiplier;
    }
    totalWidth = x + 0.5f;
  }

  void buildFromNodes(const IdIndexedMapping<intermediate::NodeIndex, eastl::string> &nodeNames)
  {
    using NodeIndex = intermediate::NodeIndex;

    columns.clear();
    nodeToColumn.clear();
    columnStartX.clear();
    totalWidth = 0.f;

    const int numNodes = (int)nodeNames.size();
    nodeToColumn.resize(numNodes, -1);

    int displayIdx = 0;
    for (NodeIndex ni : nodeNames.keys())
    {
      if (nodeNames[ni].empty())
        continue;

      Column col;
      col.firstNodeIdx = eastl::to_underlying(ni);
      col.lastNodeIdx = eastl::to_underlying(ni);
      col.displayName = nodeNames[ni];
      col.indexLabel = eastl::to_string(displayIdx++);
      col.widthMultiplier = 1.0f;

      nodeToColumn[ni] = (int)columns.size();
      columns.push_back(eastl::move(col));
    }

    float x = -0.5f;
    for (auto &col : columns)
    {
      columnStartX.push_back(x);
      x += col.widthMultiplier;
    }
    totalWidth = x + 0.5f;
  }
};

} // namespace dafg::visualization