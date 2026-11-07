// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>

#include "graph_tex_gen_service.h"
#include "image_export.h"

#include <graphEditor/graph_data.h>

#include <de3_interface.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
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
#include <textureGen/textureGenDshl.h>
#include "texgen_dshl_assemble.h"
#include "texgen_dshl_compile.h"
#include <libTools/util/strUtil.h>
#include <libTools/util/appDirRelativePath.h>
#include <util/dag_fileTimeChecker.h>
#include <util/dag_hash.h>
#include <workCycle/dag_workCycle.h>

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_eventQueryHolder.h>
#include <3d/dag_resPtr.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>
#include <EASTL/string_view.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>
#include <time.h>
#include <float.h>

namespace
{
constexpr int MAX_FENCE_POLL_USEC = 250000;
constexpr int MAX_SLEEP_OVERSHOOT_USEC = 1000;
constexpr int FENCE_YIELD_POLL_USEC = 100;
constexpr int MAX_YIELD_POLL_PREDICTION_FACTOR = 2;
constexpr int MIN_YIELD_POLL_USEC = 2000;
constexpr uint64_t MAX_MEASURED_CMD_USEC = 10000000;
constexpr int LOAD_AHEAD_NODE_WINDOW = 16;
constexpr int MAX_KEY_COSTS = 1 << 16;
constexpr int MIN_COST_KEYS_FOR_SPREAD = 2;
constexpr float MAX_COST_KEY_SPREAD = 5.0f;

int usec_for_pixels(float usec_per_pixel, int pixels)
{
  return static_cast<int>(min(usec_per_pixel * static_cast<float>(pixels), static_cast<float>(MAX_MEASURED_CMD_USEC)));
}

void wait_before_fence_poll(int64_t flush_end_ticks, int predicted_usec, int poll_msec)
{
  const int elapsedUsec = get_time_usec(flush_end_ticks);
  const bool sleepEndsInBatch = elapsedUsec + poll_msec * 1000 + MAX_SLEEP_OVERSHOOT_USEC <= predicted_usec;
  const bool batchOverdue = elapsedUsec > predicted_usec * MAX_YIELD_POLL_PREDICTION_FACTOR + MIN_YIELD_POLL_USEC;
  if (sleepEndsInBatch || batchOverdue)
  {
    sleep_msec(poll_msec);
    return;
  }
  const int64_t yieldStart = ref_time_ticks();
  do
  {
    sleep_usec(0);
  } while (get_time_usec(yieldStart) < FENCE_YIELD_POLL_USEC);
}

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

const eastl::string *find_label(const CompiledNameLabels &labels, eastl::string_view name)
{
  auto it = labels.find_as(name);
  return it != labels.end() ? &it->second : nullptr;
}

// Texgen messages put names in `<...>`; a compiled name there becomes its label, brackets included.
String apply_name_labels(const char *text, const CompiledNameLabels &graph_labels, const CompiledNameLabels &variant_labels)
{
  eastl::string out(text);
  size_t open = 0;
  while ((open = out.find('<', open)) != eastl::string::npos)
  {
    const size_t close = out.find('>', open + 1);
    if (close == eastl::string::npos)
    {
      break;
    }
    const eastl::string_view name(out.data() + open + 1, close - open - 1);
    const eastl::string *label = find_label(graph_labels, name);
    if (!label)
    {
      label = find_label(variant_labels, name);
    }
    if (label)
    {
      out.replace(open, close - open + 1, *label);
      open += label->size();
    }
    else
    {
      ++open;
    }
  }
  return String(out.c_str());
}

class GraphLoggerImpl : public TextureGenLogger
{
public:
  // Where a message logged outside a node goes, in pipeline order: the status bar shows an earlier stage first.
  enum class LogStage
  {
    DshlCompile,
    ShaderSetup,
    Pass,
    Count
  };

  // The full transcript of the pass.
  String lines;
  // Not reset by startPass(): they outlive one generation pass.
  CompiledNameLabels graphLabels;
  CompiledNameLabels variantLabels;
  // Set only around a pass step: outside one, the generator's current node is not what a message is about.
  TextureGenerator *steppingGen = nullptr;
  // GraphData::mainGraphStamp of the compile whose blocks the node logs name: the one the last pass ran.
  uint32_t logsStamp = 0;

  void startPass()
  {
    ::clear_and_shrink(lines);
    dropStage(LogStage::Pass);
  }

  void dropStage(LogStage s) { stageMessages[static_cast<size_t>(s)] = NewestMessages(); }

  // A message of a step outside texgen, which keeps it until the step runs again.
  void report(LogStage s, int level, const String &text) { logTo(s, level, text); }

  void dropAllMessages()
  {
    nodeLogs.clear();
    logsStamp = 0;
    for (NewestMessages &m : stageMessages)
    {
      m = NewestMessages();
    }
  }

  // Right after a pass starts: a node it runs logs anew, and a changed or gone node is not the one that logged.
  void checkNodeLogs(TextureGenerator &gen, const DataBlock &commands, uint32_t commands_stamp)
  {
    logsStamp = commands_stamp;
    servedRecords.clear();
    for (int i = 0; i < commands.blockCount(); ++i)
    {
      const char *id = texgen_get_cache_node_record_id(*commands.getBlock(i));
      if (id && regcache::is_record_exists(id))
      {
        servedRecords.insert(id);
      }
    }
    if (nodeLogs.empty() && (servedRecords.empty() || recordLogs.empty()))
    {
      return;
    }
    eastl::vector<const char *> passNames, usedNames;
    texgen_get_pass_node_names(gen, passNames);
    texgen_get_used_node_names(commands, usedNames);
    const eastl::hash_set<eastl::string_view> passNodes(passNames.begin(), passNames.end());
    const eastl::hash_set<eastl::string_view> usedNodes(usedNames.begin(), usedNames.end());
    auto outdated = [&](const NodeLog &l) {
      const char *name = l.messages.nodeName.c_str();
      return texgen_get_node_key(gen, name) != l.nodeKey || passNodes.count(name) != 0;
    };
    nodeLogs.erase(eastl::remove_if(nodeLogs.begin(), nodeLogs.end(), outdated), nodeLogs.end());
    // A served record stands in for its inputs, so they do not run: they get back the logs they had when it was made.
    for (const eastl::string &id : servedRecords)
    {
      auto record = recordLogs.find(id);
      if (record == recordLogs.end())
      {
        continue;
      }
      for (const NodeLog &kept : record->second)
      {
        const char *name = kept.messages.nodeName.c_str();
        if (passNodes.count(name) == 0 && texgen_get_node_key(gen, name) == kept.nodeKey && !findNodeLog(name))
        {
          nodeLogs.push_back(kept);
        }
      }
    }
    // Hidden, not dropped: a final that reads the node again can take its kept result, which does not log again.
    for (NodeLog &l : nodeLogs)
    {
      l.used = usedNodes.count(l.messages.nodeName.c_str()) != 0;
    }
  }

