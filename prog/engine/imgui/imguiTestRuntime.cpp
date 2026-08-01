// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imgui.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <EASTL/unordered_map.h>

#include <osApiWrappers/dag_localConv.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <perfMon/dag_statDrv.h>

// #define DEBUG_IMGUI_TEST_ITEM_TREE_QUERY

#ifdef IMGUI_ENABLE_TEST_ENGINE
void ImGuiTestEngineHook_ItemAdd(ImGuiContext *ctx, ImGuiID id, const ImRect &bb, const ImGuiLastItemData *item_data); // item_data may
                                                                                                                       // be NULL
void ImGuiTestEngineHook_ItemInfo(ImGuiContext *ctx, ImGuiID id, const char *label, ImGuiItemStatusFlags flags);
void ImGuiTestEngineHook_Log(ImGuiContext *ctx, const char *fmt, ...);
const char *ImGuiTestEngine_FindItemDebugLabel(ImGuiContext *ctx, ImGuiID id);
#endif

extern void ImGui_TestRuntime_StartFrame();
extern void ImGui_TestRuntime_EndFrame();

namespace
{
constexpr uint32_t NO_PARENT = 0;

struct ImGuiTestItemNode
{
  ImGuiID id = NO_PARENT;
  ImRect bb{0, 0, 0, 0};
  const char *label = nullptr;
  ImGuiID parentId = NO_PARENT;
  Tab<ImGuiID> children;

  bool hasData = false;
  ImGuiItemFlags flags = ImGuiItemFlags_None;
  ImGuiItemStatusFlags status = ImGuiItemStatusFlags_None;
  ImRect bbFull{0, 0, 0, 0};
  ImRect bbNav{0, 0, 0, 0};
  ImRect bbDisplay{0, 0, 0, 0}; // Display rectangle. ONLY VALID IF (StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) is set.
};

class ImGuiTestItemTree
{
public:
  bool isEmpty() const { return nodes.size() == 0; }

  void clear()
  {
    nodes.clear();
    roots.clear();

    desc.clear();
  }

  ImGuiTestItemNode *createNode(ImGuiID id, ImGuiID parent_id, const char *label, ImRect bb)
  {
    ImGuiTestItemNode node;
    node.id = id;
    node.parentId = parent_id;
    node.label = label;
    node.bb = bb;

    auto create = nodes.emplace(id, std::move(node));

    if (parent_id == NO_PARENT)
    {
      roots.push_back(id);
    }
    else
    {
      auto it = nodes.find(parent_id);
      if (it != nodes.end())
      {
        it->second.children.push_back(id);
      }
    }

    return &create.first->second;
  }

  ImGuiTestItemNode *getNode(ImGuiID id)
  {
    auto it = nodes.find(id);
    return it != nodes.end() ? &it->second : nullptr;
  }

  const char *createDesc(const char *str)
  {
    String &temp = desc.emplace_back(str);
    return temp.c_str();
  }

#ifdef DEBUG_IMGUI_TEST_ITEM_TREE_QUERY
  struct DebugIndentScope
  {
    const ImGuiTestItemTree &tree;
    DebugIndentScope(const ImGuiTestItemTree &t) : tree(t) { tree.queryIndent++; }
    ~DebugIndentScope() { tree.queryIndent--; }
  };
#define DEBUG_INDENT() DebugIndentScope _(*this)
#define DEBUG_INDENT_START() \
  {                          \
    queryIndent = 0;         \
  }
#define DEBUG_INDENT_MOD(by) \
  {                          \
    queryIndent += by;       \
  }
#define DEBUG_LOG_INDENT(fmt, ...)                         \
  {                                                        \
    const String indentStr(0, "%*s", queryIndent * 4, ""); \
    debug("%s" fmt "", indentStr.c_str(), __VA_ARGS__);    \
  }
#else
#define DEBUG_INDENT()
#define DEBUG_INDENT_START()
#define DEBUG_INDENT_MOD(by)
#define DEBUG_LOG_INDENT(fmt, ...)
#endif

  const ImGuiTestItemNode *query(const char *path) const
  {
    DEBUG_INDENT_START();
    DEBUG_LOG_INDENT("ImGuiTestItemTree::query(\"%s\"):", path);
    return queryLevel(path, &roots);
  }

