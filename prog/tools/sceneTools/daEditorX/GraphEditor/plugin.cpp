// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_editorEvents.h>
#include <de3_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_wndGlobal.h>
#include <oldEditor/de_cm.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_set.h>
#include <EASTL/optional.h>

#include <imgui/imgui.h>

#include <graphEditor/graph_data.h>
#include <graphEditor/graph_subgraph_expand.h>
#include <graphEditor/graph_subgraph_extract.h>

#include <math.h>

#include "node_library_panel.h"
#include "command_definitions.h"
#include "graph_command_text.h"
#include "graph_compile.h"
#include "graph_create_subgraph_dialog.h"
#include "graph_dead_paths.h"
#include "graph_document.h"
#include "graph_panel.h"
#include "graph_shortcuts_panel.h"
#include "graph_tab.h"
#include "histogram_panel.h"
#include "landscape_preview_panel.h"
#include "landscape_preview_scene.h"
#include "plugin.h"
#include "properties_panel.h"
#include "texture_preview_panel.h"

#include "pluginService/graph_tex_gen_service.h"

namespace
{
enum
{
  CM_TOOL = CM_PLUGIN_BASE,
  CM_SHOW_GRAPH,
  CM_SHOW_TEXTURE_PREVIEW,
  CM_SHOW_HISTOGRAM,
  CM_SHOW_LANDSCAPE_PREVIEW,
  CM_SHOW_NODE_LIBRARY,
  CM_SHOW_PROPERTIES,
  CM_SHOW_SHORTCUTS,
  CM_NEW_GRAPH,
  CM_LOAD_GRAPH_BLK,
  CM_SAVE_GRAPH_BLK,
  CM_SAVE_AS_SUBGRAPH_BLK,
  CM_SAVE_TEXTURES,
  CM_SAVE_TEXTURES_BY_SUBSTRING,
  CM_RELOAD_TEXTURES,
  CM_FORCE_REBUILD,
  CM_TOGGLE_GRAPH_PROPERTIES,
};

constexpr unsigned RECENT_NODE_LIMIT = 20;

constexpr const char *GENERAL_SUBGRAPH_ROOT_LABEL = "General";
constexpr float SUBGRAPH_GRID = 16.0f; // ne snaps node drags to this grid (AlignPointToGrid)
// The design puts a toast's top-left this far right of and below the cursor tip, clear of the arrow.
constexpr int TOAST_CURSOR_DX = 12;
constexpr int TOAST_CURSOR_DY = 16;

// Canvas-space cascade for spawns that have no cursor position of their own.
constexpr unsigned SPAWN_SLOT_COUNT = 8;
constexpr float SPAWN_SLOT_STEP = 32.0f;

enum
{
  GRAPH_PANEL_WTYPE = 160,
  TEXTURE_PREVIEW_PANEL_WTYPE = 161,
  HISTOGRAM_PANEL_WTYPE = 162,
  LANDSCAPE_PREVIEW_PANEL_WTYPE = 163,
  NODE_LIBRARY_PANEL_WTYPE = 164,
  PROPERTIES_PANEL_WTYPE = 165,
  SHORTCUTS_PANEL_WTYPE = 166,
};

enum
{
  PID_SAVE_TEXTURES_SUBSTRING = 11100,
  PID_PIN_COMMENT,
  PID_OPEN_GRAPH_MODE,
};

enum
{
  OPEN_GRAPH_EXISTING = 0,
  OPEN_GRAPH_NEW = 1,
};

static String last_save_textures_substring;

bool prompt_texture_substring(String &out_substring)
{
  PropPanel::DialogWindow *dlg = DAGORED2->createAutoSizedDialog("Save textures by substring");

  PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
  panel.createEditBox(PID_SAVE_TEXTURES_SUBSTRING, "Substring:", out_substring);

  const int ret = dlg->showDialog();
  const bool accepted = ret == PropPanel::DIALOG_ID_OK;
  if (accepted)
  {
    out_substring = panel.getText(PID_SAVE_TEXTURES_SUBSTRING);
  }

  del_it(dlg);
  return accepted;
}

// An edit box keeps Enter for itself (it reports the key as an onKeyDown event instead), so a dialog
// whose focus sits in one never sees the Enter that would close it. Relay it back to the Ok button.
class EnterAcceptsDialog final : public PropPanel::ControlEventHandler
{
public:
  explicit EnterAcceptsDialog(PropPanel::DialogWindow &dlg) : dialog(dlg) {}

  long onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key) override
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
    if (v_key == wingw::V_ENTER)
    {
      dialog.clickDialogButton(PropPanel::DIALOG_ID_OK);
    }
    return 0;
  }

private:
  PropPanel::DialogWindow &dialog;
};

// Adapter installed into the texgen service. compile() runs on the worker
// thread and holds the document's graph mutex for the whole compile-and-commit: it
// READS the source-of-truth graph (gd.nodes / edges / properties) and writes only
// the compiled output (gd.mainGraphBlk / shaderListBlk), so a main-thread writer
// cannot mutate the source mid-compile. Main stays the only writer of nodes and
// edges; display-side readers rely on that. The service does not commit the BLKs
// itself -- doing so under stateLock while main loaders write under graphMutex
// would race on the same memory under two different locks.
class GraphCompilerImpl final : public IGraphCompiler
{
public:
  GraphCompilerImpl(GraphEditorPlg &p, GraphDocument &d) : plugin(p), doc(d) {}

  bool compile() override
  {
    bool ok = false;
    eastl::string compileMsg;
    doc.mutateWithoutInvalidating([&](GraphData &gd) {
      // Compile into local BLKs first so a failure (e.g. iteration-limit hit)
      // leaves gd.mainGraphBlk / shaderListBlk untouched -- the service relies
      // on this to keep the previous compile's output usable on failure.
      DataBlock newMain;
      DataBlock newShader;
      eastl::vector<eastl::vector<eastl::string>> pendingNamesByIdx;
      GraphResolutions resolutions;

      // Inline-expand every subgraph instance into a local flat copy. The user-facing `gd`
      // keeps the instance nodes for display / editing; compile sees the spliced tree only.
      // Skip the deep copy when there are no instances to avoid the (cheap-ish but non-zero)
      // GraphData copy on the hot recompile path -- shader-only graphs compile every property
      // edit and shouldn't pay for a feature they don't use.
      bool hasSubgraphs = false;
      for (const GraphData::Node &n : gd.nodes)
      {
        if (is_subgraph_instance(n))
        {
          hasSubgraphs = true;
          break;
        }
      }
      const GraphData *compileGd = &gd;
      GraphData flat;
      eastl::hash_map<uint64_t, uint64_t> instanceOutputSource;
      GraphDocument::CompileResults failure;
      failure.failed = true;
      bool expanded = true;
      if (hasSubgraphs)
      {
        flat = gd;
        SubgraphExpandError expandError;
        expanded = expand_subgraphs(flat, plugin.getBaseNodesBlk(), plugin.getSubgraphsDir(), &instanceOutputSource, &expandError);
        if (!expanded)
        {
          compileMsg = eastl::string("Subgraph recompilation failed: ") + expandError.message;
          failure.failedSubgraphs = eastl::move(expandError.chain);
        }
        compileGd = &flat;
      }

      DeadPaths flatDead;
      // constraint: no compile after a failed splice, or it succeeds without the instance outputs and replaces the last good textures
      ok = expanded && compile_graph_to_blks(*compileGd, plugin.getBaseNodesBlk(), plugin.getShaderIncludesDir(), newMain, newShader,
                         pendingNamesByIdx, &compileMsg, hasSubgraphs ? &resolutions : nullptr, hasSubgraphs ? &flatDead : nullptr);
      if (ok)
      {
        gd.mainGraphBlk.setFrom(&newMain);
        gd.shaderListBlk.setFrom(&newShader);

        // Keyed by id: spliced subgraph children have ids that do not exist in gd.nodes.
        GraphDocument::CompileResults results;
        results.pinCustomTextureNames.reserve(pendingNamesByIdx.size());
        for (int i = 0; i < static_cast<int>(pendingNamesByIdx.size()) && i < static_cast<int>(compileGd->nodes.size()); ++i)
        {
          if (pendingNamesByIdx[i].empty())
          {
            continue;
          }
          results.pinCustomTextureNames.emplace_back(compileGd->nodes[i].id, eastl::move(pendingNamesByIdx[i]));
        }

        // Only a subgraph expansion gives compile a chain the canvas cannot see.
        if (hasSubgraphs)
        {
          eastl::hash_map<int, int> flatIdToIdx;
          build_node_index(*compileGd, flatIdToIdx);
          eastl::hash_map<int, int> gdIdToIdx;
          build_node_index(gd, gdIdToIdx);

          // Instance outputs the parent reads. An output nobody consumes shows nowhere, so it gets
          // no vote below.
          // constraint: live edges only, because that is what the canvas propagates along. Counting
          // a muted one lets it veto a seed the live wire beside it needed.
          DeadPaths gdDead;
          compute_dead_paths(gd, gdIdToIdx, gdDead);
          eastl::hash_set<uint64_t> consumedOutPins;
          auto noteConsumed = [&](int node_index, int pin_idx) {
            const GraphData::Node &n = gd.nodes[node_index];
            if (is_subgraph_instance(n) && n.pins[pin_idx].role == PinRole::Out)
            {
              consumedOutPins.insert(make_pin_id(n.id, pin_idx));
            }
          };
          for_each_live_edge(gd, gdDead, gdIdToIdx, [&](const LiveEdge &e) {
            noteConsumed(e.nodeA, e.pinA);
            noteConsumed(e.nodeB, e.pinB);
          });

          // A consumed output with no producer is a wire the compile dropped. The canvas still draws
          // it, so a seed here would push a size down a wire the emitted BLK does not have.
          eastl::hash_set<int> vetoed;
          for (const uint64_t pin : consumedOutPins)
          {
            if (instanceOutputSource.find(pin) == instanceOutputSource.end())
            {
              vetoed.insert(node_id_from_pin_id(pin));
            }
          }

          // Instances only; the canvas propagates the rest from them.
          // constraint: the seed is one result for the whole node, so it may go out only when every
          // output the parent reads agrees. A conflicting instance keeps the canvas's own guess,
          // which is approximate but never claims one output's size for another's.
          struct InstanceSeed
          {
            const NodeResolution *consumed = nullptr;
            const NodeResolution *lowest = nullptr; // an output nobody reads still gives a footer
            int lowestPin = 0;
            bool conflict = false;
          };
          eastl::hash_map<int, InstanceSeed> byInstance;
          for (const auto &entry : instanceOutputSource)
          {
            uint64_t producer = entry.second;
            for (size_t hop = 0; hop < instanceOutputSource.size(); ++hop)
            {
              const auto next = instanceOutputSource.find(producer);
              if (next == instanceOutputSource.end())
              {
                break;
              }
              producer = next->second;
            }
            const auto it = flatIdToIdx.find(node_id_from_pin_id(producer));
            if (it == flatIdToIdx.end() || flatDead.isDeadNode(it->second))
            {
              // The producer either spliced out with nothing behind it or died on a mute -- the
              // expander reads only the boundary edge's own mute, so one further up the body leaves
              // a producer that still resolves. The compile dropped this wire either way, so it
              // takes the same veto as a pin with no key at all.
              if (consumedOutPins.count(entry.first) != 0)
              {
                vetoed.insert(node_id_from_pin_id(entry.first));
              }
              continue;
            }
            // A producer in the flat graph keeps its wire, so this stays a skip. It carries no
            // texture (a scalar boundary), and a scalar pin never feeds a main input.
            const NodeResolution *const found = resolutions.byIndex(it->second);
            if (!found)
            {
              continue;
            }

            const NodeResolution &res = *found;
            InstanceSeed &seed = byInstance[node_id_from_pin_id(entry.first)];
            const int pin = pin_index_from_pin_id(entry.first);
            if (!seed.lowest || pin < seed.lowestPin)
            {
              seed.lowest = &res;
              seed.lowestPin = pin;
            }
            if (consumedOutPins.count(entry.first) == 0)
            {
              continue;
            }
            if (!seed.consumed)
            {
              seed.consumed = &res;
            }
            else if (!same_texture_result(*seed.consumed, res))
            {
              seed.conflict = true;
            }
          }

          results.nodeResolutions.reserve(byInstance.size());
          for (const auto &kv : byInstance)
          {
            const InstanceSeed &seed = kv.second;
            if (!seed.conflict && vetoed.count(kv.first) == 0)
            {
              results.nodeResolutions.emplace_back(kv.first, seed.consumed ? *seed.consumed : *seed.lowest);
            }
          }
        }
        doc.setPendingCompileResults(eastl::move(results));
      }
      else
      {
        doc.setPendingCompileResults(eastl::move(failure));
      }
    });

    // Pushed on every compile, empty included, so a fixed graph clears the status bar. The message
    // carries no "GraphEditor:" prefix -- the status bar is already inside the graph panel.
    if (IGraphTexGenService *svc = plugin.getTexGenService())
    {
      svc->setGraphCompileOutcome(!ok, compileMsg.c_str());
    }
    if (!ok)
    {
      DAEDITOR3.conError("GraphEditor: %s", compileMsg.empty() ? "graph compile failed" : compileMsg.c_str());
    }
    else if (!compileMsg.empty())
    {
      DAEDITOR3.conWarning("GraphEditor: %s", compileMsg.c_str());
    }
    return ok;
  }

