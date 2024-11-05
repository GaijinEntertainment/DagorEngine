// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <util/dag_tabHlp.h>
#include <osApiWrappers/dag_critSec.h>
#include <memory/dag_mem.h>
#include <memory/dag_memStat.h>
#include <math/dag_mathBase.h>
#include <stdlib.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_dynSceneRes.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_genGuiMgr.h>
#include <drv/hid/dag_hiJoystick.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_texMgr.h>

#include <gui/dag_guiStartup.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_geomTree.h>

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include <gui/dag_stdGuiRender.h>

#include <util/dag_delayedAction.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <util/dag_console.h>
#include <gui/dag_visConsole.h>
#include <visualConsole/dag_visualConsole.h>
#include <util/dag_console.h>

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_WIN
#include <windows.h>
#include <malloc.h>
#endif

#include <gui/dag_imgui.h>
#include <imgui/imguiInput.h>

#include <imgui/imgui.h>
#include <imgui/implot.h>

#include <memoryProfiler/dag_memoryStat.h>

#define INSIDE_ENGINE 0
#include "../../engine/memory/memoryTracker.h"


#define MAX_BLOCKS_IN_LIST 1000

#define INFO_TOP_MARGIN 24
#define INFO_BOT_MARGIN 64

template <size_t length>
static inline int sprintf_safe(char (&dst)[length], const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int res = vsnprintf(dst, length, fmt, args);
  va_end(args);
  dst[length - 1] = 0;
  return res;
}

