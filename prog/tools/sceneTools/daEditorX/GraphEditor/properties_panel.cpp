// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <oldEditor/de_workspace.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/strUtil.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/container.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include "properties_panel.h"
#include "curve_preview_control.h"
#include "graph_curve_value.h"
#include "graph_document.h"
#include "graph_gradient_value.h"
#include "graph_panel.h"
#include "graph_tab.h"
#include "plugin.h"
#include "pluginService/graph_tex_gen_service.h"

namespace
{
enum
{
  PID_GRAPH_BASE = 13000,
  PID_GRAPH_TEX_WRAP = PID_GRAPH_BASE + 0,
  PID_GRAPH_RENDER_DIR = PID_GRAPH_BASE + 2,
  PID_GRAPH_ENTITY_DIR = PID_GRAPH_BASE + 3,
  PID_GRAPH_HEIGHT_SCALE = PID_GRAPH_BASE + 4,
  PID_GRAPH_HEIGHT_MIN = PID_GRAPH_BASE + 5,
  PID_GRAPH_CELL_SIZE = PID_GRAPH_BASE + 6,
  PID_GRAPH_TEX_WIDTH = PID_GRAPH_BASE + 7,
  PID_GRAPH_TEX_HEIGHT = PID_GRAPH_BASE + 8,
  PID_GRAPH_TEX_DEPTH = PID_GRAPH_BASE + 9,
  PID_GRAPH_TEX_TYPE = PID_GRAPH_BASE + 10,
  PID_SUBGRAPH_INPUTS = PID_GRAPH_BASE + 11,
  PID_SUBGRAPH_OUTPUTS = PID_GRAPH_BASE + 12,
  PID_SUBGRAPH_TEXTURES = PID_GRAPH_BASE + 13,
  PID_SUBGRAPH_BOUNDARY = PID_GRAPH_BASE + 14, // every row of both lists: a static is never looked up

  PID_NODE_HEADER = 13100,
  PID_NODE_ID = 13101,
  PID_NODE_PLUGIN = 13102,
  PID_NODE_DEFINITION = 13103,
  PID_NODE_DEF_TYPE = 13104,
  PID_NODE_DEF_LOCATION = 13105,
  PID_NODE_GENERAL = 13106,
  PID_NODE_TEXTURES = 13107,