  void copyCompiledGraph(DataBlock &main_graph_blk, DataBlock &shader_list_blk) override
  {
    doc.readGraphData([&](const GraphData &gd) {
      main_graph_blk.setFrom(&gd.mainGraphBlk);
      shader_list_blk.setFrom(&gd.shaderListBlk);
    });
  }

private:
  GraphEditorPlg &plugin;
  GraphDocument &doc;
};

// The compile worker reads baseNodesBlk with no lock; clearing the compiler drains it.
class CompilerDrainScope
{
public:
  explicit CompilerDrainScope(GraphEditorPlg &p) : plugin(p)
  {
    if (depth++ > 0)
    {
      return;
    }
    if (IGraphTexGenService *svc = plugin.getTexGenService())
    {
      svc->setGraphCompiler(nullptr);
    }
  }

  ~CompilerDrainScope()
  {
    if (--depth > 0)
    {
      return;
    }
    // An empty strip stays empty: closeAllTabs already cleared the service, and texGenTab() would build a tab.
    if (IGraphTexGenService *svc = plugin.getTexGenService(); svc && plugin.getGraphPanel().hasTabs())
    {
      svc->setGraphCompiler(plugin.getGraphPanel().texGenTab().getCompiler());
      // The tab's graph can hold instances of what changed.
      svc->markGraphDirty();
    }
  }

private:
  // Nested scopes act at the outermost only: an inner rebind lets a compile start that the outer one repeats.
  static inline int depth = 0;
  GraphEditorPlg &plugin;
};

// One *.subgraph.blk into its descriptor, with its problems logged.
bool synthesize_subgraph_file(const char *path, const char *shader_includes_dir, const char *location_label, DataBlock &out_node_blk)
{
  const eastl::string stem = graph_file_stem(path);
  if (stem.empty())
  {
    return false;
  }

  GraphData childGd;
  // No registry: the boundaries need only the file, and at startup the registry is half built.
  if (!load_graph_data_blk(childGd, path, shader_includes_dir, nullptr))
  {
    DAEDITOR3.conError("GraphEditor: failed to parse subgraph %s", path);
    return false;
  }

  eastl::vector<SubgraphDescriptorMessage> messages;
  const bool ok = synthesize_subgraph_descriptor(childGd, stem.c_str(), path, location_label, out_node_blk, messages);
  for (const SubgraphDescriptorMessage &m : messages)
  {
    if (m.isError)
    {
      DAEDITOR3.conError("GraphEditor: subgraph %s: %s", path, m.text.c_str());
    }
    else
    {
      DAEDITOR3.conWarning("GraphEditor: subgraph %s: %s", path, m.text.c_str());
    }
  }
  return ok;
}

// <graph>_subgraph_<n> with the first n whose file is free: a start the user can type over.
String propose_subgraph_name(const char *source_path, const char *root_dir)
{
  const eastl::string stem = graph_file_stem(source_path);
  const String prefix = stem.empty() ? String("subgraph") : String(0, "%s_subgraph", sanitize_subgraph_name(stem.c_str()).str());
  for (int n = 1;; ++n)
  {
    String name(0, "%s_%d", prefix.str(), n);
    if (!dd_file_exist(String(0, "%s%s.subgraph.blk", root_dir, name.str())))
    {
      return name;
    }
  }
}

void show_toast_at_cursor(PropPanel::ToastType type, const char *text)
{
  if (!ImGui::IsMousePosValid())
  {
    DAEDITOR3.showToast(type, text);
    return;
  }
  const ImVec2 mouse = ImGui::GetMousePos();
  const IPoint2 position(static_cast<int>(mouse.x) + hdpi::_pxS(TOAST_CURSOR_DX),
    static_cast<int>(mouse.y) + hdpi::_pxS(TOAST_CURSOR_DY));
  DAEDITOR3.showToast(type, text, &position, Point2(0.0f, 0.0f));
}

void show_saved_toast(const char *path)
{
  show_toast_at_cursor(PropPanel::ToastType::Success, String(0, "%s saved successfully", graph_file_stem(path).c_str()).str());
}

int find_pin_by_name(const GraphData::Node &node, const char *name, bool output)
{
  for (int i = 0; i < static_cast<int>(node.pins.size()); ++i)
  {
    if ((node.pins[i].role == PinRole::Out) == output && node.pins[i].name == name)
    {
      return i;
    }
  }
  return -1;
}
} // namespace

eastl::unique_ptr<IGraphCompiler> GraphEditorPlg::createGraphCompiler(GraphDocument &doc)
{
  return eastl::make_unique<GraphCompilerImpl>(*this, doc);
}

GraphDocument &GraphEditorPlg::getActiveDocument() { return graphPanel->activeDocument(); }

void GraphEditorPlg::beforeActiveTabChanges()
{
  if (propertiesPanel)
  {
    propertiesPanel->commitPendingEdit();
  }
}

void GraphEditorPlg::beforeTabCloses(const GraphDocument &closing)
{
  if (propertiesPanel)
  {
    propertiesPanel->detachFromDocument(closing);
  }
}

GraphEditorPlg::GraphEditorPlg() : graphPanel(eastl::make_unique<GraphPanel>(*this)) {}

GraphEditorPlg::~GraphEditorPlg()
{
  // The service outlives the plugin and holds the graph plus the compiler adapter. shutdownTexGen
  // goes first because setGraphData(nullptr) frees the pipeline without stopping the worker. The
  // undo stack outlives it too, and ~GraphPanel drops the entries that point into the graph.
  if (IEditorService *svc = IDaEditor3Engine::get().findService("texgen"))
  {
    if (IGraphTexGenService *texgen = svc->queryInterface<IGraphTexGenService>())
    {
      texgen->shutdownTexGen();
      texgen->setGraphData(nullptr);
      texgen->setGraphCompiler(nullptr);
    }
  }

  // end() takes this back on a plugin switch, but it can refuse one; the call is a no-op when it did
  // not, and leaving a handler behind outlives the plugin.
  IEditorCoreEngine::get()->getWndManager()->unregisterWindowHandler(this);
}

void GraphEditorPlg::registered() {}

// Per app, not global: shader and subgraph descriptors are project-scoped, so a uid saved by another
// project would never resolve. The editor wraps this in plugins/graphEditor already.
void GraphEditorPlg::loadSettings(const DataBlock & /*global_settings*/, const DataBlock &per_app_settings)
{
  recentNodeUids.clear();

  const DataBlock &nodeLibraryBlk = *per_app_settings.getBlockByNameEx("nodeLibrary");
  const int nameId = nodeLibraryBlk.getNameId("recentNode");
  for (uint32_t i = 0; i < nodeLibraryBlk.paramCount() && recentNodeUids.size() < RECENT_NODE_LIMIT; ++i)
  {
    if (nodeLibraryBlk.getParamNameId(i) != nameId || nodeLibraryBlk.getParamType(i) != DataBlock::TYPE_STRING)
    {
      continue;
    }

    // Hand-edited settings can repeat a uid; noteRecentlyUsedNode never does.
    const char *uid = nodeLibraryBlk.getStr(i);
    if (eastl::find(recentNodeUids.begin(), recentNodeUids.end(), uid) == recentNodeUids.end())
    {
      recentNodeUids.push_back(eastl::string(uid));
    }
  }
}

