// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_wndGlobal.h>
#include <oldEditor/de_cm.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <imgui/imgui.h>

#include <graphEditor/graph_data.h>
#include <graphEditor/graph_subgraph_expand.h>

#include "base_nodes_panel.h"
#include "command_definitions.h"
#include "graph_compile.h"
#include "graph_panel.h"
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
  CM_SHOW_BASE_NODES,
  CM_SHOW_PROPERTIES,
  CM_NEW_GRAPH,
  CM_LOAD_GRAPH,
  CM_LOAD_GRAPH_BLK,
  CM_SAVE_GRAPH_BLK,
  CM_SAVE_AS_SUBGRAPH_BLK,
  CM_SAVE_TEXTURES,
  CM_SAVE_TEXTURES_BY_SUBSTRING,
  CM_RELOAD_TEXTURES,
  CM_FORCE_REBUILD,
};

enum
{
  GRAPH_PANEL_WTYPE = 160,
  TEXTURE_PREVIEW_PANEL_WTYPE = 161,
  HISTOGRAM_PANEL_WTYPE = 162,
  LANDSCAPE_PREVIEW_PANEL_WTYPE = 163,
  BASE_NODES_PANEL_WTYPE = 164,
  PROPERTIES_PANEL_WTYPE = 165,
};

enum
{
  PID_SAVE_TEXTURES_SUBSTRING = 11100,
};

static String last_save_textures_substring;

bool prompt_texture_substring(String &out_substring)
{
  PropPanel::DialogWindow *dlg = DAGORED2->createDialog(hdpi::_pxScaled(360), hdpi::_pxScaled(130), "Save textures by substring");

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

// Adapter installed into the texgen service. compile() runs on the worker
// thread; it takes the plugin's graphMutex for the full compile-and-commit so
// that source-of-truth graph state (gd.nodes / edges / properties) and the
// compiled output (gd.mainGraphBlk / shaderListBlk) are written under the same
// lock. The service does not commit the BLKs itself -- doing so under stateLock
// while main loaders write under graphMutex would race on the same memory under
// two different locks.
class GraphCompilerImpl final : public IGraphCompiler
{
public:
  explicit GraphCompilerImpl(GraphEditorPlg &p) : plugin(p) {}

  bool compile() override
  {
    bool ok = false;
    plugin.mutateGraphData([&](GraphData &gd) {
      // Compile into local BLKs first so a failure (e.g. iteration-limit hit)
      // leaves gd.mainGraphBlk / shaderListBlk untouched -- the service relies
      // on this to keep the previous compile's output usable on failure.
      DataBlock newMain;
      DataBlock newShader;
      eastl::vector<eastl::vector<eastl::string>> pendingNamesByIdx;

      // Inline-expand every subgraph instance into a local flat copy. The user-facing `gd`
      // keeps the instance nodes for display / editing; compile sees the spliced tree only.
      // Skip the deep copy when there are no instances to avoid the (cheap-ish but non-zero)
      // GraphData copy on the hot recompile path -- shader-only graphs compile every property
      // edit and shouldn't pay for a feature they don't use.
      bool hasSubgraphs = false;
      for (const GraphData::Node &n : gd.nodes)
      {
        if (n.plugin == "subgraph")
        {
          hasSubgraphs = true;
          break;
        }
      }
      const GraphData *compileGd = &gd;
      GraphData flat;
      if (hasSubgraphs)
      {
        flat = gd;
        expand_subgraphs(flat, plugin.getBaseNodesBlk(), plugin.getSubgraphsDir());
        compileGd = &flat;
      }

      ok = compile_graph_to_blks(*compileGd, plugin.getBaseNodesBlk(), plugin.getShaderIncludesDir(), newMain, newShader,
        pendingNamesByIdx);
      if (ok)
      {
        gd.mainGraphBlk.setFrom(&newMain);
        gd.shaderListBlk.setFrom(&newShader);

        // pendingNamesByIdx is parallel to compileGd->nodes (which may be the flat copy).
        // Keyed by id so the main-thread drain (applyPendingPinCustomTextureNames) can match
        // entries back to user-facing nodes -- spliced child nodes have ids that don't exist
        // in gd.nodes, and their entries are silently dropped by findNodeById there.
        eastl::vector<eastl::pair<int, eastl::vector<eastl::string>>> stashed;
        stashed.reserve(pendingNamesByIdx.size());
        for (int i = 0; i < static_cast<int>(pendingNamesByIdx.size()) && i < static_cast<int>(compileGd->nodes.size()); ++i)
        {
          if (pendingNamesByIdx[i].empty())
          {
            continue;
          }
          stashed.emplace_back(compileGd->nodes[i].id, eastl::move(pendingNamesByIdx[i]));
        }
        plugin.setPendingPinCustomTextureNames(eastl::move(stashed));
      }
    });

    if (!ok)
    {
      DAEDITOR3.conError("GraphEditor: graph compile hit iteration limit");
    }
    return ok;
  }

private:
  GraphEditorPlg &plugin;
};
} // namespace