namespace memorytracker
{
struct MemBlockInfo
{
  uint32_t index;
  uint32_t size;
};

static int sort_memblocks(int *a, int *b) { return ((MemBlockInfo *)b)->size - ((MemBlockInfo *)a)->size; }

int show_ui = 0;

void draw_memview()
{
  char tmp[100];

  MemoryTracker *mt = get_memtracker_access();
  if (mt == nullptr)
    return;

  mt->lock();
  //- Can't use memory allocator until unlock() -----------
  // can sprintf hang?
  mt->updateHeapNames();

  // use packbuffer for visualization
  MemoryTracker::PackState ps(mt->dumpBase, mt->dumpSize);

  char **heapInfo = (char **)ps.skip(sizeof(char *) * mt->numHeaps);
  char **blockLabels = (char **)ps.skip(sizeof(char *) * BLK_POW2_ELEMENTS);
  int *blockSizes = (int *)ps.skip(sizeof(int) * BLK_POW2_ELEMENTS);
  int *blockCounts = (int *)ps.skip(sizeof(int) * BLK_POW2_ELEMENTS);

  for (size_t i = 0; i < BLK_POW2_ELEMENTS; i++)
  {
    blockSizes[i] = 0;
    blockCounts[i] = 0;

    int cnt = 0;
    int size = 1 << (i + BLK_POW2_LOW);
    if (size < 1024)
      cnt = sprintf_safe(tmp, "%d", size);
    else if (size < 1024 * 1024)
      cnt = sprintf_safe(tmp, "%dK", size / 1024);
    else
      cnt = sprintf_safe(tmp, "%dM", size / (1024 * 1024));

    blockLabels[i] = (char *)ps.append(tmp, cnt + 1);
  }

  uint32_t totalBlocks = 0;
  uint64_t totalAllocated = 0;
  for (int i = 0; i < mt->numHeaps; i++)
  {
    totalAllocated += mt->heaps[i].allocated;
    totalBlocks += mt->heaps[i].count;
  }

  int maxCnt = 1;
  int maxSize = 1;

  static bool blockStatsSummary = true;
  static int selectedHeapId = 0;
  static int selectedBlockId = 0;

  selectedHeapId = min(selectedHeapId, mt->numHeaps - 1);
  // update block distribution and heap info
  for (int h = 0; h < mt->numHeaps; h++)
  {
    HeapHeader &hh = mt->heaps[h];
    int cnt = 0;
    if (hh.allocated < 1024 * 1024)
      cnt = sprintf_safe(tmp, "%10s | %4d KB | %d", hh.name, int(hh.allocated >> 10), int(hh.count));
    else
      cnt = sprintf_safe(tmp, "%10s | %4d MB | %d", hh.name, int(hh.allocated >> 20), int(hh.count));
    heapInfo[h] = (char *)ps.append(tmp, cnt + 1);

    if (blockStatsSummary || selectedHeapId == h)
    {
      for (size_t i = 0; i < BLK_POW2_ELEMENTS; i++)
      {
        BlockHeader &bh = hh.blockHeaders[i];
        blockSizes[i] += bh.allocated;
        blockCounts[i] += bh.count;
        maxSize = max<int>(maxSize, blockSizes[i]);
        maxCnt = max<int>(maxCnt, blockCounts[i]);
      }
    }
  }

  static int selectedBlockSz = 0;

  // collect memory blocks for selected bins
  MemBlockInfo *mbiStart = (MemBlockInfo *)ps.align(sizeof(uint32_t));
  for (int binNo = 0; binNo < BLK_POW2_ELEMENTS; binNo++)
  {
    // TODO: multiselect
    if (binNo != selectedBlockSz)
      continue;

    int heapIdx = selectedHeapId;
    uint32_t hbSize = mt->get_hash_bin_size(binNo);
    for (int binElemNo = 0; binElemNo < hbSize; binElemNo++)
    {
      uint32_t offset = mt->get_hash_bin_offset(binNo) + binElemNo;
      uint32_t blockIdx = mt->hashbins[heapIdx][offset]; //-V557
      while (blockIdx != MemBlock::STOP_INDEX)
      {
        // check consistency
        size_t bitmapIdx = blockIdx / BITMAP_BITS;
        uint32_t bit = blockIdx % BITMAP_BITS;
        G_ASSERT(mt->bitmap[bitmapIdx] & (1U << bit));

        MemBlockInfo mb;
        mb.index = blockIdx;
        mb.size = mt->blocks[blockIdx].size;
        ps.append(&mb, sizeof(mb));
        blockIdx = mt->blocks[blockIdx].nextIndex;
      }
      if (ps.failed)
        break;
    }
  }

  // sort blocks
  int mbiCount = (MemBlockInfo *)ps.cur - mbiStart;
  dag_qsort(mbiStart, mbiCount, sizeof(MemBlockInfo), (cmp_func_t)&sort_memblocks);

  // limit to keep good perf
  mbiCount = min<int>(mbiCount, MAX_BLOCKS_IN_LIST);

  // convert sorted block data to strings
  char **blockInfo = (char **)ps.skip(sizeof(char *) * mbiCount);
  int mbiIdx = 0;
  int mbiOut = 0;
  while (mbiIdx < mbiCount && !ps.failed)
  {
    uint32_t blockIdx = mbiStart[mbiIdx].index;
    MemBlock &mb = mt->blocks[blockIdx];
    int mbiMultiplier = 1;
    while (mbiIdx < (mbiCount - 1))
    {
      if (mb.bktraceID != mt->blocks[blockIdx + mbiMultiplier].bktraceID || mb.size != mt->blocks[blockIdx + mbiMultiplier].size)
        break;
      mbiMultiplier++;
    };

    mbiIdx += mbiMultiplier;

    int cnt;
    if (mbiMultiplier == 1)
      cnt = sprintf_safe(tmp, "%010d | %p    ", mb.size, mb.addr);
    else
      cnt = sprintf_safe(tmp, "%010d | %p X %-d", mb.size, mb.addr, mbiMultiplier);

    blockInfo[mbiOut++] = (char *)ps.append(tmp, cnt + 1);
  }

  int btDepth = 0;
  void **btStart = nullptr;

  selectedBlockId = min(mbiCount - 1, selectedBlockId);
  if (selectedBlockId >= 0) // has blocks to show
  {
    // copy callstack for selected item
    uint32_t blockIdx = mbiStart[selectedBlockId].index;
    MemBlock &mb = mt->blocks[blockIdx];

    if (mb.bktraceID == 0)
    {
      mt->unlock();
      return;
    }

    uint32_t btIdx = mb.bktraceID;
    btDepth = mt->unpackBtHdrDepth(mt->btElems[btIdx]);

    ps.align(sizeof(uint64_t));
    btStart = (void **)ps.append((void *)&mt->btElems[btIdx + 1], sizeof(void *) * btDepth);
  }

  mt->unlock();

  //- now we can allocate mem --------------------------

  // find symbols
  size_t btTextBufsz = 10000;
  char *btTextBuf = (char *)ps.skip(btTextBufsz); // enough buffer to dump call stack

  if (ps.failed)
  {
    // increase MTR_DUMP_SIZE_PERCENTS
    logerr("memtracker:: buffer is too small");
    return;
  }

  const char *btTextBufx = "<no backtrace>";
  if (btStart != nullptr && btDepth > 0)
    btTextBufx = stackhlp_get_call_stack(btTextBuf, btTextBufsz, btStart, btDepth);

#if 0 // TODO: show independent window from console
  int scrWidth, scrHeight;
  d3d::get_target_size(scrWidth, scrHeight);
  float fullWidth = scrWidth/2;
  float fullHeight = scrHeight - INFO_TOP_MARGIN - INFO_BOT_MARGIN;
  ImGui::SetNextWindowPos(ImVec2(4, INFO_TOP_MARGIN));
  ImGui::SetNextWindowSize(ImVec2(fullWidth, fullHeight));
  ImGui::Begin("Memory tracker");
#endif

  ImGui::BeginGroup();

  ImVec2 windowSz = ImGui::GetContentRegionAvail();
  float blockListWidth = ImGui::GetFontSize() * 24;

  ImGui::SameLine();

  //////////////////////////
  // left column

  ImVec2 wsz = windowSz;
  wsz.x = windowSz.x - blockListWidth;
  ImGui::BeginChild("", wsz, ImGuiChildFlags_None);

  ImGui::Checkbox("All heaps", &blockStatsSummary);
  ImGui::SameLine();

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + blockListWidth * 2);
  ImGui::SetNextItemWidth(-80);
  ImGui::Combo("Block Size", &selectedBlockSz, blockLabels, BLK_POW2_ELEMENTS);