  // A block of PIDs per property, so a type can add sub-controls without colliding with the next.
  PID_NODE_PROP_BASE = 13200,
  PID_NODE_PROP_STRIDE = 8,
  PROP_SLOT_AUX = 1,
};

// The block base of the property a control belongs to. Only valid for pid >= PID_NODE_PROP_BASE.
int prop_pid_base(int pid) { return PID_NODE_PROP_BASE + ((pid - PID_NODE_PROP_BASE) / PID_NODE_PROP_STRIDE) * PID_NODE_PROP_STRIDE; }

constexpr int HEIGHT_FLOAT_PREC = 4;

// The Textures group holds these and shows them without the prefix.
constexpr const char *TEXTURE_SETTINGS[] = {"texture width", "texture height", "texture depth", "texture type", "texture wrap"};
constexpr size_t TEXTURE_SETTING_PREFIX_LEN = sizeof("texture ") - 1;

const char *texture_setting_caption(const char *prop_name)
{
  for (const char *name : TEXTURE_SETTINGS)
  {
    if (!strcmp(name, prop_name))
    {
      return prop_name + TEXTURE_SETTING_PREFIX_LEN;
    }
  }
  return nullptr;
}

bool parse_bool(const char *s)
{
  if (!s || !*s)
  {
    return false;
  }
  return !strcmp(s, "true") || !strcmp(s, "1") || !strcmp(s, "yes");
}

const char *value_or(const eastl::string &s, const char *fallback) { return s.empty() ? fallback : s.c_str(); }

// True (and out_rel set to the app-relative form) if `text` resolves to a location inside the
// application directory. The graph .blk stores these dirs app-relative, so anything that escapes
// appDir is rejected: a different drive (make_path_relative keeps it absolute) or a parent/sibling
// (a leading "..").
bool resolve_under_app_dir(const char *text, String &out_rel)
{
  const char *appDir = DAGORED2->getWorkspace().getAppDir();
  if (!appDir || !*appDir)
  {
    return false;
  }
  const String full = ::make_full_path(appDir, text); // joins; returns text unchanged if it is absolute
  // appDir itself -> project root, stored as "". Handled up front because make_path_relative cannot
  // relativize an equal-length path on Linux (it falls back to the absolute path, which would be
  // rejected below). dd_fname_equal simplifies both sides and compares case-insensitively.
  if (::dd_fname_equal(full.str(), appDir))
  {
    out_rel = "";
    return true;
  }
  out_rel = ::make_path_relative(full.str(), appDir);
  // make_path_relative routes its result through make_good_path, which re-prefixes a slash-less
  // relative path with "./"; simplify_fname strips that so the stored value is canonical ("develop",
  // not "./develop") and a bare parent reads as ".." rather than "./..".
  ::simplify_fname(out_rel);
  // A pick on a different drive leaves the result absolute; a parent/sibling leaves a leading ".."
  // segment. Both escape appDir, so reject. PATH_DELIM is '/' on every platform; "..foo" is kept.
  if (out_rel.empty() || ::is_full_path(out_rel.str()))
  {
    return false;
  }
  const char *s = out_rel.str();
  if (s[0] == '.' && s[1] == '.' && (s[2] == '/' || s[2] == 0))
  {
    return false;
  }
  return true;
}

// File-picker base dir for a `filepath` node property. Its optional `root` hint names the dir it lives
// under: "app" (default) -> app dir; "renderDir"/"entityDir" -> that output dir (defaulting to
// "render"/"entity" like the texgen service); "none" -> no base (stored verbatim, e.g. font_path).
String filepath_base(const DataBlock *prop_desc, const GraphData &gd)
{
  const char *appDir = DAGORED2->getWorkspace().getAppDir();
  const char *root = prop_desc ? prop_desc->getStr("root", "app") : "app";
  if (!strcmp(root, "none"))
  {
    return String();
  }
  if (!strcmp(root, "renderDir"))
  {
    return ::make_full_path(appDir, gd.renderDir.empty() ? "render" : gd.renderDir.c_str());
  }
  if (!strcmp(root, "entityDir"))
  {
    return ::make_full_path(appDir, gd.entityDir.empty() ? "entity" : gd.entityDir.c_str());
  }
  return String(appDir);
}

// Lenient relativization for a `filepath` value: make `text` relative to `base` when it resolves under
// (or near) it, otherwise keep it as-is. Unlike resolve_under_app_dir this never rejects -- a
// parent/sibling stays a ".." path (still resolves once re-joined onto base), a different drive stays
// absolute, and an empty base means "store verbatim".
String relativize_under(const char *base, const char *text)
{
  if (!base || !*base || !text || !*text)
  {
    return String(text);
  }
  const String full = ::make_full_path(base, text);
  String rel = ::make_path_relative(full.str(), base);
  ::simplify_fname(rel);
  return rel.empty() ? String(text) : rel;
}

// Parse "r,g,b" or "r,g,b,a" with components in [0,1] into an E3DCOLOR. Defaults to opaque
// black on parse failure -- matches the createColorBox documented default.
E3DCOLOR parse_color(const char *s)
{
  float r = 0, g = 0, b = 0, a = 1;
  if (s && *s)
  {
    const int got = sscanf(s, "%f,%f,%f,%f", &r, &g, &b, &a);
    if (got < 3)
    {
      r = g = b = 0;
      a = 1;
    }
  }
  auto clamp01 = [](float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
  return E3DCOLOR_MAKE((unsigned char)(clamp01(r) * 255.f + 0.5f), (unsigned char)(clamp01(g) * 255.f + 0.5f),
    (unsigned char)(clamp01(b) * 255.f + 0.5f), (unsigned char)(clamp01(a) * 255.f + 0.5f));
}

E3DCOLOR gradient_stop_color(const GradientStop &s)
{
  return E3DCOLOR_MAKE(gradient_color_byte(s.r), gradient_color_byte(s.g), gradient_color_byte(s.b), gradient_color_byte(s.a));
}

void gradient_stops_to_keys(const eastl::vector<GradientStop> &stops, PropPanel::Gradient &out_keys)
{
  clear_and_shrink(out_keys);
  for (const GradientStop &s : stops)
  {
    out_keys.push_back(PropPanel::GradientKey(s.t, gradient_stop_color(s)));
  }
}

void gradient_keys_to_stops(const PropPanel::Gradient &keys, eastl::vector<GradientStop> &out_stops)
{
  out_stops.clear();
  out_stops.reserve(keys.size());
  for (int i = 0; i < keys.size(); ++i)
  {
    GradientStop s;
    s.t = keys[i].position;
    s.r = keys[i].color.r / 255.f;
    s.g = keys[i].color.g / 255.f;
    s.b = keys[i].color.b / 255.f;
    s.a = keys[i].color.a / 255.f;
    out_stops.push_back(s);
  }
}

// The descriptor's `style` names the stroke colour the JS editor draws the curve with.
E3DCOLOR curve_style_color(const char *style)
{
  if (!strcmp(style, "red"))
  {
    return E3DCOLOR(255, 80, 80);
  }
  if (!strcmp(style, "green"))
  {
    return E3DCOLOR(80, 255, 80);
  }
  if (!strcmp(style, "blue"))
  {
    return E3DCOLOR(80, 160, 255);
  }
  return E3DCOLOR(200, 200, 200);
}

template <size_t N>
int build_combo_items(Tab<String> &out_items, const char *current, const char *const (&src)[N], const char *strip_prefix = nullptr)
{
  out_items.clear();
  const size_t strip_len = strip_prefix ? strlen(strip_prefix) : 0;
  int selectedIdx = 0;
  for (size_t i = 0; i < N; ++i)
  {
    const char *s = src[i];
    if (strip_len > 0 && strncmp(s, strip_prefix, strip_len) == 0)
    {
      s += strip_len;
    }
    if (current && strcmp(current, s) == 0)
    {
      selectedIdx = (int)out_items.size();
    }
    out_items.push_back(String(s));
  }
  return selectedIdx;
}
} // namespace

PropertiesPanel::PropertiesPanel(GraphEditorPlg &plg) : plugin(plg)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
}

