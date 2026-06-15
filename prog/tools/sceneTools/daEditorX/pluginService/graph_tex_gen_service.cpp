// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>

#include "graph_tex_gen_service.h"
#include "image_export.h"

#include <graphEditor/graph_data.h>

#include <de3_interface.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_tex3d.h>
#include <image/dag_tga.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_cpuFreq.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_globalSettings.h>
#include <textureGen/textureDataCache.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include <textureGen/textureRegManager.h>
#include <textureGen/entitiesSaver.h>
#include <libTools/util/strUtil.h>
#include <util/dag_fileTimeChecker.h>
#include <workCycle/dag_workCycle.h>

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_resPtr.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>
#include <time.h>
#include <float.h>

namespace
{
int fmtToChannelsCount(int fmt)
{
  switch (fmt & TEXFMT_MASK)
  {
    case TEXFMT_R16F:
    case TEXFMT_R32F:
    case TEXFMT_L16:
    case TEXFMT_R8: return 1;

    case TEXFMT_G16R16:
    case TEXFMT_G16R16F:
    case TEXFMT_G32R32F: return 2;

    case TEXFMT_A8R8G8B8:
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_A16B16G16R16:
    case TEXFMT_A32B32G32R32F: return 4;

    default: return 3;
  }
}


class GraphLoggerImpl : public TextureGenLogger
{
public:
  bool hasErrors = false;
  bool hasWarnings = false;
  // Last (most recent) error / warning of the pass -- the status bar shows only these. Every
  // error / warning is echoed to the editor console as it happens (see log()); the full ordered
  // transcript also accumulates in `lines`.
  String lastError;
  String lastWarning;
  String lines;

  void clear()
  {
    ::clear_and_shrink(lines);
    ::clear_and_shrink(lastError);
    ::clear_and_shrink(lastWarning);
    hasErrors = false;
    hasWarnings = false;
  }

  String getFullLog() const { return lines; }

  void log(int level, const String &s) override
  {
    // Every error / warning is mirrored to the editor console (not just the first), and the
    // most recent of each is retained for the status-bar message.
    if (level == LOGLEVEL_ERR)
    {
      lastError = s;
      hasErrors = true;
      DAEDITOR3.conError("GraphTexGenService: %s", s.str());
    }
    // Warnings e.g. a shader that compiled with a recoverable issue (texturePSGenShader.cpp).
    else if (level == LOGLEVEL_WARN)
    {
      lastWarning = s;
      hasWarnings = true;
      DAEDITOR3.conWarning("GraphTexGenService: %s", s.str());
    }

    if (level <= startLevel)
    {
      logmessage(level, "%s", s.str());
      lines.aprintf(1024, "%s\n", s.str());
    }
  }
};

bool writeStringToFile(const char *file_name, const char *str)
{
  file_ptr_t h = df_open(file_name, DF_WRITE | DF_CREATE);
  if (!h)
  {
    return false;
  }

  df_write(h, str, strlen(str));
  df_close(h);
  return true;
}

// Guard against reacting to a file that is still being written.
bool is_file_ready_to_open(const char *filename)
{
  file_ptr_t fp = df_open(filename, DF_READ);
  if (!fp)
  {
    return false;
  }

  df_close(fp);
  return true;
}
} // namespace

class TextureGenEntitiesSaverImpl : public ITextureGenEntitiesSaver
{
  Tab<String> blkNames;
  Tab<DataBlock> entitiesBlk;
  Tab<String> usedNames;

public:
  bool onEntitiesGenerated(const Tab<TextureGenEntity> &entities, const char *blk_file_name, const Tab<String> &entity_names) override
  {
    DAEDITOR3.conNote("GraphTexGenService: Entities Generated: count=%d file=%s", entities.size(), blk_file_name);

    if (!blk_file_name || !*blk_file_name)
    {
      return false;
    }

    if (entity_names.size() == 0)
    {
      return false;
    }

    DataBlock blk;

    for (int i = 0; i < entities.size(); i++)
    {
      Point3 fwd = normalize(entities[i].forward);
      Point3 up = normalize(entities[i].up);
      Point3 left = normalizeDef(fwd % up, Point3(0, 0, 1));
      up = normalize(left % fwd);
      TMatrix tm;
      tm.setcol(0, fwd * entities[i].scale.x);
      tm.setcol(1, up * entities[i].scale.y);
      tm.setcol(2, left * entities[i].scale.z);
      tm.setcol(3, entities[i].worldPos);

      DataBlock *entityBlk = blk.addNewBlock("entity");
      entityBlk->addStr("note", "");

      const char *entName = entity_names[entities[i].nameIndex % entity_names.size()].str();
      String name(0, "%s__gen_%d", entName, i);
      entityBlk->addStr("name", name.str());
      entityBlk->addStr("entName", entName);
      entityBlk->addInt("place_type", entities[i].placeType);
      entityBlk->addInt("entSeed", entities[i].seed);
      entityBlk->addTm("tm", tm);
    }

    blkNames.push_back(String(blk_file_name));
    entitiesBlk.push_back(blk);
    return true;
  }

  void clear()
  {
    clear_and_shrink(blkNames);
    clear_and_shrink(entitiesBlk);
    clear_and_shrink(usedNames);
  }

  void saveAllEntitiesToFiles(const char *dir)
  {
    for (int i = 0; i < blkNames.size(); i++)
    {
      String fn(128, "%s/%s", dir, blkNames[i]);
      if (entitiesBlk[i].saveToTextFile(fn.str()))
      {
        DAEDITOR3.conNote("entities saved to '%s'", fn.str());
      }
      else
      {
        DAEDITOR3.conError("cannot save entities to file '%s'", fn.str());
      }
    }
  }

  void clearUsedBlkNames() { clear_and_shrink(usedNames); }

  void pushUsedBlkName(const char *blk_name) { usedNames.push_back(String(blk_name)); }

  void removeUnusedBlk()
  {
    for (int j = blkNames.size() - 1; j >= 0; j--)
    {
      bool found = false;
      for (int i = 0; i < usedNames.size(); i++)
      {
        if (!strcmp(blkNames[j].str(), usedNames[i].str()))
        {
          found = true;
        }
      }

      if (!found)
      {
        erase_items(blkNames, j, 1);
        erase_items(entitiesBlk, j, 1);
      }
    }
  }
};


class GraphTexGenServiceImpl : public IEditorService, public IGraphTexGenService
{
  class TexGenWorker;

public:
  GraphTexGenServiceImpl() = default;

  ~GraphTexGenServiceImpl() override
  {
    stopWorker();
    closeTexGen();
  }