  const ImGuiTestItemNode *queryLevel(const char *path, const Tab<ImGuiID> *level) const
  {
    DEBUG_INDENT();
    if (path == nullptr || !path[0])
      return nullptr;

    const ImGuiTestItemNode *current = nullptr;

    String queryExp(path);
    const char *pos = queryExp.begin();
    while (true)
    {
      DEBUG_LOG_INDENT("ImGuiTestItemTree::queryLevel(\"%s\"):", pos);

      String token("");
      const char *next = queryExp.find('/', pos);
      if (next != nullptr)
      {
        while (next != nullptr && next > pos && (*(next - 1)) == '\\')
        {
          token.append(pos, next - pos - 1);
          token.append("/");
          pos = next + 1;
          next = queryExp.find('/', pos);

          DEBUG_LOG_INDENT("+- part = \"%s\"", token.c_str());
        }

        if (next)
          token.append(pos, next - pos);
        else
          token.append(pos);
      }
      else
        token.setSubStr(pos, queryExp.end());

      DEBUG_LOG_INDENT("+-> token = \"%s\"", token.c_str());

      if (token.empty())
        return nullptr;

      const ImGuiTestItemNode *match = nullptr;
      if (strcmp(token.c_str(), "*") == 0 || strcmp(token.c_str(), "$") == 0)
      {
        tryWildcardTokenMatching(match, level, next, /*greedy*/ nullptr);
      }
      else if (strcmp(token.c_str(), "**") == 0 || strcmp(token.c_str(), "$$") == 0)
      {
        tryWildcardTokenMatching(match, level, next, /*greedy*/ pos);
      }
      else
      {
        for (ImGuiID id : (*level))
        {
          auto it = nodes.find(id);
          if (it != nodes.end())
          {
            if (it->second.label != nullptr && (strcmp(token.c_str(), it->second.label) == 0))
            {
              match = &it->second;
              break;
            }
          }
        }
      }

      if (!match)
        return nullptr;

      current = match;

      if (next == nullptr)
        break; // last token handled

      DEBUG_INDENT_MOD(+1);

      level = &current->children;
      pos = next + 1;
    }

    return current;
  }

  const char *queryDebugLabel(ImGuiID id)
  {
    String labelBuilder;
    debugLabel = "";

    const ImGuiTestItemNode *current = getNode(id);
    while (current)
    {
      const char *sep = "";
      if (current->parentId != NO_PARENT)
        sep = "/";

      String label(current->label != nullptr ? current->label : "");
      label.replaceAll("/", "\\/");

      labelBuilder.setStr(debugLabel.c_str());
      debugLabel.setStrCat3(sep, label.c_str(), labelBuilder.c_str());

      current = getNode(current->parentId);
    }

    return debugLabel.c_str();
  }

  void dump()
  {
    for (ImGuiID root_id : roots)
    {
      dumpNode(getNode(root_id), 0);
    }
  }

private:
  void dumpNode(const ImGuiTestItemNode *node, int indent)
  {
    if (node == nullptr)
      return;

    String indentStr("");
    for (int i = 0; i < indent; ++i)
      indentStr += "  ";

    const char *label = node->label != nullptr ? node->label : "<NULL>";
    debug("%s%s [%f, %f, %f, %f]", indentStr.c_str(), label, node->bb.Min.x, node->bb.Min.y, node->bb.GetWidth(),
      node->bb.GetHeight());

    indent++;
    for (ImGuiID child_id : node->children)
    {
      dumpNode(getNode(child_id), indent);
    }
  }

  void tryWildcardTokenMatching(const ImGuiTestItemNode *&match, const Tab<ImGuiID> *level, const char *&next,
    const char *greedy) const
  {
    if (level->empty())
      return;

    if (next == nullptr)
    {
      auto it = nodes.find(level->at(0));
      if (it != nodes.end())
      {
        DEBUG_LOG_INDENT("+-> wildcard == \"%s\"", (it->second.label ? it->second.label : "<NULL>"));
        match = &it->second;
      }
    }
    else
    {
      for (ImGuiID id : (*level))
      {
        auto it = nodes.find(id);
        if (it != nodes.end())
        {
          DEBUG_LOG_INDENT("+-> wildcard == \"%s\"", (it->second.label ? it->second.label : "<NULL>"));
          const ImGuiTestItemNode *wildcard = queryLevel(next + 1, &it->second.children);
          if (wildcard == nullptr && greedy != nullptr)
          {
            // Greedy token matching:
            // Retry from the same path pos, but from one level lower in the tree...
            wildcard = queryLevel(greedy, &it->second.children);
          }

          if (wildcard != nullptr)
          {
            match = wildcard;
            next = nullptr;
            break;
          }
        }
      }
    }
  }

private:
  eastl::unordered_map<unsigned int, ImGuiTestItemNode> nodes;
  Tab<ImGuiID> roots;