PropertiesPanel::~PropertiesPanel()
{
  editGesture.end();
  IEditorCoreEngine::get()->deleteCustomPanel(panelWindow);
}

bool PropertiesPanel::shouldWarnOnce(int node_id, const char *prop_name)
{
  return warnedProperties.insert(eastl::string(String(0, "%d:%s", node_id, prop_name))).second;
}

const GraphData::Node *PropertiesPanel::findNodeById(int id) const
{
  if (id < 0 || !doc)
  {
    return nullptr;
  }
  return find_node_by_id(doc->getGraphData(), id);
}

void PropertiesPanel::commitPendingEdit()
{
  IEditorCoreEngine::get()->sendImmediateFocusLossNotification();
  editGesture.end();
}

void PropertiesPanel::dropControls()
{
  // A destroyed control sends no change finish, so the open edit lands before they go.
  commitPendingEdit();
  if (currentMode == Mode::Subgraph)
  {
    inputsCollapsed = panelWindow->getBool(PID_SUBGRAPH_INPUTS);
    outputsCollapsed = panelWindow->getBool(PID_SUBGRAPH_OUTPUTS);
    texturesCollapsed = panelWindow->getBool(PID_SUBGRAPH_TEXTURES);
  }
  panelWindow->clear();
  pidToPropertyName.clear();
  curvePreviews.clear();
}

void PropertiesPanel::detachFromDocument(const GraphDocument &closing)
{
  if (doc != &closing)
  {
    return;
  }
  dropControls();
  doc = nullptr;
  currentMode = Mode::None;
  lastRenderedNodeId = -1;
  lastRenderedTabId = -1;
}

void PropertiesPanel::updateImgui()
{
  if (!panelWindow)
  {
    return;
  }

  GraphPanel &graphPanel = plugin.getGraphPanel();
  // Taken before anything reads it. On the frame a tab switch lands, the document left behind would
  // otherwise answer for the selected node and the values the controls are built from, and the next
  // edit would write them into the graph now shown.
  doc = &plugin.getActiveDocument();
  const int selId = graphPanel.getSelectedNodeId();
  const bool tabChanged = graphPanel.activeTabId() != lastRenderedTabId;
  // A node selected anew, or found selected on the graph or window switched to, shows its properties.
  if (selId >= 0 && (selId != lastSeenSelectedId || tabChanged))
  {
    graphView = false;
  }
  lastSeenSelectedId = selId;

  // Resolve target mode. A selected id that no longer maps to a live node (deleted while
  // selected) collapses to the graph-level view -- safer than rendering stale fields.
  Mode targetMode = graphPanel.activeSurface().getHostWindow() != nullptr ? Mode::Subgraph : Mode::Graph;
  int targetNodeId = -1;
  if (selId >= 0 && !graphView)
  {
    if (findNodeById(selId))
    {
      targetMode = Mode::Node;
      targetNodeId = selId;
    }
  }

  // Cheap path: nothing relevant has changed -> just pump the panel and return.
  bool needRebuild = (targetMode != currentMode) || tabChanged;
  if (!needRebuild && targetMode == Mode::Node)
  {
    needRebuild = (targetNodeId != lastRenderedNodeId);
  }
  if (!needRebuild && targetMode == Mode::Subgraph)
  {
    needRebuild = doc->getStructureRevision() != lastRenderedRevision || doc->getPositionsStamp() != lastRenderedPositionsStamp;
  }
  if (forceRebuild)
  {
    needRebuild = true;
  }

  if (needRebuild)
  {
    forceRebuild = false;
    dropControls();
    if (tabChanged) // node ids restart per graph, so a stale key would mute a real warning
    {
      warnedProperties.clear();
    }

    if (targetMode == Mode::Node)
    {
      rebuildForNode(targetNodeId);
    }
    else if (targetMode == Mode::Subgraph)
    {
      rebuildForSubgraph();
    }
    else
    {
      rebuildForGraph();
    }

    currentMode = targetMode;
    lastRenderedNodeId = targetNodeId;
    lastRenderedTabId = graphPanel.activeTabId();
    lastRenderedRevision = doc->getStructureRevision();
    lastRenderedPositionsStamp = doc->getPositionsStamp();
  }

  panelWindow->updateImgui();
}