  // At every pass end, stop or cut: each record keeps the logs of its inputs, for a later graph bind that the record serves.
  void snapshotRecordLogs(const DataBlock &commands)
  {
    for (auto it = recordLogs.begin(); it != recordLogs.end();)
    {
      if (regcache::is_record_exists(it->first.c_str()))
      {
        ++it;
      }
      else
      {
        it = recordLogs.erase(it);
      }
    }
    eastl::vector<const char *> inputs;
    for (int i = 0; i < commands.blockCount(); ++i)
    {
      const DataBlock &node = *commands.getBlock(i);
      const char *id = texgen_get_cache_node_record_id(node);
      if (!id || !regcache::is_record_exists(id))
      {
        continue;
      }
      // An input of a record served since the pass start ran, if at all, with the same block and inputs, so the logs it keeps
      // still hold.
      if (servedRecords.find_as(id) != servedRecords.end() && recordLogs.find_as(id) != recordLogs.end())
      {
        continue;
      }
      texgen_get_node_input_names(commands, node.getBlockName(), inputs);
      eastl::vector<NodeLog> &kept = recordLogs[id];
      kept.clear();
      for (const char *name : inputs)
      {
        if (const NodeLog *l = findNodeLog(name))
        {
          kept.push_back(*l);
        }
      }
    }
  }

  // Next to each regcache::clear(): the records are gone.
  void dropRecordLogs()
  {
    recordLogs.clear();
    servedRecords.clear();
  }

  // A stage message first, as an earlier stage can cause the later messages; else the newest node message.
  bool findShown(bool error, String &text, eastl::string &node) const
  {
    for (const NewestMessages &m : stageMessages)
    {
      if (m.seq(error) != 0)
      {
        text = m.text(error);
        node.clear();
        return true;
      }
    }
    const NodeLog *newest = nullptr;
    for (const NodeLog &l : nodeLogs)
    {
      if (l.used && l.newest.seq(error) > (newest ? newest->newest.seq(error) : 0))
      {
        newest = &l;
      }
    }
    if (!newest)
    {
      return false;
    }
    text = newest->newest.text(error);
    node = newest->messages.nodeName;
    return true;
  }

  void copyNodeMessages(eastl::vector<TexGenNodeMessages> &out) const
  {
    for (const NodeLog &l : nodeLogs)
    {
      if (l.used)
      {
        out.push_back(l.messages);
      }
    }
  }

  String getFullLog() const { return lines; }

  void log(int level, const String &s) override { logTo(LogStage::Pass, level, s); }

private:
  void logTo(LogStage stage, int level, const String &s)
  {
    if (level == LOGLEVEL_ERR || level == LOGLEVEL_WARN)
    {
      const bool error = level == LOGLEVEL_ERR;
      const String text = apply_name_labels(s.str(), graphLabels, variantLabels);
      if (error)
      {
        DAEDITOR3.conError("GraphTexGenService: %s", text.str());
      }
      else
      {
        DAEDITOR3.conWarning("GraphTexGenService: %s", text.str());
      }
      ++messageSeq;
      if (const char *node = steppingGen ? texgen_get_current_node_name(*steppingGen) : nullptr)
      {
        addNodeMessage(*steppingGen, node, error, text);
      }
      else
      {
        stageMessages[static_cast<size_t>(stage)].add(error, text, messageSeq);
      }
    }

    if (level <= startLevel)
    {
      logmessage(level, "%s", s.str());
      lines.aprintf(1024, "%s\n", s.str());
    }
  }

  // The newest error and warning of one source; the seq orders them across sources.
  struct NewestMessages
  {
    String error, warning;
    uint32_t errorSeq = 0, warningSeq = 0; // 0: none

    void add(bool is_error, const String &s, uint32_t seq)
    {
      (is_error ? error : warning) = s;
      (is_error ? errorSeq : warningSeq) = seq;
    }
    uint32_t seq(bool is_error) const { return is_error ? errorSeq : warningSeq; }
    const String &text(bool is_error) const { return is_error ? error : warning; }
  };
  struct NodeLog
  {
    TexGenNodeMessages messages;
    NewestMessages newest;
    uint64_t nodeKey = 0; // the node's content hash when it logged
    bool used = true;     // a final reads the node; else the log is not shown
  };
  // A source keeps its messages until it runs again.
  eastl::vector<NodeLog> nodeLogs;
  NewestMessages stageMessages[static_cast<size_t>(LogStage::Count)];
  // Never reset, so a kept message never looks newer than a later one.
  uint32_t messageSeq = 0;
  // By cache record key: the logs of the record's inputs when it was made. A graph bind can take the record, not run them.
  eastl::hash_map<eastl::string, eastl::vector<NodeLog>> recordLogs;
  // The records that stood in for their inputs at the last pass start.
  eastl::hash_set<eastl::string> servedRecords;

  NodeLog *findNodeLog(const char *node)
  {
    auto it = eastl::find_if(nodeLogs.begin(), nodeLogs.end(), [node](const NodeLog &l) { return l.messages.nodeName == node; });
    return it != nodeLogs.end() ? &*it : nullptr;
  }

  void addNodeMessage(TextureGenerator &gen, const char *node, bool error, const String &s)
  {
    NodeLog *entry = findNodeLog(node);
    if (!entry)
    {
      entry = &nodeLogs.push_back();
      entry->messages.nodeName = node;
      entry->nodeKey = texgen_get_node_key(gen, node);
    }
    else
    {
      entry->messages.text.push_back('\n');
    }
    entry->messages.text.append(s.str());
    entry->messages.hasErrors = entry->messages.hasErrors || error;
    entry->newest.add(error, s, messageSeq);
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

  // The service plugin unregisters before GraphEditorPlg (lower render order), so at exit and on a workspace
  // switch the plugin's own shutdownTexGen call finds no service; this is the path that always runs.
  ~GraphTexGenServiceImpl() override { shutdownTexGen(); }

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

    texgen_set_command_sync(TextureGenCommandSync::None);
    texgen_set_max_tile_size(clamp(dgs_get_settings()->getBlockByNameEx("texgen")->getInt("maxTileSize", 1024), 0, 16384));
    startWorker();
  }

  void shutdownTexGen() override
  {
    stopWorker();
    {
      d3d::GpuAutoLock gpuLock;
      closeTexGen();
      texGenReg.close();
      close_pixel_shader_texgen();
      if (dshlBindump != INVALID_BINDUMP_HANDLE)
      {
        unload_shaders_bindump(dshlBindump);
        dshlBindump = INVALID_BINDUMP_HANDLE;
        // The dump stays on disk; a pass before the next compile loads it again.
        dshlNeedLoad = true;
      }
      selectedTexPreview.close();
      selectedTexState = SelectedTextureState{};
    }
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
        texGenReg.markFileChanged(inputFileCheckers[i].getFileName());
      }
    }

