// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_wndGlobal.h>
#include <oldEditor/de_cm.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <libTools/util/strUtil.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <imgui/imgui.h>

#include <graphEditor/graph_data.h>

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
  CM_LOAD_GRAPH,
  CM_LOAD_GRAPH_BLK,
  CM_SAVE_GRAPH_BLK,
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

static bool prompt_texture_substring(String &out_substring)
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
      ok = compile_graph_to_blks(gd, plugin.getBaseNodesBlk(), plugin.getShaderIncludesDir(), newMain, newShader);
      if (ok)
      {
        gd.mainGraphBlk.setFrom(&newMain);
        gd.shaderListBlk.setFrom(&newShader);
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
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_LOAD_GRAPH, LOAD_GRAPH, "Load graph...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph (BLK)...");
  commandSystem->addMenuItem(*mainMenu, menu_id, CM_SAVE_GRAPH_BLK, SAVE_GRAPH_BLK, "Save graph (BLK)...");
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

  commandSystem->createToolbarButton(*tool, CM_LOAD_GRAPH, LOAD_GRAPH, "Load graph...");
  tool->setButtonPictures(CM_LOAD_GRAPH, "obj_lib");

  commandSystem->createToolbarButton(*tool, CM_LOAD_GRAPH_BLK, LOAD_GRAPH_BLK, "Load graph (BLK)...");
  tool->setButtonPictures(CM_LOAD_GRAPH_BLK, "obj_lib");

  commandSystem->createToolbarButton(*tool, CM_SAVE_GRAPH_BLK, SAVE_GRAPH_BLK, "Save graph (BLK)...");
  tool->setButtonPictures(CM_SAVE_GRAPH_BLK, "save_mission");

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

void GraphEditorPlg::promptAndLoadGraph()
{
  String mainGraphsDir = ::make_full_path(sgg::get_exe_path_full(), "../../graphEditor/mainGraphs");
  String picked = wingw::file_open_dlg(nullptr, "Select graph JSON to load", "Graph JSON (*.json)|*.json|All files (*.*)|*.*", "json",
    mainGraphsDir);
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
  String mainGraphsDir = ::make_full_path(sgg::get_exe_path_full(), "../../graphEditor/mainGraphs");
  String picked =
    wingw::file_open_dlg(nullptr, "Select graph BLK to load", "Graph BLK (*.blk)|*.blk|All files (*.*)|*.*", "blk", mainGraphsDir);
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
  String mainGraphsDir = ::make_full_path(sgg::get_exe_path_full(), "../../graphEditor/mainGraphs");
  // If we have a previously-loaded source path, pass it as the dialog seed so the user
  // gets the same filename pre-populated; otherwise fall back to the mainGraphs dir.
  String initPath = !graphData.sourcePath.empty() ? String(graphData.sourcePath.c_str()) : mainGraphsDir;
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
  command_system.addCommand(LOAD_GRAPH);
  command_system.addCommand(LOAD_GRAPH_BLK);
  command_system.addCommand(SAVE_GRAPH_BLK);
  command_system.addCommand(SAVE_TEXTURES);
  command_system.addCommand(SAVE_TEXTURES_BY_SUBSTRING);
  command_system.addCommand(RELOAD_TEXTURES);
  command_system.addCommand(FORCE_REBUILD);

  // Canvas-local shortcuts
  command_system.addCommand(CANVAS_DELETE_SELECTED, ImGuiKey_Delete);
  command_system.addCommand(CANVAS_FRAME_SELECTED, ImGuiKey_F);
  command_system.addCommand(CANVAS_FRAME_SELECTED_WITH_MARGIN, ImGuiMod_Shift | ImGuiKey_F);
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
    eastl::string val;
    int paramIdx = child->findParam("val");
    if (paramIdx >= 0)
    {
      const int t = child->getParamType(paramIdx);
      char buf[128] = {};
      switch (t)
      {
        case DataBlock::TYPE_STRING:
        {
          val = child->getStr(paramIdx);
          break;
        }
        case DataBlock::TYPE_INT:
        {
          _snprintf(buf, sizeof(buf), "%d", child->getInt(paramIdx));
          val = buf;
          break;
        }
        case DataBlock::TYPE_REAL:
        {
          _snprintf(buf, sizeof(buf), "%g", child->getReal(paramIdx));
          val = buf;
          break;
        }
        case DataBlock::TYPE_BOOL:
        {
          val = child->getBool(paramIdx) ? "true" : "false";
          break;
        }
        case DataBlock::TYPE_POINT4:
        {
          const Point4 p = child->getPoint4(paramIdx);
          _snprintf(buf, sizeof(buf), "%g,%g,%g,%g", p.x, p.y, p.z, p.w);
          val = buf;
          break;
        }
        default: break;
      }
    }
    out.propertyValues.emplace_back(eastl::move(name), eastl::move(val));
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