void PropertiesPanel::rebuildForGraph()
{
  if (!panelWindow)
  {
    return;
  }

  const GraphData &gd = doc->getGraphData();

  const char *appDir = DAGORED2->getWorkspace().getAppDir();
  panelWindow->createFileEditBox(PID_GRAPH_RENDER_DIR, "Render dir", gd.renderDir.c_str());
  panelWindow->setUserData(PID_GRAPH_RENDER_DIR, appDir);
  panelWindow->setInt(PID_GRAPH_RENDER_DIR, PropPanel::FS_DIALOG_DIRECTORY);

  panelWindow->createFileEditBox(PID_GRAPH_ENTITY_DIR, "Entity dir", gd.entityDir.c_str());
  panelWindow->setUserData(PID_GRAPH_ENTITY_DIR, appDir);
  panelWindow->setInt(PID_GRAPH_ENTITY_DIR, PropPanel::FS_DIALOG_DIRECTORY);

  panelWindow->createSeparator();

  panelWindow->createEditFloat(PID_GRAPH_HEIGHT_SCALE, "Height scale", effective_height(gd.heightmapScale, DEFAULT_HEIGHT_SCALE),
    HEIGHT_FLOAT_PREC);
  panelWindow->createEditFloat(PID_GRAPH_HEIGHT_MIN, "Height min", effective_height(gd.heightmapMin, DEFAULT_HEIGHT_MIN),
    HEIGHT_FLOAT_PREC);
  panelWindow->createEditFloat(PID_GRAPH_CELL_SIZE, "Cell size", effective_height(gd.heightmapCellSize, DEFAULT_CELL_SIZE),
    HEIGHT_FLOAT_PREC);

  panelWindow->createSeparator();
  createTextureCombos(*panelWindow, /*in_group=*/false);
}

// The compile takes the main graph's dirs and heightmap, so a subgraph lists its pins.
void PropertiesPanel::rebuildForSubgraph()
{
  const GraphData &gd = doc->getGraphData();
  eastl::vector<const GraphData::Node *> boundaries;
  collect_subgraph_boundaries(gd, /*with_separators=*/false, boundaries);
  PropPanel::ContainerPropertyControl *inputs = panelWindow->createGroup(PID_SUBGRAPH_INPUTS, "Inputs");
  inputs->setBoolValue(inputsCollapsed);
  PropPanel::ContainerPropertyControl *outputs = panelWindow->createGroup(PID_SUBGRAPH_OUTPUTS, "Outputs");
  outputs->setBoolValue(outputsCollapsed);
  int inputCount = 0;
  int outputCount = 0;
  for (const GraphData::Node *n : boundaries)
  {
    const bool isInput = is_input_boundary(*n);
    PropPanel::ContainerPropertyControl *list = isInput ? inputs : outputs;
    const int number = isInput ? ++inputCount : ++outputCount;
    const eastl::string name = effective_subgraph_boundary_name(gd, n->id);
    list->createStatic(PID_SUBGRAPH_BOUNDARY, String(0, "%d. %s", number, name.empty() ? "<unnamed>" : name.c_str()));
  }
  PropPanel::ContainerPropertyControl *textures = panelWindow->createGroup(PID_SUBGRAPH_TEXTURES, "Textures");
  textures->setBoolValue(texturesCollapsed);
  createTextureCombos(*textures, /*in_group=*/true);
}

void PropertiesPanel::createTextureCombos(PropPanel::ContainerPropertyControl &container, bool in_group)
{
  const GraphData &gd = doc->getGraphData();
  {
    Tab<String> items;
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", gd.graphTextureWidth);
    const int sel = build_combo_items(items, buf, BASE_SIZES, "= ");
    container.createCombo(PID_GRAPH_TEX_WIDTH, in_group ? "width" : "Texture width", items, sel);
  }
  {
    Tab<String> items;
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", gd.graphTextureHeight);
    const int sel = build_combo_items(items, buf, BASE_SIZES, "= ");
    container.createCombo(PID_GRAPH_TEX_HEIGHT, in_group ? "height" : "Texture height", items, sel);
  }
  {
    Tab<String> items;
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", gd.graphTextureDepth);
    const int sel = build_combo_items(items, buf, BASE_SIZES, "= ");
    container.createCombo(PID_GRAPH_TEX_DEPTH, in_group ? "depth" : "Texture depth", items, sel);
  }
  {
    Tab<String> items;
    const int sel = build_combo_items(items, gd.graphTextureType.c_str(), BASE_TYPES);
    container.createCombo(PID_GRAPH_TEX_TYPE, in_group ? "type" : "Texture type", items, sel);
  }
  {
    Tab<String> items;
    const int sel = build_combo_items(items, gd.graphTextureWrap.c_str(), BASE_WRAPS);
    container.createCombo(PID_GRAPH_TEX_WRAP, in_group ? "wrap" : "Texture wrap", items, sel);
  }
}