void GraphEditorPlg::saveSettings(DataBlock & /*global_settings*/, DataBlock &per_app_settings)
{
  if (recentNodeUids.empty())
  {
    return;
  }

  DataBlock &nodeLibraryBlk = *per_app_settings.addBlock("nodeLibrary");
  for (const eastl::string &uid : recentNodeUids)
  {
    nodeLibraryBlk.addStr("recentNode", uid.c_str());
  }
}

void GraphEditorPlg::unregistered() {}

void GraphEditorPlg::beforeMainLoop() {}

bool GraphEditorPlg::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();
  IEditorCommandSystem *commandSystem = DAGORED2->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);

  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_GRAPH, SHOW_GRAPH, "Show graph");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_TEXTURE_PREVIEW, SHOW_TEXTURE_PREVIEW, "Show texture preview");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_HISTOGRAM, SHOW_HISTOGRAM, "Show histogram");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_LANDSCAPE_PREVIEW, SHOW_LANDSCAPE_PREVIEW, "Show landscape preview");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_NODE_LIBRARY, SHOW_NODE_LIBRARY, "Show node library");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_PROPERTIES, SHOW_PROPERTIES, "Show properties");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_TOGGLE_GRAPH_PROPERTIES, TOGGLE_GRAPH_PROPERTIES, "Toggle graph properties");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_SHORTCUTS, SHOW_SHORTCUTS, "Show shortcuts");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_NEW_GRAPH, NEW_GRAPH, "New graph");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_GRAPH_BLK, SAVE_GRAPH_BLK, "Save graph...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_AS_SUBGRAPH_BLK, SAVE_AS_SUBGRAPH_BLK, "Save as subgraph...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_TEXTURES, SAVE_TEXTURES, "Save textures");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_TEXTURES_BY_SUBSTRING, SAVE_TEXTURES_BY_SUBSTRING,
    "Save textures by substring");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_RELOAD_TEXTURES, RELOAD_TEXTURES, "Reload textures");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_FORCE_REBUILD, FORCE_REBUILD, "Force rebuild");

  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);

  toolbar->setEventHandler(this);
  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_GRAPH, SHOW_GRAPH, "Show graph");
  tool->setButtonPictures(CM_SHOW_GRAPH, "show_panel");
  tool->setBool(CM_SHOW_GRAPH, isGraphVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_TEXTURE_PREVIEW, SHOW_TEXTURE_PREVIEW, "Show texture preview");
  tool->setButtonPictures(CM_SHOW_TEXTURE_PREVIEW, "show_panel");
  tool->setBool(CM_SHOW_TEXTURE_PREVIEW, isTexturePreviewVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_HISTOGRAM, SHOW_HISTOGRAM, "Show histogram");
  tool->setButtonPictures(CM_SHOW_HISTOGRAM, "show_panel");
  tool->setBool(CM_SHOW_HISTOGRAM, isHistogramVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_LANDSCAPE_PREVIEW, SHOW_LANDSCAPE_PREVIEW, "Show landscape preview");
  tool->setButtonPictures(CM_SHOW_LANDSCAPE_PREVIEW, "show_panel");
  tool->setBool(CM_SHOW_LANDSCAPE_PREVIEW, isLandscapeVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_NODE_LIBRARY, SHOW_NODE_LIBRARY, "Show node library");
  tool->setButtonPictures(CM_SHOW_NODE_LIBRARY, "node_library");
  tool->setBool(CM_SHOW_NODE_LIBRARY, isNodeLibraryVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_PROPERTIES, SHOW_PROPERTIES, "Show properties");
  tool->setButtonPictures(CM_SHOW_PROPERTIES, "show_panel");
  tool->setBool(CM_SHOW_PROPERTIES, isPropertiesVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_SHORTCUTS, SHOW_SHORTCUTS, "Show shortcuts");
  tool->setButtonPictures(CM_SHOW_SHORTCUTS, "show_panel");
  tool->setBool(CM_SHOW_SHORTCUTS, isShortcutsVisible);

  commandSystem->createToolbarButton(*tool, CM_NEW_GRAPH, NEW_GRAPH, "New graph");
  tool->setButtonPictures(CM_NEW_GRAPH, "clear");

  commandSystem->createToolbarButton(*tool, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph...");
  tool->setButtonPictures(CM_LOAD_GRAPH_BLK, "obj_lib");

  commandSystem->createToolbarButton(*tool, CM_SAVE_TEXTURES, SAVE_TEXTURES, "Save textures");
  tool->setButtonPictures(CM_SAVE_TEXTURES, "save_mission");

  commandSystem->createToolbarButton(*tool, CM_SAVE_TEXTURES_BY_SUBSTRING, SAVE_TEXTURES_BY_SUBSTRING, "Save textures by substring");
  tool->setButtonPictures(CM_SAVE_TEXTURES_BY_SUBSTRING, "save_mission");

  commandSystem->createToolbarButton(*tool, CM_RELOAD_TEXTURES, RELOAD_TEXTURES, "Reload textures");
  tool->setButtonPictures(CM_RELOAD_TEXTURES, "compile");

  commandSystem->createToolbarButton(*tool, CM_FORCE_REBUILD, FORCE_REBUILD, "Force rebuild");
  tool->setButtonPictures(CM_FORCE_REBUILD, "compile");

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->registerWindowHandler(this);

  if (isGraphVisible)
  {
    EDITORCORE->addPropPanel(GRAPH_PANEL_WTYPE, hdpi::_pxScaled(1000));
  }

  if (isTexturePreviewVisible)
  {
    EDITORCORE->addPropPanel(TEXTURE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(300));
  }

  if (isHistogramVisible)
  {
    EDITORCORE->addPropPanel(HISTOGRAM_PANEL_WTYPE, hdpi::_pxScaled(300));
  }

  if (isLandscapeVisible)
  {
    EDITORCORE->addPropPanel(LANDSCAPE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(300));
  }

  if (isNodeLibraryVisible)
  {
    EDITORCORE->addPropPanel(NODE_LIBRARY_PANEL_WTYPE, hdpi::_pxScaled(280));
  }

  if (isPropertiesVisible)
  {
    EDITORCORE->addPropPanel(PROPERTIES_PANEL_WTYPE, hdpi::_pxScaled(320));
  }

  if (isShortcutsVisible)
  {
    EDITORCORE->addPropPanel(SHORTCUTS_PANEL_WTYPE, hdpi::_pxScaled(420));
  }

  if (isGraphVisible || isTexturePreviewVisible || isHistogramVisible || isLandscapeVisible || isNodeLibraryVisible ||
      isPropertiesVisible || isShortcutsVisible)
  {
    EDITORCORE->managePropPanels();
  }

  return true;
}

bool GraphEditorPlg::end()
{
  // A queued edit records its undo entry under this tab, and can open a modal about this graph, so
  // it is resolved here rather than over the plugin they switched to. actObjects drains the queue
  // on the next tick, so a second attempt goes through.
  if (graphPanel->hasPendingActions())
  {
    return false;
  }
  // A destroyed control drops its pending change, so a value typed into the Properties panel lands before the removal.
  if (propertiesPanel)
    propertiesPanel->commitPendingEdit();

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();

  // The manager's reset() at exit asserts on a panel left behind the handler. Removal clears the flag that begin() reopens by.
  auto removePanel = [manager](void *panel, bool &visible) {
    const bool wasVisible = visible;
    manager->removeWindow(panel);
    visible = wasVisible;
  };
  removePanel(graphPanel.get(), isGraphVisible);
  removePanel(previewPanel.get(), isTexturePreviewVisible);
  removePanel(histogramPanel.get(), isHistogramVisible);
  removePanel(landscapePanel.get(), isLandscapeVisible);
  removePanel(nodeLibraryPanel.get(), isNodeLibraryVisible);
  removePanel(propertiesPanel.get(), isPropertiesVisible);
  removePanel(shortcutsPanel.get(), isShortcutsVisible);

  manager->unregisterWindowHandler(this);
  return true;
}

void GraphEditorPlg::onNewProject() {}

// Graphs live outside the project file but inside its tree, so they go when it does. Without
// this a tab outlived its project and went on pointing at a file the editor had left.
void GraphEditorPlg::clearObjects() { graphPanel->closeAllTabs(); }

void GraphEditorPlg::saveObjects([[maybe_unused]] DataBlock &blk, [[maybe_unused]] DataBlock &local_data,
  [[maybe_unused]] const char *base_path)
{}

void GraphEditorPlg::loadObjects([[maybe_unused]] const DataBlock &blk, [[maybe_unused]] const DataBlock &local_data,
  [[maybe_unused]] const char *base_path)
{
  initResourcePaths();

  if (!texGenService)
  {
    if (IEditorService *svc = IDaEditor3Engine::get().findService("texgen"))
    {
      texGenService = svc->queryInterface<IGraphTexGenService>();
    }
  }

  if (texGenService)
  {
    texGenService->initTexGen(resourcePaths.defaultShadersPath, resourcePaths.defaultTexgenPath);
    texGenService->setGraphData(nullptr);
    // Binds the first tab, with its compiler: the graph is empty, but it is the one on screen.
    graphPanel->rebindTexGen();
  }
}

void GraphEditorPlg::promptAndLoadGraphBlk()
{
  String picked = wingw::file_open_dlg(nullptr, "Select graph BLK to load", "Graph BLK (*.blk)|*.blk|All files (*.*)|*.*", "blk",
    resourcePaths.mainGraphsDir);
  if (!picked.length())
  {
    return;
  }

  bool isSubgraph = false;
  graph_file_stem(picked.str(), &isSubgraph);
  if (isSubgraph)
  {
    // A subgraph is edited only in a floating window, over a graph that uses it.
    const char *const openKey = command_hint(CANVAS_OPEN_SUBGRAPH);
    wingw::message_box(wingw::MBS_OK | wingw::MBS_INFO, "Load graph",
      "%s is a subgraph: it opens in a floating window over a graph that holds an instance of it. Only a subgraph in %s "
      "can have instances.\n\n"
      "Open it from that instance: %s%sits context menu or its header icon.",
      dd_get_fname(picked.str()), resourcePaths.subgraphsDir.str(), openKey, *openKey ? ", " : "");
    return;
  }

  if (!graphPanel->openGraphFile(picked.str()))
  {
    DAEDITOR3.conError("GraphEditor: failed to load graph BLK %s", picked.str());
  }
}

void GraphEditorPlg::promptAndSaveGraphBlk()
{
  GraphTab &surface = graphPanel->activeSurface();
  // A window holds only a subgraph, so it saves only as one.
  if (surface.getHostWindow())
  {
    saveAsSubgraphBlk(surface);
  }
  else
  {
    saveGraphAs(surface);
  }
}