  static const double positions[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24};
  G_ASSERT(countof(positions) >= BLK_POW2_ELEMENTS);
  if (ImPlot::BeginPlot("Block distribution | count", "Block >=", "Count"))
  {
    ImPlot::SetupAxisTicks(ImAxis_X1, positions, BLK_POW2_ELEMENTS, blockLabels);
    ImPlot::SetupAxesLimits(0, BLK_POW2_ELEMENTS, 0, maxCnt, ImGuiCond_Always);
    ImPlot::PlotBars("", blockCounts, BLK_POW2_ELEMENTS);
    ImPlot::EndPlot();
  }
  if (ImPlot::BeginPlot("Block distribution | size", "Block >=", "Size"))
  {
    ImPlot::SetupAxisTicks(ImAxis_X1, positions, BLK_POW2_ELEMENTS, blockLabels);
    ImPlot::SetupAxesLimits(0, BLK_POW2_ELEMENTS, 0, maxSize, ImGuiCond_Always);
    ImPlot::PlotBars("", blockSizes, BLK_POW2_ELEMENTS);
    ImPlot::EndPlot();
  }

  ImGui::BeginGroup();

  ImGui::PushItemWidth(ImGui::GetFontSize() * 18.0f);
  ImGui::BeginGroup();
  ImGui::TextColored(ImVec4(1, 1, 0, 1), "Heaps (alloc size and blocks)");
  ImGui::ListBox("", &selectedHeapId, heapInfo, mt->numHeaps, MTR_MAX_HEAPS);
  ImGui::TextColored(ImVec4(1, 1, 1, 1), "Dump space used: %d of %d KB", int(ps.cur - ps.beg) >> 10, int(ps.end - ps.beg) >> 10);
  ImGui::TextColored(ImVec4(1, 1, 1, 1), "Blocks %d of %d (%d%%)", totalBlocks, mt->maxBlocks, 100 * totalBlocks / mt->maxBlocks);
  ImGui::TextColored(ImVec4(1, 1, 1, 1), "BtElems %d of %d (%d%%)", mt->currentBtIndex, mt->maxBtElems,
    100 * mt->currentBtIndex / mt->maxBtElems);
  ImGui::EndGroup();
  ImGui::PopItemWidth();