void PropertiesPanel::rebuildForNode(int node_id)
{
  if (!panelWindow)
  {
    return;
  }

  const GraphData::Node *n = findNodeById(node_id);
  if (!n)
  {
    rebuildForGraph();
    return;
  }

  panelWindow->createStatic(PID_NODE_HEADER, String(0, "Node: %s", n->descName.empty() ? "<unnamed>" : n->descName.c_str()));
  panelWindow->createStatic(PID_NODE_ID, String(0, "id: %d", n->id));
  if (!n->plugin.empty())
  {
    panelWindow->createStatic(PID_NODE_PLUGIN, String(0, "plugin: %s", n->plugin.c_str()));
  }

  const DataBlock *nodeDesc = plugin.findBaseNodeBlockByUid(n->templateUid.c_str());
  if (!nodeDesc)
  {
    panelWindow->createSeparator();
    panelWindow->createStatic(PID_NODE_PROP_BASE, "(no descriptor in base_nodes.blk)");
    return;
  }

  panelWindow->createSeparator();

  if (is_subgraph_instance(*n))
  {
    PropPanel::ContainerPropertyControl *definition = panelWindow->createGroup(PID_NODE_DEFINITION, "Definition");
    // The design names the root the file lives in "type", and the file "location".
    definition->createEditBox(PID_NODE_DEF_TYPE, "type", nodeDesc->getStr("location", ""), /*enabled=*/false);
    const char *sourcePath = nodeDesc->getStr("sourcePath", "");
    definition->createEditBox(PID_NODE_DEF_LOCATION, "location", sourcePath, /*enabled=*/false);
    definition->setTooltipId(PID_NODE_DEF_LOCATION, sourcePath); // the field clips a long path
  }

  // Classified once: a group exists exactly when something goes in it, General first in any descriptor order.
  struct ShownProperty
  {
    const DataBlock *desc;
    const char *name;
    const char *textureCaption; // null for the node's own settings
  };
  eastl::vector<ShownProperty> shown;
  bool hasOwnProperties = false;
  bool hasTextureSettings = false;
  for (uint32_t i = 0; i < nodeDesc->blockCount(); ++i)
  {
    const DataBlock *propDesc = nodeDesc->getBlock(i);
    const char *propName = propDesc->getStr("name", "");
    if (strcmp(propDesc->getBlockName(), "property") != 0 || !propName[0])
    {
      continue;
    }
    const char *textureCaption = texture_setting_caption(propName);
    if (textureCaption)
    {
      hasTextureSettings = true;
    }
    else
    {
      hasOwnProperties = true;
    }
    shown.push_back({propDesc, propName, textureCaption});
  }
  PropPanel::ContainerPropertyControl *general =
    hasOwnProperties ? panelWindow->createGroup(PID_NODE_GENERAL, "General") : panelWindow;
  PropPanel::ContainerPropertyControl *textures =
    hasTextureSettings ? panelWindow->createGroup(PID_NODE_TEXTURES, "Textures") : panelWindow;

  for (int propIndex = 0; propIndex < static_cast<int>(shown.size()); ++propIndex)
  {
    const DataBlock *propDesc = shown[propIndex].desc;
    const char *propName = shown[propIndex].name;
    const char *textureCaption = shown[propIndex].textureCaption;
    const char *propType = infer_prop_type(propDesc);
    PropPanel::ContainerPropertyControl *container = textureCaption ? textures : general;
    const char *caption = textureCaption ? textureCaption : propName;

    // Resolve the current value: live node value first, descriptor default second.
    eastl::string valStr;
    if (const eastl::string *cur = find_property_value(*n, propName))
    {
      valStr = *cur;
    }
    else
    {
      valStr = param_as_string(propDesc, "val");
    }

    const int pid = PID_NODE_PROP_BASE + propIndex * PID_NODE_PROP_STRIDE;
    pidToPropertyName.emplace(pid, eastl::string(propName));

    if (!strcmp(propType, "string"))
    {
      container->createEditBox(pid, caption, valStr.c_str());
    }
    else if (!strcmp(propType, "bool"))
    {
      container->createCheckBox(pid, caption, parse_bool(valStr.c_str()));
    }
    else if (!strcmp(propType, "int"))
    {
      const int v = atoi(valStr.c_str());
      const int hasMin = propDesc->findParam("minVal");
      const int hasMax = propDesc->findParam("maxVal");
      if (hasMin >= 0 && hasMax >= 0)
      {
        const int mn = propDesc->getInt("minVal", 0);
        const int mx = propDesc->getInt("maxVal", 0);
        const int step = propDesc->getInt("step", 1);
        container->createTrackInt(pid, caption, v, mn, mx, step <= 0 ? 1 : step);
      }
      else
      {
        container->createEditInt(pid, caption, v);
      }
    }
    else if (!strcmp(propType, "float"))
    {
      const float v = (float)atof(valStr.c_str());
      const int hasMin = propDesc->findParam("minVal");
      const int hasMax = propDesc->findParam("maxVal");
      if (hasMin >= 0 && hasMax >= 0)
      {
        const float mn = propDesc->getReal("minVal", 0.f);
        const float mx = propDesc->getReal("maxVal", 0.f);
        const float step = propDesc->getReal("step", 0.001f);
        container->createTrackFloat(pid, caption, v, mn, mx, step <= 0.f ? 0.001f : step);
      }
      else
      {
        container->createEditFloat(pid, caption, v);
      }
    }
    else if (!strcmp(propType, "combobox"))
    {
      Tab<String> items;
      const DataBlock *itemsBlk = propDesc->getBlockByName("items");
      int selectedIdx = 0;
      if (itemsBlk)
      {
        const int count = itemsBlk->paramCount();
        for (int j = 0; j < count; ++j)
        {
          if (strcmp(itemsBlk->getParamName(j), "item") != 0)
          {
            continue;
          }
          if (itemsBlk->getParamType(j) != DataBlock::TYPE_STRING)
          {
            continue;
          }
          const char *s = itemsBlk->getStr(j);
          if (valStr == s)
          {
            selectedIdx = items.size();
          }
          items.push_back(String(s));
        }
      }
      if (items.empty())
      {
        // Fall back to a disabled edit box so the value remains visible.
        container->createEditBox(pid, caption, valStr.c_str(), false);
      }
      else
      {
        container->createCombo(pid, caption, items, selectedIdx);
      }
    }
    else if (!strcmp(propType, "color"))
    {
      container->createColorBox(pid, caption, parse_color(valStr.c_str()));
    }
    else if (!strcmp(propType, "filepath"))
    {
      // File picker based at the property's root (see filepath_base). Outputs (renderDir/entityDir)
      // use a save dialog, inputs/absolute an open dialog; an optional `mask` sets the file filter.
      const String base = filepath_base(propDesc, doc->getGraphData());
      const char *root = propDesc->getStr("root", "app");
      const bool isOutput = !strcmp(root, "renderDir") || !strcmp(root, "entityDir");
      container->createFileEditBox(pid, caption, valStr.c_str());
      if (!base.empty())
      {
        container->setUserData(pid, base.str());
      }
      container->setInt(pid, isOutput ? PropPanel::FS_DIALOG_SAVE_FILE : PropPanel::FS_DIALOG_OPEN_FILE);
      const char *mask = propDesc->getStr("mask", "");
      if (mask[0])
      {
        Tab<String> masks;
        masks.push_back(String(mask));
        container->setStrings(pid, masks);
      }
    }
    else if (!strcmp(propType, "gradient_editor"))
    {
      eastl::vector<GradientStop> stops;
      bool nearest = false;
      const int strayFloats = parse_gradient(valStr.c_str(), stops, nearest);
      const int storedStops = static_cast<int>(stops.size());
      if (strayFloats > 0 && shouldWarnOnce(n->id, propName))
      {
        DAEDITOR3.conWarning("GraphEditor: node %d property '%s' ends with %d float(s) that do not make a stop, ignored", n->id,
          propName, strayFloats);
      }
      normalize_gradient_for_editing(stops);

      if (can_edit_gradient(stops))
      {
        PropPanel::Gradient keys(tmpmem);
        gradient_stops_to_keys(stops, keys);
        container->createGradientBox(pid, caption);
        // Bounds interactive adds, so the value can never grow past what fillGradientTexture takes.
        if (PropPanel::PropertyControlBase *ctrl = container->getById(pid))
        {
          ctrl->setGradientMinMaxPointCount(2, GRADIENT_MAX_STOPS);
        }
        container->setGradient(pid, &keys);
        container->setTooltipId(pid, "Double-click adds a key, right-click removes one, click picks its color, Ctrl+C / Ctrl+V "
                                     "copy and paste.\nThe strip previews RGB only: alpha and 'nearest' are not drawn.");
        container->createCheckBox(pid + PROP_SLOT_AUX, "nearest (no interpolation)", nearest);
      }
      else
      {
        // Editing would drop keys and the generator refuses the value anyway, so show it as it is.
        container->createStatic(pid, String(0, "%s: %s", caption, valStr.c_str()));
        container->createStatic(pid + PROP_SLOT_AUX, String(0, "  (%d stops, too many to edit here)", storedStops));
      }
    }
    else if (CurveKind curveKind; curve_kind_for_prop_type(propType, curveKind))
    {
      eastl::vector<Point2> points;
      CurveParse parsed = parse_curve_points(valStr.c_str(), points);
      if (parsed == CurveParse::Empty)
      {
        parsed = parse_curve_points(param_as_string(propDesc, "val").c_str(), points);
      }

      if (parsed != CurveParse::Ok)
      {
        // The points are what this editor edits: guessing them would overwrite the curve on the
        // first click.
        if (shouldWarnOnce(n->id, propName))
        {
          DAEDITOR3.conWarning("GraphEditor: node %d property '%s' has no usable /*...*/ point list, shown read-only", n->id,
            propName);
        }
        container->createStatic(pid, String(0, "%s: %s", caption, valStr.c_str()));
        container->createStatic(pid + PROP_SLOT_AUX, "  (no control points to edit)");
      }
      else
      {
        Tab<Point2> controlPoints(tmpmem);
        for (const Point2 &pt : points)
        {
          controlPoints.push_back(pt);
        }

        container->createCurveEdit(pid, caption);
        // setInt creates the approximator; every call below it is a no-op before that. The control
        // has no steps and no monotone mode, so all four kinds show their control polygon.
        container->setInt(pid, PropPanel::CURVE_LINEAR_APP);
        // Locks the point order and pins the end points: a double-click adds its point at the end.
        container->setBool(pid, true);
        container->setMinMaxStep(pid, CURVE_MIN_POINTS, CURVE_MAX_POINTS, PropPanel::CURVE_MIN_MAX_POINTS);
        // Both axes, or the control's auto zoom divides by a zero wide view box.
        container->setMinMaxStep(pid, 0.f, 1.f, PropPanel::CURVE_MIN_MAX_X);
        container->setMinMaxStep(pid, 0.f, 1.f, PropPanel::CURVE_MIN_MAX_Y);
        container->setColor(pid, curve_style_color(propDesc->getStr("style", "gray")));
        container->setControlPoints(pid, controlPoints);
        container->setTooltipId(pid, "Drag to move a point, double-click to add one or to type its coordinates, right-click a "
                                     "point to remove it.\nThe end points keep their x.");
      }
    }
    else if (!strcmp(propType, "gradient_preview"))
    {
      container->createStatic(pid + PROP_SLOT_AUX, caption);
      curvePreviews.push_back(
        eastl::make_unique<CurvePreviewControl>(*doc, node_id, nodeDesc, propDesc->getBlockByName("background")));
      container->createCustomControlHolder(pid, curvePreviews.back().get());
    }
    else
    {
      // Unknown type -- still show something rather than silently dropping the property.
      String label(0, "%s [%s]: %s", caption, propType, valStr.c_str());
      container->createStatic(pid, label);
    }
  }
}

void PropertiesPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (!panel)
  {
    return;
  }

  if (pcb_id == PID_GRAPH_TEX_WIDTH || pcb_id == PID_GRAPH_TEX_HEIGHT || pcb_id == PID_GRAPH_TEX_DEPTH ||
      pcb_id == PID_GRAPH_TEX_TYPE || pcb_id == PID_GRAPH_TEX_WRAP)
  {
    const char *text = static_cast<const char *>(panel->getText(pcb_id));
    if (!text)
    {
      return;
    }
    GraphSettings s;
    doc->getGraphSettings(s);
    if (pcb_id == PID_GRAPH_TEX_WIDTH)
    {
      s.graphTextureWidth = parse_graph_size(text, s.graphTextureWidth);
    }
    else if (pcb_id == PID_GRAPH_TEX_HEIGHT)
    {
      s.graphTextureHeight = parse_graph_size(text, s.graphTextureHeight);
    }
    else if (pcb_id == PID_GRAPH_TEX_DEPTH)
    {
      s.graphTextureDepth = parse_graph_size(text, s.graphTextureDepth);
    }
    else if (pcb_id == PID_GRAPH_TEX_TYPE)
    {
      s.graphTextureType = text;
    }
    else
    {
      s.graphTextureWrap = text;
    }
    doc->setGraphSettings(s);
    // Made in this panel, so the subgraph view must not rebuild under it.
    lastRenderedRevision = doc->getStructureRevision();
    return;
  }

  commitNodeProperty(pcb_id, panel, /*finished=*/false);
}