bool GraphEditorPlg::writeGraphTo(GraphTab &tab, const char *path)
{
  GraphDocument &doc = tab.getDocument();
  bool isSubgraph = false;
  const eastl::string stem = graph_file_stem(path, &isSubgraph);
  // The tab's compile reads instance bodies from the subgraph files, so it must not run while one is written.
  eastl::optional<CompilerDrainScope> drain;
  if (isSubgraph)
  {
    drain.emplace(*this);
  }
  if (!save_graph_data_blk(doc.getGraphData(), path))
  {
    DAEDITOR3.conError("GraphEditor: failed to save graph BLK to %s", path);
    return false;
  }
  doc.mutateWithoutInvalidating([&](GraphData &gd) { gd.sourcePath = path; });
  tab.markSaved();
  if (SubgraphWindow *window = tab.getHostWindow())
  {
    window->setStem(stem.c_str()); // a Save as moves the window to the new file
  }
  if (isSubgraph)
  {
    // Every save ends here, and a subgraph file is also a palette entry.
    registerSubgraphFile(path, /*replace_existing=*/true);
  }
  return true;
}

bool GraphEditorPlg::saveGraphAs(GraphTab &tab)
{
  // Seeded with the graph's own file so the user gets that name back; one with no file yet starts
  // in the mainGraphs dir.
  const eastl::string &curPath = tab.getDocument().getGraphData().sourcePath;
  String initPath = !curPath.empty() ? String(curPath.c_str()) : resourcePaths.mainGraphsDir;
  const String picked =
    wingw::file_save_dlg(nullptr, "Save graph BLK", "Graph BLK (*.blk)|*.blk|All files (*.*)|*.*", "blk", initPath);
  if (!picked.length())
  {
    return false;
  }
  bool isSubgraph = false;
  graph_file_stem(picked.str(), &isSubgraph);
  // A subgraph is edited only in a floating window, never in a graph tab.
  if (isSubgraph && !tab.getHostWindow())
  {
    wingw::message_box(wingw::MBS_OK | wingw::MBS_INFO, "Save graph",
      "%s is a subgraph name, and a graph tab does not hold a subgraph.\n\nSave the graph under a name without .subgraph.",
      dd_get_fname(picked.str()));
    return false;
  }
  return writeGraphTo(tab, picked.str());
}

bool GraphEditorPlg::saveGraph(GraphTab &tab)
{
  const eastl::string &curPath = tab.getDocument().getGraphData().sourcePath;
  // No file to write back to, so this raises the same prompt the Save command does.
  return !curPath.empty() ? writeGraphTo(tab, curPath.c_str()) : saveGraphAs(tab);
}

// All the schema errors in one message box, not a prompt for each.
bool GraphEditorPlg::confirmSubgraphSchema(GraphTab &tab, const char *title)
{
  const GraphData &gd = tab.getDocument().getGraphData();
  eastl::vector<SubgraphSchemaError> errors;
  if (!validate_subgraph_schema(gd, errors))
  {
    String text("Cannot save the subgraph -- schema invalid:");
    for (const SubgraphSchemaError &err : errors)
    {
      eastl::string line = format_subgraph_schema_error(err);
      text.aprintf(0, "\n  - %s", line.c_str());
      DAEDITOR3.conError("GraphEditor: subgraph schema -- %s", line.c_str());
    }
    wingw::message_box(wingw::MBS_OK | wingw::MBS_EXCL, title, text);
    return false;
  }

  // Legal, but usually a wire forgotten inside the subgraph, so the user confirms.
  eastl::vector<DeadInputBoundary> dead;
  find_dead_input_boundaries(gd, dead);
  if (!dead.empty())
  {
    String warning("The following input pins do not propagate to any output:");
    for (const DeadInputBoundary &d : dead)
    {
      const char *label = !d.interfaceName.empty() ? d.interfaceName.c_str() : (!d.type.empty() ? d.type.c_str() : "<unnamed>");
      warning.aprintf(0, "\n  - %s", label);
      DAEDITOR3.conWarning("GraphEditor: subgraph input '%s' (node id=%d) does not reach any output", label, d.nodeId);
    }
    warning.aprintf(0, "\n\nSave subgraph anyway?");
    const int ret = wingw::message_box(wingw::MBS_YESNO | wingw::MBS_EXCL, title, warning);
    if (ret != wingw::MB_ID_YES)
    {
      return false;
    }
  }
  return true;
}

bool GraphEditorPlg::saveSubgraph(GraphTab &tab)
{
  const eastl::string path = tab.getDocument().getGraphData().sourcePath;
  if (path.empty())
  {
    return saveAsSubgraphBlk(tab);
  }
  if (!confirmSubgraphSchema(tab, "Save subgraph") || !writeGraphTo(tab, path.c_str()))
  {
    return false;
  }
  show_saved_toast(path.c_str());
  return true;
}

bool GraphEditorPlg::saveAsSubgraphBlk(GraphTab &tab)
{
  // A subgraph is edited only in a floating window, never in a graph tab.
  if (!tab.getHostWindow())
  {
    wingw::message_box(wingw::MBS_OK | wingw::MBS_INFO, "Save as subgraph",
      "A graph tab does not hold a subgraph: a subgraph is edited only in a floating window.\n\n"
      "To make one from this graph, select its nodes and create a subgraph from the selection.");
    return false;
  }
  if (!confirmSubgraphSchema(tab, "Save as subgraph"))
  {
    return false;
  }

  GraphDocument &doc = tab.getDocument();
  const String &subgraphsDir = resourcePaths.subgraphsDir;
  // Seed the dialog with the previous source path only if it already lives under the
  // subgraphs convention; otherwise drop to the subgraphsDir so the user doesn't end up
  // saving a subgraph back into mainGraphs/ by accident.
  String initPath = subgraphsDir;
  if (!doc.getGraphData().sourcePath.empty())
  {
    const String existing(doc.getGraphData().sourcePath.c_str());
    if (existing.suffix(".subgraph.blk"))
    {
      initPath = existing;
    }
  }
  String picked = wingw::file_save_dlg(nullptr, "Save as subgraph BLK",
    "Subgraph BLK (*.subgraph.blk)|*.subgraph.blk|All files (*.*)|*.*", "blk", initPath);
  if (!picked.length())
  {
    return false;
  }

  String finalPath = picked;
  if (!finalPath.suffix(".subgraph.blk"))
  {
    if (finalPath.suffix(".blk"))
    {
      finalPath = String(0, "%.*s.subgraph.blk", finalPath.length() - 4, finalPath.str());
    }
    else
    {
      finalPath.aprintf(0, ".subgraph.blk");
    }
  }

  // The library and the compile read subgraphs only from the subgraphs folder.
  if (!subgraphRootOf(finalPath.str()))
  {
    wingw::message_box(wingw::MBS_OK | wingw::MBS_INFO, "Save as subgraph",
      "%s is outside the subgraphs folder, so no graph could use it.\n\nSave it into %s", finalPath.str(), subgraphsDir.str());
    return false;
  }

  if (!writeGraphTo(tab, finalPath.str()))
  {
    return false;
  }
  DAEDITOR3.conNote("GraphEditor: saved subgraph to %s", finalPath.str());
  show_saved_toast(finalPath.str());
  return true;
}

void GraphEditorPlg::selectAll() { graphPanel->requestSelectAll(); }

void GraphEditorPlg::deselectAll() { graphPanel->requestDeselectAll(); }

void GraphEditorPlg::invertSelection() { graphPanel->requestInvertSelection(); }

// Texture generation runs on a worker thread inside the service. The poll here is cheap
// (no D3D, no generation steps) and just feeds the worker file-change notifications and
// kicks it when new work shows up. Done in actObjects so it still runs when the GraphEditor
// tab is hidden.
void GraphEditorPlg::actObjects(float dt)
{
  if (texGenService)
  {
    texGenService->tickTexGeneration();

    if (texGenService->isGenerationPending())
    {
      DAGORED2->invalidateViewportCache();
    }
  }

  graphPanel->applyPendingCompileResults();

  graphPanel->actObjects(dt);
}

void GraphEditorPlg::beforeRenderObjects([[maybe_unused]] IGenViewportWnd *vp) {}

// Execute the landscape preview render during the normal render cycle, when d3d state,
// shader blocks, and all shader globals are valid.
void GraphEditorPlg::renderObjects()
{
  if (texGenService && landscapePanel)
  {
    if (LandscapePreviewScene *scene = landscapePanel->getScene())
    {
      scene->updateHeightmapParams(*texGenService);
      scene->executeRender();
    }
  }
}

void GraphEditorPlg::renderTransObjects() {}

