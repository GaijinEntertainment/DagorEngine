// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_e3dColor.h>
#include <propPanel/control/container.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>

#include "properties_panel.h"
#include "graph_panel.h"
#include "plugin.h"
#include "pluginService/graph_tex_gen_service.h"

namespace
{
enum
{
  PID_GRAPH_BASE = 13000,
  PID_GRAPH_SOURCE_PATH = PID_GRAPH_BASE + 0,
  PID_GRAPH_TEXROOT = PID_GRAPH_BASE + 1,
  PID_GRAPH_RENDER_DIR = PID_GRAPH_BASE + 2,
  PID_GRAPH_ENTITY_DIR = PID_GRAPH_BASE + 3,
  PID_GRAPH_HEIGHT_SCALE = PID_GRAPH_BASE + 4,
  PID_GRAPH_HEIGHT_MIN = PID_GRAPH_BASE + 5,
  PID_GRAPH_CELL_SIZE = PID_GRAPH_BASE + 6,

  PID_NODE_HEADER = 13100,
  PID_NODE_ID = 13101,
  PID_NODE_PLUGIN = 13102,

  PID_NODE_PROP_BASE = 13200,
};

// Fallbacks for the heightmap fields when the loaded graph JSON has no markers.
// Mirrors the defaults the standalone graphEditor app reads from landSettings (app.cpp:3074-3077)
// so the user always sees an editable value rather than FLT_MAX.
constexpr float DEFAULT_HEIGHT_SCALE = 2000.0f;
constexpr float DEFAULT_HEIGHT_MIN = -0.05f;
constexpr float DEFAULT_CELL_SIZE = 4.0f;

constexpr const char *EMPTY_DIR_PLACEHOLDER = "<empty>";
constexpr const char *NO_GRAPH_PLACEHOLDER = "<none>";

constexpr int HEIGHT_FLOAT_PREC = 4;

bool parse_bool(const char *s)
{
  if (!s || !*s)
  {
    return false;
  }
  return !strcmp(s, "true") || !strcmp(s, "1") || !strcmp(s, "yes");
}

const char *value_or(const eastl::string &s, const char *fallback) { return s.empty() ? fallback : s.c_str(); }

float effective_height(float v, float fallback) { return v == FLT_MAX ? fallback : v; }

// Look up a node property value by name. Returns nullptr when the value isn't set yet
// (caller should fall back to the descriptor default).
const eastl::string *find_property_value(const GraphData::Node &n, const char *prop_name)
{
  for (const auto &pv : n.propertyValues)
  {
    if (pv.first == prop_name)
    {
      return &pv.second;
    }
  }
  return nullptr;
}

// Read a `val` field from a property descriptor, normalized to a string the same way
// makeNodeFromBaseBlk does (plugin.cpp:920-958). Empty string when absent.
eastl::string read_descriptor_default(const DataBlock *prop_desc)
{
  if (!prop_desc)
  {
    return {};
  }
  const int idx = prop_desc->findParam("val");
  if (idx < 0)
  {
    return {};
  }
  char buf[128] = {};
  switch (prop_desc->getParamType(idx))
  {
    case DataBlock::TYPE_STRING: return eastl::string(prop_desc->getStr(idx));
    case DataBlock::TYPE_INT: _snprintf(buf, sizeof(buf), "%d", prop_desc->getInt(idx)); return buf;
    case DataBlock::TYPE_REAL: _snprintf(buf, sizeof(buf), "%g", prop_desc->getReal(idx)); return buf;
    case DataBlock::TYPE_BOOL: return prop_desc->getBool(idx) ? eastl::string("true") : eastl::string("false");
    case DataBlock::TYPE_POINT4:
    {
      const Point4 p = prop_desc->getPoint4(idx);
      _snprintf(buf, sizeof(buf), "%g,%g,%g,%g", p.x, p.y, p.z, p.w);
      return buf;
    }
    default: return {};
  }
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

bool is_readonly_complex_type(const char *t)
{
  return !strcmp(t, "gradient_editor") || !strcmp(t, "monotonic_curve") || !strcmp(t, "linear_curve") || !strcmp(t, "polynom_curve") ||
         !strcmp(t, "steps_curve") || !strcmp(t, "gradient_preview");
}
} // namespace

PropertiesPanel::PropertiesPanel(GraphEditorPlg &plg) : plugin(plg)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
}

PropertiesPanel::~PropertiesPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

const GraphData::Node *PropertiesPanel::findNodeById(int id) const
{
  if (id < 0)
  {
    return nullptr;
  }
  const GraphData &gd = plugin.getGraphData();
  for (const auto &n : gd.nodes)
  {
    if (n.id == id)
    {
      return &n;
    }
  }
  return nullptr;
}