void PropertiesPanel::onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (!panel)
  {
    return;
  }

  // Heightmap spin edits commit here (Enter / spin-button release / focus loss) rather than on every
  // intermediate onChange, so holding a spin button doesn't regenerate every frame.
  if (pcb_id == PID_GRAPH_HEIGHT_SCALE || pcb_id == PID_GRAPH_HEIGHT_MIN || pcb_id == PID_GRAPH_CELL_SIZE)
  {
    GraphSettings s;
    doc->getGraphSettings(s);
    if (pcb_id == PID_GRAPH_HEIGHT_SCALE)
    {
      s.heightmapScale = panel->getFloat(pcb_id);
    }
    else if (pcb_id == PID_GRAPH_HEIGHT_MIN)
    {
      s.heightmapMin = panel->getFloat(pcb_id);
    }
    else
    {
      s.heightmapCellSize = panel->getFloat(pcb_id);
    }
    doc->setGraphSettings(s);
    return;
  }

  // Output directory edits commit here -- on Enter, on focus loss, or just before the panel
  // is rebuilt (see send_immediate_focus_loss_notification in updateImgui) -- rather than on
  // every keystroke. Skipping an unchanged value avoids a needless recompile/regen (and the
  // transient "texgen failed" it would otherwise surface for a half-typed path).
  if (pcb_id == PID_GRAPH_RENDER_DIR || pcb_id == PID_GRAPH_ENTITY_DIR)
  {
    const SimpleString text = panel->getText(pcb_id);
    String rel; // app-relative form to store ("" clears the dir)
    if (!text.empty() && !resolve_under_app_dir(text.str(), rel))
    {
      wingw::message_box(wingw::MBS_EXCL, "Invalid folder",
        "'%s' is outside the application directory:\n%s\n\nChoose a folder inside it.", text.str(),
        DAGORED2->getWorkspace().getAppDir());
      const GraphData &gd = doc->getGraphData();
      const char *prev = pcb_id == PID_GRAPH_RENDER_DIR ? gd.renderDir.c_str() : gd.entityDir.c_str();
      panel->setText(pcb_id, prev);
      return;
    }
    const char *newValue = rel.str();
    if (strcmp(newValue, text.str()) != 0)
    {
      panel->setText(pcb_id, newValue); // reflect the normalized app-relative form
    }

    GraphSettings s;
    doc->getGraphSettings(s);
    if (pcb_id == PID_GRAPH_RENDER_DIR)
    {
      s.renderDir = newValue;
    }
    else
    {
      s.entityDir = newValue;
    }
    doc->setGraphSettings(s);
    return;
  }

  commitNodeProperty(pcb_id, panel, /*finished=*/true);

  // The value already arrived through onChange. Closing by id leaves a gesture another control opened
  // in between alone, it is still being edited.
  if (pcb_id >= PID_NODE_PROP_BASE)
  {
    editGesture.end(prop_pid_base(pcb_id));
  }
}