void GraphEditorPlg::updateImgui()
{
  graphPanel->stampUndoOwner();

  if (PropPanel::PanelWindowPropertyControl *panelWindow = graphPanel->getPanelWindow())
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*panelWindow, &open); window.canSubmitContents())
    {
      graphPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(graphPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (previewPanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*previewPanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      previewPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(previewPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (histogramPanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*histogramPanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      histogramPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(histogramPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (landscapePanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*landscapePanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      landscapePanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(landscapePanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (nodeLibraryPanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*nodeLibraryPanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      nodeLibraryPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(nodeLibraryPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  // Properties panel last: it reads getSelectedNodeId() from graphPanel, which is refreshed
  // by graphPanel->updateImgui() above. Running last keeps the displayed properties in sync
  // with the ne::GetSelectedNodes value that graphPanel just captured.
  if (propertiesPanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*propertiesPanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      propertiesPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(propertiesPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (shortcutsPanel)
  {
    bool open = true;
    if (auto window = IDaEditor3Engine::ScopedImguiWindow(*shortcutsPanel->getPanelWindow(), &open); window.canSubmitContents())
    {
      shortcutsPanel->updateImgui();
    }

    if (!open)
    {
      EDITORCORE->removePropPanel(shortcutsPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  // After the panels: it reads the canvas drop target's verdict from this same frame.
  dragOverlay.draw(*this, graphPanel->isBaseNodeDropTargetHot());
}

void *GraphEditorPlg::queryInterfacePtr([[maybe_unused]] unsigned huid) { return nullptr; }

bool GraphEditorPlg::onPluginMenuClick([[maybe_unused]] unsigned id)
{
  switch (id)
  {
    case CM_SHOW_GRAPH:
    {
      if (!graphPanel->getPanelWindow())
      {
        EDITORCORE->addPropPanel(GRAPH_PANEL_WTYPE, hdpi::_pxScaled(600));
      }
      else
      {
        EDITORCORE->removePropPanel(graphPanel.get());
      }

      EDITORCORE->managePropPanels();
      return true;
    }
    case CM_SHOW_TEXTURE_PREVIEW:
    {
      if (!previewPanel)
      {
        EDITORCORE->addPropPanel(TEXTURE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(300));
      }
      else
      {
        EDITORCORE->removePropPanel(previewPanel.get());
      }

      EDITORCORE->managePropPanels();
      return true;
    }
    case CM_SHOW_HISTOGRAM:
    {
      if (!histogramPanel)
      {
        EDITORCORE->addPropPanel(HISTOGRAM_PANEL_WTYPE, hdpi::_pxScaled(300));
      }
      else
      {
        EDITORCORE->removePropPanel(histogramPanel.get());
      }

      EDITORCORE->managePropPanels();
      return true;
    }
    case CM_SHOW_LANDSCAPE_PREVIEW:
    {
      if (!landscapePanel)
      {
        EDITORCORE->addPropPanel(LANDSCAPE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(400));
      }
      else
      {
        EDITORCORE->removePropPanel(landscapePanel.get());
      }

      EDITORCORE->managePropPanels();
      return true;
    }
    case CM_SHOW_NODE_LIBRARY:
    {
      toggleNodeLibraryPanel();
      return true;
    }
    case CM_SHOW_PROPERTIES:
    {
      if (!propertiesPanel)
      {
        EDITORCORE->addPropPanel(PROPERTIES_PANEL_WTYPE, hdpi::_pxScaled(320));
      }
      else
      {
        EDITORCORE->removePropPanel(propertiesPanel.get());
      }

      EDITORCORE->managePropPanels();
      return true;
    }
    case CM_SHOW_SHORTCUTS:
    {
      toggleShortcutsPanel();
      return true;
    }
    case CM_TOGGLE_GRAPH_PROPERTIES:
    {
      if (propertiesPanel)
      {
        propertiesPanel->toggleGraphView();
      }
      return true;
    }
    case CM_NEW_GRAPH:
    {
      graphPanel->openNewTab();
      return true;
    }
    case CM_LOAD_GRAPH_BLK:
    {
      promptAndLoadGraphBlk();
      return true;
    }
    case CM_SAVE_GRAPH_BLK:
    {
      promptAndSaveGraphBlk();
      return true;
    }
    case CM_SAVE_AS_SUBGRAPH_BLK:
    {
      saveAsSubgraphBlk(graphPanel->activeSurface());
      return true;
    }
    case CM_SAVE_TEXTURES:
    {
      if (texGenService && !texGenService->isSaving())
      {
        texGenService->saveTextures();
      }
      return true;
    }
    case CM_SAVE_TEXTURES_BY_SUBSTRING:
    {
      if (texGenService && !texGenService->isSaving())
      {
        String substring = last_save_textures_substring;
        if (prompt_texture_substring(substring))
        {
          last_save_textures_substring = substring;
          texGenService->saveTextures(substring.str());
        }
      }
      return true;
    }
    case CM_RELOAD_TEXTURES:
    {
      if (texGenService)
      {
        texGenService->requestRegenerate();
      }
      return true;
    }
  }

  return false;
}

void GraphEditorPlg::handleViewportAcceleratorCommand([[maybe_unused]] unsigned id) {}

bool GraphEditorPlg::catchEvent(unsigned event_huid, void *user_data)
{
  if (event_huid == HUID_ClaimUndoOpOwner)
  {
    return graphPanel->ownsUndoToken(user_data);
  }
  if (event_huid == HUID_KeepEditorOpen)
  {
    return !graphPanel->confirmCloseAllTabs();
  }
  if (event_huid != HUID_ZoomAndCenter)
  {
    return false;
  }
  graphPanel->requestFrameSelected();
  // Handled even with the window closed: this plugin has no 3D selection for the viewport fallback to frame.
  return true;
}

void GraphEditorPlg::toggleNodeLibraryPanel()
{
  if (!nodeLibraryPanel)
  {
    EDITORCORE->addPropPanel(NODE_LIBRARY_PANEL_WTYPE, hdpi::_pxScaled(280));
  }
  else
  {
    EDITORCORE->removePropPanel(nodeLibraryPanel.get());
  }

  EDITORCORE->managePropPanels();
}

void GraphEditorPlg::toggleShortcutsPanel()
{
  if (!shortcutsPanel)
  {
    EDITORCORE->addPropPanel(SHORTCUTS_PANEL_WTYPE, hdpi::_pxScaled(420));
  }
  else
  {
    EDITORCORE->removePropPanel(shortcutsPanel.get());
  }

  EDITORCORE->managePropPanels();
}

void GraphEditorPlg::registerEditorCommands(IEditorCommandSystem &command_system)
{
  command_system.addCommand(SHOW_GRAPH);
  command_system.addCommand(SHOW_TEXTURE_PREVIEW);
  command_system.addCommand(SHOW_HISTOGRAM);
  command_system.addCommand(SHOW_LANDSCAPE_PREVIEW);
  command_system.addCommand(SHOW_NODE_LIBRARY);
  command_system.addCommand(SHOW_PROPERTIES);
  command_system.addCommand(SHOW_SHORTCUTS, ImGuiKey_F1);
  command_system.addCommand(NEW_GRAPH);
  command_system.addCommand(LOAD_GRAPH_BLK);
  command_system.addCommand(SAVE_GRAPH_BLK);
  command_system.addCommand(SAVE_AS_SUBGRAPH_BLK);
  command_system.addCommand(SAVE_TEXTURES);
  command_system.addCommand(SAVE_TEXTURES_BY_SUBSTRING);
  command_system.addCommand(RELOAD_TEXTURES);
  command_system.addCommand(FORCE_REBUILD, ImGuiMod_Ctrl | ImGuiKey_R);
  command_system.addCommand(TOGGLE_GRAPH_PROPERTIES, ImGuiKey_P);

  // Canvas-local shortcuts
  command_system.addCommand(CANVAS_DELETE_SELECTED, ImGuiKey_Delete);
  command_system.addCommand(CANVAS_COPY, ImGuiMod_Ctrl | ImGuiKey_C);
  command_system.addCommand(CANVAS_CUT, ImGuiMod_Ctrl | ImGuiKey_X);
  command_system.addCommand(CANVAS_PASTE, ImGuiMod_Ctrl | ImGuiKey_V);
  command_system.addCommand(CANVAS_ZOOM_AND_CENTER, ImGuiMod_Ctrl | ImGuiKey_Space);
  command_system.addCommand(CANVAS_SELECT_NODES_NO_OUTPUTS, ImGuiMod_Ctrl | ImGuiKey_M);
  command_system.addCommand(CANVAS_SHOW_NEXT_SELECTED, ImGuiMod_Shift | ImGuiKey_F);
  command_system.addCommand(CANVAS_REMOVE_KEEP_CONNECTIONS, ImGuiKey_Backspace);
  command_system.addCommand(CANVAS_REMOVE_EDGES_AT_PIN, ImGuiKey_X);
  command_system.addCommand(CANVAS_MODIFY_EDGE_AT_PIN, ImGuiKey_A);
  command_system.addCommand(CANVAS_JUMP_OPPOSITE_PIN, ImGuiKey_Tab);
  command_system.addCommand(CANVAS_COMMENT_PIN, ImGuiKey_C);
  // No Ctrl+D alias for Duplicate, and no Ctrl+A of our own: the editor already binds those to
  // Deselect all and Select all, which reach this plugin through deselectAll() and selectAll().
  command_system.addCommand(CANVAS_DUPLICATE, ImGuiKey_D);
  command_system.addCommand(CANVAS_TOGGLE_AUTOUPDATE, ImGuiMod_Shift | ImGuiKey_Q);
  // The editor also binds bare Space, to free camera, but only as a viewport accelerator: it routes
  // RouteGlobal, which the canvas dispatch outranks while this panel has focus.
  command_system.addCommand(CANVAS_ADD_NODE_AT_PIN, ImGuiKey_Space);
  command_system.addCommand(CANVAS_ADD_TRANSIT_NODE, ImGuiMod_Shift | ImGuiKey_Space);
  command_system.addCommand(CANVAS_CREATE_SUBGRAPH, ImGuiMod_Ctrl | ImGuiKey_G);
  // Open and Go to parent share Tab with the pin jump; separate ids keep each one rebindable.
  command_system.addCommand(CANVAS_OPEN_SUBGRAPH, ImGuiKey_Tab);
  command_system.addCommand(CANVAS_EXPLODE_SUBGRAPH, ImGuiMod_Ctrl | ImGuiKey_E);
  command_system.addCommand(CANVAS_GO_TO_PARENT_GRAPH, ImGuiKey_Tab);
}

void GraphEditorPlg::registerMenuAccelerators()
{
  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  G_ASSERT(manager);
  manager->addAccelerator(CM_SHOW_SHORTCUTS, SHOW_SHORTCUTS);
  // The menu item and toolbar button show this command's chord, so it has to work outside the canvas.
  manager->addAccelerator(CM_FORCE_REBUILD, FORCE_REBUILD);
  // Global, so P works from any panel; a focused text field still types the letter.
  manager->addAccelerator(CM_TOGGLE_GRAPH_PROPERTIES, TOGGLE_GRAPH_PROPERTIES);
}

bool GraphEditorPlg::promptPinComment(eastl::string &inout_comment)
{
  String text(inout_comment.c_str());
  PropPanel::DialogWindow *dlg = DAGORED2->createAutoSizedDialog("Pin comment");
  dlg->setInitialFocus(PID_PIN_COMMENT);

  PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
  EnterAcceptsDialog enterHandler(*dlg);
  panel.setEventHandler(&enterHandler); // must precede createEditBox: a control copies the panel's handler at creation
  panel.createEditBox(PID_PIN_COMMENT, "Comment:", text);

  const int ret = dlg->showDialog();
  const bool accepted = ret == PropPanel::DIALOG_ID_OK;
  if (accepted)
  {
    text = panel.getText(PID_PIN_COMMENT);
    inout_comment = text.str();
  }

  del_it(dlg);
  return accepted;
}

void GraphEditorPlg::promptOpenGraph()
{
  // The size is a floor: autoSize below fits the box to the two radios, as the design has it.
  PropPanel::DialogWindow *dlg = DAGORED2->createDialog(hdpi::_pxScaled(330), hdpi::_pxScaled(60), "Open graph");
  dlg->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Open");

  PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
  PropPanel::ContainerPropertyControl *group = panel.createRadioGroup(PID_OPEN_GRAPH_MODE, "");
  group->createRadio(OPEN_GRAPH_EXISTING, "existing graph");
  group->createRadio(OPEN_GRAPH_NEW, "new graph");
  panel.setInt(PID_OPEN_GRAPH_MODE, OPEN_GRAPH_EXISTING);
  dlg->autoSize();

  const bool accepted = dlg->showDialog() == PropPanel::DIALOG_ID_OK;
  const int mode = panel.getInt(PID_OPEN_GRAPH_MODE); // read before the dialog goes
  del_it(dlg);
  if (!accepted)
  {
    return;
  }
  if (mode == OPEN_GRAPH_NEW)
  {
    graphPanel->openNewTab();
  }
  else
  {
    promptAndLoadGraphBlk();
  }
}

bool GraphEditorPlg::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

bool GraphEditorPlg::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

bool GraphEditorPlg::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool GraphEditorPlg::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

bool GraphEditorPlg::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool GraphEditorPlg::handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

bool GraphEditorPlg::handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return false;
}

bool GraphEditorPlg::handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }

bool GraphEditorPlg::handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }

void GraphEditorPlg::handleViewportPaint(IGenViewportWnd *wnd) {}

void GraphEditorPlg::handleViewChange(IGenViewportWnd *wnd) {}

void *GraphEditorPlg::onWmCreateWindow(int type)
{
  PropPanel::ContainerPropertyControl *toolBar = DAGORED2->getCustomPanel(toolBarId);

  switch (type)
  {
    case GRAPH_PANEL_WTYPE:
    {
      if (graphPanel->getPanelWindow())
      {
        return nullptr;
      }

      graphPanel->createWindow();

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_GRAPH, true);
      }

      isGraphVisible = true;
      return graphPanel.get();
    }
    case TEXTURE_PREVIEW_PANEL_WTYPE:
    {
      if (previewPanel || !texGenService)
      {
        return nullptr;
      }

      previewPanel = eastl::make_unique<TexturePreviewPanel>(*this, *texGenService);
      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_TEXTURE_PREVIEW, true);
      }

      isTexturePreviewVisible = true;
      return previewPanel.get();
    }
    case HISTOGRAM_PANEL_WTYPE:
    {
      if (histogramPanel || !texGenService)
      {
        return nullptr;
      }

      histogramPanel = eastl::make_unique<HistogramPanel>(*this, *texGenService);

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_HISTOGRAM, true);
      }

      isHistogramVisible = true;
      return histogramPanel.get();
    }
    case LANDSCAPE_PREVIEW_PANEL_WTYPE:
    {
      if (landscapePanel || !texGenService)
      {
        return nullptr;
      }

      landscapePanel = eastl::make_unique<LandscapePreviewPanel>(*this, *texGenService);

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_LANDSCAPE_PREVIEW, true);
      }

      isLandscapeVisible = true;
      return landscapePanel.get();
    }
    case NODE_LIBRARY_PANEL_WTYPE:
    {
      if (nodeLibraryPanel)
      {
        return nullptr;
      }

      nodeLibraryPanel = eastl::make_unique<NodeLibraryPanel>(*this);

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_NODE_LIBRARY, true);
      }

      isNodeLibraryVisible = true;
      return nodeLibraryPanel.get();
    }
    case PROPERTIES_PANEL_WTYPE:
    {
      if (propertiesPanel)
      {
        return nullptr;
      }

      propertiesPanel = eastl::make_unique<PropertiesPanel>(*this);

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_PROPERTIES, true);
      }

      isPropertiesVisible = true;
      return propertiesPanel.get();
    }
    case SHORTCUTS_PANEL_WTYPE:
    {
      if (shortcutsPanel)
      {
        return nullptr;
      }

      shortcutsPanel = eastl::make_unique<ShortcutsPanel>();

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_SHORTCUTS, true);
      }

      isShortcutsVisible = true;
      return shortcutsPanel.get();
    }
  }

  return nullptr;
}