GraphEditorPlg::GraphEditorPlg() {}

GraphEditorPlg::~GraphEditorPlg()
{
  if (graphCompiler)
  {
    if (IEditorService *svc = IDaEditor3Engine::get().findService("texgen"))
    {
      if (IGraphTexGenService *texgen = svc->queryInterface<IGraphTexGenService>())
      {
        texgen->setGraphCompiler(nullptr);
      }
    }
  }
}

void GraphEditorPlg::registered() {}

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
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_BASE_NODES, SHOW_BASE_NODES, "Show base nodes");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SHOW_PROPERTIES, SHOW_PROPERTIES, "Show properties");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_NEW_GRAPH, NEW_GRAPH, "New graph");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_LOAD_GRAPH, LOAD_GRAPH, "Load graph...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph (BLK)...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_GRAPH_BLK, SAVE_GRAPH_BLK, "Save graph (BLK)...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_AS_SUBGRAPH_BLK, SAVE_AS_SUBGRAPH_BLK, "Save as subgraph (BLK)...");
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

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_BASE_NODES, SHOW_BASE_NODES, "Show base nodes");
  tool->setButtonPictures(CM_SHOW_BASE_NODES, "show_panel");
  tool->setBool(CM_SHOW_BASE_NODES, isBaseNodesVisible);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_PROPERTIES, SHOW_PROPERTIES, "Show properties");
  tool->setButtonPictures(CM_SHOW_PROPERTIES, "show_panel");
  tool->setBool(CM_SHOW_PROPERTIES, isPropertiesVisible);

  commandSystem->createToolbarButton(*tool, CM_NEW_GRAPH, NEW_GRAPH, "New graph");
  tool->setButtonPictures(CM_NEW_GRAPH, "clear");

  commandSystem->createToolbarButton(*tool, CM_LOAD_GRAPH, LOAD_GRAPH, "Load graph...");
  tool->setButtonPictures(CM_LOAD_GRAPH, "obj_lib");

  commandSystem->createToolbarButton(*tool, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph (BLK)...");
  tool->setButtonPictures(CM_LOAD_GRAPH_BLK, "obj_lib");

  commandSystem->createToolbarButton(*tool, CM_SAVE_GRAPH_BLK, SAVE_GRAPH_BLK, "Save graph (BLK)...");
  tool->setButtonPictures(CM_SAVE_GRAPH_BLK, "save_mission");

  commandSystem->createToolbarButton(*tool, CM_SAVE_AS_SUBGRAPH_BLK, SAVE_AS_SUBGRAPH_BLK, "Save as subgraph (BLK)...");
  tool->setButtonPictures(CM_SAVE_AS_SUBGRAPH_BLK, "save_mission");

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

  if (isBaseNodesVisible)
  {
    EDITORCORE->addPropPanel(BASE_NODES_PANEL_WTYPE, hdpi::_pxScaled(280));
  }

  if (isPropertiesVisible)
  {
    EDITORCORE->addPropPanel(PROPERTIES_PANEL_WTYPE, hdpi::_pxScaled(320));
  }

  if (
    isGraphVisible || isTexturePreviewVisible || isHistogramVisible || isLandscapeVisible || isBaseNodesVisible || isPropertiesVisible)
  {
    EDITORCORE->managePropPanels();
  }

  return true;
}

bool GraphEditorPlg::end()
{
  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->unregisterWindowHandler(this);
  return true;
}

void GraphEditorPlg::onNewProject() {}

void GraphEditorPlg::clearObjects() {}

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

    if (!graphCompiler)
    {
      graphCompiler.reset(new GraphCompilerImpl(*this));
    }
    texGenService->setGraphCompiler(graphCompiler.get());
  }
}