  // IEditorService
  const char *getServiceName() const override { return "texgen"; }
  const char *getServiceFriendlyName() const override { return "(srv) GraphEditor TexGen Service"; }
  void setServiceVisible(bool) override {}
  bool getServiceVisible() const override { return false; }
  void actService(float) override {}
  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IGraphTexGenService);
    return nullptr;
  }

  void initTexGen(const char *default_shader_path, const char *default_texgen_path) override
  {
    defaultShadersPath = default_shader_path;
    defaultTexgenPath = default_texgen_path;

    startWorker();
  }

  void shutdownTexGen() override
  {
    stopWorker();
    closeTexGen();
    texGenReg.close();
    // Worker is gone; drain whatever it queued so the dtor doesn't free GPU textures
    // outside a GpuAutoLock scope.
    drainPendingTextureDestroys();
  }

  // Mirrors the polling loop in prog/tools/graphEditor/app.cpp:checkInputFilesForUpdate().
  // Any input file registered by the texture generator (e.g. loft masks written by HeightmapLand's
  // exportLoftMasks()) triggers an automatic rebuild when its mtime changes, after a short debounce
  // that prevents reacting to partially-written files.
  void checkInputFilesForUpdate()
  {
    if (texGenStep > 0)
    {
      return;
    }

    if (timeToRecalcGraphSec > 0.f)
    {
      timeToRecalcGraphSec -= dagor_game_act_time;
      if (timeToRecalcGraphSec <= 0.f)
      {
        shouldGenerateTex = true;
        forceRebuild = true;
      }
    }

    static int sparse = 0;
    sparse++;
    if (sparse % 16 != 0)
    {
      return;
    }

    for (const auto &fi : texGenReg.getInputFilesList())
    {
      inputFileCheckers.addFile(fi.c_str());
    }

    bool changed = false;
    for (int i = 0; i < inputFileCheckers.size(); i++)
    {
      if (inputFileCheckers[i].fileChanged() && is_file_ready_to_open(inputFileCheckers[i].getFileName()))
      {
        changed = true;
        texGenReg.validateFile(inputFileCheckers[i].getFileName());
      }
    }

    if (changed)
    {
      timeToRecalcGraphSec = max(timeToRecalcGraphSec, inputFileCheckers.size() <= 1 ? 0.2f : 1.2f);
      inputFileCheckers.updateFileTimes();
      regcache::clear();
    }
  }

  // Main-thread poller. Runs every frame, does no D3D / no heavy work:
  // polls input files (on main because it's pure file I/O plus a cheap reg query),
  // then wakes the worker if there is pending work. The actual generation runs on the
  // worker thread inside workerLoop() under GpuAutoLock + stateLock.
  void tickTexGeneration() override
  {
    if (!graphData)
    {
      return;
    }

    drainPendingTextureDestroys();

    bool shouldKick = false;
    {
      WinAutoLock guard(stateLock);
      checkInputFilesForUpdate();
      shouldKick = hasWorkUnlocked();
    }
    if (shouldKick && worker)
    {
      os_event_set(&wakeEvent);
    }
  }

  // Free textures that the worker swapped out of selectedTexState in a prior tick. Called at the
  // start of tickTexGeneration -- by that point the previous frame's imguiRender has completed,
  // so any drawlist references to those textures are no longer accessed.
  void drainPendingTextureDestroys()
  {
    eastl::vector<UniqueTex> doomed;
    {
      WinAutoLock guard(stateLock);
      if (pendingTextureDestroy.empty())
      {
        return;
      }
      doomed.swap(pendingTextureDestroy);
    }
    // UniqueTex destructors hit the GPU resource table; serialize against the worker.
    d3d::GpuAutoLock gpuLock;
    doomed.clear();
  }

  bool isGenerationPending() const override
  {
    WinAutoLock guard(stateLock);
    return hasWorkUnlocked();
  }

  TexGenPipelineStatus getPipelineStatus() const override
  {
    WinAutoLock guard(stateLock);
    TexGenPipelineStatus st;
    st.memUsedCurrent = texGenReg.getCurrentMemSize();
    st.memUsedMax = texGenReg.getMaxMemSize();
    st.gpuMemTotal = static_cast<uint64_t>(d3d::get_dedicated_gpu_memory_size_kb()) << 10;
    // texGenStep counts the commands attempted so far while a pass is stepping; it parks at
    // 0 (idle / finished) or -1 (failed), which both render as "no pass in flight".
    st.commandsDone = texGenStep > 0 ? texGenStep : 0;
    st.commandsTotal = texGenTotalCmds;
    // Outputs = declared graph finals captured at generation start (declaredOutputsCount), NOT
    // finalStrings.size(). On a generation failure the pipeline closes texGenReg and then
    // finalizeTexGen prunes every final whose register no longer exists, emptying finalStrings --
    // which would otherwise make the Outputs counter read 0 the moment a compile fails.
    st.outputsCount = declaredOutputsCount;
    st.generating = hasWorkUnlocked();

    // Build outcome for the status-bar message. graphLogger is written by the worker under
    // stateLock (same lock held here), so this snapshot is consistent.
    st.graphCompileFailed = graphCompileFailed;
    st.hasErrors = graphLogger.hasErrors;
    st.hasWarnings = graphLogger.hasWarnings;
    st.generationCompleted = generationCompleted;
    st.lastError = graphLogger.lastError.c_str();
    st.lastWarning = graphLogger.lastWarning.c_str();
    return st;
  }

  SelectedTextureState getSelectedTextureState() const override
  {
    WinAutoLock guard(stateLock);
    return selectedTexState;
  }

  const eastl::hash_set<eastl::string> &getFinalStrings() const override { return finalStrings; }

  void saveTextures(const char *mask) override
  {
    {
      WinAutoLock guard(stateLock);
      saveTexturesToFiles = true;
      if (mask)
      {
        saveTextureMask = mask;
      }
      else
      {
        saveTextureMask = "";
      }
    }

    const char *rDir = (graphData && !graphData->renderDir.empty()) ? graphData->renderDir.c_str() : "render";
    const char *eDir = (graphData && !graphData->entityDir.empty()) ? graphData->entityDir.c_str() : "entity";

    dd_mkdir(rDir);
    dd_mkdir(eDir);

    DAEDITOR3.conNote("GraphTexGenService: render directory: '%s'", rDir);
    DAEDITOR3.conNote("GraphTexGenService: entity directory: '%s'", eDir);

    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }

  bool isSaving() const override
  {
    WinAutoLock guard(stateLock);
    return saveTexturesToFiles;
  }

  void setGraphData(GraphData *gd) override
  {
    if (!gd)
    {
      // Going idle: drop any pipeline state tied to the previous graph data.
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      texGenStep = -1;
      texGenTotalCmds = 0;
      declaredOutputsCount = 0;
      graphCompileFailed = false;
      generationCompleted = false;
      previewFinalName.clear();
      previewFinalAddedToBlk = false;
      pendingPreviewReg.clear();
      selectedTexState = SelectedTextureState{};
      graphData = nullptr;
      // Reset both gens so the next setGraphData(non-null) followed by
      // markGraphDirty() starts cleanly at 0/0 -> 1/0 (dirty).
      graphDirtyGen = 0;
      graphCompiledGen = 0;
      return;
    }

    graphData = gd;
    // Drop any preview-final inherited from the previous graph -- the new BLK is unrelated, and a
    // stale name typically won't match any output reg in it (which would assert in the texgen lib's
    // get_nodes_generating_regs). The panel will re-issue setPreviewFinal on its next selection.
    previewFinalName.clear();
    previewFinalAddedToBlk = false;
    pendingPreviewReg.clear();
    selectedTexState = SelectedTextureState{};
    // Old graph's output count / build outcome are meaningless for the new one; recomputed on the
    // first compile + generation.
    declaredOutputsCount = 0;
    graphCompileFailed = false;
    generationCompleted = false;
    shouldGenerateTex = true;
    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }
  void requestForceRebuild() override
  {
    {
      WinAutoLock guard(stateLock);
      forceRebuild = true;
    }
    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }
  void requestRegenerate() override
  {
    {
      WinAutoLock guard(stateLock);
      shouldGenerateTex = true;
      generateOnce = true;
    }
    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }

  void setGraphCompiler(IGraphCompiler *compiler) override
  {
    {
      WinAutoLock guard(stateLock);
      graphCompiler = compiler;
    }
    if (worker)
    {
      // Compiler appearing while a compile is pending (graphDirtyGen !=
      // graphCompiledGen) should trigger an immediate compile pass; nudge the
      // worker either way (cheap when nothing to do).
      os_event_set(&wakeEvent);
    }

    // When clearing the pointer (plugin teardown), drain any in-flight compile()
    // call before returning so the caller can safely destroy the impl. Polls at
    // 1 ms granularity; compiles are sub-ms in practice.
    if (!compiler)
    {
      while (true)
      {
        {
          WinAutoLock guard(stateLock);
          if (!compileInProgress)
          {
            break;
          }
        }
        sleep_msec(1);
      }
    }
  }

  void markGraphDirty() override
  {
    {
      WinAutoLock guard(stateLock);
      ++graphDirtyGen;
    }
    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }
  bool getAutoupdate() const override
  {
    WinAutoLock guard(stateLock);
    return autoupdate;
  }
  void toggleAutoupdate() override
  {
    bool becameEnabled = false;
    {
      WinAutoLock guard(stateLock);
      autoupdate = !autoupdate;
      becameEnabled = autoupdate && shouldGenerateTex;
    }
    if (becameEnabled && worker)
    {
      os_event_set(&wakeEvent);
    }
  }
  String getCompilerLog() const override
  {
    WinAutoLock guard(stateLock);
    return graphLogger.getFullLog();
  }

  void setPreviewFinal(const char *tex_name) override
  {
    // Mutates state shared with the worker (previewFinalName, pendingPreviewReg, selectedTexState,
    // shouldGenerateTex) and writes to graphData->mainGraphBlk via add/removePreviewFinalFromBlk.
    // Hold stateLock for all of it; release before waking the worker.
    {
      WinAutoLock guard(stateLock);
      if (!graphData)
      {
        return;
      }

      eastl::string newName = (tex_name && tex_name[0]) ? tex_name : "";
      if (newName == previewFinalName)
      {
        return;
      }

      removePreviewFinalFromBlk();
      previewFinalName = newName;
      addPreviewFinalToBlk();
      pendingPreviewReg = eastl::move(newName);
      // Drop the stale preview from the previous selection right away so the panel doesn't keep
      // showing it across the regen window. If the new pipeline produces pendingPreviewReg, the
      // post-finalize lookup repopulates this; otherwise the panel renders "No texture selected",
      // which is the truthful state.
      selectedTexState = SelectedTextureState{};
      shouldGenerateTex = true;
    }
    if (worker)
    {
      os_event_set(&wakeEvent);
    }
  }

  // True if any node block in `blk` declares an `output { reg:t = name }` for `name`. The texgen
  // library's regs_name_mapping is populated from these sub-blocks, and any final:t=... that
  // doesn't resolve there asserts in get_nodes_generating_regs.
  static bool blk_has_output_reg(const DataBlock &blk, const char *name)
  {
    if (!name || !*name)
    {
      return false;
    }

    for (int i = 0; i < blk.blockCount(); ++i)
    {
      const DataBlock *node = blk.getBlock(i);
      if (!node)
      {
        continue;
      }

      for (int bi = 0; bi < node->blockCount(); ++bi)
      {
        const DataBlock *sub = node->getBlock(bi);
        if (!sub || strcmp(sub->getBlockName(), "output") != 0)
        {
          continue;
        }

        if (strcmp(sub->getStr("reg", ""), name) == 0)
        {
          return true;
        }
      }
    }

    return false;
  }

  // Mutates graphData->mainGraphBlk so the texgen library's optimizer (which reads final:t=...
  // directly from the BLK in textureGenerator.cpp) sees the preview register as a live target.
  // Otherwise the texture for an intermediate node gets freed after generation and the preview
  // lookup fails.
  void addPreviewFinalToBlk()
  {
    if (!graphData || previewFinalName.empty())
    {
      return;
    }

    if (!blk_has_output_reg(graphData->mainGraphBlk, previewFinalName.c_str()))
    {
      debug("TexGenService: preview final '%s' is not an output reg of any node; not promoting", previewFinalName.c_str());
      return;
    }
    graphData->mainGraphBlk.addStr("final", previewFinalName.c_str());
    previewFinalAddedToBlk = true;
  }

  void removePreviewFinalFromBlk()
  {
    if (!graphData || !previewFinalAddedToBlk || previewFinalName.empty())
    {
      previewFinalAddedToBlk = false;
      return;
    }
    DataBlock &blk = graphData->mainGraphBlk;
    for (int i = blk.paramCount() - 1; i >= 0; --i)
    {
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && strcmp(blk.getParamName(i), "final") == 0 &&
          previewFinalName == blk.getStr(i))
      {
        blk.removeParam(i);
        break;
      }
    }
    previewFinalAddedToBlk = false;
  }

  // Heightmap metadata extracted from main graph (for landscape preview)
  void setHeightmapParams(float scale, float min_height, float cell_size) override
  {
    WinAutoLock guard(stateLock);
    heightmapScale = scale;
    heightmapMin = min_height;
    heightmapCellSize = cell_size;
  }
  float getHeightmapScale() const override
  {
    WinAutoLock guard(stateLock);
    return heightmapScale;
  }
  float getHeightmapMin() const override
  {
    WinAutoLock guard(stateLock);
    return heightmapMin;
  }
  float getHeightmapCellSize() const override
  {
    WinAutoLock guard(stateLock);
    return heightmapCellSize;
  }
  TEXTUREID getHeightmapTextureId() const override
  {
    WinAutoLock guard(stateLock);
    if (!texGen)
    {
      return BAD_TEXTUREID;
    }

    for (const char *name : {"heightmap", "tex_hmap_low"})
    {
      eastl::string regName = texgen_get_final_reg_name(texGen, name);
      int regNo = texGenReg.getRegNo(regName.c_str());
      if (regNo >= 0)
      {
        return texGenReg.getTextureId(regNo);
      }
    }
    return BAD_TEXTUREID;
  }

  bool readSelectedTexturePixel(int x, int y, Color4 &out) const override
  {
    // Same lock order as updateSelectedTexture: GpuAutoLock outer, stateLock inner.
    // Guards against the worker closing selectedTexPreview or touching the GPU resource
    // table concurrently with this UI-thread read.
    d3d::GpuAutoLock gpuLock;
    WinAutoLock stateGuard(stateLock);
    if (!selectedTexState.texture || x < 0 || y < 0 || x >= selectedTexState.width || y >= selectedTexState.height)
    {
      return false;
    }

    uint8_t *data = nullptr;
    int stride = 0;
    if (!selectedTexState.texture->lockimg(reinterpret_cast<void **>(&data), stride, 0, TEXLOCK_READ))
    {
      return false;
    }

    out = reinterpret_cast<const Color4 *>(data + stride * y)[x];
    selectedTexState.texture->unlockimg();
    return true;
  }

  // Called from finalizeTexGen with pendingPreviewReg. tex_name comes from pendingPreviewReg, so
  // it's never null/empty here.
  void updateSelectedTexture(const char *tex_name)
  {
    if (!tex_name || !tex_name[0])
    {
      return;
    }

    // Lock order must match the worker: GpuAutoLock outer, stateLock inner.
    // GpuAutoLock serializes against the worker's generation batches (which call
    // stretch_rect / set_render_target / lockimg); stateLock serializes reg-table reads.
    d3d::GpuAutoLock gpuLock;
    WinAutoLock stateGuard(stateLock);

    if (!texGen)
    {
      return;
    }

    eastl::string regName = texgen_get_final_reg_name(texGen, tex_name);
    int regNo = texGenReg.getRegNo(regName.c_str());
    if (regNo < 0)
    {
      return;
    }

    BaseTexture *tex = texGenReg.getTexture(regName.c_str());
    if (!tex)
    {
      return;
    }

    selectedTexState.name = tex_name;

    TextureInfo info;
    tex->getinfo(info);
    selectedTexState.width = info.w;
    selectedTexState.height = info.h;
    selectedTexState.channels = fmtToChannelsCount(info.cflg);

    // Hand the previous UniqueTex to the main-thread destroy queue instead of closing it here.
    // BaseTexture is not refcounted, so any imgui drawlist pointer captured by a panel during the
    // previous frame's updateImgui must remain valid until the main thread runs drainPendingTextureDestroys
    // at the start of the next tickTexGeneration (after that frame's imguiRender has finished).
    if (selectedTexPreview)
    {
      pendingTextureDestroy.push_back(eastl::move(selectedTexPreview));
    }
    int fmt = TEXFMT_A32B32G32R32F | TEXCF_RTARGET;
    String texName(0, "graphed_selected_texture_%u__", ++selectedTexPreviewSeq);
    selectedTexPreview = dag::create_tex(NULL, info.w, info.h, fmt, 1, texName);
    d3d::stretch_rect(tex, selectedTexPreview.getBaseTex());
    selectedTexState.texture = selectedTexPreview.getBaseTex();

    uint8_t *data = nullptr;
    int stride = 0;
    if (selectedTexPreview->lockimg(reinterpret_cast<void **>(&data), stride, 0, TEXLOCK_READ))
    {
      calculateHistogram01(data, stride);
      calculateFullHistogram(data, stride);
      selectedTexPreview->unlockimg();
    }
  }