bool GraphEditorPlg::onWmDestroyWindow(void *window)
{
  PropPanel::ContainerPropertyControl *toolBar = DAGORED2->getCustomPanel(toolBarId);

  if (window == graphPanel.get())
  {
    graphPanel->destroyWindow();
    isGraphVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_GRAPH, false);
    }

    return true;
  }

  if (previewPanel && window == previewPanel.get())
  {
    previewPanel.reset();
    isTexturePreviewVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_TEXTURE_PREVIEW, false);
    }

    return true;
  }

  if (histogramPanel && window == histogramPanel.get())
  {
    histogramPanel.reset();
    isHistogramVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_HISTOGRAM, false);
    }

    return true;
  }

  if (landscapePanel && window == landscapePanel.get())
  {
    landscapePanel.reset();
    isLandscapeVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_LANDSCAPE_PREVIEW, false);
    }
    return true;
  }

  if (nodeLibraryPanel && window == nodeLibraryPanel.get())
  {
    nodeLibraryPanel.reset();
    isNodeLibraryVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_NODE_LIBRARY, false);
    }
    return true;
  }

  if (propertiesPanel && window == propertiesPanel.get())
  {
    propertiesPanel.reset();
    isPropertiesVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_PROPERTIES, false);
    }
    return true;
  }

  if (shortcutsPanel && window == shortcutsPanel.get())
  {
    shortcutsPanel.reset();
    isShortcutsVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_SHORTCUTS, false);
    }
    return true;
  }

  return false;
}

void GraphEditorPlg::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_SHOW_GRAPH:
    {
      if (!graphPanel->getPanelWindow())
      {
        EDITORCORE->addPropPanel(GRAPH_PANEL_WTYPE, hdpi::_pxScaled(600));
      }
      else
      {
        EDITORCORE->removePropPanel(graphPanel.get());
      }

      EDITORCORE->managePropPanels();
      break;
    }
    case CM_SHOW_TEXTURE_PREVIEW:
    {
      if (!previewPanel)
      {
        EDITORCORE->addPropPanel(TEXTURE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(300));
      }
      else
      {
        EDITORCORE->removePropPanel(previewPanel.get());
      }

      EDITORCORE->managePropPanels();
      break;
    }
    case CM_SHOW_HISTOGRAM:
    {
      if (!histogramPanel)
      {
        EDITORCORE->addPropPanel(HISTOGRAM_PANEL_WTYPE, hdpi::_pxScaled(300));
      }
      else
      {
        EDITORCORE->removePropPanel(histogramPanel.get());
      }

      EDITORCORE->managePropPanels();
      break;
    }
    case CM_SHOW_LANDSCAPE_PREVIEW:
    {
      if (!landscapePanel)
      {
        EDITORCORE->addPropPanel(LANDSCAPE_PREVIEW_PANEL_WTYPE, hdpi::_pxScaled(400));
      }
      else
      {
        EDITORCORE->removePropPanel(landscapePanel.get());
      }

      EDITORCORE->managePropPanels();
      break;
    }
    case CM_SHOW_NODE_LIBRARY:
    {
      toggleNodeLibraryPanel();
      break;
    }
    case CM_SHOW_PROPERTIES:
    {
      if (!propertiesPanel)
      {
        EDITORCORE->addPropPanel(PROPERTIES_PANEL_WTYPE, hdpi::_pxScaled(320));
      }
      else
      {
        EDITORCORE->removePropPanel(propertiesPanel.get());
      }

      EDITORCORE->managePropPanels();
      break;
    }
    case CM_SHOW_SHORTCUTS:
    {
      toggleShortcutsPanel();
      break;
    }
    case CM_NEW_GRAPH:
    {
      graphPanel->openNewTab();
      break;
    }
    case CM_LOAD_GRAPH_BLK:
    {
      promptAndLoadGraphBlk();
      break;
    }
    case CM_SAVE_GRAPH_BLK:
    {
      promptAndSaveGraphBlk();
      break;
    }
    case CM_SAVE_AS_SUBGRAPH_BLK:
    {
      saveAsSubgraphBlk(graphPanel->activeSurface());
      break;
    }
    case CM_SAVE_TEXTURES:
    {
      if (texGenService && !texGenService->isSaving())
      {
        texGenService->saveTextures();
      }
      break;
    }
    case CM_SAVE_TEXTURES_BY_SUBSTRING:
    {
      if (texGenService && !texGenService->isSaving())
      {
        String substring = last_save_textures_substring;
        if (prompt_texture_substring(substring))
        {
          last_save_textures_substring = substring;
          texGenService->saveTextures(substring.str());
        }
      }
      break;
    }
    case CM_RELOAD_TEXTURES:
    {
      if (texGenService)
      {
        texGenService->requestRegenerate();
      }
      break;
    }
    case CM_FORCE_REBUILD:
    {
      if (texGenService)
      {
        texGenService->requestForceRebuild();
      }
      break;
    }
  }
}

void GraphEditorPlg::initResourcePaths()
{
  String graphEditorDataDir = ::make_full_path(sgg::get_common_data_dir(), "graphEditor/");

  resourcePaths.defaultShadersPath = graphEditorDataDir + "default_shaders.blk";
  resourcePaths.defaultTexgenPath = graphEditorDataDir + "default_texgen.blk";
  resourcePaths.shaderIncludesDir = graphEditorDataDir + "generated/";
  resourcePaths.mainGraphsDir = ::make_full_path(sgg::get_exe_path_full(), "../../graphEditor/mainGraphs/");
  resourcePaths.subgraphsDir = graphEditorDataDir + "subgraphs/";
  subgraphRoots.clear();
  // constraint: every lookup of a subgraph file reads subgraphsDir alone; a second root must reach each lookup first.
  subgraphRoots.push_back(SubgraphRoot{String(GENERAL_SUBGRAPH_ROOT_LABEL), resourcePaths.subgraphsDir});
}