    if (changed)
    {
      timeToRecalcGraphSec = max(timeToRecalcGraphSec, inputFileCheckers.size() <= 1 ? 0.2f : 1.2f);
      inputFileCheckers.updateFileTimes();
      regcache::clear();
      graphLogger.dropRecordLogs();
    }
  }

  // Main-thread poller. Runs every frame, does no D3D / no heavy work:
  // polls input files (on main because it's pure file I/O plus a cheap reg query),
  // then wakes the worker if there is pending work. The actual generation runs on the
  // worker thread inside workerLoop() under GpuAutoLock + stateLock.
  void tickTexGeneration() override
  {
    drainPendingTextureDestroys();

    if (!graphData)
    {
      return;
    }

    bool shouldKick = false;
    {
      WinAutoLock guard(stateLock);
      // A full device reset empties every render target; a failed one is retried.
      const unsigned fullResetGen = get_d3d_full_reset_counter();
      if (fullResetGen != texGenFullResetGen && !is_restoring_3d_device())
      {
        forceRebuild = true;
        texGenFullResetGen = fullResetGen;
      }
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
    // texGenStep is positive only while a pass is stepping; it parks at 0 (idle / finished) or -1
    // (failed), which both render as "no pass in flight".
    st.nodesDone = texGenStep > 0 && texGen ? texgen_get_done_node_count(*texGen) : 0;
    st.nodesTotal = texGenTotalNodes;
    // Outputs = declared graph finals captured at generation start (declaredOutputsCount), NOT
    // finalStrings.size(). On a generation failure the pipeline closes texGenReg and then
    // finalizeTexGen prunes every final whose register no longer exists, emptying finalStrings --
    // which would otherwise make the Outputs counter read 0 the moment a compile fails.
    st.outputsCount = declaredOutputsCount;
    st.generating = hasWorkUnlocked();

    // Build outcome for the status-bar message. graphLogger is written by the worker under
    // stateLock (same lock held here), so this snapshot is consistent.
    st.graphCompileFailed = graphCompileFailed;
    st.generationCompleted = generationCompleted;
    st.passHeldByAutoupdate = shouldGenerateTex && !autoupdate && !generateOnce && boundGraphCompiled;
    st.nodeMessagesStamp = graphLogger.logsStamp;
    st.compilePending = compilePending();
    String texgenError, texgenWarning;
    st.hasErrors = graphLogger.findShown(/*error=*/true, texgenError, st.lastErrorNode);
    const bool texgenWarned = graphLogger.findShown(/*error=*/false, texgenWarning, st.lastWarningNode);
    // The compile's own text outranks the logger's -- stage 2 never saw the part it is about --
    // and graphCompileFailed decides which channel it lands on. Empty on a failure leaves the panel
    // its generic line.
    st.lastError = graphCompileFailed ? graphCompileMessage.str() : texgenError.str();
    if (graphCompileFailed)
    {
      st.lastErrorNode.clear();
    }
    const bool compileWarned = !graphCompileFailed && !graphCompileMessage.empty();
    st.graphCompileWarned = compileWarned;
    st.hasWarnings = compileWarned || texgenWarned;
    st.lastWarning = compileWarned ? graphCompileMessage.str() : texgenWarning.str();
    if (compileWarned)
    {
      st.lastWarningNode.clear();
    }
    graphLogger.copyNodeMessages(st.nodeMessages);
    return st;
  }

  uint32_t getNodeMessagesStamp() const override
  {
    WinAutoLock guard(stateLock);
    return graphLogger.logsStamp;
  }

  SelectedTextureState getSelectedTextureState() const override
  {
    WinAutoLock guard(stateLock);
    return selectedTexState;
  }

  void saveTextures(const char *mask) override
  {
    bool heldByAutoupdate = false;
    {
      WinAutoLock guard(stateLock);
      heldByAutoupdate = !autoupdate && !generateOnce && !forceRebuild && (shouldGenerateTex || compilePending());
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

    const String rDirPath =
      make_eff_app_relative_path((graphData && !graphData->renderDir.empty()) ? graphData->renderDir.c_str() : "render");
    const String eDirPath =
      make_eff_app_relative_path((graphData && !graphData->entityDir.empty()) ? graphData->entityDir.c_str() : "entity");
    const char *rDir = rDirPath.str();
    const char *eDir = eDirPath.str();

    dd_mkdir(rDir);
    dd_mkdir(eDir);

    DAEDITOR3.conNote("GraphTexGenService: render directory: '%s'", rDir);
    DAEDITOR3.conNote("GraphTexGenService: entity directory: '%s'", eDir);
    if (heldByAutoupdate)
    {
      DAEDITOR3.conNote("GraphTexGenService: the save waits for the next pass, as auto-update is off");
    }

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
    {
      // Drop the pass and any pipeline state tied to the previous graph data. The worker may be inside
      // a generation batch, so take its locks in its order: GpuAutoLock outer, stateLock inner.
      d3d::GpuAutoLock gpuLock;
      WinAutoLock guard(stateLock);
      // A record that the cut pass made keeps the logs of its inputs, which dropAllMessages below drops.
      if (graphData && texGenStep > 0)
      {
        graphLogger.snapshotRecordLogs(graphData->mainGraphBlk);
      }
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      texGenStep = -1;
      texGenTotalNodes = 0;
      declaredOutputsCount = 0;
      graphCompileFailed = false;
      graphCompileMessage.clear();
      generationCompleted = false;
      graphLogger.dropAllMessages(); // node ids restart per graph
      pendingPreviewReg.clear();
      selectedTexState = SelectedTextureState{};
      if (gd != graphData && saveTexturesToFiles)
      {
        saveTexturesToFiles = false;
        DAEDITOR3.conError("GraphTexGenService: save dropped, the graph changed before it ran");
      }
      graphData = gd;
      texGenFullResetGen = get_d3d_full_reset_counter();
      boundGraphCompiled = false;
      // A waiting pass belonged to the old graph; the new one asks again when it compiles, and waits for auto-update.
      shouldGenerateTex = false;
      generateOnce = false;
      forceRebuild = false;
      if (!gd)
      {
        if (selectedTexPreview)
        {
          pendingTextureDestroy.push_back(eastl::move(selectedTexPreview));
        }
        // Reset both gens so the next setGraphData(non-null) followed by
        // markGraphDirty() starts cleanly at 0/0 -> 1/0 (dirty).
        graphDirtyGen = 0;
        graphCompiledGen = 0;
        return;
      }
    }
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
      generateOnce = true;
      // Only a compile runs prepareDshlShaders, which reads the default shader files again.
      ++graphDirtyGen;
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

  void setGraphCompileOutcome(bool failed, const char *msg) override
  {
    WinAutoLock guard(stateLock);
    graphCompileFailed = failed;
    graphCompileMessage = msg ? msg : "";
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
    // Mutates state shared with the worker (pendingPreviewReg, selectedTexState, shouldGenerateTex).
    // The worker moves the final into mainGraphBlk at the start of the next pass.
    {
      WinAutoLock guard(stateLock);
      if (!graphData)
      {
        return;
      }

      eastl::string newName = (tex_name && tex_name[0]) ? tex_name : "";
      if (newName == pendingPreviewReg)
      {
        return;
      }

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

  // Worker only, under stateLock: the worker is the only writer of graphData->mainGraphBlk. The texgen
  // library's optimizer reads final:t=... from the BLK, so a preview register that is not a final is
  // freed after generation and the preview lookup fails.
  void applyPreviewFinalToBlk()
  {
    DataBlock &blk = graphData->mainGraphBlk;
    if (!previewFinalInBlk.empty())
    {
      for (int i = blk.paramCount() - 1; i >= 0; --i)
      {
        if (blk.getParamType(i) == DataBlock::TYPE_STRING && strcmp(blk.getParamName(i), "final") == 0 &&
            previewFinalInBlk == blk.getStr(i))
        {
          blk.removeParam(i);
          break;
        }
      }
      previewFinalInBlk.clear();
    }
    if (pendingPreviewReg.empty())
    {
      return;
    }
    if (!blk_has_output_reg(blk, pendingPreviewReg.c_str()))
    {
      debug("TexGenService: preview final '%s' is not an output reg of any node; not promoting", pendingPreviewReg.c_str());
      return;
    }
    blk.addStr("final", pendingPreviewReg.c_str());
    previewFinalInBlk = pendingPreviewReg;
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
      int regNo = getUsedRegNo(regName.c_str());
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
    int regNo = getUsedRegNo(regName.c_str());
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
    // No timeout: a batch that runs after the return re-creates the texgen state that the caller closes next.
    worker->terminate(true);
    worker.reset();
    {
      d3d::GpuAutoLock gpuLock;
      batchFence.reset();
      releaseBatchTimestamps();
    }
    os_event_destroy(&wakeEvent);
  }

  // Callers must hold stateLock.
  bool passRequested() const { return shouldGenerateTex && (autoupdate || generateOnce) && boundGraphCompiled; }
  bool compilePending() const { return graphDirtyGen != graphCompiledGen && graphCompiler && graphData; }
  bool passStartPendingUnlocked() const { return forceRebuild || passRequested(); }
  bool hasGenerationWorkUnlocked() const { return passStartPendingUnlocked() || texGenStep > 0; }
  // Finals stay empty until the reset rebuild. A waiting pass, also one that auto-update holds back, holds the save.
  bool savePendingUnlocked() const
  {
    return saveTexturesToFiles && !shouldGenerateTex && !compilePending() && !hasGenerationWorkUnlocked() &&
           (!graphData || get_d3d_full_reset_counter() == texGenFullResetGen);
  }
  bool hasWorkUnlocked() const { return compilePending() || hasGenerationWorkUnlocked() || savePendingUnlocked(); }

  void tickGeneration()
  {
    if (forceRebuild)
    {
      regcache::clear();
      graphLogger.dropRecordLogs();
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      texGenStep = -1;
      forceRebuild = false;
      shouldGenerateTex = true;
      // A queued save waits for this pass, also with autoupdate off.
      generateOnce |= saveTexturesToFiles;
    }

    if (passRequested())
    {
      shouldGenerateTex = false;
      generateOnce = false;
      startGenerateTex();
    }
    else if (texGenStep > 0)
    {
      doGenerateTexStep();
    }
  }

  void workerLoop()
  {
    const DataBlock *cfg = dgs_get_settings()->getBlockByNameEx("texgen");
    const int submitUsec = clamp(cfg->getInt("workerSubmitUsec", 3000), 500, 20000);
    const int gpuTargetUsec = clamp(cfg->getInt("workerBatchGpuTargetUsec", 8000), 1000, 50000);
    const int maxBatchCmds = clamp(cfg->getInt("workerBatchMaxCmds", 256), 1, 1024);
    const int pollMsec = clamp(cfg->getInt("workerBetweenBatchesMsec", 1), 1, 50);
    const bool fencePollUnlocked = d3d::get_driver_code().is(d3d::dx11);
    {
      WinAutoLock stateGuard(stateLock);
      texGenReg.setTexturePoolLimit(static_cast<uint64_t>(clamp(cfg->getInt("workerTexturePoolMb", 1024), 0, 16384)) << 20);
    }
    batchGpuStamps.resize(maxBatchCmds + 1, nullptr);

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
        // A force rebuild drops the pass anyway, so its compile does not wait for it.
        if (compilePending() && (texGenStep <= 0 || forceRebuild))
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
      // INSIDE compile() under its own mutex (see IGraphCompiler doc).
      // A fresh mainGraphBlk drops the injected preview `final:t`; startGenerateTex adds it again.
      if (compilerSnap)
      {
        const bool ok = compilerSnap->compile();
        // Snapshot the freshly-compiled BLKs under the plugin's graphMutex while compilerSnap is still
        // guaranteed alive (compileInProgress is cleared only below). The slow dsc2 pass then assembles
        // from this private copy, never reading graphData -- which a main-thread graph load can wipe
        // under graphMutex during the multi-second compile.
        DataBlock dshlMainCopy, dshlShaderListCopy;
        CompiledNameLabels labelsCopy;
        if (ok)
        {
          compilerSnap->copyCompiledGraph(dshlMainCopy, dshlShaderListCopy, labelsCopy);
        }
        const GraphData *compiledFor = nullptr;
        {
          WinAutoLock guard(stateLock);
          // graphCompileFailed is not set here: compile() pushes it with its message through
          // setGraphCompileOutcome so the bar can never pair one with the other's verdict.
          if (ok && graphData)
          {
            graphLogger.graphLabels = eastl::move(labelsCopy);
            previewFinalInBlk.clear();
            shouldGenerateTex = true;
            boundGraphCompiled = true;
          }
          // Advance graphCompiledGen even on failure: compile_graph_to_blks is
          // deterministic, so a retry without new input would just fail again
          // and spin the worker. The user has to edit the graph (incrementing
          // graphDirtyGen) to retrigger compile.
          graphCompiledGen = compileGenSnap;
          // A bind waits only for compileInProgress, so the dsc2 pass below can outlive this graph.
          compiledFor = graphData;
          compileInProgress = false;
        }
        // dshl phase 1: run the slow dsc2 compile of the freshly-compiled graph HERE -- still OUTSIDE
        // GpuAutoLock (only the generation batch below takes it) -- so it never stalls the main render
        // thread (which blocks in acquireD3dOwnership while the worker holds the GPU lock).
        if (ok && !interlocked_acquire_load(workerTerminating))
        {
          prepareDshlShaders(dshlMainCopy, dshlShaderListCopy, compiledFor);
        }
      }

      int submitted = 0;
      int predictedUsec = 0;
      bool fencePending = false;
      int64_t flushEndTicks = 0;
      {
        d3d::GpuAutoLock gpuLock;
        ensureBatchFence();
        measureLastBatch();
        const int64_t t0 = ref_time_ticks();
        while (submitted < maxBatchCmds && !interlocked_acquire_load(workerTerminating))
        {
          WinAutoLock stateGuard(stateLock);
          if (!hasGenerationWorkUnlocked())
          {
            break;
          }
          TextureGenCommand cmd;
          const bool stepping = !passStartPendingUnlocked() && texGen && texgen_get_next_command(*texGen, cmd);
          const int costUsec = stepping ? predictCmdGpuUsec(cmd.shaderName, cmd.pixels, cmd.costKey, gpuTargetUsec) : gpuTargetUsec;
          // A texture created late in a batch stalls the DX11 driver; at a batch start it does not.
          if (submitted > 0 && (!stepping || cmd.mapsOnCpu || cmd.loadsFiles || cmd.createsTextures ||
                                 predictedUsec + costUsec > gpuTargetUsec || waitsForKeyMeasure(cmd)))
          {
            break;
          }
          if (submitted == 0)
          {
            d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &batchGpuStamps[0]);
          }
          tickGeneration();
          ++submitted;
          d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &batchGpuStamps[submitted]);
          // A failed command closes texGen.
          batchCmds.push_back({stepping && texGen ? cmd.shaderName : "",
            stepping && texGen ? texgen_get_last_command_pixels(*texGen) : 0, cmd.pixels == 0, cmd.costKey});
          predictedUsec += costUsec;
          if (get_time_usec(t0) >= submitUsec)
          {
            break;
          }
        }
        if (submitted > 0)
        {
          if (batchFence && d3d::issue_event_query(batchFence.get()))
          {
            fencePending = !d3d::get_event_query_status(batchFence.get(), true);
          }
          else
          {
            d3d::driver_command(Drv3dCommand::D3D_FLUSH);
          }
          flushEndTicks = ref_time_ticks();
        }
      }
      loadInputFilesAhead();

      bool yielded = false;
      const int64_t pollStart = ref_time_ticks();
      while (fencePending && !interlocked_acquire_load(workerTerminating))
      {
        if (fencePollUnlocked && get_time_usec(pollStart) < MAX_FENCE_POLL_USEC)
        {
          {
            // The DX11 status call needs no GPU ownership once the submit queued the fence signal;
            // a device reset releases the fence under the loading lock taken for writing.
            d3d::LoadingAutoLock resetGuard;
            fencePending = get_d3d_reset_counter() == fenceResetGen && !d3d::get_event_query_status(batchFence.get(), false);
          }
          if (fencePending)
          {
            wait_before_fence_poll(flushEndTicks, predictedUsec, pollMsec);
            yielded = true;
          }
          continue;
        }
        sleep_msec(pollMsec);
        yielded = true;
        d3d::GpuAutoLock gpuLock;
        if (get_d3d_reset_counter() != fenceResetGen)
        {
          break;
        }
        // A Vulkan query completes only at a later present or flush, and a minimized editor does not present.
        if (get_time_usec(pollStart) >= MAX_FENCE_POLL_USEC)
        {
          d3d::driver_command(Drv3dCommand::D3D_FLUSH);
        }
        fencePending = !d3d::get_event_query_status(batchFence.get(), true);
      }

      bool savePending = false;
      {
        WinAutoLock stateGuard(stateLock);
        savePending = savePendingUnlocked();
      }
      if (savePending && !interlocked_acquire_load(workerTerminating))
      {
        d3d::GpuAutoLock gpuLock;
        WinAutoLock stateGuard(stateLock);
        doSaveTextures();
      }

      if (submitted > 0 && !yielded && !interlocked_acquire_load(workerTerminating))
      {
        sleep_msec(pollMsec);
      }
    }
  }

  // Worker thread only, holding neither GpuAutoLock nor stateLock.
  void loadInputFilesAhead()
  {
    eastl::vector<TextureRegManager::FileLoad> loads;
    {
      WinAutoLock stateGuard(stateLock);
      if (!texGen || texGenStep <= 0 || passStartPendingUnlocked())
      {
        return;
      }
      eastl::vector<const char *> regNames;
      texgen_get_upcoming_tex_inputs(*texGen, LOAD_AHEAD_NODE_WINDOW, regNames);
      for (const char *regName : regNames)
      {
        TextureRegManager::FileLoad load;
        if (texGenReg.prepareFileLoad(regName, load))
        {
          loads.push_back(eastl::move(load));
        }
      }
    }
    for (TextureRegManager::FileLoad &load : loads)
    {
      if (interlocked_acquire_load(workerTerminating))
      {
        return;
      }
      TextureRegManager::loadFile(load);
      WinAutoLock stateGuard(stateLock);
      // setGraphData(nullptr) can close the pass while the file loads.
      if (!texGen || texGenStep <= 0 || passStartPendingUnlocked())
      {
        TextureRegManager::releaseLoad(load);
        return;
      }
      texGenReg.addLoadedFile(load);
    }
  }

  void ensureBatchFence()
  {
    const unsigned resetGen = get_d3d_reset_counter();
    if (batchFence && fenceResetGen == resetGen)
    {
      return;
    }
    if (fenceResetGen != resetGen)
    {
      releaseBatchTimestamps();
    }
    batchFence.reset(d3d::create_event_query());
    fenceResetGen = resetGen;
    if (!batchFence && !fenceUnavailableLogged)
    {
      fenceUnavailableLogged = true;
      DAEDITOR3.conWarning("GraphTexGenService: event queries unavailable, texgen batches flush under the GPU lock");
    }
  }

  void releaseBatchTimestamps()
  {
    for (void *&stamp : batchGpuStamps)
    {
      d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &stamp);
    }
  }

  void measureLastBatch()
  {
    if (batchCmds.empty())
    {
      return;
    }
    uint64_t freq = 0;
    uint64_t prev = 0;
    d3d::driver_command(Drv3dCommand::TIMESTAMPFREQ, &freq);
    lastBatchTimed = freq != 0 && d3d::driver_command(Drv3dCommand::TIMESTAMPGET, batchGpuStamps[0], &prev);
    if (lastBatchTimed)
    {
      for (int i = 0; i < batchCmds.size(); ++i)
      {
        uint64_t stamp = 0;
        if (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, batchGpuStamps[i + 1], &stamp) || stamp < prev)
        {
          break;
        }
        const int usec = static_cast<int>(min<uint64_t>((stamp - prev) * 1000000 / freq, MAX_MEASURED_CMD_USEC));
        prev = stamp;
        const BatchCmd &cmd = batchCmds[i];
        if (!cmd.name.empty())
        {
          CmdCost &cost = cmdCosts[cmd.name];
          int cmdUsec = usec;
          if (cmd.pixels > 0)
          {
            const float usecPerPixel = learnKeyCost(cost.costKeys, cmd.costKey, cmd.pixels, cmd.startsNode, usec);
            cmdUsec = usec_for_pixels(usecPerPixel, cmd.pixels);
            cost.usecPerPixel = cost.usecPerPixel > 0.0f ? (cost.usecPerPixel + usecPerPixel) * 0.5f : usecPerPixel;
          }
          cost.maxUsec = max(cmdUsec, cost.maxUsec - cost.maxUsec / 8);
          avgCmdGpuUsec = avgCmdGpuUsec > 0 ? (avgCmdGpuUsec * 7 + cmdUsec) / 8 : cmdUsec;
        }
      }
    }
    batchCmds.clear();
  }

  // Records a command's cost under its cost key; returns the upper median of the key's recent times per pixel,
  // so an even count leans to the slower sample and one inflated sample shifts it by at most one rank.
  float learnKeyCost(eastl::vector<uint64_t> &shader_keys, uint64_t cost_key, int pixels, bool starts_node, int usec)
  {
    auto keyed = keyCosts.find(cost_key);
    if (keyed == keyCosts.end())
    {
      if (keyCosts.size() >= MAX_KEY_COSTS)
      {
        keyCosts.clear();
        for (auto &named : cmdCosts)
        {
          named.second.costKeys.clear();
        }
      }
      keyed = keyCosts.emplace(cost_key, KeyCost()).first;
      shader_keys.push_back(cost_key);
    }
    KeyCost &keyCost = keyed->second;
    if (starts_node)
    {
      keyCost.firstPixels = pixels;
    }
    keyCost.add(static_cast<float>(usec) / static_cast<float>(pixels));
    return keyCost.median;
  }

  int predictCmdGpuUsec(const char *name, int pixels, uint64_t cost_key, int target_usec) const
  {
    const auto keyed = keyCosts.find(cost_key);
    if (keyed != keyCosts.end())
    {
      const int keyPixels = pixels > 0 ? pixels : keyed->second.firstPixels;
      if (keyPixels > 0)
      {
        return usec_for_pixels(keyed->second.median, keyPixels);
      }
    }
    const auto known = cmdCosts.find_as(name);
    if (known == cmdCosts.end())
    {
      // With no GPU timestamps, the submit time alone bounds the batch.
      return lastBatchTimed ? max(avgCmdGpuUsec, target_usec / 4) : 0;
    }
    if (pixels > 0 && known->second.usecPerPixel > 0.0f)
    {
      return usec_for_pixels(known->second.usecPerPixel, pixels);
    }
    return known->second.maxUsec;
  }

  // A node with a cost key not measured yet is predicted at its shader's cost, which can be far below its own when the shader's
  // nodes differ widely: while batches give GPU timestamps, only its first command then joins the batch, and the batch measures it.
  bool waitsForKeyMeasure(const TextureGenCommand &cmd) const
  {
    if (!lastBatchTimed || keyCosts.find(cmd.costKey) != keyCosts.end() ||
        eastl::none_of(batchCmds.begin(), batchCmds.end(), [&cmd](const BatchCmd &c) { return c.costKey == cmd.costKey; }))
    {
      return false;
    }
    const auto known = cmdCosts.find_as(cmd.shaderName);
    if (known == cmdCosts.end())
    {
      return true;
    }
    eastl::fixed_vector<float, 32> medians;
    for (uint64_t key : known->second.costKeys)
    {
      const auto keyed = keyCosts.find(key);
      if (keyed != keyCosts.end())
      {
        medians.push_back(keyed->second.median);
      }
    }
    if (static_cast<int>(medians.size()) < MIN_COST_KEYS_FOR_SPREAD)
    {
      return true;
    }
    // The lower middle for an even count, so that two keys can show a spread.
    const auto middle = medians.begin() + (medians.size() - 1) / 2;
    eastl::nth_element(medians.begin(), middle, medians.end());
    const float typical = *middle;
    return typical <= 0.0f || *eastl::max_element(medians.begin(), medians.end()) > MAX_COST_KEY_SPREAD * typical;
  }

  // dshl pipeline, phase 1 (worker COMPILE pass, OUTSIDE GpuAutoLock): assemble the per-graph .dshl and
  // run the slow dsc2 subprocess to build the bindump on disk, stashing the result for phase 2. The dsc2
  // run MUST stay off the GPU lock -- doing it under GpuAutoLock starves the main render thread (it blocks
  // in acquireD3dOwnership). Param-only edits skip the assemble (input hash) and the recompile (body hash).
  // main_graph_blk / shader_list_blk are a private snapshot of the compiled graph (taken by the worker
  // via IGraphCompiler::copyCompiledGraph under the plugin's graphMutex); this runs the slow dsc2 pass
  // without ever touching graphData's content, so a concurrent main-thread graph load can't corrupt it.
  void prepareDshlShaders(const DataBlock &main_graph_blk, const DataBlock &shader_list_blk, const GraphData *compiled_for)
  {
    dshlReady = false;
    {
      WinAutoLock guard(stateLock);
      graphLogger.dropStage(GraphLoggerImpl::LogStage::DshlCompile);
    }
    // dshlNeedLoad is sticky: cleared only after setupDshlShaders actually loads the dump, so a dump
    // compiled by one prepare pass is not lost if another prepare (unchanged hash) runs before the load.
    DataBlock defaultTexgen, defaultShaders;
    dblk::load(defaultTexgen, defaultTexgenPath, dblk::ReadFlag::ROBUST);
    dblk::load(defaultShaders, defaultShadersPath, dblk::ReadFlag::ROBUST);

    TexgenDshlSkeleton sk;
    sk.vprog = defaultTexgen.getStr("vprog", "");
    sk.particlesVprog = defaultTexgen.getStr("particles_vprog", "");
    sk.premain = defaultTexgen.getStr("premain", "");
    sk.postmain = defaultTexgen.getStr("postmain", "");

    // shader_sources = every named shader block from default_shaders.blk + the graph's shaderListBlk.
    dshlSources.reset();
    const DataBlock *srcBlks[] = {&defaultShaders, &shader_list_blk};
    for (const DataBlock *src : srcBlks)
    {
      for (int i = 0; i < src->blockCount(); ++i)
      {
        dshlSources.addNewBlock(src->getBlock(i), src->getBlock(i)->getBlockName());
      }
    }

    String err;
    const uint64_t inputHash = texgen_dshl_input_hash(main_graph_blk, dshlSources, sk);
    if (inputHash != dshlInputHash)
    {
      if (!assemble_texgen_dshl(main_graph_blk, dshlSources, sk, dshlResult, err))
      {
        DAEDITOR3.conError("GraphTexGenService(dshl): assemble failed: %s", err.str());
        return;
      }
      if (!err.empty())
      {
        DAEDITOR3.conWarning("GraphTexGenService(dshl): %s", err.str());
      }
      dshlInputHash = inputHash;
    }

    if (dshlBindump == INVALID_BINDUMP_HANDLE || dshlResult.bodyHash != dshlBodyHash)
    {
      if (!TexgenDshlCompiler::initPaths(err) ||
          !TexgenDshlCompiler::compileToDump(dshlResult.dshlText.c_str(), "texgen_graph", dshlDumpBase, err))
      {
        WinAutoLock guard(stateLock);
        // A graph bound since the compile is not the one that failed, so only the console keeps the error.
        if (graphData == compiled_for)
        {
          graphLogger.report(GraphLoggerImpl::LogStage::DshlCompile, LOGLEVEL_ERR, err);
        }
        else
        {
          DAEDITOR3.conError("GraphTexGenService(dshl): dsc2 compile of the previous graph failed: %s", err.str());
        }
        return;
      }
      dshlBodyHash = dshlResult.bodyHash;
      dshlNeedLoad = true;
      DAEDITOR3.conNote("GraphTexGenService(dshl): compiled %d shader variant(s)", dshlResult.variantCount);
    }
    dshlReady = true;
  }

  // dshl pipeline, phase 2 (under GpuAutoLock and stateLock at generation init): fast bindump load (only if recompiled)
  // + register each variant as a ShaderElement-backed texgen shader, from the state prepareDshlShaders
  // stashed. The shader:t rewrite is applied later via applyDshlShaderRewrite.
  void setupDshlShaders()
  {
    dshlNodeToVariant.clear();
    if (!dshlReady)
    {
      return;
    }

    if (dshlNeedLoad)
    {
      graphLogger.dropStage(GraphLoggerImpl::LogStage::ShaderSetup);
      String err;
      if (!TexgenDshlCompiler::loadOrReload(dshlDumpBase.c_str(), dshlBindump, err))
      {
        graphLogger.report(GraphLoggerImpl::LogStage::ShaderSetup, LOGLEVEL_ERR, err);
        return;
      }
      dshlNeedLoad = false;
      dshlRegisteredHash = 0;
    }
    if (dshlBindump == INVALID_BINDUMP_HANDLE)
    {
      return;
    }

    eastl::vector<DshlRegistration> registrations;
    auto addRegistration = [&](const eastl::string &reg_name, const eastl::string &shader_name, const eastl::string &src_name) {
      const DataBlock *src = dshlSources.getBlockByName(src_name.c_str());
      registrations.push_back({reg_name.c_str(), shader_name.c_str(), src ? src->getBlockByName("params") : nullptr,
        src ? src->getInt("inputs", 0) : 0, src ? src->getInt("outputs", 1) : 1, src ? src->getInt("defMaxSubSize", 4096) : 4096});
    };
    graphLogger.variantLabels.clear();
    // Per-node variants, registered under the variant name (node shader:t is rewritten to it).
    for (auto &kv : dshlResult.variantToSource)
    {
      addRegistration(kv.first, kv.first, kv.second);
      graphLogger.variantLabels[kv.first] = "'" + kv.second + "'";
    }
    // Builtin/library sub-shaders, registered under their ORIGINAL names, so the C++ orchestrators
    // (distance_field/blur/...) resolve their REQUIRE'd sub-shaders via texgen_get_shader(name).
    for (auto &kv : dshlResult.sourceToVariant)
    {
      addRegistration(kv.first, kv.second, kv.first);
    }

    // Re-registering replaces each shader object, which makes texGen mark every node that uses it dirty.
    const uint64_t registrationHash = dshlRegistrationHash(registrations);
    dshlNodeToVariant = dshlResult.nodeToVariant;
    const bool sameSet = dshlRegisteredHash == registrationHash;
    eastl::hash_set<eastl::string> failed;
    for (const DshlRegistration &r : registrations)
    {
      // Each pass clears graphLogger: retry failed adds.
      if (sameSet && dshlFailedRegs.find_as(r.regName) == dshlFailedRegs.end())
      {
        continue;
      }
      if (!add_dshl_shader_texgen(texGen, r.regName, r.shaderName, dshlBindump, r.params, r.inputs, r.outputs, r.defMaxSubSize))
      {
        failed.insert(r.regName);
      }
    }
    dshlFailedRegs = eastl::move(failed);
    dshlRegisteredHash = registrationHash;
  }

  // The add_dshl_shader_texgen arguments of one variant.
  struct DshlRegistration
  {
    const char *regName;
    const char *shaderName;
    const DataBlock *params;
    int inputs, outputs, defMaxSubSize;
  };

  static uint64_t dshlRegistrationHash(const eastl::vector<DshlRegistration> &registrations)
  {
    // Sum the entries: they come from hash maps, which give no stable order.
    uint64_t sum = 0;
    for (const DshlRegistration &r : registrations)
    {
      uint64_t h = mem_hash_fnv1<64>(r.regName, strlen(r.regName) + 1);
      h = mem_hash_fnv1<64>(r.shaderName, strlen(r.shaderName) + 1, h);
      const int counts[3] = {r.inputs, r.outputs, r.defMaxSubSize};
      h = mem_hash_fnv1<64>((const char *)counts, sizeof(counts), h);
      const uint64_t paramsHash = r.params ? texgen_hash_block_content(*r.params) : 0;
      h = mem_hash_fnv1<64>((const char *)&paramsHash, sizeof(paramsHash), h);
      sum += h;
    }
    return sum;
  }

  // Rewrite each main-graph node's shader:t to its dshl variant name, so the scheduler resolves the
  // ShaderElement-backed variant.
  void applyDshlShaderRewrite()
  {
    if (!graphData || dshlNodeToVariant.empty())
    {
      return;
    }
    DataBlock &mainBlk = graphData->mainGraphBlk;
    for (int i = 0; i < mainBlk.blockCount(); ++i)
    {
      DataBlock *n = mainBlk.getBlock(i);
      auto it = dshlNodeToVariant.find_as(n->getBlockName());
      if (it != dshlNodeToVariant.end())
      {
        n->setStr("shader", it->second.c_str());
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
    graphLogger.startPass();

    // Still need the shared texgen resources (textureParamsCB, textureNodeParamsCB)
    // that process() binds -- normally created inside load_pixel_shader_texgen. dshl only skips the vprog
    // HLSL compile (the VS now lives in each bindump variant), not the resources.
    init_pixel_shader_texgen();
    // Register the variants prepareDshlShaders() already compiled. The shader:t rewrite is deferred to
    // startGenerateTex.
    setupDshlShaders();

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
    dshlRegisteredHash = 0;
  }

  void startGenerateTex()
  {
    if (!graphData)
    {
      return;
    }

    if (texGenStep > 0)
    {
      graphLogger.snapshotRecordLogs(graphData->mainGraphBlk);
      // Before removeOldFinalStrings below, which can free a texture the stopped node still uses.
      texgen_stop_process_commands(*texGen);
      texGenStep = 0;
    }

    texGenReg.releaseLoadedFiles();
    texGenReg.setEntitiesSaver(&entitiesSaver);
    // Node input paths are stored app-relative (no per-graph texture root), so the registry resolves
    // them against the application dir directly. Strip any trailing slash: getRelativeTexName joins
    // "<root>/<name>" by naive concat, so a trailing slash would yield "<appDir>//<name>" -- which
    // passes df_stat but makes the raw-texture loader fail ("can not load texture ... although file exist").
    String appDir(DAGORED2->getWorkspace().getAppDir());
    ::simplify_fname(appDir);
    ::remove_trailing_string(appDir, "/");
    texGenReg.setTextureRootDir(appDir.str());
    debug("TexGenService: texture root (app dir) '%s'", appDir.str());
    texGenStep = 0;

    initTexGenInternal();
    debug("TexGenService: graph changed");

    regcache::unpin_all();

    applyPreviewFinalToBlk();
    DataBlock &mainBlk = graphData->mainGraphBlk;
    texgen_pin_cache_node_records(mainBlk);
    regcache::update();

    applyDshlShaderRewrite();

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
    const int defaultBudgetMb = clamp<int>(int(d3d::get_dedicated_gpu_memory_size_kb() >> 12), 256, 4096);
    const int budgetMb = dgs_get_settings()->getBlockByNameEx("texgen")->getInt("retainBudgetMb", defaultBudgetMb);
    texGenReg.setRetainBudget(budgetMb > 0 ? uint64_t(budgetMb) << 20 : 0);
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
    // by applyPreviewFinalToBlk), matching how it is excluded from the texture save path.
    declaredOutputsCount = static_cast<int>(newFinalStrings.size());
    if (!previewFinalInBlk.empty() && newFinalStrings.find(previewFinalInBlk) != newFinalStrings.end())
    {
      --declaredOutputsCount;
    }
    texGenReg.clearStat();
    doGenerateTexStep();
  }

  void doGenerateTexStep()
  {
    int generated;
    if (texGenStep <= 0)
    {
      generated = texgen_start_process_commands(*texGen, graphData->mainGraphBlk, texGenReg);
      graphLogger.checkNodeLogs(*texGen, graphData->mainGraphBlk, graphData->mainGraphStamp);
      // Remember the pass size for getPipelineStatus' progress readout (nodesDone/Total).
      texGenTotalNodes = generated > 0 ? generated : 0;
    }
    else
    {
      graphLogger.steppingGen = texGen;
      generated = texgen_process_commands(*texGen, 1);
      graphLogger.steppingGen = nullptr;
    }

    if (generated < 0)
    {
      DAEDITOR3.conNote("GraphTexGenService: texgen failed");
      graphLogger.snapshotRecordLogs(graphData->mainGraphBlk);
      texGenReg.close();
      finalizeTexGen();
      closeTexGen();
      texGenStep = -1;
    }
    else if (generated == 0)
    {
      DAEDITOR3.conNote("GraphTexGenService: success, produced %d textures, maxMemUsed = %.1fMB, current = %.1fMB, retained = %.1fMB",
        texGenReg.getRegsAlive() - texGenReg.getRegsRetained(), texGenReg.getMaxMemSize() / float(1 << 20),
        texGenReg.getCurrentMemSize() / float(1 << 20), texGenReg.getRetainedMemSize() / float(1 << 20));
      graphLogger.snapshotRecordLogs(graphData->mainGraphBlk);
      finalizeTexGen();
      texGenStep = 0;
      generationCompleted = true;
    }
    else
    {
      texGenStep = 1;
    }

    if (texGenStep >= 0)
    {
      for (const auto &str : finalStrings)
      {
        int varId = get_shader_variable_id(str.c_str(), true);
        if (varId >= 0)
        {
          int resultTexReg = getUsedRegNo(texgen_get_final_reg_name(texGen, str.c_str()));
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
    regcache::unpin_all();
    texGenReg.releaseLoadedFiles();
    removeOldFinalStrings();
    texGenReg.releaseTexturePool();
    for (auto it = finalStrings.begin(); it != finalStrings.end();)
    {
      int resultTexReg = getUsedRegNo(texgen_get_final_reg_name(texGen, it->c_str()));
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

  // A retained register is a cache entry with no use count, and the texture getters refuse it.
  int getUsedRegNo(const char *name) const
  {
    const int regNo = texGenReg.getRegNo(name);
    return regNo >= 0 && texGenReg.getRegUsageLeft(regNo) > 0 ? regNo : -1;
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
    if (!savePendingUnlocked())
    {
      return;
    }
    if (texGenStep < 0 || !texGen)
    {
      saveTexturesToFiles = false;
      DAEDITOR3.conError("GraphTexGenService: nothing to save, no finished generation of the current graph");
      return;
    }

    const String rDirPath =
      make_eff_app_relative_path((graphData && !graphData->renderDir.empty()) ? graphData->renderDir.c_str() : "render");
    const String eDirPath =
      make_eff_app_relative_path((graphData && !graphData->entityDir.empty()) ? graphData->entityDir.c_str() : "entity");
    const char *rDir = rDirPath.str();
    const char *eDir = eDirPath.str();

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
  // The preview final that applyPreviewFinalToBlk added to mainGraphBlk; empty when none.
  eastl::string previewFinalInBlk;
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

  // dshl pipeline: node shaders are dsc2-compiled into this bindump and rendered as ShaderElements.
  // dshlBodyHash gates the (slow) dsc2 recompile so param-only edits don't retrigger it. dshlNodeToVariant
  // maps each main-graph node block to its variant shader name; applied to shader:t at each pass start.
  ShaderBindumpHandle dshlBindump = INVALID_BINDUMP_HANDLE;
  uint32_t dshlBodyHash = 0;
  eastl::hash_map<eastl::string, eastl::string> dshlNodeToVariant;
  // Worker-thread-only dshl state. prepareDshlShaders() (worker compile pass, OUTSIDE GpuAutoLock) runs
  // the slow dsc2 subprocess and stashes the result here; setupDshlShaders() (under GpuAutoLock at init)
  // then does the fast bindump load + shader registration. Both run on the texgen worker thread; the one
  // main thread writer is shutdownTexGen, after stopWorker() joined the worker, so these need no extra locking.
  TexgenDshlResult dshlResult;
  DataBlock dshlSources;
  String dshlDumpBase;
  uint64_t dshlInputHash = 0;
  bool dshlReady = false;
  bool dshlNeedLoad = false;
  // The hash of the dshl variant set that the last pass registered or tried, 0 when none.
  // The worker writes it in setupDshlShaders and clears it through closeTexGen.
  // The main thread writers are closeTexGen from setGraphData, under GpuAutoLock + stateLock (the locks the worker holds),
  // and from shutdownTexGen and the destructor, which are safe because stopWorker ran first.
  uint64_t dshlRegisteredHash = 0;
  eastl::hash_set<eastl::string> dshlFailedRegs;
  eastl::hash_set<eastl::string> finalStrings;
  eastl::hash_set<eastl::string> oldFinalStrings;
  eastl::hash_set<eastl::string> skipSavingNames;
  int texGenStep = 0;
  int texGenTotalNodes = 0;
  // Declared graph outputs (final:t entries) captured at the start of the most recent generation
  // pass, minus the preview-final UI artifact. Reported by the status bar's Outputs counter
  // instead of finalStrings.size(), which a generation failure prunes to 0 (see getPipelineStatus).
  int declaredOutputsCount = 0;
  // Build outcome for the status bar (see TexGenPipelineStatus). graphCompileFailed: stage-1 graph
  // compile returned false. graphCompileMessage: why it failed, or what it dropped; every compile
  // sets it. generationCompleted: a stage-2 generation pass reached success. All reset on graph swap;
  // generationCompleted also resets at each pass start.
  bool graphCompileFailed = false;
  String graphCompileMessage;
  bool generationCompleted = false;

  // Flags
  bool shouldGenerateTex = false;
  bool autoupdate = true;
  // One-shot regen request that bypasses the autoupdate gate, which an edit alone waits for; cleared on the
  // next startGenerateTex() pass.
  bool generateOnce = false;
  bool forceRebuild = false;
  unsigned texGenFullResetGen = 0;
  // Plugin-supplied graph compiler. The worker invokes compile() outside stateLock
  // when the graph is dirty. nullptr = no compile path wired (service ignores dirty).
  IGraphCompiler *graphCompiler = nullptr;
  // Generation counter: incremented by markGraphDirty() on every mutation. The
  // worker captures its current value before calling compile() and writes the
  // captured value into `graphCompiledGen` after the call returns. The compare
  // `graphDirtyGen != graphCompiledGen` decides whether a compile is pending.
  uint64_t graphDirtyGen = 0;
  uint64_t graphCompiledGen = 0;
  // A pass waits for a good compile of the graph setGraphData bound last.
  bool boundGraphCompiled = false;
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
  // Worker thread only, under GpuAutoLock (the DX11 fence poll: LoadingAutoLock); stopWorker frees them after the join.
  EventQueryHolder batchFence;
  unsigned fenceResetGen = 0;
  bool fenceUnavailableLogged = false;
  struct BatchCmd
  {
    eastl::string name;
    int pixels = 0;
    bool startsNode = false;
    uint64_t costKey = 0;
  };
  struct CmdCost
  {
    float usecPerPixel = 0.0f;
    int maxUsec = 0;
    eastl::vector<uint64_t> costKeys;
  };
  // Nodes of one shader differ in GPU cost by their params, so a cost key keeps its own recent samples.
  struct KeyCost
  {
    static constexpr int SAMPLES = 5;
    float usecPerPixel[SAMPLES] = {};
    float median = 0.0f;
    int count = 0;
    int next = 0;
    int firstPixels = 0;
    void add(float usec_per_pixel)
    {
      usecPerPixel[next] = usec_per_pixel;
      next = (next + 1) % SAMPLES;
      count = min(count + 1, SAMPLES);
      float sorted[SAMPLES];
      eastl::copy(usecPerPixel, usecPerPixel + count, sorted);
      eastl::sort(sorted, sorted + count);
      median = sorted[count / 2];
    }
  };
  eastl::vector<void *> batchGpuStamps;
  eastl::vector<BatchCmd> batchCmds;
  bool lastBatchTimed = true;
  eastl::hash_map<eastl::string, CmdCost> cmdCosts;
  eastl::hash_map<uint64_t, KeyCost> keyCosts;
  int avgCmdGpuUsec = 0;
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