  Tab<String> desc;

  String debugLabel;

#ifdef DEBUG_IMGUI_TEST_ITEM_TREE_QUERY
  mutable int queryIndent;
#endif
};

ImGuiTestItemTree treeA, treeB;
ImGuiTestItemTree *treeCurr = nullptr;
ImGuiTestItemTree *treePrev = nullptr;
ImGuiTestItemNode *pending = nullptr;

} // namespace

#ifdef IMGUI_ENABLE_TEST_ENGINE
void ImGuiTestEngineHook_ItemAdd(ImGuiContext *ctx, ImGuiID id, const ImRect &bb, const ImGuiLastItemData *item_data)
{
  TIME_PROFILE_UNIQUE_EVENT_DEV("ImGuiTestEngineHook_ItemAdd");

  ImGuiID parentId = ::NO_PARENT;

  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window != nullptr)
  {
    if (id == window->ID)
    {
      if (window->ParentWindow != nullptr)
      {
        parentId = window->ParentWindow->ID;
      }
    }
    else
    {
      parentId = window->ID;
    }
  }

  ::ImGuiTestItemNode *node = ::treeCurr->getNode(id);
  if (node == nullptr)
    node = ::treeCurr->createNode(id, parentId, nullptr, bb);

  if (ctx != nullptr && window != nullptr)
  {
    ImGuiDebugItemPathQuery &pathQuery = ctx->DebugItemPathQuery;
    if (pathQuery.Step >= 0 && pathQuery.Results.size() > 0)
    {
      for (int i = (pathQuery.Results.size() - 1); i >= 0; --i)
      {
        ImGuiStackLevelInfo &info = pathQuery.Results[i];
        if (info.ID == id)
        {
          if (info.DataType == ImGuiDataType_String && info.DescOffset > -1)
          {
            node->label = ::treeCurr->createDesc(pathQuery.ResultsDescBuf.begin() + info.DescOffset);
          }
          break;
        }
      }
    }
  }

  if (item_data != nullptr)
  {
    if (node != nullptr)
    {
      node->hasData = true;
      node->flags = item_data->ItemFlags;
      node->status = item_data->StatusFlags;
      node->bbFull = item_data->Rect;
      node->bbNav = item_data->NavRect;
      node->bbDisplay = item_data->DisplayRect;
    }
  }

  ::pending = node;
}

void ImGuiTestEngineHook_ItemInfo(ImGuiContext *ctx, ImGuiID id, const char *label, ImGuiItemStatusFlags flags)
{
  TIME_PROFILE_UNIQUE_EVENT_DEV("ImGuiTestEngineHook_ItemInfo");

  if (::pending == nullptr || ::pending->id != id)
  {
    ::pending = ::treeCurr->getNode(id);
  }

  if (::pending)
  {
    // ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (ctx != nullptr && ctx->DebugHookIdInfoId == id)
      ImGui::DebugHookIdInfo(id, ImGuiDataType_String, label, nullptr);

    ::pending->label = label;
    ::pending = nullptr;
  }
}

void ImGuiTestEngineHook_Log(ImGuiContext *ctx, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  cvlogmessage(LOGLEVEL_DEBUG, fmt, args);
  va_end(args);
}

const char *ImGuiTestEngine_FindItemDebugLabel(ImGuiContext *ctx, ImGuiID id)
{
  const char *label = ::treeCurr->queryDebugLabel(id);
  if (label == nullptr || label[0] == '\0')
    return ::treePrev->queryDebugLabel(id);
  return label;
}
#endif

void ImGui_TestRuntime_StartFrame()
{
  if (::treeCurr != nullptr)
  {
    ::ImGuiTestItemTree *curr = ::treeCurr;
    ::treeCurr = ::treePrev;
    ::treePrev = curr;
  }
  else
  {
    ::treeCurr = &::treeA;
    ::treePrev = &::treeB;
  }

  ::treeCurr->clear();
  ::pending = nullptr;
}