void GraphEditorPlg::appendShaderTemplatesToBaseNodes()
{
  String shadersDir = ::make_full_path(sgg::get_common_data_dir(), "graphEditor/shaders/");
  String pathMask(0, "%s*.blk", shadersDir.str());

  int loaded = 0;
  for (const alefind_t &fs : dd_find_iterator(pathMask, DA_FILE))
  {
    String filePath(0, "%s%s", shadersDir.str(), fs.name);
    char stem[128] = {};
    dd_get_fname_without_path_and_ext(stem, sizeof(stem), fs.name);

    DataBlock shaderFile;
    if (!shaderFile.load(filePath))
    {
      DAEDITOR3.conWarning("GraphEditor: failed to load shader template %s", filePath.str());
      continue;
    }

    const char *templateUid = shaderFile.getStr("templateUid", "");
    if (!templateUid[0])
    {
      DAEDITOR3.conWarning("GraphEditor: shader template %s has no top-level templateUid:t", filePath.str());
      continue;
    }

    const DataBlock *descBlk = shaderFile.getBlockByName("description");
    if (!descBlk)
    {
      DAEDITOR3.conWarning("GraphEditor: shader template %s has no description{} block", filePath.str());
      continue;
    }

    // Synthesise a node{} descriptor in baseNodesBlk so the node library panel /
    // properties panel / spawn factory all treat shader templates uniformly with
    // built-ins. name overrides the JS-era "[[description-name]]" template macro
    // with the file stem; category overrides "[[description-category]]" with a
    // fixed "Shaders" label so the panel groups them together.
    DataBlock *nodeBlk = baseNodesBlk.addNewBlock("node");
    nodeBlk->setStr("templateUid", templateUid);
    nodeBlk->setStr("name", stem);
    nodeBlk->setStr("category", "Shaders");
    nodeBlk->setStr("plugin", "shader_editor");
    nodeBlk->setBool("generated", true);
    nodeBlk->setBool("allowLoop", descBlk->getBool("allowLoop", false));
    nodeBlk->setInt("width", descBlk->getInt("width", 200));

    // Copy pin{} and property{} blocks verbatim. The converter
    // (tools/graphEditor/translate_shaders_to_blk.py) already emits them in
    // base_nodes.blk's typed convention (`val:r` / `val:i` / `val:b` / `val:p4`
    // driven by the property's `type:t`), so PropertiesPanel's typed
    // getInt/getReal reads pick them up without any re-typing here.
    for (uint32_t i = 0; i < descBlk->blockCount(); ++i)
    {
      const DataBlock *child = descBlk->getBlock(i);
      const char *childName = child->getBlockName();
      if (strcmp(childName, "pin") == 0 || strcmp(childName, "property") == 0)
      {
        nodeBlk->addNewBlock(child, childName);
      }
    }
    ++loaded;
  }

  if (loaded == 0)
  {
    DAEDITOR3.conNote("GraphEditor: no shader templates found under %s", shadersDir.str());
  }
}

void GraphEditorPlg::appendSubgraphTemplatesToBaseNodes()
{
  // Scans `subgraphsDir` for *.subgraph.blk files, synthesising one node{} descriptor in
  // baseNodesBlk per file so the NodeLibraryPanel can list them under category "Subgraphs" and
  // the existing drag-drop / spawn machinery (makeNodeFromBaseBlk) handles insertion. Each
  // synthesised pin{} mirrors a `subgraph in: TYPE` / `subgraph out` boundary node found
  // inside the child graph; the pin's `name` is the boundary's `name` property value (the
  // interface name authored by the user) -- NOT the boundary's descriptor pin name (which
  // is always literally "subgraph in" / "subgraph out"). Compile-time inline expansion
  // looks up boundaries by interface name to splice edges, so this naming is load-bearing.
  const String &subgraphsDir = resourcePaths.subgraphsDir;
  if (subgraphsDir.empty())
  {
    return;
  }

  // Scan `*.subgraph.blk` under subgraphsDir. dd_find_iterator's glob mask is shell-style
  // "*", and the double-dot filename "<stem>.subgraph.blk" still matches the single "*"
  // wildcard because the "*" portion absorbs "<stem>.subgraph".
  int loaded = 0;
  String mask(0, "%s*.subgraph.blk", subgraphsDir.str());
  for (const alefind_t &fs : dd_find_iterator(mask, DA_FILE))
  {
    String filePath(0, "%s%s", subgraphsDir.str(), fs.name);
    DataBlock desc;
    if (synthesize_subgraph_file(filePath, resourcePaths.shaderIncludesDir, GENERAL_SUBGRAPH_ROOT_LABEL, desc))
    {
      baseNodesBlk.addNewBlock(&desc, "node");
    }
    ++loaded;
  }

  if (loaded == 0)
  {
    DAEDITOR3.conNote("GraphEditor: no *.subgraph.blk files found under %s", subgraphsDir.str());
  }
}

bool GraphEditorPlg::loadBaseNodesBlkIfNeeded()
{
  if (baseNodesBlkLoaded)
  {
    return baseNodesBlk.blockCount() > 0;
  }

  String path = ::make_full_path(sgg::get_common_data_dir(), "graphEditor/base_nodes.blk");
  if (!baseNodesBlk.load(path))
  {
    DAEDITOR3.conError("GraphEditor: failed to load %s", path.str());
    return false;
  }
  baseNodesBlkLoaded = true;

  // Append shader sub-graph templates so the preprocess pass and uid index treat them
  // uniformly with built-in nodes. Order matters: appendShaderTemplatesToBaseNodes must
  // run before preprocess so synthesised shader descriptors get synthetic texture
  // properties injected, and before the uid index so baseNodesByUid sees them.
  appendShaderTemplatesToBaseNodes();
  // Append subgraph instance templates (one per file under mainGraphsDir). Same ordering
  // constraints as shader synthesis: must precede preprocess (subgraph instances with
  // texture-typed output pins still benefit from synthetic texture-property injection,
  // since the parent treats them like any other producer) and the uid index build.
  appendSubgraphTemplatesToBaseNodes();
  // Materialise the synthetic texture properties the JS editor injects at descriptor load
  // (see preprocess_node_descriptor / mainNodes.js GE_preprocessDescription). Done before
  // the uid index is built so subsequent lookups see the augmented descriptors. Must run
  // before makeNodeFromBaseBlk so newly spawned nodes get a propertyValues entry per
  // synthetic property, matching JS round-trip behaviour.
  for (uint32_t i = 0; i < baseNodesBlk.blockCount(); ++i)
  {
    DataBlock *node = baseNodesBlk.getBlock(i);
    if (strcmp(node->getBlockName(), "node") != 0)
    {
      continue;
    }
    preprocess_node_descriptor(*node);
  }
  // Build uid -> node{} index. All editor-side descriptor lookups go through this map;
  // it is the reason template renames don't break saved graphs.
  baseNodesByUid.clear();
  for (uint32_t i = 0; i < baseNodesBlk.blockCount(); ++i)
  {
    DataBlock *node = baseNodesBlk.getBlock(i);
    if (strcmp(node->getBlockName(), "node") != 0)
    {
      continue;
    }
    const char *uid = node->getStr("templateUid", "");
    if (!uid[0])
    {
      // base_nodes.blk should have a uid on every node{} (run tools/graphEditor/add_template_uids.py).
      // If one is missing it stays reachable only via findBaseNodeBlockByName (legacy fallback path).
      DAEDITOR3.conWarning("GraphEditor: base_nodes.blk node '%s' has no templateUid", node->getStr("name", ""));
      continue;
    }
    baseNodesByUid.emplace(eastl::string(uid), node);
  }
  return true;
}

const DataBlock &GraphEditorPlg::getBaseNodesBlk()
{
  loadBaseNodesBlkIfNeeded();
  return baseNodesBlk;
}

void GraphEditorPlg::reloadBaseNodes()
{
  CompilerDrainScope drain(*this);
  baseNodesByUid.clear();
  baseNodesBlk.reset();
  baseNodesBlkLoaded = false;
  loadBaseNodesBlkIfNeeded();
  if (nodeLibraryPanel)
  {
    nodeLibraryPanel->refresh();
  }
  graphPanel->invalidateDerivedState();
  // The panel's controls point into the freed descriptors.
  invalidatePropertiesPanel();
}

bool GraphEditorPlg::registerSubgraphFile(const char *path, bool replace_existing)
{
  const SubgraphRoot *root = subgraphRootOf(path);
  if (!root)
  {
    // The compile looks only in the roots, so this entry would never resolve.
    DAEDITOR3.conWarning("GraphEditor: %s is outside the subgraph roots, not registered", path);
    return false;
  }
  DataBlock fresh;
  if (!synthesize_subgraph_file(path, resourcePaths.shaderIncludesDir, root->label, fresh))
  {
    return false;
  }

  const eastl::string uid = eastl::string("subgraph::") + graph_file_stem(path);
  CompilerDrainScope drain(*this);
  if (!loadBaseNodesBlkIfNeeded())
  {
    return false;
  }
  auto it = baseNodesByUid.find(uid);
  DataBlock *target = it != baseNodesByUid.end() ? it->second : nullptr;
  if (target)
  {
    // An entry of this very file is no clash: the first load's scan made it, or the file was deleted outside the editor.
    if (!replace_existing && !dd_fname_equal(target->getStr("sourcePath", ""), path))
    {
      DAEDITOR3.conWarning("GraphEditor: %s not registered, the palette already has %s", path, uid.c_str());
      return false;
    }
    target->setFrom(&fresh); // in place: tabs, panels and the compile hold pointers to it
  }
  else
  {
    target = baseNodesBlk.addNewBlock(&fresh, "node");
    baseNodesByUid.emplace(uid, target);
  }
  preprocess_node_descriptor(*target); // as the startup load does for every descriptor
  subgraphRegistrationGenerations[uid] = ++subgraphDescriptorGeneration;
  if (nodeLibraryPanel)
  {
    nodeLibraryPanel->refresh();
  }
  graphPanel->invalidateDerivedState();
  invalidatePropertiesPanel();
  return true;
}

bool GraphEditorPlg::explodeSubgraphInstance(GraphTab &tab, int node_id)
{
  ExplodeResult res;
  if (!explode_subgraph_instance(tab.getDocument().getGraphData(), node_id, getBaseNodesBlk(), resourcePaths.subgraphsDir,
        SUBGRAPH_GRID, res))
  {
    DAEDITOR3.conError("GraphEditor: cannot explode the subgraph: %s", res.error.c_str());
    show_toast_at_cursor(PropPanel::ToastType::Alert, String(0, "Subgraph explode failed: %s", res.error.c_str()).str());
    return false;
  }
  for (const GraphData::Edge &e : res.droppedParentEdges)
  {
    if (e.muted)
    {
      DAEDITOR3.conWarning("GraphEditor: explode dropped muted edge %d", e.id);
    }
    else
    {
      DAEDITOR3.conWarning("GraphEditor: explode dropped edge %d, the subgraph has no wire behind that pin", e.id);
    }
  }
  tab.getDocument().replaceNodes({node_id}, eastl::move(res.addedNodes), eastl::move(res.addedEdges), "Explode subgraph",
    /*select_added=*/true);
  return true;
}