void GraphEditorPlg::newEmptyGraph()
{
  // Resets graphData to clear_graph_data's factory state (no nodes, no edges, default
  // graph-level texture knobs, empty sourcePath). The mutate-lock keeps the texgen worker
  // from observing a half-cleared graph mid-clear. notifyGraphSourceChanged pushes the
  // reset through to the service and the canvas; graphPanel->onGraphDataChanged rebuilds
  // the canvas widget tree. Same shape as the post-load tail in loadGraphFromFile.
  // No dirty-tracking confirmation here -- the user explicitly asked to skip it for now.
  mutateGraphData([](GraphData &gd) { clear_graph_data(gd); });
  notifyGraphSourceChanged();
  if (graphPanel)
  {
    graphPanel->onGraphDataChanged();
  }
}

void GraphEditorPlg::promptAndLoadGraph()
{
  String picked = wingw::file_open_dlg(nullptr, "Select graph JSON to load", "Graph JSON (*.json)|*.json|All files (*.*)|*.*", "json",
    resourcePaths.mainGraphsDir);
  if (!picked.length())
  {
    return;
  }
  loadGraphFromFile(picked.str());
}

void GraphEditorPlg::loadGraphFromFile(const char *path)
{
  bool ok = false;
  mutateGraphData([&](GraphData &gd) { ok = load_graph_data(gd, path, resourcePaths.shaderIncludesDir, &getBaseNodesBlk()); });
  if (!ok)
  {
    return;
  }
  notifyGraphSourceChanged();
  if (graphPanel)
  {
    graphPanel->onGraphDataChanged();
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

  bool ok = false;
  mutateGraphData(
    [&](GraphData &gd) { ok = load_graph_data_blk(gd, picked.str(), resourcePaths.shaderIncludesDir, &getBaseNodesBlk()); });
  if (!ok)
  {
    return;
  }
  notifyGraphSourceChanged();
  if (graphPanel)
  {
    graphPanel->onGraphDataChanged();
  }
}

void GraphEditorPlg::promptAndSaveGraphBlk()
{
  // If we have a previously-loaded source path, pass it as the dialog seed so the user
  // gets the same filename pre-populated; otherwise fall back to the mainGraphs dir.
  String initPath = !graphData.sourcePath.empty() ? String(graphData.sourcePath.c_str()) : resourcePaths.mainGraphsDir;
  String picked = wingw::file_save_dlg(nullptr, "Save graph BLK", "Graph BLK (*.blk)|*.blk|All files (*.*)|*.*", "blk", initPath);
  if (!picked.length())
  {
    return;
  }

  if (save_graph_data_blk(graphData, picked.str()))
  {
    graphData.sourcePath = picked.str();
  }
  else
  {
    DAEDITOR3.conError("GraphEditor: failed to save graph BLK to %s", picked.str());
  }
}

void GraphEditorPlg::promptAndSaveAsSubgraphBlk()
{
  // Validate the current graph against the subgraph schema FIRST. Reject the save (no
  // file written) when any check fails: NO_BOUNDARIES, EMPTY_INTERFACE_NAME, or
  // DUPLICATE_NAME. The user gets one consolidated message box listing every problem,
  // not one per error -- chasing them down individually with a sequence of prompts is
  // worse UX than a flat list.
  eastl::vector<SubgraphSchemaError> errors;
  if (!validate_subgraph_schema(graphData, errors))
  {
    String text("Cannot save as subgraph -- schema invalid:");
    for (const SubgraphSchemaError &err : errors)
    {
      eastl::string line = format_subgraph_schema_error(err);
      text.aprintf(0, "\n  - %s", line.c_str());
      DAEDITOR3.conError("GraphEditor: subgraph schema -- %s", line.c_str());
    }
    wingw::message_box(wingw::MBS_OK | wingw::MBS_EXCL, "Save as subgraph", text);
    return;
  }

  // Soft warning: input boundaries whose data doesn't propagate to any output. The graph
  // is still saveable, but the parent will see input pins that have no effect on the
  // subgraph's outputs -- usually a forgotten wire inside the child. Show the names (or
  // type as fallback for any pin we couldn't name) and let the user confirm.
  eastl::vector<DeadInputBoundary> dead;
  find_dead_input_boundaries(graphData, dead);
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
    const int ret = wingw::message_box(wingw::MBS_YESNO | wingw::MBS_EXCL, "Save as subgraph", warning);
    if (ret != wingw::MB_ID_YES)
    {
      return;
    }
  }

  const String &subgraphsDir = resourcePaths.subgraphsDir;
  // Seed the dialog with the previous source path only if it already lives under the
  // subgraphs convention; otherwise drop to the subgraphsDir so the user doesn't end up
  // saving a subgraph back into mainGraphs/ by accident.
  String initPath = subgraphsDir;
  if (!graphData.sourcePath.empty())
  {
    const String existing(graphData.sourcePath.c_str());
    if (existing.suffix(".subgraph.blk"))
    {
      initPath = existing;
    }
  }
  String picked = wingw::file_save_dlg(nullptr, "Save as subgraph BLK",
    "Subgraph BLK (*.subgraph.blk)|*.subgraph.blk|All files (*.*)|*.*", "blk", initPath);
  if (!picked.length())
  {
    return;
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

  if (!save_graph_data_blk(graphData, finalPath.str()))
  {
    DAEDITOR3.conError("GraphEditor: failed to save subgraph BLK to %s", finalPath.str());
    return;
  }

  graphData.sourcePath = finalPath.str();
  DAEDITOR3.conNote("GraphEditor: saved subgraph to %s", finalPath.str());

  // Make the freshly-saved file appear in the Subgraphs palette without a restart.
  // reloadBaseNodes re-runs the lazy loader (re-scanning subgraphsDir) and rebuilds the
  // tree; the next time the user opens a different graph they can drop this subgraph in.
  reloadBaseNodes();
}

void GraphEditorPlg::selectAll() {}

void GraphEditorPlg::deselectAll() {}

void GraphEditorPlg::invertSelection() {}

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

  applyPendingPinCustomTextureNames();

  if (graphPanel)
  {
    graphPanel->actObjects(dt);
  }
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
  if (graphPanel)
  {
    PropPanel::PanelWindowPropertyControl *panelWindow = graphPanel->getPanelWindow();

    bool open = true;
    DAEDITOR3.imguiBegin(*panelWindow, &open);
    graphPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(graphPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (previewPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*previewPanel->getPanelWindow(), &open);
    previewPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(previewPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (histogramPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*histogramPanel->getPanelWindow(), &open);
    histogramPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(histogramPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (landscapePanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*landscapePanel->getPanelWindow(), &open);
    landscapePanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(landscapePanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  if (baseNodesPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*baseNodesPanel->getPanelWindow(), &open);
    baseNodesPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(baseNodesPanel.get());
      EDITORCORE->managePropPanels();
    }
  }

  // Properties panel last: it reads getSelectedNodeId() from graphPanel, which is refreshed
  // by graphPanel->updateImgui() above. Running last keeps the displayed properties in sync
  // with the ne::GetSelectedNodes value that graphPanel just captured.
  if (propertiesPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*propertiesPanel->getPanelWindow(), &open);
    propertiesPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
    {
      EDITORCORE->removePropPanel(propertiesPanel.get());
      EDITORCORE->managePropPanels();
    }
  }
}

void *GraphEditorPlg::queryInterfacePtr([[maybe_unused]] unsigned huid) { return nullptr; }

bool GraphEditorPlg::onPluginMenuClick([[maybe_unused]] unsigned id)
{
  switch (id)
  {
    case CM_SHOW_GRAPH:
    {
      if (!graphPanel)
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
    case CM_SHOW_BASE_NODES:
    {
      if (!baseNodesPanel)
      {
        EDITORCORE->addPropPanel(BASE_NODES_PANEL_WTYPE, hdpi::_pxScaled(280));
      }
      else
      {
        EDITORCORE->removePropPanel(baseNodesPanel.get());
      }

      EDITORCORE->managePropPanels();
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
    case CM_NEW_GRAPH:
    {
      newEmptyGraph();
      return true;
    }
    case CM_LOAD_GRAPH:
    {
      promptAndLoadGraph();
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
      promptAndSaveAsSubgraphBlk();
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

void GraphEditorPlg::registerEditorCommands(IEditorCommandSystem &command_system)
{
  command_system.addCommand(SHOW_GRAPH);
  command_system.addCommand(SHOW_TEXTURE_PREVIEW);
  command_system.addCommand(SHOW_HISTOGRAM);
  command_system.addCommand(SHOW_LANDSCAPE_PREVIEW);
  command_system.addCommand(SHOW_BASE_NODES);
  command_system.addCommand(SHOW_PROPERTIES);
  command_system.addCommand(NEW_GRAPH);
  command_system.addCommand(LOAD_GRAPH);
  command_system.addCommand(LOAD_GRAPH_BLK);
  command_system.addCommand(SAVE_GRAPH_BLK);
  command_system.addCommand(SAVE_AS_SUBGRAPH_BLK);
  command_system.addCommand(SAVE_TEXTURES);
  command_system.addCommand(SAVE_TEXTURES_BY_SUBSTRING);
  command_system.addCommand(RELOAD_TEXTURES);
  command_system.addCommand(FORCE_REBUILD);

  // Canvas-local shortcuts
  command_system.addCommand(CANVAS_DELETE_SELECTED, ImGuiKey_Delete);
  command_system.addCommand(CANVAS_FRAME_SELECTED, ImGuiKey_F);
  command_system.addCommand(CANVAS_FRAME_SELECTED_WITH_MARGIN, ImGuiMod_Shift | ImGuiKey_F);
  command_system.addCommand(CANVAS_COPY, ImGuiMod_Ctrl | ImGuiKey_C);
  command_system.addCommand(CANVAS_CUT, ImGuiMod_Ctrl | ImGuiKey_X);
  command_system.addCommand(CANVAS_PASTE, ImGuiMod_Ctrl | ImGuiKey_V);
}

void GraphEditorPlg::registerMenuAccelerators() {}

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
      if (graphPanel)
      {
        return nullptr;
      }

      graphPanel = eastl::make_unique<GraphPanel>(*this, texGenService, graphData);

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
    case BASE_NODES_PANEL_WTYPE:
    {
      if (baseNodesPanel)
      {
        return nullptr;
      }

      baseNodesPanel = eastl::make_unique<BaseNodesPanel>(*this);

      if (toolBar)
      {
        toolBar->setBool(CM_SHOW_BASE_NODES, true);
      }

      isBaseNodesVisible = true;
      return baseNodesPanel.get();
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
  }

  return nullptr;
}

bool GraphEditorPlg::onWmDestroyWindow(void *window)
{
  PropPanel::ContainerPropertyControl *toolBar = DAGORED2->getCustomPanel(toolBarId);

  if (graphPanel && window == graphPanel.get())
  {
    graphPanel.reset();
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

  if (baseNodesPanel && window == baseNodesPanel.get())
  {
    baseNodesPanel.reset();
    isBaseNodesVisible = false;

    if (toolBar)
    {
      toolBar->setBool(CM_SHOW_BASE_NODES, false);
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

  return false;
}

void GraphEditorPlg::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_SHOW_GRAPH:
    {
      if (!graphPanel)
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
    case CM_SHOW_BASE_NODES:
    {
      if (!baseNodesPanel)
      {
        EDITORCORE->addPropPanel(BASE_NODES_PANEL_WTYPE, hdpi::_pxScaled(280));
      }
      else
      {
        EDITORCORE->removePropPanel(baseNodesPanel.get());
      }

      EDITORCORE->managePropPanels();
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
    case CM_NEW_GRAPH:
    {
      newEmptyGraph();
      break;
    }
    case CM_LOAD_GRAPH:
    {
      promptAndLoadGraph();
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
      promptAndSaveAsSubgraphBlk();
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
  String graphEditorDataDir = ::make_full_path(sgg::get_common_data_dir(), "/graphEditor/");

  resourcePaths.defaultShadersPath = graphEditorDataDir + "default_shaders.blk";
  resourcePaths.defaultTexgenPath = graphEditorDataDir + "default_texgen.blk";
  resourcePaths.shaderIncludesDir = graphEditorDataDir + "generated/";
  resourcePaths.mainGraphsDir = ::make_full_path(sgg::get_exe_path_full(), "../../graphEditor/mainGraphs/");
  resourcePaths.subgraphsDir = graphEditorDataDir + "subgraphs/";
}

void GraphEditorPlg::appendShaderTemplatesToBaseNodes()
{
  String shadersDir = ::make_full_path(sgg::get_common_data_dir(), "/graphEditor/shaders/");
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

    // Synthesise a node{} descriptor in baseNodesBlk so the base-nodes panel /
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
  // baseNodesBlk per file so the BaseNodesPanel can list them under category "Subgraphs" and
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

  struct Boundary
  {
    eastl::string ifaceName;
    eastl::string types; // CSV form, matches base_nodes.blk's `types:t` schema
    bool isInput;        // mirrors subgraph in:TYPE boundary (true) vs subgraph out (false)
  };

  auto processFile = [&](const String &filePath, const char *fileName) {
    // dd_get_fname_without_path_and_ext strips only the LAST extension and writes the
    // result into `buf`. If passed a full path the buffer still contains the directory
    // components (the function returns a pointer past the last separator -- but we use
    // `buf` directly here, so we MUST pass just the filename, not the full path). For
    // "foo.subgraph.blk" the buffer comes back as "foo.subgraph"; the trailing
    // ".subgraph" then needs to be stripped so the synthesised templateUid / descName
    // (and the child-file resolver in the expander) are the plain stem "foo".
    char stem[128] = {};
    dd_get_fname_without_path_and_ext(stem, sizeof(stem), fileName);
    if (!stem[0])
    {
      return;
    }
    {
      constexpr const char *SUBGRAPH_SUFFIX = ".subgraph";
      const size_t stemLen = strlen(stem);
      const size_t suffixLen = strlen(SUBGRAPH_SUFFIX);
      if (stemLen > suffixLen && strcmp(stem + stemLen - suffixLen, SUBGRAPH_SUFFIX) == 0)
      {
        stem[stemLen - suffixLen] = 0;
      }
    }

    GraphData childGd;
    // Pass nullptr for base_nodes_blk -- pin resolution against the descriptor registry isn't
    // needed for boundary scanning (descName + propertyValues come straight from the file).
    // Avoids re-entering reconcile_template_uid mid-synthesis while baseNodesByUid is still
    // empty.
    if (!load_graph_data_blk(childGd, filePath.str(), resourcePaths.shaderIncludesDir, nullptr))
    {
      DAEDITOR3.conError("GraphEditor: failed to parse subgraph %s", filePath.str());
      return;
    }

    // Schema check first -- log every issue to the editor console so the user can see all
    // problems with a subgraph at startup, not silently. Any error (DUPLICATE_NAME,
    // EMPTY_INTERFACE_NAME, or NO_BOUNDARIES) blocks the descriptor from entering the
    // palette: an invalid subgraph can't safely be inlined at compile time, so showing it
    // as a draggable entry would just defer the failure to a worse moment.
    {
      eastl::vector<SubgraphSchemaError> schemaErrors;
      const bool schemaOk = validate_subgraph_schema(childGd, schemaErrors);
      for (const SubgraphSchemaError &err : schemaErrors)
      {
        const eastl::string msg = format_subgraph_schema_error(err);
        if (err.code == SubgraphSchemaError::NO_BOUNDARIES)
        {
          DAEDITOR3.conWarning("GraphEditor: subgraph %s: %s", filePath.str(), msg.c_str());
        }
        else
        {
          DAEDITOR3.conError("GraphEditor: subgraph %s: %s", filePath.str(), msg.c_str());
        }
      }
      if (!schemaOk)
      {
        return;
      }
    }

    // Dead-input warnings -- never blocks (the subgraph still works for its other pins),
    // but the author should know which inputs are wired into nothing useful.
    {
      eastl::vector<DeadInputBoundary> dead;
      find_dead_input_boundaries(childGd, dead);
      for (const DeadInputBoundary &d : dead)
      {
        const char *label = !d.interfaceName.empty() ? d.interfaceName.c_str() : (!d.type.empty() ? d.type.c_str() : "<unnamed>");
        DAEDITOR3.conWarning("GraphEditor: subgraph %s input '%s' (id=%d) does not propagate to any output", filePath.str(), label,
          d.nodeId);
      }
    }

    // Boundary collection. Validation above guarantees every boundary has a non-empty
    // effective name and no two boundaries collide on (role, name), so this loop just
    // packs the descriptor inputs in load order (already y-sorted by the converter).
    eastl::vector<Boundary> boundaries;
    for (const GraphData::Node &n : childGd.nodes)
    {
      const char *dn = n.descName.c_str();
      const bool isInBoundary = strncmp(dn, "subgraph in:", 12) == 0;
      const bool isOutBoundary = strcmp(dn, "subgraph out") == 0;
      if (!isInBoundary && !isOutBoundary)
      {
        continue;
      }

      Boundary b;
      b.ifaceName = effective_subgraph_boundary_name(childGd, n.id);
      b.isInput = isInBoundary;
      if (isInBoundary)
      {
        // descName format: "subgraph in: <type>". Slice past the colon + spaces.
        const char *colon = strchr(dn, ':');
        const char *t = colon ? colon + 1 : "";
        while (*t == ' ')
        {
          ++t;
        }
        b.types = t;
      }
      else
      {
        // For output boundaries we keep the multi-type CSV from base_nodes.blk so the instance
        // pin accepts whatever the parent connects. Compile-time expansion resolves the actual
        // single type from the child's upstream producer and type-checks against the parent
        // edge -- so a per-instance type narrowing here would only affect editor pin colouring,
        // not correctness, and isn't worth the upstream-edge scan at synthesis time.
        b.types = "texture2D,particles,bool,int,float,float2,float3,float4";
      }
      boundaries.push_back(eastl::move(b));
    }

    DataBlock *nodeBlk = baseNodesBlk.addNewBlock("node");
    eastl::string templateUid("subgraph::");
    templateUid.append(stem);
    nodeBlk->setStr("templateUid", templateUid.c_str());
    nodeBlk->setStr("name", stem);
    nodeBlk->setStr("category", "Subgraphs");
    // `plugin:t = "subgraph"` is the marker the compile-time expander uses to identify instance
    // nodes (graph_subgraph_expand.cpp). It mirrors the shader_editor convention -- a marker
    // routed through Node::plugin rather than a templateUid string-prefix check.
    nodeBlk->setStr("plugin", "subgraph");
    nodeBlk->setBool("generated", true);
    nodeBlk->setInt("width", 200);

    // Inputs first then outputs for canonical layout. The boundaries vector already holds
    // entries in load order (which the converter emits sorted by the boundary node's view.y
    // -- matching what the user authored visually); the only re-ordering this loop applies
    // is grouping inputs ahead of outputs.
    auto emitPin = [&](const Boundary &b) {
      DataBlock *pinBlk = nodeBlk->addNewBlock("pin");
      pinBlk->setStr("name", b.ifaceName.c_str());
      pinBlk->setStr("types", b.types.c_str());
      pinBlk->setStr("role", b.isInput ? "in" : "out");
      // singleConnect on instance inputs mirrors the boundary's single-driver semantics:
      // the `subgraph in: TYPE` boundary inside the child accepts exactly one source, so a
      // multi-source parent edge would have nowhere to splice. resolve_node_pins's default
      // for inputs is multi-connect, so set explicitly. Outputs default to single-connect.
      pinBlk->setBool("singleConnect", true);
    };
    for (const Boundary &b : boundaries)
    {
      if (b.isInput)
      {
        emitPin(b);
      }
    }
    for (const Boundary &b : boundaries)
    {
      if (!b.isInput)
      {
        emitPin(b);
      }
    }
  };

  // Scan `*.subgraph.blk` under subgraphsDir. dd_find_iterator's glob mask is shell-style
  // "*", and the double-dot filename "<stem>.subgraph.blk" still matches the single "*"
  // wildcard because the "*" portion absorbs "<stem>.subgraph".
  int loaded = 0;
  String mask(0, "%s*.subgraph.blk", subgraphsDir.str());
  for (const alefind_t &fs : dd_find_iterator(mask, DA_FILE))
  {
    String filePath(0, "%s%s", subgraphsDir.str(), fs.name);
    processFile(filePath, fs.name);
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
  baseNodesBlkLoaded = true;
  String path = ::make_full_path(sgg::get_common_data_dir(), "/graphEditor/base_nodes.blk");
  if (!baseNodesBlk.load(path))
  {
    DAEDITOR3.conError("GraphEditor: failed to load %s", path.str());
    return false;
  }
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
    const DataBlock *node = baseNodesBlk.getBlock(i);
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
  baseNodesByUid.clear();
  baseNodesBlk.reset();
  baseNodesBlkLoaded = false;
  loadBaseNodesBlkIfNeeded();
  if (baseNodesPanel)
  {
    baseNodesPanel->refresh();
  }
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

  out = GraphData::Node{};
  out.x = x;
  out.y = y;
  // Both fields come from the descriptor: uid is the stable key; descName is captured
  // at spawn time but will be refreshed from the descriptor on every subsequent load.
  out.templateUid = template_uid;
  out.descName = desc->getStr("name", "");
  out.plugin = desc->getStr("plugin", "");

  // Pin stubs in descriptor order. Edges (which reference pins by index) will line up with
  // the descriptor naturally for fresh nodes; for loaded nodes the loader preserves JSON
  // pin order instead. customTextureName stays empty here. Resolved-cache fields (type /
  // isInput / hidden) are filled in a separate pass below via resolve_node_pins so the
  // logic stays in one place.
  for (uint32_t i = 0; i < desc->blockCount(); ++i)
  {
    const DataBlock *child = desc->getBlock(i);
    if (strcmp(child->getBlockName(), "pin") != 0)
    {
      continue;
    }
    GraphData::Pin pin;
    pin.name = child->getStr("name", "");
    out.pins.emplace_back(eastl::move(pin));
  }
  resolve_node_pins(out, &baseNodesBlk);

  // Default property values from descriptor `val` field. Descriptor stores typed scalars
  // (val:i / val:r / val:b / val:p4 / val:t) -- normalize to a string here.
  for (uint32_t i = 0; i < desc->blockCount(); ++i)
  {
    const DataBlock *child = desc->getBlock(i);
    if (strcmp(child->getBlockName(), "property") != 0)
    {
      continue;
    }
    eastl::string name = child->getStr("name", "");
    if (name.empty())
    {
      continue;
    }
    out.propertyValues.emplace_back(eastl::move(name), param_as_string(child, "val"));
  }

  return true;
}

void GraphEditorPlg::spawnBaseNode(const char *template_uid, float x, float y)
{
  if (!graphPanel)
  {
    return;
  }
  GraphData::Node n;
  if (!makeNodeFromBaseBlk(template_uid, x, y, n))
  {
    DAEDITOR3.conWarning("GraphEditor: drag-drop unknown base-node uid '%s'", template_uid ? template_uid : "");
    return;
  }
  n.id = graphPanel->allocateNodeId();
  graphPanel->addNode(eastl::move(n)); // already locks via mutateGraphData internally
  markGraphDirtyAndRegen();
}

void GraphEditorPlg::markGraphDirtyAndRegen()
{
  if (!texGenService)
  {
    return;
  }
  // Cheap per-edit path: just kick the worker. The service's worker compiles
  // the graph and, on successful commit, sets shouldGenerateTex + generateOnce
  // so the regen pushes through even when the user has autoupdate disabled --
  // matching the old `svc->requestRegenerate()` semantics that edit-side
  // callers used before they migrated to this entrypoint.
  // Crucially does NOT call setGraphData -- that wipes preview-final state and
  // pipeline caches, which is correct for a load but disastrous mid-drag
  // (drops the preview every slider tick + forces a full pipeline reset every
  // frame).
  texGenService->markGraphDirty();
}

void GraphEditorPlg::notifyGraphSourceChanged()
{
  if (!texGenService)
  {
    return;
  }
  texGenService->setHeightmapParams(graphData.heightmapScale, graphData.heightmapMin, graphData.heightmapCellSize);
  texGenService->setGraphData(&graphData);
  texGenService->markGraphDirty();
}

void GraphEditorPlg::applyPendingPinCustomTextureNames()
{
  // Skip the lock when there's nothing to apply. Reading the size without the
  // mutex is safe-ish (worst case we miss a just-arrived batch and pick it up
  // next tick), and lets the typical no-pending-work tick avoid the
  // WinCritSec round-trip entirely.
  if (pendingPinCustomTextureNames.empty())
  {
    return;
  }
  mutateGraphData([&](GraphData &gd) {
    if (pendingPinCustomTextureNames.empty())
    {
      return;
    }
    eastl::hash_map<int, int> idToIdx;
    idToIdx.reserve(gd.nodes.size());
    for (int i = 0; i < static_cast<int>(gd.nodes.size()); ++i)
    {
      idToIdx[gd.nodes[i].id] = i;
    }
    for (auto &entry : pendingPinCustomTextureNames)
    {
      const auto it = idToIdx.find(entry.first);
      if (it == idToIdx.end())
      {
        continue; // node deleted between compile and drain
      }
      GraphData::Node &n = gd.nodes[it->second];
      const int count = static_cast<int>(eastl::min(entry.second.size(), n.pins.size()));
      for (int j = 0; j < count; ++j)
      {
        n.pins[j].customTextureName = eastl::move(entry.second[j]);
      }
    }
    pendingPinCustomTextureNames.clear();
  });
}