  ImGui::SameLine();

  ImGui::BeginGroup();
  ImGui::TextColored(ImVec4(1, 1, 0, 1), "Callstack for block");
  ImGui::BeginChild("Callstack");

  if (btTextBufx[0] != 0)
    ImGui::Text("%s", btTextBufx);
  else
  {
    if (btStart != nullptr)
      for (int i = 0; i < btDepth; i++)
        ImGui::Text("%p", btStart[i]);
  }
  ImGui::EndChild();
  ImGui::EndGroup();

  ImGui::EndGroup();

  ImGui::EndChild();
  /////////////////////

  ImGui::SameLine();

  // right column

  //////////////////////
  //- block list
  wsz.x = blockListWidth * 1.3;

  int lbHeight = wsz.y / ImGui::GetFontSize() * 0.8f;

  ImGui::BeginChild("Blocks", wsz, ImGuiChildFlags_Border);
  if (mbiOut > 0)
    ImGui::ListBox("", &selectedBlockId, blockInfo, mbiOut, lbHeight);
  //    for (int i = 0; i < mbiOut; i++)
  //      ImGui::Text("%s", blockInfo[i]);
  ImGui::EndChild();

  ImGui::EndGroup();

#if 0
  ImGui::End();
#endif
}

} // namespace memorytracker

using namespace memorytracker;

bool mtrk_console_handler(const char *argv[], int argc)
{
  MemoryTracker *mt = get_memtracker_access();
  if (mt == nullptr)
  {
    if (argc >= 1 && strncmp(argv[0], "memtrk", sizeof("memtrk") - 1) == 0)
      debug("memory tracker is disabled");
    return false;
  }

  int found = 0;
  CONSOLE_CHECK_NAME("memtrk", "stats", 1, 1)
  {
    // keep multiline
    mt->printStats();
  }
  CONSOLE_CHECK_NAME("memtrk", "gui", 1, 2)
  {
    if (argc > 1)
      show_ui = atoi(argv[1]);
    else
      show_ui ^= 1;
    imgui_window_set_visible("Memory", "Tracker", show_ui);
  }
  CONSOLE_CHECK_NAME("memtrk", "save", 1, 1)
  {
    static int count = 0;
    char name[100];
    sprintf_safe(name, "/hostapp/memtrace-%d.bin", count);
    mt->prepareFullDump();
    mt->saveDump(name);
    count++;
  }
  CONSOLE_CHECK_NAME("memtrk", "blocks", 1, 6)
  {
    int args[5] = {4, 0, 1024, 20, 0};
    for (int i = 1; i < argc; i++)
      args[i - 1] = atoi(argv[i]);

    int min_size = args[0] << 10;
    if (min_size <= 0)
      min_size = 16;
    int max_size = args[1] << 10;
    if (max_size <= 0)
      max_size = INT_MAX;

    uint32_t frame = uint32_t(args[4]);
    if (args[4] < 0)
      frame = uint32_t(max<int64_t>(0LL, int64_t(::dagor_frame_no()) + args[4]));

    mt->printDetailedStats(min_size, max_size, args[2], args[3], frame);
  }
  CONSOLE_CHECK_NAME("memtrk", "sblocks", 1, 6)
  {
    int args[5] = {0, 512, 1024, 20, 0};
    for (int i = 1; i < argc; i++)
      args[i - 1] = atoi(argv[i]);

    int max_size = args[1];
    if (max_size <= 0)
      max_size = INT_MAX;

    uint32_t frame = uint32_t(args[4]);
    if (args[4] < 0)
      frame = uint32_t(max<int64_t>(0LL, int64_t(::dagor_frame_no()) + args[4]));

    mt->printDetailedStats(args[0], max_size, args[2], args[3], frame);
  }
  return bool(found);
}

REGISTER_CONSOLE_HANDLER(mtrk_console_handler);

void memtracker_imgui() { memorytracker::draw_memview(); }
REGISTER_IMGUI_WINDOW("Memory", "Tracker", memtracker_imgui);