void GraphEditorPlg::createSubgraphFromSelection(GraphTab &tab, const eastl::vector<int> &node_ids)
{
  if (subgraphRoots.empty())
  {
    return; // set by the project load, like the other resource paths
  }
  GraphDocument &doc = tab.getDocument();
  GraphData child;
  ExtractPlan plan;
  // Before the dialog, so a refusal costs no typing.
  if (!extract_subgraph(doc.getGraphData(), make_span_const(node_ids), getBaseNodesBlk(), resourcePaths.subgraphsDir, child, plan))
  {
    wingw::message_box(wingw::MBS_OK | wingw::MBS_EXCL, "Create subgraph", plan.error.c_str());
    return;
  }
  for (const eastl::string &warning : plan.warnings)
  {
    DAEDITOR3.conWarning("GraphEditor: create subgraph: %s", warning.c_str());
  }

  CreateSubgraphChoice choice;
  const String proposed = propose_subgraph_name(doc.getGraphData().sourcePath.c_str(), subgraphRoots[0].dir);
  if (!prompt_create_subgraph(getSubgraphRoots(), proposed, choice))
  {
    return;
  }
  auto toastFailure = [](const char *reason) {
    show_toast_at_cursor(PropPanel::ToastType::Alert, String(0, "Subgraph creation failed: %s", reason).str());
  };
  dd_mkdir(subgraphRoots[choice.rootIndex].dir);
  if (dd_file_exist(choice.finalPath) || !save_graph_data_blk(child, choice.finalPath))
  {
    DAEDITOR3.conError("GraphEditor: create subgraph: cannot write %s", choice.finalPath.str());
    toastFailure(String(0, "cannot write %s", choice.finalPath.str()));
    return;
  }
  if (!registerSubgraphFile(choice.finalPath, /*replace_existing=*/false))
  {
    // Nothing refers to the file yet, and left behind it would come back at every startup scan.
    dd_erase(choice.finalPath);
    DAEDITOR3.conError("GraphEditor: create subgraph: %s was not registered and is removed", choice.finalPath.str());
    // registerSubgraphFile has logged why.
    toastFailure(String(0, "the node library refused %s, see the console", dd_get_fname(choice.finalPath)));
    return;
  }

  const eastl::string uid = eastl::string("subgraph::") + graph_file_stem(choice.finalPath);
  // On the grid Explode snaps its offset to, so the round trip stays exact.
  const float x = roundf(plan.centroidX / SUBGRAPH_GRID) * SUBGRAPH_GRID;
  const float y = roundf(plan.centroidY / SUBGRAPH_GRID) * SUBGRAPH_GRID;
  GraphData::Node instance;
  if (!makeNodeFromBaseBlk(uid.c_str(), x, y, instance))
  {
    DAEDITOR3.conError("GraphEditor: create subgraph: no descriptor '%s'", uid.c_str());
    toastFailure(String(0, "no node library entry %s", uid.c_str()));
    return;
  }
  instance.id = tab.allocateNodeId(); // before the removal, so never one of the removed ids

  eastl::vector<GraphData::Edge> edges;
  for (const ExtractCrossing &c : plan.crossings)
  {
    const int pin = find_pin_by_name(instance, c.ifaceName.c_str(), /*output=*/!c.isInput);
    if (pin < 0)
    {
      DAEDITOR3.conWarning("GraphEditor: create subgraph: the instance has no pin '%s', edge %d dropped", c.ifaceName.c_str(),
        c.parentEdgeId);
      continue;
    }
    // Stored Out -> In, as the loader and the spawn paths store them.
    GraphData::Edge &e = edges.push_back();
    e.id = c.parentEdgeId;
    e.muted = c.muted;
    e.elemA = c.isInput ? c.outsideNode : instance.id;
    e.pinA = c.isInput ? c.outsidePin : pin;
    e.elemB = c.isInput ? instance.id : c.outsideNode;
    e.pinB = c.isInput ? pin : c.outsidePin;
  }
  eastl::vector<GraphData::Node> added;
  added.push_back(eastl::move(instance));
  doc.replaceNodes(node_ids, eastl::move(added), eastl::move(edges), "Create subgraph", /*select_added=*/true);
  noteRecentlyUsedNode(uid.c_str());
  DAEDITOR3.conNote("GraphEditor: created subgraph %s", choice.finalPath.str());
  show_toast_at_cursor(PropPanel::ToastType::Success, String(0, "%s created successfully", dd_get_fname(choice.finalPath)).str());
}

uint32_t GraphEditorPlg::getSubgraphRegistrationGeneration(const char *template_uid) const
{
  auto it = subgraphRegistrationGenerations.find_as(template_uid);
  return it != subgraphRegistrationGenerations.end() ? it->second : 0;
}

const SubgraphRoot *GraphEditorPlg::subgraphRootOf(const char *path) const
{
  char dir[DAGOR_MAX_PATH];
  dd_get_fname_location(dir, path);
  for (const SubgraphRoot &root : subgraphRoots)
  {
    if (dd_fname_equal(dir, root.dir))
    {
      return &root;
    }
  }
  return nullptr;
}

const DataBlock *GraphEditorPlg::findBaseNodeBlockByUid(const char *template_uid)
{
  if (!template_uid || !*template_uid || !loadBaseNodesBlkIfNeeded())
  {
    return nullptr;
  }
  auto it = baseNodesByUid.find_as(template_uid);
  if (it == baseNodesByUid.end())
  {
    return nullptr;
  }
  return it->second;
}

const DataBlock *GraphEditorPlg::findBaseNodeBlockByName(const char *desc_name)
{
  if (!desc_name || !*desc_name || !loadBaseNodesBlkIfNeeded())
  {
    return nullptr;
  }
  for (uint32_t i = 0; i < baseNodesBlk.blockCount(); ++i)
  {
    const DataBlock *node = baseNodesBlk.getBlock(i);
    if (strcmp(node->getBlockName(), "node") != 0)
    {
      continue;
    }
    if (strcmp(node->getStr("name", ""), desc_name) == 0)
    {
      return node;
    }
  }
  return nullptr;
}

bool GraphEditorPlg::makeNodeFromBaseBlk(const char *template_uid, float x, float y, GraphData::Node &out)
{
  const DataBlock *desc = findBaseNodeBlockByUid(template_uid);
  if (!desc)
  {
    return false;
  }
  make_node_from_descriptor(baseNodesBlk, *desc, x, y, out);
  return true;
}

bool GraphEditorPlg::buildSpawnedNode(const char *template_uid, float x, float y, GraphData::Node &out)
{
  if (!graphPanel->getPanelWindow())
  {
    return false;
  }
  if (!makeNodeFromBaseBlk(template_uid, x, y, out))
  {
    DAEDITOR3.conWarning("GraphEditor: unknown base-node uid '%s'", template_uid ? template_uid : "");
    return false;
  }
  out.id = graphPanel->allocateNodeId();
  return true;
}

void GraphEditorPlg::spawnBaseNode(const char *template_uid, float x, float y)
{
  GraphData::Node n;
  if (!buildSpawnedNode(template_uid, x, y, n))
  {
    return;
  }
  getActiveDocument().createNode(eastl::move(n));
  noteRecentlyUsedNode(template_uid);
}

int GraphEditorPlg::spawnBaseNodeWired(const char *template_uid, float x, float y, int anchor_node, int anchor_pin, bool splice,
  int anchor_edge)
{
  GraphData::Node n;
  if (!buildSpawnedNode(template_uid, x, y, n))
  {
    return -1;
  }

  const int newId = n.id;
  const eastl::string shownName = n.descName; // n is moved from below
  GraphDocument &doc = getActiveDocument();
  const int wiredPin = splice ? doc.recordInsertNodeOnPin(eastl::move(n), anchor_node, anchor_pin, anchor_edge)
                              : doc.recordAddConnectedNode(eastl::move(n), anchor_node, anchor_pin);
  if (wiredPin < 0)
  {
    // The whole-graph pass refused every pin. Creating it unconnected beats discarding the node the
    // user picked or dropped.
    DAEDITOR3.conWarning("GraphEditor: '%s' has no pin that can take this connection; added unconnected", shownName.c_str());
  }
  noteRecentlyUsedNode(template_uid);
  return newId;
}

void GraphEditorPlg::spawnBaseNodeAtCanvasCenter(const char *template_uid)
{
  if (!graphPanel->getPanelWindow())
  {
    return;
  }

  // Stepped along a short diagonal by the node count: successive spawns would otherwise land on
  // the identical centre and overlap exactly, which reads as nothing having happened. Derived
  // rather than counted because a node dragged away leaves graphData holding its old position.
  const ImVec2 center = graphPanel->getLastCanvasCenter();
  const float slot = static_cast<float>(getActiveDocument().getGraphData().nodes.size() % SPAWN_SLOT_COUNT);
  spawnBaseNode(template_uid, center.x + slot * SPAWN_SLOT_STEP, center.y + slot * SPAWN_SLOT_STEP);
}

void GraphEditorPlg::noteRecentlyUsedNode(const char *template_uid)
{
  if (!template_uid || !template_uid[0])
  {
    return;
  }

  for (auto it = recentNodeUids.begin(); it != recentNodeUids.end(); ++it)
  {
    if (*it == template_uid)
    {
      recentNodeUids.erase(it);
      break;
    }
  }

  recentNodeUids.insert(recentNodeUids.begin(), eastl::string(template_uid));
  if (recentNodeUids.size() > RECENT_NODE_LIMIT)
  {
    recentNodeUids.resize(RECENT_NODE_LIMIT);
  }

  if (nodeLibraryPanel)
  {
    nodeLibraryPanel->onRecentNodesChanged();
  }
}

void GraphEditorPlg::invalidatePropertiesPanel()
{
  if (propertiesPanel)
  {
    propertiesPanel->invalidateControls();
  }
}

void GraphEditorPlg::markGraphDirtyAndRegen(const GraphDocument &doc)
{
  // constraint: no setGraphData here, it drops the preview and the pipeline caches on every edit.
  if (IGraphTexGenService *svc = graphPanel->texGenServiceFor(doc))
  {
    svc->markGraphDirty();
  }
}