void PropertiesPanel::commitNodeProperty(int pcb_id, PropPanel::ContainerPropertyControl *panel, bool finished)
{
  if (pcb_id < PID_NODE_PROP_BASE)
  {
    return;
  }

  // Sub-controls report their own pid; every read below goes through the property's block base.
  const int pid = prop_pid_base(pcb_id);
  auto it = pidToPropertyName.find(pid);
  if (it == pidToPropertyName.end())
  {
    return;
  }

  const GraphData::Node *n = findNodeById(lastRenderedNodeId);
  if (!n)
  {
    return;
  }

  const DataBlock *nodeDesc = plugin.findBaseNodeBlockByUid(n->templateUid.c_str());
  const DataBlock *propDesc = find_property_desc(nodeDesc, it->second.c_str());
  if (!propDesc)
  {
    return;
  }

  const char *t = infer_prop_type(propDesc);

  // Text entry writes back only when the edit is committed (Enter, focus loss): createEditBox sends a
  // change per keystroke, and a half typed value -- a path above all -- would reach the graph and regen.
  // Everything else writes back per change too, so the preview follows a slider drag, a held spin button
  // or an open color picker. A spin edit sends nothing while typing, so no half typed number gets here.
  const bool textEntry = !strcmp(t, "string") || !strcmp(t, "filepath");
  if (!finished && textEntry)
  {
    return;
  }

  eastl::string newVal;
  if (!strcmp(t, "string"))
  {
    newVal = static_cast<const char *>(panel->getText(pid));
  }
  else if (!strcmp(t, "bool"))
  {
    newVal = panel->getBool(pid) ? "true" : "false";
  }
  else if (!strcmp(t, "int"))
  {
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", panel->getInt(pid));
    newVal = buf;
  }
  else if (!strcmp(t, "float"))
  {
    char buf[32];
    _snprintf(buf, sizeof(buf), "%g", panel->getFloat(pid));
    newVal = buf;
  }
  else if (!strcmp(t, "combobox"))
  {
    // createCombo's current selection is exposed as text via getText.
    newVal = static_cast<const char *>(panel->getText(pid));
  }
  else if (!strcmp(t, "color"))
  {
    const E3DCOLOR c = panel->getColor(pid);
    char buf[64];
    _snprintf(buf, sizeof(buf), "%g,%g,%g,%g", c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
    newVal = buf;
  }
  else if (!strcmp(t, "filepath"))
  {
    const String base = filepath_base(propDesc, doc->getGraphData());
    const SimpleString raw = panel->getText(pid);
    const String stored = relativize_under(base.str(), raw.str());
    if (strcmp(stored.str(), raw.str()) != 0)
    {
      panel->setText(pid, stored.str()); // reflect the canonical stored form
    }
    newVal = stored.str();
  }
  else if (!strcmp(t, "gradient_editor"))
  {
    PropPanel::Gradient keys(tmpmem);
    panel->getGradient(pid, &keys);
    if (keys.size() < 2)
    {
      return; // the control always keeps its two pinned keys; writing back nothing would clear the value
    }
    eastl::vector<GradientStop> stops;
    gradient_keys_to_stops(keys, stops);
    // A key dragged past its neighbour, or a nan from a zero width strip, must not reach the .blk.
    sanitize_gradient(stops);
    newVal = format_gradient(stops, panel->getBool(pid + PROP_SLOT_AUX));
  }
  else if (CurveKind curveKind; curve_kind_for_prop_type(t, curveKind))
  {
    Tab<Point2> controlPoints(tmpmem);
    panel->getCurveCoefs(pid, controlPoints); // despite its name this hands back the control points
    if (controlPoints.size() < CURVE_MIN_POINTS)
    {
      return;
    }
    eastl::vector<Point2> points;
    points.reserve(controlPoints.size());
    for (int i = 0; i < controlPoints.size(); ++i)
    {
      points.push_back(controlPoints[i]);
    }
    newVal = format_curve(curveKind, points);
  }
  else
  {
    return; // gradient_preview and unknown types have nothing to write back
  }

  // Skip a no-op edit; gradients and curves compare in the form this editor writes them.
  const eastl::string *curVal = find_property_value(*n, it->second.c_str());
  eastl::string compareVal = curVal ? *curVal : param_as_string(propDesc, "val");
  if (!strcmp(t, "gradient_editor"))
  {
    compareVal = canonical_gradient(compareVal.c_str());
  }
  else if (CurveKind curveKind; curve_kind_for_prop_type(t, curveKind))
  {
    compareVal = canonical_curve(compareVal.c_str(), curveKind);
  }
  if (compareVal == newVal)
  {
    return;
  }

  // The gesture opens here and not before the reads above, so only a write that really happens holds one
  // open, and an open gesture is always one whose first write took the snapshot. That snapshot covers the
  // whole property vector, so the rest of the edit undoes with it. Opening by pid also closes a gesture
  // another property left open, which would otherwise swallow this write into that property's entry.
  const bool firstOfEdit = !editGesture.isOpenFor(pid);
  if (firstOfEdit)
  {
    editGesture.begin(*EDITORCORE->getUndoSystem(), pid, "Change property");
  }
  doc->setNodeProperty(lastRenderedNodeId, it->second, eastl::move(newVal), firstOfEdit);
}