const DataBlock *PropertiesPanel::findPropertyDesc(const DataBlock *node_desc, const char *prop_name) const
{
  if (!node_desc || !prop_name)
  {
    return nullptr;
  }
  for (uint32_t i = 0; i < node_desc->blockCount(); ++i)
  {
    const DataBlock *child = node_desc->getBlock(i);
    if (strcmp(child->getBlockName(), "property") != 0)
    {
      continue;
    }
    if (strcmp(child->getStr("name", ""), prop_name) == 0)
    {
      return child;
    }
  }
  return nullptr;
}

void PropertiesPanel::updateImgui()
{
  if (!panelWindow)
  {
    return;
  }

  GraphPanel *gp = plugin.getGraphPanel();
  const int selId = gp ? gp->getSelectedNodeId() : -1;

  // Resolve target mode. A selected id that no longer maps to a live node (deleted while
  // selected) collapses to graph mode -- safer than rendering stale fields.
  Mode targetMode = Mode::Graph;
  int targetNodeId = -1;
  if (selId >= 0)
  {
    if (findNodeById(selId))
    {
      targetMode = Mode::Node;
      targetNodeId = selId;
    }
  }

  // Cheap path: nothing relevant has changed -> just pump the panel and return.
  const GraphData &gd = plugin.getGraphData();
  const eastl::string &curSourcePath = gd.sourcePath;
  const bool sourcePathChanged = (curSourcePath != lastRenderedSourcePath);

  bool needRebuild = (targetMode != currentMode);
  if (!needRebuild && targetMode == Mode::Node)
  {
    needRebuild = (targetNodeId != lastRenderedNodeId);
  }
  if (!needRebuild && targetMode == Mode::Graph)
  {
    needRebuild = sourcePathChanged;
  }

  if (needRebuild)
  {
    panelWindow->clear();
    pidToPropertyName.clear();

    if (targetMode == Mode::Node)
    {
      rebuildForNode(targetNodeId);
    }
    else
    {
      rebuildForGraph();
    }

    currentMode = targetMode;
    lastRenderedNodeId = targetNodeId;
    lastRenderedSourcePath = curSourcePath;
  }

  panelWindow->updateImgui();
}

void PropertiesPanel::rebuildForGraph()
{
  if (!panelWindow)
  {
    return;
  }

  const GraphData &gd = plugin.getGraphData();

  panelWindow->createStatic(PID_GRAPH_SOURCE_PATH,
    String(0, "Source: %s", gd.sourcePath.empty() ? NO_GRAPH_PLACEHOLDER : gd.sourcePath.c_str()));
  panelWindow->createStatic(PID_GRAPH_TEXROOT,
    String(0, "Texture root: %s", gd.textureRootDir.empty() ? EMPTY_DIR_PLACEHOLDER : gd.textureRootDir.c_str()));
  panelWindow->createStatic(PID_GRAPH_RENDER_DIR,
    String(0, "Render dir: %s", gd.renderDir.empty() ? EMPTY_DIR_PLACEHOLDER : gd.renderDir.c_str()));
  panelWindow->createStatic(PID_GRAPH_ENTITY_DIR,
    String(0, "Entity dir: %s", gd.entityDir.empty() ? EMPTY_DIR_PLACEHOLDER : gd.entityDir.c_str()));

  panelWindow->createSeparator();

  panelWindow->createEditFloat(PID_GRAPH_HEIGHT_SCALE, "Height scale", effective_height(gd.heightmapScale, DEFAULT_HEIGHT_SCALE),
    HEIGHT_FLOAT_PREC);
  panelWindow->createEditFloat(PID_GRAPH_HEIGHT_MIN, "Height min", effective_height(gd.heightmapMin, DEFAULT_HEIGHT_MIN),
    HEIGHT_FLOAT_PREC);
  panelWindow->createEditFloat(PID_GRAPH_CELL_SIZE, "Cell size", effective_height(gd.heightmapCellSize, DEFAULT_CELL_SIZE),
    HEIGHT_FLOAT_PREC);
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

  int pid = PID_NODE_PROP_BASE;
  for (uint32_t i = 0; i < nodeDesc->blockCount(); ++i)
  {
    const DataBlock *propDesc = nodeDesc->getBlock(i);
    if (strcmp(propDesc->getBlockName(), "property") != 0)
    {
      continue;
    }

    const char *propName = propDesc->getStr("name", "");
    if (!propName[0])
    {
      continue;
    }
    const char *propType = propDesc->getStr("type", "");

    // Resolve the current value: live node value first, descriptor default second.
    eastl::string valStr;
    if (const eastl::string *cur = find_property_value(*n, propName))
    {
      valStr = *cur;
    }
    else
    {
      valStr = read_descriptor_default(propDesc);
    }

    pidToPropertyName.emplace(pid, eastl::string(propName));

    if (!strcmp(propType, "string"))
    {
      panelWindow->createEditBox(pid, propName, valStr.c_str());
    }
    else if (!strcmp(propType, "bool"))
    {
      panelWindow->createCheckBox(pid, propName, parse_bool(valStr.c_str()));
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
        panelWindow->createTrackInt(pid, propName, v, mn, mx, step <= 0 ? 1 : step);
      }
      else
      {
        panelWindow->createEditInt(pid, propName, v);
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
        panelWindow->createTrackFloat(pid, propName, v, mn, mx, step <= 0.f ? 0.001f : step);
      }
      else
      {
        panelWindow->createEditFloat(pid, propName, v);
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
        panelWindow->createEditBox(pid, propName, valStr.c_str(), false);
      }
      else
      {
        panelWindow->createCombo(pid, propName, items, selectedIdx);
      }
    }
    else if (!strcmp(propType, "color"))
    {
      panelWindow->createColorBox(pid, propName, parse_color(valStr.c_str()));
    }
    else if (is_readonly_complex_type(propType))
    {
      // V1: read-only display. Editing happens in the legacy JS editor.
      String label(0, "%s: %s", propName, valStr.empty() ? "<unset>" : valStr.c_str());
      panelWindow->createStatic(pid, label);
      panelWindow->createStatic(pid + 50000, "  (edit in JS editor)");
    }
    else
    {
      // Unknown type -- still show something rather than silently dropping the property.
      String label(0, "%s [%s]: %s", propName, propType, valStr.c_str());
      panelWindow->createStatic(pid, label);
    }

    ++pid;
  }
}

void PropertiesPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (!panel)
  {
    return;
  }

  // Graph-mode heightmap edits.
  if (pcb_id == PID_GRAPH_HEIGHT_SCALE || pcb_id == PID_GRAPH_HEIGHT_MIN || pcb_id == PID_GRAPH_CELL_SIZE)
  {
    plugin.mutateGraphData([&](GraphData &gd) {
      if (pcb_id == PID_GRAPH_HEIGHT_SCALE)
      {
        gd.heightmapScale = panel->getFloat(pcb_id);
      }
      else if (pcb_id == PID_GRAPH_HEIGHT_MIN)
      {
        gd.heightmapMin = panel->getFloat(pcb_id);
      }
      else
      {
        gd.heightmapCellSize = panel->getFloat(pcb_id);
      }
    });

    if (IGraphTexGenService *svc = plugin.getTexGenService())
    {
      const GraphData &gd = plugin.getGraphData();
      svc->setHeightmapParams(effective_height(gd.heightmapScale, DEFAULT_HEIGHT_SCALE),
        effective_height(gd.heightmapMin, DEFAULT_HEIGHT_MIN), effective_height(gd.heightmapCellSize, DEFAULT_CELL_SIZE));
      svc->requestRegenerate();
    }
    return;
  }

  // Node-mode property edits.
  auto it = pidToPropertyName.find(pcb_id);
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
  const DataBlock *propDesc = findPropertyDesc(nodeDesc, it->second.c_str());
  if (!propDesc)
  {
    return;
  }

  const char *t = propDesc->getStr("type", "");
  eastl::string newVal;

  if (!strcmp(t, "string"))
  {
    newVal = static_cast<const char *>(panel->getText(pcb_id));
  }
  else if (!strcmp(t, "bool"))
  {
    newVal = panel->getBool(pcb_id) ? "true" : "false";
  }
  else if (!strcmp(t, "int"))
  {
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", panel->getInt(pcb_id));
    newVal = buf;
  }
  else if (!strcmp(t, "float"))
  {
    char buf[32];
    _snprintf(buf, sizeof(buf), "%g", panel->getFloat(pcb_id));
    newVal = buf;
  }
  else if (!strcmp(t, "combobox"))
  {
    // createCombo's current selection is exposed as text via getText.
    newVal = static_cast<const char *>(panel->getText(pcb_id));
  }
  else if (!strcmp(t, "color"))
  {
    const E3DCOLOR c = panel->getColor(pcb_id);
    char buf[64];
    _snprintf(buf, sizeof(buf), "%g,%g,%g,%g", c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
    newVal = buf;
  }
  else
  {
    return; // read-only stub types -- onChange is not expected, but ignore defensively.
  }

  // Re-find the node inside mutateGraphData so the mutable reference comes from the
  // locked-access path. lastRenderedNodeId is stable across the lambda; main is the
  // only writer that could remove a node and we're on main.
  plugin.mutateGraphData([&](GraphData &gd) {
    for (auto &node : gd.nodes)
    {
      if (node.id != lastRenderedNodeId)
      {
        continue;
      }
      bool found = false;
      for (auto &pv : node.propertyValues)
      {
        if (pv.first == it->second)
        {
          pv.second = eastl::move(newVal);
          found = true;
          break;
        }
      }
      if (!found)
      {
        node.propertyValues.emplace_back(it->second, eastl::move(newVal));
      }
      break;
    }
  });

  // Worker thread runs compile_graph_to_blks asynchronously and then regenerates.
  plugin.markGraphDirtyAndRegen();
}