static ImGuiTestRuntimeOptions runtime_options;

void ImGui_TestRuntime_EndFrame()
{
  ImGuiContext *ctx = ImGui::GetCurrentContext();
  ImGuiTestRuntimeOptions *options =
    ctx && ctx->TestEngine ? static_cast<ImGuiTestRuntimeOptions *>(ctx->TestEngine) : &runtime_options;

  ImGuiID id = 0;
  if (options->drawItemId != 0)
    id = options->drawItemId;
  else if (runtime_options.drawItemId != 0)
    id = runtime_options.drawItemId;

  if (runtime_options.drawHovered || options->drawHovered)
  {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    if (ctx != nullptr)
      id = ctx->HoveredId;
  }

  if (id != 0)
  {
    ImGuiWindow *window = ImGui::GetCurrentWindowRead();
    if (window != nullptr && !window->SkipItems)
    {
      ImDrawList *drawList = ImGui::GetForegroundDrawList(window);
      ::ImGuiTestItemNode *curr = ::treeCurr->getNode(id);
      ImU32 colGreen;
      const char *label = nullptr;
      ImRect bb;
      if (curr)
      {
        colGreen = IM_COL32(24, 255, 24, 255);
        drawList->AddRect(curr->bb.Min, curr->bb.Max, colGreen, 0, 0, 2.0f);
        label = ::treeCurr->queryDebugLabel(id);
        bb = curr->bb;
        if (curr->hasData)
        {
          drawList->AddRect(curr->bbFull.Min, curr->bbFull.Max, colGreen, 0, 0, 1.0f);
          if (curr->status & ImGuiItemStatusFlags_HasDisplayRect)
          {
            drawList->AddRect(curr->bbDisplay.Min, curr->bbDisplay.Max, IM_COL32(128, 128, 128, 255), 0, 0, 1.0f);
          }
        }
        curr = ::treeCurr->getNode(curr->parentId);
      }

      ImU32 colRed = IM_COL32(255, 24, 24, 255);
      while (curr)
      {
        drawList->AddRect(curr->bb.Min, curr->bb.Max, colRed, 0, 0, 2.0f);
        curr = ::treeCurr->getNode(curr->parentId);
      }

      if (label)
      {
        const float pad = 4;
        ImVec2 size = ImGui::CalcTextSize(label);
        size.x += pad * 2;
        size.y += pad * 2;
        ImVec2 pos = bb.GetBL();
        pos.y += pad;
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), pad);
        pos.x += pad;
        pos.y += pad;
        drawList->AddText(pos, colGreen, label);
      }
    }
  }

  if (runtime_options.drawFakeMousePointer || options->drawFakeMousePointer)
  {
    ImGuiWindow *window = ImGui::GetCurrentWindowRead();
    if (window != nullptr && !window->SkipItems)
    {
      ImVec2 mousePos = ImGui::GetMousePos();
      ImDrawList *drawList = ImGui::GetForegroundDrawList(window);

      float s = 16.0f * ImGui::GetStyle()._MainScale;

      ImVec2 tip = mousePos;
      ImVec2 leftBottom = ImVec2(mousePos.x, mousePos.y + s);
      float cs = cosf(M_PI_4);
      float sn = sinf(M_PI_4);
      ImVec2 rightDiagonal = ImVec2(mousePos.x + s * cs, mousePos.y + s * sn);

      drawList->AddTriangleFilled(tip, leftBottom, rightDiagonal, IM_COL32(255, 255, 255, 255));
      drawList->AddTriangle(tip, leftBottom, rightDiagonal, IM_COL32(0, 0, 0, 255), 1.0f);
    }
  }
}

bool imgui_test_runtime_set(bool enabled, ImGuiTestRuntimeOptions *options)
{
  ImGuiContext *g = ImGui::GetCurrentContext();
  const bool changed = g->TestEngineHookItems != enabled;
  g->TestEngineHookItems = enabled;
  g->TestEngine = static_cast<void *>(options);
  if (changed && enabled)
    ImGui_TestRuntime_StartFrame();
  return changed;
}

bool imgui_test_runtime_query_item(const char *path, uint32_t &id, ImRect &bb, uint32_t &parent_id)
{
  const ::ImGuiTestItemNode *node = ::treeCurr->query(path);
  if (node == nullptr)
    node = ::treePrev->query(path);

  if (node != nullptr)
  {
    id = node->id;
    bb = node->bb;
    parent_id = node->parentId;
    return true;
  }
  return false;
}