private:
  // Nested worker thread. Each tick of workerLoop() takes (GpuAutoLock, stateLock) and runs
  // the existing tickTexGenerationCore() for up to workerLockHoldUsec microseconds, then
  // releases both so the main thread can render a frame between batches.
  class TexGenWorker final : public DaThread
  {
    GraphTexGenServiceImpl *svc;

  public:
    TexGenWorker(GraphTexGenServiceImpl *s) : DaThread("GraphTexGen", 256 << 10, 0, 0), svc(s) {}
    void execute() override { svc->workerLoop(); }
  };

  void startWorker()
  {
    if (worker)
    {
      return;
    }

    interlocked_release_store(workerTerminating, 0);
    os_event_create(&wakeEvent, "GraphTexGen_wake");
    worker.reset(new TexGenWorker(this));
    if (!worker->start())
    {
      DAEDITOR3.conError("GraphTexGenService: failed to start worker thread, falling back to synchronous ticks");
      worker.reset();
      os_event_destroy(&wakeEvent);
    }
  }

  void stopWorker()
  {
    if (!worker)
    {
      return;
    }

    interlocked_release_store(workerTerminating, 1);
    os_event_set(&wakeEvent);
    worker->terminate(true, 10000);
    worker.reset();
    os_event_destroy(&wakeEvent);
  }

  // Caller must hold stateLock.
  bool hasWorkUnlocked() const
  {
    return shouldGenerateTex || texGenStep > 0 || forceRebuild || saveTexturesToFiles || generateOnce ||
           (graphDirtyGen != graphCompiledGen && graphCompiler && graphData);
  }

  // Old tickTexGeneration body, minus checkInputFilesForUpdate (that stays on the main-thread
  // poller). Runs under stateLock + GpuAutoLock on the worker thread. Each call performs
  // one state-machine transition (start / step / save), matching the original per-frame
  // cadence semantics.
  void tickTexGenerationCore()
  {
    if (forceRebuild)
    {
      regcache::clear();
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      texGenStep = -1;
      forceRebuild = false;
      shouldGenerateTex = true;
    }

    if (shouldGenerateTex && (autoupdate || generateOnce))
    {
      shouldGenerateTex = false;
      generateOnce = false;
      startGenerateTex();
    }
    else if (texGenStep > 0)
    {
      doGenerateTexStep(texGenStep, texGenStepCount);
    }

    doSaveTextures();
  }

  void workerLoop()
  {
    const int lockHoldUsec = clamp(dgs_get_settings()->getBlockByNameEx("texgen")->getInt("workerLockHoldUsec", 15000), 1000, 100000);
    const int betweenBatchesMsec = clamp(dgs_get_settings()->getBlockByNameEx("texgen")->getInt("workerBetweenBatchesMsec", 1), 0, 50);

    while (!interlocked_acquire_load(workerTerminating))
    {
      // Idle until someone kicks us (main-thread poller, setGraphData, setPreviewFinal,
      // requestForceRebuild, requestRegenerate, markGraphDirty, saveTextures, etc.) or
      // until a spurious wake happens. Cheap when idle.
      bool haveWork = false;
      IGraphCompiler *compilerSnap = nullptr;
      uint64_t compileGenSnap = 0;
      {
        WinAutoLock guard(stateLock);
        haveWork = hasWorkUnlocked();
        // Compile-commit overwrites graphData->mainGraphBlk; doing it while the
        // texgen pipeline is mid-pass (texGenStep > 0) would corrupt the steps
        // (later steps would see different shader names than earlier ones) AND
        // trigger a pipeline restart that throws away the partial work. Defer
        // until the pipeline finalizes; graphDirtyGen != graphCompiledGen stays
        // true so the very next iteration picks it up cleanly. This naturally
        // coalesces all the intermediate compiles during a slider drag into one
        // compile + one pipeline pass per pipeline-duration window.
        if (graphDirtyGen != graphCompiledGen && graphCompiler && graphData && texGenStep <= 0)
        {
          // Snapshot the pointer; the actual call happens outside stateLock so
          // the plugin's compiler can take its own mutex without lock-order risk.
          // compileInProgress lets setGraphCompiler(nullptr) drain this call
          // before the plugin destroys the impl. compileGenSnap captures the
          // current dirty generation so a markGraphDirty() arriving during the
          // compile increments graphDirtyGen past it -- hasWorkUnlocked() will
          // see graphDirtyGen != graphCompiledGen on the next iteration and
          // trigger a follow-up compile. This closes the TOCTOU window that an
          // unconditional `dirty = false` post-compile would have lost.
          compilerSnap = graphCompiler;
          compileInProgress = true;
          compileGenSnap = graphDirtyGen;
        }
      }
      if (!haveWork)
      {
        os_event_wait(&wakeEvent, OS_WAIT_INFINITE);
        continue;
      }

      // Compile pass. Runs outside stateLock + outside GpuAutoLock. The plugin
      // commits the compile result into graphData->mainGraphBlk / shaderListBlk
      // INSIDE compile() under its own mutex (see IGraphCompiler doc) -- the
      // service does not write those BLKs itself, otherwise the worker's
      // stateLock and the plugin's mutation lock would both guard the same
      // memory under different names, which is not a guard at all.
      // Re-applies the preview-final under stateLock since a fresh mainGraphBlk
      // just clobbered the previously-injected `final:t`.
      if (compilerSnap)
      {
        const bool ok = compilerSnap->compile();
        {
          WinAutoLock guard(stateLock);
          // Stage-1 outcome for the status bar. A failed graph compile produces no generation
          // pass, so the bar must learn about it here -- it can't infer it from the texgen logger.
          graphCompileFailed = !ok;
          if (ok && graphData)
          {
            previewFinalAddedToBlk = false;
            addPreviewFinalToBlk();
            shouldGenerateTex = true;
            generateOnce = true;
          }
          // Advance graphCompiledGen even on failure: compile_graph_to_blks is
          // deterministic, so a retry without new input would just fail again
          // and spin the worker. The user has to edit the graph (incrementing
          // graphDirtyGen) to retrigger compile.
          graphCompiledGen = compileGenSnap;
          compileInProgress = false;
        }
      }

      // Run one batch. Both locks are held for up to lockHoldUsec microseconds; the
      // main thread is blocked from rendering / reading shared state during that window.
      {
        d3d::GpuAutoLock gpuLock;
        WinAutoLock stateGuard(stateLock);
        if (hasWorkUnlocked())
        {
          const int64_t t0 = ref_time_ticks();
          do
          {
            tickTexGenerationCore();
          } while (hasWorkUnlocked() && get_time_usec(t0) < lockHoldUsec && !interlocked_acquire_load(workerTerminating));
        }
      }

      // Yield to the main thread so it gets a chance to acquire GpuAutoLock and render
      // a frame of the landscape preview before we grab the locks again. Without this
      // the worker can keep both locks saturated and starve rendering. 1 ms is enough
      // for one main-loop render on a desktop editor; tunable via texgen.workerBetweenBatchesMsec.
      if (!interlocked_acquire_load(workerTerminating) && betweenBatchesMsec > 0)
      {
        sleep_msec(betweenBatchesMsec);
      }
    }
  }

  void initTexGenInternal()
  {
    if (!texGen)
    {
      texGen = create_texture_generator();
    }

    texgen_set_logger(texGen, &graphLogger);
    graphLogger.clear();

    // ROBUST suppresses the "BLK parsed string is really long" warnings --
    // default_texgen.blk's `premain` and default_shaders.blk's entries legitimately
    // carry multi-KB shader sources as triple-quoted strings.
    DataBlock defaultTexgen;
    dblk::load(defaultTexgen, defaultTexgenPath, dblk::ReadFlag::ROBUST);
    load_pixel_shader_texgen(defaultTexgen);

    DataBlock defaultShaders;
    dblk::load(defaultShaders, defaultShadersPath, dblk::ReadFlag::ROBUST);
    add_pixel_shader_texgen(defaultShaders, texGen);
    if (graphData)
    {
      add_pixel_shader_texgen(graphData->shaderListBlk, texGen);
    }

    extern void init_texgen_df_shader(TextureGenerator *);
    init_texgen_df_shader(texGen);
    extern void init_texgen_blur_iter_shader(TextureGenerator *);
    init_texgen_blur_iter_shader(texGen);
    extern void init_texgen_convolution_shader(TextureGenerator *);
    init_texgen_convolution_shader(texGen);
    extern void init_texgen_autolevels_shader(TextureGenerator *);
    init_texgen_autolevels_shader(texGen);
    extern void init_texgen_fill_areas_shader(TextureGenerator *);
    init_texgen_fill_areas_shader(texGen);
    extern void init_texgen_erosion_shader(TextureGenerator *);
    init_texgen_erosion_shader(texGen);
    extern void init_texgen_cache_shader(TextureGenerator *);
    init_texgen_cache_shader(texGen);
    extern void init_texgen_pseudo_dla_shader(TextureGenerator *);
    init_texgen_pseudo_dla_shader(texGen);
    extern void init_texgen_render_text_shader(TextureGenerator *);
    init_texgen_render_text_shader(texGen);
    extern void init_texgen_img_to_gradient_shader(TextureGenerator *);
    init_texgen_img_to_gradient_shader(texGen);
    extern void init_texgen_heightmap_occlusion_shader(TextureGenerator *);
    init_texgen_heightmap_occlusion_shader(texGen);
  }

  void closeTexGen()
  {
    finalStrings.clear();
    delete_texture_generator(texGen);
    texGen = nullptr;
  }

  void startGenerateTex()
  {
    if (!graphData)
    {
      return;
    }

    if (texGenStep > 0)
    {
      texGenStep = 0;
    }

    texGenReg.setEntitiesSaver(&entitiesSaver);
    texGenReg.setTextureRootDir(graphData->textureRootDir.c_str());
    texGenStepCount = clamp(dgs_get_settings()->getBlockByNameEx("texgen")->getInt("texGenPerFrame", 2), 1, 256);
    texGenStep = 0;

    initTexGenInternal();
    debug("TexGenService: graph changed");

    regcache::update();

    DataBlock &mainBlk = graphData->mainGraphBlk;
    for (int i = 0; i < mainBlk.blockCount(); ++i)
    {
      if (DataBlock *node = mainBlk.getBlock(i))
      {
        const char *shaderName = node->getStr("shader", "");
        if (!strcmp(shaderName, "cache_node"))
        {
          const char *id = node->getBlockByNameEx("params")->getStr("preceding_subgraph_hash", "");
          if (regcache::is_record_exists(id) && strcmp(node->getBlockByNameEx("input")->getStr("reg", ""), "tex:const:0") != 0)
          {
            node->getBlockByName("input")->setStr("reg", "tex:const:0");
            node->getBlockByName("params")->setBool("use_data_from_cache", true);
          }
        }
      }
    }

    entitiesSaver.clearUsedBlkNames();
    for (int i = 0; i < mainBlk.blockCount(); i++)
    {
      const char *blkName = mainBlk.getBlock(i)->getBlockByNameEx("params")->getStr("saveEntitiesToFile", nullptr);
      if (blkName)
      {
        entitiesSaver.pushUsedBlkName(blkName);
      }
    }
    entitiesSaver.removeUnusedBlk();

    skipSavingNames.clear();

    eastl::hash_set<eastl::string> newFinalStrings;
    for (int i = 0; i < mainBlk.paramCount(); ++i)
    {
      if (mainBlk.getParamType(i) == DataBlock::TYPE_STRING && strcmp(mainBlk.getParamName(i), "final") == 0)
      {
        newFinalStrings.insert(mainBlk.getStr(i));
      }
      if (mainBlk.getParamType(i) == DataBlock::TYPE_STRING && strcmp(mainBlk.getParamName(i), "skip_saving") == 0)
      {
        skipSavingNames.insert(mainBlk.getStr(i));
      }
    }

    texGenReg.setAutoShrink(true);
    for (const auto &fi : finalStrings)
    {
      if (newFinalStrings.find_as(fi.c_str()) == newFinalStrings.end())
      {
        oldFinalStrings.insert(texgen_get_final_reg_name(texGen, fi.c_str()));
      }
    }
    removeOldFinalStrings();
    finalStrings = newFinalStrings;

    generationCompleted = false;
    // Snapshot the declared-output count now, before doGenerateTexStep can fail and prune
    // finalStrings to empty. Exclude the preview-final (a UI artifact injected into mainGraphBlk
    // by setPreviewFinal), matching how it is excluded from the texture save path.
    declaredOutputsCount = static_cast<int>(newFinalStrings.size());
    if (previewFinalAddedToBlk && newFinalStrings.find(previewFinalName) != newFinalStrings.end())
    {
      --declaredOutputsCount;
    }
    texGenReg.clearStat();
    doGenerateTexStep(texGenStep, 1);
  }

  void doGenerateTexStep(int &start, int count)
  {
    int generated = 0;
    if (start <= 0)
    {
      int totalCount = texgen_start_process_commands(*texGen, graphData->mainGraphBlk, texGenReg);
      if (totalCount < 0)
      {
        generated = -1;
      }
      // Remember the pass size for getPipelineStatus' progress readout (commandsDone/Total).
      texGenTotalCmds = totalCount > 0 ? totalCount : 0;
    }

    if (!generated)
    {
      generated = texgen_process_commands(*texGen, count);
    }

    if (generated < 0)
    {
      DAEDITOR3.conNote("GraphTexGenService: texgen failed");
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      start = -1;
    }
    else if (generated == 0)
    {
      DAEDITOR3.conNote("GraphTexGenService: success, produced %d textures, maxMemUsed = %.1fMB, current = %.1fMB",
        texGenReg.getRegsAlive(), texGenReg.getMaxMemSize() / float(1 << 20), texGenReg.getCurrentMemSize() / float(1 << 20));
      finalizeTexGen();
      start = 0;
      generationCompleted = true;
    }
    else
    {
      start += count;
    }

    if (start >= 0)
    {
      for (const auto &str : finalStrings)
      {
        int varId = get_shader_variable_id(str.c_str(), true);
        if (varId >= 0)
        {
          int resultTexReg = texGenReg.getRegNo(texgen_get_final_reg_name(texGen, str.c_str()));
          if (resultTexReg >= 0)
          {
            ShaderGlobal::set_texture(varId, texGenReg.getTextureId(resultTexReg));
          }
        }
      }
    }
  }

  void finalizeTexGen()
  {
    removeOldFinalStrings();
    for (auto it = finalStrings.begin(); it != finalStrings.end();)
    {
      int resultTexReg = texGenReg.getRegNo(texgen_get_final_reg_name(texGen, it->c_str()));
      if (resultTexReg < 0)
      {
        it = finalStrings.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // Pull the texture for the panel's pending preview register now that the pipeline that was
    // built around it has finished. Lookup may still fail (the register isn't an output reg in the
    // BLK, or the pipeline cancelled before producing it) -- selectedTexState then stays at its
    // previous value, and the panel keeps showing whatever it was showing.
    if (!pendingPreviewReg.empty() && texGen)
    {
      updateSelectedTexture(pendingPreviewReg.c_str());
    }
  }

  void removeOldFinalStrings()
  {
    for (const auto &fi : oldFinalStrings)
    {
      int texReg = texGenReg.getRegNo(fi.c_str());
      if (texReg >= 0)
      {
        while (texGenReg.getRegUsageLeft(texReg))
        {
          texGenReg.textureUsed(texReg);
        }
      }
    }
    oldFinalStrings.clear();
  }

  void calculateHistogram01(const uint8_t *data, int stride)
  {
    auto &h = selectedTexState.histogram01;
    h.resize(256);
    memset(h.data(), 0, h.size() * sizeof(Color4));

    const int w = selectedTexState.width;
    const int texH = selectedTexState.height;
    for (int y = 0; y < texH; ++y)
    {
      const Color4 *scanline = reinterpret_cast<const Color4 *>(data + stride * y);
      for (int x = 0; x < w; ++x)
      {
        const Color4 &px = scanline[x];
        int ir = clamp(int(px.r * 255.f + 0.5f), 0, 255);
        int ig = clamp(int(px.g * 255.f + 0.5f), 0, 255);
        int ib = clamp(int(px.b * 255.f + 0.5f), 0, 255);
        int ia = clamp(int(px.a * 255.f + 0.5f), 0, 255);
        h[ir].r += 1.f;
        h[ig].g += 1.f;
        h[ib].b += 1.f;
        h[ia].a += 1.f;
      }
    }

    Color4 maxCount(1e-10f, 1e-10f, 1e-10f, 1e-10f);
    for (const Color4 &px : h)
    {
      maxCount = max(maxCount, px);
    }

    float k = (texH * texH + 64) / 64.0f;
    maxCount = min(Color4(k, k, k, k), maxCount);

    for (Color4 &px : h)
    {
      px = min(px / maxCount, Color4(1, 1, 1, 1));
    }
  }

  void calculateFullHistogram(const uint8_t *data, int stride)
  {
    auto &h = selectedTexState.histogramFull;
    h.resize(256);
    selectedTexState.minHistValue = VERY_BIG_NUMBER;
    selectedTexState.maxHistValue = -VERY_BIG_NUMBER;
    memset(h.data(), 0, h.size() * sizeof(Color4));

    const int w = selectedTexState.width;
    const int texH = selectedTexState.height;
    int ch = selectedTexState.channels;
    for (int y = 0; y < texH; ++y)
    {
      const Color4 *scanline = reinterpret_cast<const Color4 *>(data + stride * y);
      for (int x = 0; x < w; ++x)
      {
        const Color4 &px = scanline[x];
        selectedTexState.maxHistValue = max(selectedTexState.maxHistValue, px.r);
        selectedTexState.minHistValue = min(selectedTexState.minHistValue, px.r);
        if (ch >= 2)
        {
          selectedTexState.maxHistValue = max(selectedTexState.maxHistValue, px.g);
          selectedTexState.minHistValue = min(selectedTexState.minHistValue, px.g);
        }
        if (ch >= 3)
        {
          selectedTexState.maxHistValue = max(selectedTexState.maxHistValue, px.b);
          selectedTexState.minHistValue = min(selectedTexState.minHistValue, px.b);
        }
        if (ch >= 4)
        {
          selectedTexState.maxHistValue = max(selectedTexState.maxHistValue, px.a);
          selectedTexState.minHistValue = min(selectedTexState.minHistValue, px.a);
        }
      }
    }

    float range = max(selectedTexState.maxHistValue - selectedTexState.minHistValue, 1e-10f);
    float invRange = 255.0f / range;

    for (int y = 0; y < texH; ++y)
    {
      const Color4 *scanline = reinterpret_cast<const Color4 *>(data + stride * y);
      for (int x = 0; x < w; ++x)
      {
        const Color4 &px = scanline[x];
        int ir = clamp(int((px.r - selectedTexState.minHistValue) * invRange + 0.5f), 0, 255);
        int ig = clamp(int((px.g - selectedTexState.minHistValue) * invRange + 0.5f), 0, 255);
        int ib = clamp(int((px.b - selectedTexState.minHistValue) * invRange + 0.5f), 0, 255);
        int ia = clamp(int((px.a - selectedTexState.minHistValue) * invRange + 0.5f), 0, 255);
        h[ir].r += 1.f;
        h[ig].g += 1.f;
        h[ib].b += 1.f;
        h[ia].a += 1.f;
      }
    }

    Color4 maxCount(1e-10f, 1e-10f, 1e-10f, 1e-10f);
    for (const Color4 &px : h)
    {
      maxCount = max(maxCount, px);
    }

    float k = (texH * texH + 64) / 64.0f;
    maxCount = min(Color4(k, k, k, k), maxCount);

    for (Color4 &px : h)
    {
      px = min(px / maxCount, Color4(1, 1, 1, 1));
    }
  }

  bool saveEntities(const char *fname, const char *entity_name, const float *buf, int width, int height, int stride)
  {
    float fcount = buf[0];
    if (fcount != buf[1] || fcount != buf[2] || fcount != buf[3] || fabsf(fcount - int(fcount + 0.5f)) > 1e-3)
    {
      DAEDITOR3.conError("GraphTexGenService: Save Entities: '%s': invalid header", fname);
      return false;
    }

    Tab<String> entityNames;
    for (;;)
    {
      const char *p = strchr(entity_name, ',');
      if (!p)
      {
        entityNames.push_back(String(entity_name));
        break;
      }
      else
      {
        entityNames.push_back(String(entity_name, p - entity_name));
        entity_name = p + 1;
      }
    }

    if (entityNames.empty())
    {
      DAEDITOR3.conError("GraphTexGenService: Save Entities: '%s': entityNames is empty", fname);
      return false;
    }

    int count = static_cast<int>(fcount + 0.5f);
    int posX = 0;
    int posY = 0;

    DataBlock *blk = nullptr;
    auto it = entitiesContent.find(eastl::string(fname));
    if (it != entitiesContent.end())
    {
      blk = it->second;
    }
    else
    {
      blk = new DataBlock;
      entitiesContent.insert(eastl::make_pair(eastl::string(fname), blk));
    }

    for (int i = 0; i < count; i++)
    {
      posX++;
      if (posX * 4 >= width)
      {
        posX = 0;
        posY++;
        if (posY >= height)
        {
          DAEDITOR3.conError("GraphTexGenService: Save Entities: '%s': entities out of texture, count = %d, but got only %d", fname,
            count, i);
          return false;
        }
      }
      const float *ptr = buf + posX * 16 + posY * stride;

      Point3 pos = Point3(ptr[0], ptr[1], ptr[2]);
      Point3 up = Point3(ptr[3], ptr[4], ptr[5]);
      Point3 fwd = Point3(ptr[6], ptr[7], ptr[8]);
      int placeType = int(ptr[9] + 0.5f);
      int nameIndex = abs(int(ptr[10] + 0.5f));
      int entitySeed = int(ptr[11] + 0.5f);
      float fwdScale = fabsf(ptr[12]) < 1e-10f ? 1.f : ptr[12];
      float upScale = fabsf(ptr[13]) < 1e-10f ? 1.f : ptr[13];
      float leftScale = fabsf(ptr[14]) < 1e-10f ? 1.f : ptr[14];

      if (up.lengthSq() < 1e-8 || fwd.lengthSq() < 1e-8)
      {
        continue;
      }

      fwd = normalize(fwd);
      up = normalize(up);
      Point3 left = normalize(fwd % up);
      up = normalize(left % fwd);
      TMatrix tm;
      tm.setcol(0, fwd * fwdScale);
      tm.setcol(1, up * upScale);
      tm.setcol(2, left * leftScale);
      tm.setcol(3, pos);

      DataBlock *entityBlk = blk->addNewBlock("entity");
      entityBlk->addStr("note", "");

      const char *entName = entityNames[nameIndex % entityNames.size()].str();
      String name(0, "%s__generated_%d", entName, i);
      entityBlk->addStr("name", name.str());
      entityBlk->addStr("entName", entName);
      entityBlk->addInt("place_type", placeType);
      entityBlk->addInt("entSeed", entitySeed);
      entityBlk->addTm("tm", tm);
    }

    return true;
  }

  void saveEntitiesContent()
  {
    if (!entitiesContent.empty())
    {
      for (auto &ec : entitiesContent)
      {
        DataBlock *blk = ec.second;
        const char *fileName = ec.first.c_str();
        blk->saveToTextFile(fileName);
        delete blk;
      }
      entitiesContent.clear();
    }
  }

  bool saveToTiff(const char *fname, const uint8_t *buf, int width, int height, int channels, int stride)
  {
    G_ASSERT(channels == 4);
    G_ASSERT(fname);
    G_ASSERT(buf);

    TIFF *image = NULL;
    const int bpp = 8;

    if (width < 2)
    {
      return false;
    }

    if ((image = TIFFOpen(fname, "w")) == NULL)
    {
      DAEDITOR3.conError("GraphTexGenService: Could not open <%s> for write", (char *)fname);
      return false;
    }

    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bpp);
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, channels);
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    if (channels == 4)
    {
      uint16_t out[1] = {EXTRASAMPLE_UNSPECIFIED};
      TIFFSetField(image, TIFFTAG_EXTRASAMPLES, 1, &out);
    }

    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);

    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(image, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);

    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField(image, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    int lineBytes = width * bpp * channels / 8;
    uint8_t *bits = new uint8_t[lineBytes];

    for (int strip = 0; strip < height; strip++)
    {
      memcpy(bits, buf, lineBytes);
      G_ASSERT(channels == 4);
      for (int i = 0; i < lineBytes; i += 4)
      {
        eastl::swap(bits[i], bits[i + 2]);
      }

      if (TIFFWriteEncodedStrip(image, strip, (tdata_t)bits, lineBytes) < 0)
      {
        G_ASSERTF(0, "Failed to write tiff file");
        break;
      }
      buf += stride;
    }

    delete[] bits;
    TIFFClose(image);
    return true;
  }

  bool saveR16ToBitmapTiff(const char *fname, const uint8_t *buf, int width, int height, int stride, int pixel_step, bool is_grayscale)
  {
    G_ASSERT(pixel_step == 1 || pixel_step == 2);
    G_ASSERT(fname);
    G_ASSERT(buf);

    TIFF *image = NULL;
    const int bpp = is_grayscale ? 8 : 1;
#if !_TARGET_CPU_BE
    buf += pixel_step - 1;
#endif

    if (width < 8)
    {
      return false;
    }

    if ((image = TIFFOpen(fname, "w")) == NULL)
    {
      DAEDITOR3.conError("GraphTexGenService: Could not open <%s> for write", (char *)fname);
      return false;
    }

    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bpp);
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);

    if (bpp == 1)
    {
      TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
    }
    else
    {
      TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
      TIFFSetField(image, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    }

    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField(image, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    uint8_t *bits = new uint8_t[width * bpp / 8];

    for (int strip = 0; strip < height; strip++)
    {
      if (bpp == 1)
      {
        for (int x = 0; x < width / 8; x++)
        {
          uint8_t b = 0;
          for (int bit = 0; bit < 8; bit++)
          {
            if (buf[(x * 8 + 7 - bit) * pixel_step] > 127)
            {
              b |= 1 << bit;
            }
          }

          bits[x] = b;
        }
      }
      else
      {
        if (pixel_step == 1)
        {
          memcpy(bits, buf, width);
        }
        else
        {
          for (int x = 0; x < width; x++)
          {
            bits[x] = buf[x * pixel_step];
          }
        }
      }

      if (TIFFWriteEncodedStrip(image, strip, (tdata_t)bits, width * bpp / 8) < 0)
      {
        G_ASSERTF(0, "Failed to write tiff file");
        break;
      }
      buf += stride;
    }

    delete[] bits;
    TIFFClose(image);
    return true;
  }

  void doSaveTextures()
  {
    if (!saveTexturesToFiles || shouldGenerateTex || texGenStep != 0 || !texGen)
    {
      return;
    }

    const char *rDir = (graphData && !graphData->renderDir.empty()) ? graphData->renderDir.c_str() : "render";
    const char *eDir = (graphData && !graphData->entityDir.empty()) ? graphData->entityDir.c_str() : "entity";

    for (const auto &fStr : finalStrings)
    {
      if (!saveTextureMask.empty() && strstr(fStr.c_str(), saveTextureMask.str()) == nullptr)
      {
        continue;
      }

      BaseTexture *baseTex = texGenReg.getTexture(texgen_get_final_reg_name(texGen, fStr.c_str()));
      if (!baseTex)
      {
        continue;
      }

      Tab<const char *> fnames;
      String tmp(fStr.c_str());
      {
        char *p = (char *)tmp.str();
        while (char *next = strstr(p, "$delimiter$"))
        {
          *next = 0;
          fnames.push_back(p);
          p = next + 11;
        }
        if (*p)
        {
          fnames.push_back(p);
        }
      }

      for (int k = 0; k < fnames.size(); k++)
      {
        const char *fname = fnames[k];
        if (!strncmp(fname, "_t_", 3) && fname[3] >= '0' && fname[3] <= '9')
        {
          continue;
        }

        if (skipSavingNames.find(fname) != skipSavingNames.end())
        {
          continue;
        }

        if (baseTex->getType() == D3DResourceType::TEX)
        {
          Texture *tex = (Texture *)baseTex;
          TextureInfo texInfo;
          tex->getinfo(texInfo, 0);
          char *data = NULL;
          int stride = 0;

          if (strstr(fname, "export: ") == fname && tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) != 0)
          {
            int fmt = (texInfo.cflg & TEXFMT_MASK);
            int in_channels = 0;
            int in_bits_per_channel = 0;
            bool in_float = false;
            int out_channels = 0;
            int out_bits_per_channel = 0;
            bool out_float = false;
            bool swapRB = false;

            switch (fmt)
            {
              case TEXFMT_R8:
                in_channels = 1;
                in_bits_per_channel = 8;
                break;
              case TEXFMT_L16:
                in_channels = 1;
                in_bits_per_channel = 16;
                break;
              case TEXFMT_R32UI:
                in_channels = 1;
                in_bits_per_channel = 32;
                break;
              case TEXFMT_R16F:
                in_channels = 1;
                in_bits_per_channel = 16;
                in_float = true;
                break;
              case TEXFMT_R32F:
                in_channels = 1;
                in_bits_per_channel = 32;
                in_float = true;
                break;

              case TEXFMT_R8G8:
                in_channels = 2;
                in_bits_per_channel = 8;
                break;
              case TEXFMT_G16R16:
                in_channels = 2;
                in_bits_per_channel = 16;
                break;
              case TEXFMT_G16R16F:
                in_channels = 2;
                in_bits_per_channel = 16;
                in_float = true;
                break;
              case TEXFMT_G32R32F:
                in_channels = 2;
                in_bits_per_channel = 32;
                in_float = true;
                break;

              case TEXFMT_A8R8G8B8:
                in_channels = 4;
                in_bits_per_channel = 8;
                swapRB = true;
                break;
              case TEXFMT_A16B16G16R16:
              case TEXFMT_A16B16G16R16UI:
                in_channels = 4;
                in_bits_per_channel = 16;
                break;
              case TEXFMT_A16B16G16R16F:
                in_channels = 4;
                in_bits_per_channel = 16;
                in_float = true;
                break;
              case TEXFMT_A32B32G32R32F:
                in_channels = 4;
                in_bits_per_channel = 32;
                in_float = true;
                break;
              default:
                DAEDITOR3.conError("GraphTexGenService: export: unsupported input texture format (%s)", fname);
                tex->unlockimg();
                continue;
            }

            if (strstr(fname, "; bitmap") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 1;
            }
            else if (strstr(fname, "; R8") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 8;
            }
            else if (strstr(fname, "; R16F") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 16;
              out_float = true;
            }
            else if (strstr(fname, "; R16") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 16;
            }
            else if (strstr(fname, "; R32F") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 32;
              out_float = true;
            }
            else if (strstr(fname, "; R32") != nullptr)
            {
              out_channels = 1;
              out_bits_per_channel = 32;
            }
            else if (strstr(fname, "; RGB8") != nullptr)
            {
              out_channels = 3;
              out_bits_per_channel = 8;
            }
            else if (strstr(fname, "; RGB16F") != nullptr)
            {
              out_channels = 3;
              out_bits_per_channel = 16;
              out_float = true;
            }
            else if (strstr(fname, "; RGB16") != nullptr)
            {
              out_channels = 3;
              out_bits_per_channel = 16;
            }
            else if (strstr(fname, "; RGB32F") != nullptr)
            {
              out_channels = 3;
              out_bits_per_channel = 32;
              out_float = true;
            }
            else if (strstr(fname, "; RGB32") != nullptr)
            {
              out_channels = 3;
              out_bits_per_channel = 32;
            }
            else if (strstr(fname, "; ARGB8") != nullptr)
            {
              out_channels = 4;
              out_bits_per_channel = 8;
            }
            else if (strstr(fname, "; ARGB16F") != nullptr)
            {
              out_channels = 4;
              out_bits_per_channel = 16;
              out_float = true;
            }
            else if (strstr(fname, "; ARGB16") != nullptr)
            {
              out_channels = 4;
              out_bits_per_channel = 16;
            }
            else if (strstr(fname, "; ARGB32F") != nullptr)
            {
              out_channels = 4;
              out_bits_per_channel = 32;
              out_float = true;
            }
            else if (strstr(fname, "; ARGB32") != nullptr)
            {
              out_channels = 4;
              out_bits_per_channel = 32;
            }
            else
            {
              G_ASSERTF(0, "export: unsupported output texture format (%s)", fname);
            }

            String res(0, "export: unsupported extension (%s)", fname);

            String extractedName;
            extractedName.setStr(fname + sizeof("export: ") - 1);
            char *nameEndChar = strchr(extractedName.str(), ';');
            G_ASSERT_CONTINUE(nameEndChar);
            *nameEndChar = 0;
            String fileName(128, "%s/%s", rDir, extractedName.str());

            if (strstr(fname, ".tiff;") != nullptr || strstr(fname, ".tif;") != nullptr)
            {
              res = export_any_tiff(fileName.str(), data, texInfo.w, texInfo.h, stride, in_channels, in_bits_per_channel, in_float,
                out_channels, out_bits_per_channel, out_float, swapRB);
            }

            if (strstr(fname, ".tga;") != nullptr)
            {
              res = export_any_tga(fileName.str(), data, texInfo.w, texInfo.h, stride, in_channels, in_bits_per_channel, in_float,
                out_channels, out_bits_per_channel, out_float, swapRB);
            }

            if (strstr(fname, ".raw;") != nullptr)
            {
              res = export_any_raw(fileName.str(), data, texInfo.w, texInfo.h, stride, in_channels, in_bits_per_channel, in_float,
                out_channels, out_bits_per_channel, out_float, swapRB);
            }

            tex->unlockimg();
            continue;
          }

          const bool isTIFF =
            (strstr(fname, ".tif") + 4 - fname == (int)strlen(fname)) || (strstr(fname, ".tiff") + 5 - fname == (int)strlen(fname));
          const bool isGrayscale = strstr(fname, "Grayscale.") || strstr(fname, "grayscale.");
          const bool isEntities = strstr(fname, ".blk:");

          if (((texInfo.cflg & TEXFMT_MASK) == TEXFMT_L16) && tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) != 0)
          {
            DAEDITOR3.conNote("GraphTexGenService: processing '%s'", fname);

            if (isTIFF)
            {
              String fileName(128, "%s/%s", rDir, fname);
              bool res = saveR16ToBitmapTiff(fileName.str(), (const uint8_t *)data, texInfo.w, texInfo.h, stride, 2, isGrayscale);
              if (!res)
              {
                DAEDITOR3.conError("GraphTexGenService: cannot save '%s'", fname);
              }
            }
            else
            {
              String fileName(128, "%s/%s.raw", rDir, fname);
              file_ptr_t h = df_open(fileName.str(), DF_WRITE | DF_CREATE);
              if (!h)
              {
                DAEDITOR3.conError("GraphTexGenService:Cannot create file '%s' for R16 texture", fileName.str());
              }
              else
              {
                int memSize = stride * texInfo.h;
                df_write(h, data, memSize);
                df_close(h);
              }
            }

            tex->unlockimg();
          }
          else if (isEntities && ((texInfo.cflg & TEXFMT_MASK) == TEXFMT_A32B32G32R32F) &&
                   tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) != 0)
          {
            const char *delim = strchr(fname, ':');
            G_ASSERT(delim);
            String fn(fname, delim - fname);
            String entitiesList(delim + 1);
            DAEDITOR3.conNote("GraphTexGenService: processing '%s' as entities", fn.str());

            String fileName(128, "%s/%s", eDir, fn.str());
            bool res =
              saveEntities(fileName.str(), entitiesList.str(), (const float *)data, texInfo.w, texInfo.h, stride / sizeof(float));

            if (!res)
            {
              DAEDITOR3.conError("GraphTexGenService: cannot save '%s'", fn.str());
            }
            tex->unlockimg();
          }
          else if (((texInfo.cflg & TEXFMT_MASK) == TEXFMT_R8) && tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) != 0)
          {
            if (isTIFF)
            {
              DAEDITOR3.conNote("GraphTexGenService: processing '%s'", fname);

              String fileName(128, "%s/%s", rDir, fname);
              bool res = saveR16ToBitmapTiff(fileName.str(), (const uint8_t *)data, texInfo.w, texInfo.h, stride, 1, isGrayscale);
              if (!res)
              {
                DAEDITOR3.conError("GraphTexGenService:cannot save '%s'", fname);
              }
            }
            else
            {
              DAEDITOR3.conWarning("GraphTexGenService: '%s' ignored", fname);
            }

            tex->unlockimg();
          }
          else if (((texInfo.cflg & TEXFMT_MASK) == TEXFMT_A8R8G8B8) && tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) != 0)
          {
            DAEDITOR3.conNote("GraphTexGenService:processing '%s'", fname);

            if (isTIFF)
            {
              String fileName(128, "%s/%s", rDir, fname);
              bool res = saveToTiff(fileName.str(), (const uint8_t *)data, texInfo.w, texInfo.h, 4, texInfo.w * 4);
              if (!res)
              {
                DAEDITOR3.conError("GraphTexGenService: cannot save '%s'", fname);
              }
            }
            else
            {
              String fileName(128, "%s/%s.tga", rDir, fname);
              if (!save_tga32(fileName.str(), (TexPixel32 *)data, texInfo.w, texInfo.h, stride))
              {
                DAEDITOR3.conError("GraphTexGenService: Cannot save ARGB texture '%s'", fileName.str());
              }
            }

            tex->unlockimg();
          }
          else
          {
            DAEDITOR3.conWarning("GraphTexGenService: '%s' has unsupported texture format, skipped.", fname);
          }
        }
      }
    }

    saveTexturesToFiles = false;
    saveEntitiesContent();

    const char *eDir2 = (graphData && !graphData->entityDir.empty()) ? graphData->entityDir.c_str() : "entity";
    entitiesSaver.saveAllEntitiesToFiles(eDir2);

    static int generationCounter = 0;
    const time_t now = time(nullptr);
    const tm calendarTime = *localtime(&now);
    DAEDITOR3.conNote("GraphTexGenService: --- save finished #%d, at %02d:%02d:%02d", ++generationCounter, calendarTime.tm_hour,
      calendarTime.tm_min, calendarTime.tm_sec);
  }

  // Recompiler state
  String defaultTexgenPath;
  String defaultShadersPath;
  GraphData *graphData = nullptr; // owned by the plugin; null = idle
  eastl::string previewFinalName;
  bool previewFinalAddedToBlk = false;
  // What the panel last asked to preview. After each finalizeTexGen we re-pull this register's
  // texture into selectedTexState so the panel doesn't have to poll.
  eastl::string pendingPreviewReg;

  // Heightmap metadata from main graph
  float heightmapScale = FLT_MAX;
  float heightmapMin = FLT_MAX;
  float heightmapCellSize = FLT_MAX;

  // Texture generation
  TextureGenerator *texGen = nullptr;
  TextureRegManager texGenReg;
  eastl::hash_set<eastl::string> finalStrings;
  eastl::hash_set<eastl::string> oldFinalStrings;
  eastl::hash_set<eastl::string> skipSavingNames;
  int texGenStep = 0;
  int texGenStepCount = 2;
  // Command count returned by the most recent texgen_start_process_commands; pairs with
  // texGenStep to form the status bar's "done/total" progress readout.
  int texGenTotalCmds = 0;
  // Declared graph outputs (final:t entries) captured at the start of the most recent generation
  // pass, minus the preview-final UI artifact. Reported by the status bar's Outputs counter
  // instead of finalStrings.size(), which a generation failure prunes to 0 (see getPipelineStatus).
  int declaredOutputsCount = 0;
  // Build outcome for the status bar (see TexGenPipelineStatus). graphCompileFailed: stage-1 graph
  // compile returned false. generationCompleted: a stage-2 generation pass reached success. Both
  // reset on graph swap; generationCompleted also resets at each pass start.
  bool graphCompileFailed = false;
  bool generationCompleted = false;

  // Flags
  bool shouldGenerateTex = false;
  bool autoupdate = true;
  // One-shot regen request that bypasses the autoupdate gate. Set by requestRegenerate(),
  // cleared on the next startGenerateTex() pass.
  bool generateOnce = false;
  bool forceRebuild = false;
  // Plugin-supplied graph compiler. The worker invokes compile() outside stateLock
  // when the graph is dirty. nullptr = no compile path wired (service ignores dirty).
  IGraphCompiler *graphCompiler = nullptr;
  // Generation counter: incremented by markGraphDirty() on every mutation. The
  // worker captures its current value before calling compile() and writes the
  // captured value into `graphCompiledGen` after the call returns. The compare
  // `graphDirtyGen != graphCompiledGen` decides whether a compile is pending.
  uint64_t graphDirtyGen = 0;
  uint64_t graphCompiledGen = 0;
  // True while the worker has captured graphCompiler and is mid-call. Used by
  // setGraphCompiler(nullptr) to drain in-flight calls before the plugin destroys
  // the impl. Set/cleared under stateLock around the compile() invocation.
  bool compileInProgress = false;

  // Auto-rebuild when any texgen input file changes on disk (e.g. loft masks re-exported by HeightmapLand).
  FileTimeCheckerArray inputFileCheckers;
  float timeToRecalcGraphSec = -1.f;

  // Save state
  bool saveTexturesToFiles = false;
  String saveTextureMask;
  TextureGenEntitiesSaverImpl entitiesSaver;
  eastl::unordered_map<eastl::string, DataBlock *> entitiesContent;

  // Texture preview
  SelectedTextureState selectedTexState;
  UniqueTex selectedTexPreview;
  // Bumped each time selectedTexPreview is replaced so each UniqueTex registers under a unique
  // managed name. A constant name collided in the managed resource table with the prior instance
  // still held in pendingTextureDestroy, inflating refcount and asserting on drain.
  uint32_t selectedTexPreviewSeq = 0;
  // BaseTexture is not refcounted, so the worker can't destroy a UniqueTex inline if a panel may
  // still hold the underlying pointer in an imgui drawlist from this frame. Worker pushes outgoing
  // textures here; the main thread drains and destroys them at the start of the next
  // tickTexGeneration, after the previous frame's imguiRender has completed.
  eastl::vector<UniqueTex> pendingTextureDestroy;

  // Logger
  GraphLoggerImpl graphLogger;

  // Worker thread and its synchronization. All shared-state members above are accessed
  // from the worker only while holding stateLock (for the data) and d3d::GpuAutoLock (for
  // the D3D calls inside doGenerateTexStep / doSaveTextures). Main-thread accessors take
  // stateLock (and, for those that do D3D, GpuAutoLock as well). Lock order is always
  // GpuAutoLock outer, stateLock inner.
  eastl::unique_ptr<TexGenWorker> worker;
  os_event_t wakeEvent{};
  mutable WinCritSec stateLock;
  volatile int workerTerminating = 0;
};


void init_texgen_service()
{
  // daEditorX startup registers the common tool tex factories but not the raw-image or TIFF
  // ones (see dagorEd.cpp / regCommonToolTex.cpp). `raw texture` graph nodes resolve to a
  // tex:...;name:t=*.r16/.r32/.raw spec and TIFF texture-name nodes to *.tif; both need their
  // factory to load, just as the standalone graphEditor tool registers them in
  // prog/tools/graphEditor/main.cpp.
  extern void register_raw_tex_create_factory();
  register_raw_tex_create_factory();
  extern void register_tiff_tex_create_factory();
  register_tiff_tex_create_factory();

  IDaEditor3Engine::get().registerService(new (inimem) GraphTexGenServiceImpl);
}