static bool imgui_test_runtime_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;

#ifdef IMGUI_ENABLE_TEST_ENGINE
  CONSOLE_CHECK_NAME("imgui", "set_test_runtime", 1, 2)
  {
    bool enabled = (argc > 1) ? console::to_bool(argv[1]) : true;
    imgui_test_runtime_set(enabled, nullptr);
  }
  CONSOLE_CHECK_NAME("imgui", "hovered", 1, 1)
  {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    if (ctx != nullptr && ctx->HoveredId != 0)
    {
      const char *label = ImGuiTestEngine_FindItemDebugLabel(ctx, ctx->HoveredId);
      console::print("HoveredId = 0x%08X", ctx->HoveredId);
      console::print("Label = %s", label != nullptr ? label : "<NULL>");
    }
  }
  CONSOLE_CHECK_NAME("imgui", "query_id", 2, 2)
  {
    uint32_t id = 0;
    if (strlen(argv[1]) > 1 && dd_strnicmp(argv[1], "0x", 2) == 0)
      id = strtoll(argv[1], nullptr, 0);
    else
      id = atoll(argv[1]);

    ::ImGuiTestItemNode *node = ::treeCurr->getNode(id);
    if (node == nullptr)
      node = ::treePrev->getNode(id);

    if (node != nullptr)
    {
      ImRect bb = node->bb;
      console::print("BB = [%f, %f, %f, %f]", bb.Min.x, bb.Min.y, bb.GetWidth(), bb.GetHeight());
      ImGuiContext *ctx = ImGui::GetCurrentContext();
      if (ctx != nullptr)
      {
        const char *label = ImGuiTestEngine_FindItemDebugLabel(ctx, id);
        console::print("Label = %s", label != nullptr ? label : "<NULL>");
      }
      console::print("Parent Id = 0x%08X", node->parentId);
    }
  }
  CONSOLE_CHECK_NAME("imgui", "dump_tree", 1, 1)
  {
    if (::treeCurr->isEmpty())
    {
      debug("ImGuiTestItemTree (prev):");
      ::treePrev->dump();
    }
    else
    {
      debug("ImGuiTestItemTree (curr):");
      ::treeCurr->dump();
    }
  }
  CONSOLE_CHECK_NAME("imgui", "query", 2, 16)
  {
    String path("");
    for (int i = 1; i < argc; ++i)
    {
      if (i > 1)
        path.append(" ");
      path.append(argv[i]);
    }

    ImGuiID id;
    ImGuiID parentId;
    ImRect bb;
    if (imgui_test_runtime_query_item(path.c_str(), id, bb, parentId))
    {
      console::print("Id = 0x%08X", id);
      console::print("BB = [%f, %f, %f, %f]", bb.Min.x, bb.Min.y, bb.GetWidth(), bb.GetHeight());
      console::print("Parent Id = 0x%08X", parentId);
    }
  }
  CONSOLE_CHECK_NAME("imgui", "draw_hovered", 2, 2) { runtime_options.drawHovered = console::to_bool(argv[1]); }
  CONSOLE_CHECK_NAME("imgui", "draw_fake_mouse", 2, 2) { runtime_options.drawFakeMousePointer = console::to_bool(argv[1]); }
  CONSOLE_CHECK_NAME("imgui", "draw_item_id", 1, 2)
  {
    uint32_t id = 0;
    if (argc > 1)
    {
      if (strlen(argv[1]) > 1 && dd_strnicmp(argv[1], "0x", 2) == 0)
        id = strtoll(argv[1], nullptr, 0);
      else
        id = atoll(argv[1]);
    }
    runtime_options.drawItemId = id;
  }
  CONSOLE_CHECK_NAME("imgui", "draw_item", 2, 16)
  {
    String path("");
    for (int i = 1; i < argc; ++i)
    {
      if (i > 1)
        path.append(" ");
      path.append(argv[i]);
    }

    const ::ImGuiTestItemNode *node = ::treeCurr->query(path.c_str());
    if (node == nullptr)
      node = ::treeCurr->query(path.c_str());

    if (node != nullptr)
    {
      runtime_options.drawItemId = node->id;
    }
  }
#endif
  return found;
}

REGISTER_CONSOLE_HANDLER(imgui_test_runtime_console_handler);
