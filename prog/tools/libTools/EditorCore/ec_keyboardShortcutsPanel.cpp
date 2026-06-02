// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_keyboardShortcutsPanel.h>
#include "ec_editorCommand.h"
#include "ec_singleHotkeyEditor.h"

#include <EditorCore/ec_imguiInitialization.h>
#include <propPanel/colors.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>

#include <dag/dag_vector.h>
#include <util/dag_simpleString.h>

#include <EASTL/algorithm.h>

#include <imgui/imgui.h>
#include <gui/dag_imgui.h>

#include <ctype.h>
#include <float.h>
#include <stdio.h>

namespace
{

struct CommandEntry
{
  dag::Vector<SimpleString> idTokens;
  SimpleString cachedShortcutText;
  dag::Vector<SimpleString> shortcutTokens;
  float matchScore = 0.0f;
  bool matches = true;
};

struct GroupNode
{
  SimpleString name;
  dag::Vector<GroupNode> children;
  dag::Vector<int> bindings;
  bool hasNonDefaultRecursive = false;
  bool openPersistent = true;
  bool containsMatchRecursive = true;
};

static const float kChildPadding = 8.0f;
static const float kCommandTextInset = 20.0f;
static const float kHeaderDividerShiftLeft = 16.0f;
static const float kScrollDividerWidth = 10.0f;

static void lowercaseInPlace(char *s)
{
  for (; *s; ++s)
    *s = (char)tolower((unsigned char)*s);
}

static void tokenize(const char *str, const char *seps, dag::Vector<SimpleString> &out)
{
  char buf[64];
  const char *p = str;
  while (*p)
  {
    while (*p && strchr(seps, *p))
      ++p;
    if (!*p)
      break;
    const char *end = p;
    while (*end && !strchr(seps, *end))
      ++end;
    const int len = (int)(end - p);
    if (len > 0 && len < (int)sizeof(buf))
    {
      memcpy(buf, p, len);
      buf[len] = '\0';
      lowercaseInPlace(buf);
      if (strcmp(buf, "or") != 0)
        out.push_back(SimpleString(buf));
    }
    p = end;
  }
}

// spilts by '.' and camelCase (does not split consecutive upper letters)
static void tokenizeId(const char *id, dag::Vector<SimpleString> &out)
{
  char buf[64];
  const char *p = id;
  while (*p)
  {
    while (*p == '.')
      ++p;
    if (!*p)
      break;
    const char *segEnd = p;
    while (*segEnd && *segEnd != '.')
      ++segEnd;

    const char *tok = p;
    for (const char *c = p + 1; c <= segEnd; ++c)
    {
      const bool atEnd = (c == segEnd);
      const bool camelBoundary = !atEnd && isupper((unsigned char)*c) && islower((unsigned char)*(c - 1));
      if (atEnd || camelBoundary)
      {
        const int len = (int)(c - tok);
        if (len > 0 && len < (int)sizeof(buf))
        {
          memcpy(buf, tok, len);
          buf[len] = '\0';
          lowercaseInPlace(buf);
          out.push_back(SimpleString(buf));
        }
        tok = c;
      }
    }
    p = segEnd;
  }
}

static void tokenizeShortcut(const char *text, dag::Vector<SimpleString> &out) { tokenize(text, "+ ", out); }

// splits by any non-alphanumeric character
static void tokenizeSearch(const char *text, dag::Vector<SimpleString> &out)
{
  char buf[64];
  const char *p = text;
  while (*p)
  {
    while (*p && !isalnum((unsigned char)*p))
      ++p;
    if (!*p)
      break;
    const char *end = p;
    while (*end && isalnum((unsigned char)*end))
      ++end;
    const int len = (int)(end - p);
    if (len > 0 && len < (int)sizeof(buf))
    {
      memcpy(buf, p, len);
      buf[len] = '\0';
      lowercaseInPlace(buf);
      out.push_back(SimpleString(buf));
    }
    p = end;
  }
}

// prefix score (searchLen^2 / tokenLen) or 0 if doesn't match
static float scoreAgainst(const char *search, int search_len, const dag::Vector<SimpleString> &tokens)
{
  float best = 0.0f;
  for (const SimpleString &tok : tokens)
  {
    const int tokLen = (int)strlen(tok.c_str());
    if (tokLen == 0 || search_len > tokLen)
      continue;
    if (strncmp(tok.c_str(), search, search_len) == 0)
    {
      const float s = (float)(search_len * search_len) / tokLen;
      if (s > best)
        best = s;
    }
  }
  return best;
}

// total score per entry, -1 if any single search token have 0 match
static float computeScore(const CommandEntry &e, const dag::Vector<SimpleString> &tokens)
{
  if (tokens.empty())
    return 0.0f;

  float total = 0.0f;
  for (const SimpleString &st : tokens)
  {
    const int len = (int)strlen(st.c_str());
    if (len == 0)
      continue;
    float best = scoreAgainst(st.c_str(), len, e.idTokens);
    const float sh = scoreAgainst(st.c_str(), len, e.shortcutTokens);
    if (sh > best)
      best = sh;
    if (best == 0.0f)
      return -1.0f;
    total += best;
  }
  return total;
}

static GroupNode *findOrAddChild(GroupNode &parent, const char *name, int name_len)
{
  for (GroupNode &c : parent.children)
    if (strncmp(c.name.c_str(), name, name_len) == 0 && c.name.c_str()[name_len] == '\0')
      return &c;
  parent.children.push_back(GroupNode{});
  GroupNode &added = parent.children.back();
  char buf[128];
  const int copyLen = name_len < (int)sizeof(buf) - 1 ? name_len : (int)sizeof(buf) - 1;
  memcpy(buf, name, copyLen);
  buf[copyLen] = '\0';
  added.name = SimpleString(buf);
  return &added;
}

static void sortGroupTree(GroupNode &g)
{
  eastl::sort(g.children.begin(), g.children.end(),
    [](const GroupNode &a, const GroupNode &b) { return strcmp(a.name.c_str(), b.name.c_str()) < 0; });
  for (GroupNode &c : g.children)
    sortGroupTree(c);

  eastl::sort(g.bindings.begin(), g.bindings.end(), [](int a, int b) {
    const char *idA = ec_editor_commands.getCommandId(a);
    const char *idB = ec_editor_commands.getCommandId(b);
    return strcmp(idA ? idA : "", idB ? idB : "") < 0;
  });
}

static void recomputeNonDefault(GroupNode &g)
{
  bool any = false;
  for (GroupNode &c : g.children)
  {
    recomputeNonDefault(c);
    any |= c.hasNonDefaultRecursive;
  }
  for (int idx : g.bindings)
  {
    const char *id = ec_editor_commands.getCommandId(idx);
    if (!id)
      continue;
    const EditorCommand *cmd = ec_editor_commands.getCommand(id);
    if (cmd && !cmd->isUsingDefaultHotkeys())
      any = true;
  }
  g.hasNonDefaultRecursive = any;
}

static void resetGroupRecursive(GroupNode &g)
{
  for (GroupNode &c : g.children)
    resetGroupRecursive(c);
  for (int idx : g.bindings)
  {
    const char *id = ec_editor_commands.getCommandId(idx);
    if (id)
      ec_editor_commands.resetToDefaultHotkeys(id);
  }
}

static const char *lastIdSegment(const char *id)
{
  const char *last = id;
  for (const char *p = id; *p; ++p)
    if (*p == '.')
      last = p + 1;
  return last;
}

} // namespace


struct KeyboardShortcutsPanel::State
{
  String searchText;
  const int searchInputFocusId = 0;

  dag::Vector<CommandEntry> cache;
  GroupNode root;

  dag::Vector<SimpleString> searchTokens;
  bool filterDirty = true;

  PropPanel::IconId searchIconId = PropPanel::IconId::Invalid;
  PropPanel::IconId clearIconId = PropPanel::IconId::Invalid;
  PropPanel::IconId resetIconId = PropPanel::IconId::Invalid;
  PropPanel::IconId deleteIconId = PropPanel::IconId::Invalid;

  GroupNode *pendingResetGroup = nullptr;
  bool openGroupResetPopup = false;
  bool groupResetPopupOpen = false;

  int activeEditCmdIndex = -1;

  float col1StartX = 0.0f;
  float childLeftX = 0.0f;
  float childRightX = 0.0f;

  void buildCache();
  bool checkAndUpdateShortcuts();
  void buildTree();
  bool rebuildFilter(GroupNode &g);
  void renderGroup(GroupNode &g);
  void renderBinding(int cmd_index);
  void update();
};


void KeyboardShortcutsPanel::State::buildCache()
{
  const int count = (int)ec_editor_commands.getCommandCount();
  cache.resize(count);
  for (int i = 0; i < count; ++i)
  {
    const char *id = ec_editor_commands.getCommandId(i);
    CommandEntry &e = cache[i];
    e.idTokens.clear();
    if (id)
      tokenizeId(id, e.idTokens);

    const EditorCommand *cmd = id ? ec_editor_commands.getCommand(id) : nullptr;
    const char *shortcutText = (cmd && cmd->getHotkeyCount() > 0) ? cmd->getKeyChordsAsText() : "";
    e.cachedShortcutText = shortcutText;
    e.shortcutTokens.clear();
    if (shortcutText[0] != '\0')
      tokenizeShortcut(shortcutText, e.shortcutTokens);
    e.matchScore = 0.0f;
    e.matches = true;
  }
}

bool KeyboardShortcutsPanel::State::checkAndUpdateShortcuts()
{
  bool changed = false;
  const int count = (int)cache.size();
  for (int i = 0; i < count; ++i)
  {
    const char *id = ec_editor_commands.getCommandId(i);
    if (!id)
      continue;
    const EditorCommand *cmd = ec_editor_commands.getCommand(id);
    const char *shortcutText = (cmd && cmd->getHotkeyCount() > 0) ? cmd->getKeyChordsAsText() : "";
    if (strcmp(shortcutText, cache[i].cachedShortcutText) != 0)
    {
      cache[i].cachedShortcutText = shortcutText;
      cache[i].shortcutTokens.clear();
      if (shortcutText[0] != '\0')
        tokenizeShortcut(shortcutText, cache[i].shortcutTokens);
      changed = true;
    }
  }
  return changed;
}

void KeyboardShortcutsPanel::State::buildTree()
{
  root = GroupNode{};
  const int count = (int)cache.size();
  for (int i = 0; i < count; ++i)
  {
    const char *id = ec_editor_commands.getCommandId(i);
    if (!id)
      continue;
    GroupNode *cur = &root;
    const char *segStart = id;
    while (true)
    {
      const char *dot = strchr(segStart, '.');
      if (!dot)
        break;
      const int segLen = (int)(dot - segStart);
      if (segLen > 0)
        cur = findOrAddChild(*cur, segStart, segLen);
      segStart = dot + 1;
    }
    cur->bindings.push_back(i);
  }
  sortGroupTree(root);
}

bool KeyboardShortcutsPanel::State::rebuildFilter(GroupNode &g)
{
  bool any = false;
  for (GroupNode &c : g.children)
    if (rebuildFilter(c))
      any = true;
  for (int idx : g.bindings)
  {
    CommandEntry &e = cache[idx];
    const float score = computeScore(e, searchTokens);
    e.matches = (score >= 0.0f);
    e.matchScore = e.matches ? score : 0.0f;
    if (e.matches)
      any = true;
  }
  if (!searchTokens.empty() && any)
  {
    eastl::sort(g.bindings.begin(), g.bindings.end(), [this](int a, int b) {
      const CommandEntry &ea = cache[a];
      const CommandEntry &eb = cache[b];
      if (ea.matches != eb.matches)
        return ea.matches && !eb.matches;
      if (ea.matchScore != eb.matchScore)
        return ea.matchScore > eb.matchScore;
      const char *idA = ec_editor_commands.getCommandId(a);
      const char *idB = ec_editor_commands.getCommandId(b);
      return strcmp(idA ? idA : "", idB ? idB : "") < 0;
    });
  }
  else if (searchTokens.empty())
  {
    eastl::sort(g.bindings.begin(), g.bindings.end(), [](int a, int b) {
      const char *idA = ec_editor_commands.getCommandId(a);
      const char *idB = ec_editor_commands.getCommandId(b);
      return strcmp(idA ? idA : "", idB ? idB : "") < 0;
    });
  }
  g.containsMatchRecursive = any;
  return any;
}

void KeyboardShortcutsPanel::State::renderGroup(GroupNode &g)
{
  const bool searchActive = !searchTokens.empty();
  if (searchActive && !g.containsMatchRecursive)
    return;

  ImGui::PushID(&g);

  const bool forcedOpen = searchActive;
  const bool wasMinimized = !g.openPersistent && !forcedOpen;
  if (wasMinimized)
    ImGui::PushFont(imgui_get_bold_font(), 0.0f);

  ImGui::SetNextItemOpen(forcedOpen || g.openPersistent);
  const float headerLeftX = ImGui::GetCursorScreenPos().x;
  char displayName[160];
  snprintf(displayName, sizeof(displayName), g.hasNonDefaultRecursive ? "%s *" : "%s", g.name.c_str());
  const float headerMaxWidth = eastl::max(childRightX - headerLeftX, 0.0f);
  const bool expanded = PropPanel::ImguiHelper::collapsingHeaderWidth(displayName, headerMaxWidth, ImGuiTreeNodeFlags_AllowOverlap);
  const ImVec2 headerMin = ImGui::GetItemRectMin();
  const ImVec2 headerMax = ImGui::GetItemRectMax();

  if (wasMinimized)
    ImGui::PopFont();

  if (!searchActive)
    g.openPersistent = expanded;

  const ImVec2 cursorAfterHeader = ImGui::GetCursorScreenPos();
  {
    const float headerH = headerMax.y - headerMin.y;
    const float inset = 2.0f;
    const float btnSize = eastl::max(headerH - inset * 2.0f, 12.0f);
    const float rightEdge = childRightX - kChildPadding;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(rightEdge - btnSize, headerMin.y + inset));
    ImGui::PushID("resetBtn");
    const bool disabled = !g.hasNonDefaultRecursive;
    if (disabled)
      ImGui::BeginDisabled();
    const ImVec2 iconSize(btnSize - ImGui::GetStyle().FramePadding.x * 2.0f, btnSize - ImGui::GetStyle().FramePadding.y * 2.0f);
    const bool pressed = ImGui::ImageButton("reset", PropPanel::get_im_texture_id_from_icon_id(resetIconId), iconSize);
    if (pressed)
    {
      pendingResetGroup = &g;
      openGroupResetPopup = true;
    }
    if (disabled)
      ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
      ImGui::SetTooltip("Reset to defaults all shortcuts in %s", g.name.c_str());
    ImGui::PopID();
  }
  ImGui::SetCursorScreenPos(cursorAfterHeader);
  ImGui::Dummy(ImVec2(0.0f, 0.0f));

  if (expanded)
  {
    const float indent = floorf(ImGui::GetStyle().IndentSpacing / 2.0f);
    ImGui::Indent(indent);
    for (GroupNode &c : g.children)
      renderGroup(c);
    for (int idx : g.bindings)
    {
      if (searchActive && !cache[idx].matches)
        continue;
      renderBinding(idx);
    }
    ImGui::Unindent(indent);

    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    PropPanel::ImguiHelper::drawHalfFrame(headerLeftX, headerMax.y, headerMax.x, cursorPos.y);
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + ImGui::GetStyle().ItemSpacing.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
  }

  ImGui::PopID();
}

void KeyboardShortcutsPanel::State::renderBinding(int cmd_index)
{
  const char *id = ec_editor_commands.getCommandId(cmd_index);
  if (!id)
    return;
  const EditorCommand *cmd = ec_editor_commands.getCommand(id);
  if (!cmd)
    return;

  const float kRowExtraPadY = 4.0f;
  const float kRowGapPx = 1.0f;
  const float buttonH = ImGui::GetFrameHeight();
  const float rowHeight = buttonH + kRowExtraPadY * 2.0f;
  const ImVec2 rowStart = ImGui::GetCursorScreenPos();
  const ImVec2 rowMin = ImVec2(rowStart.x, rowStart.y);
  const ImVec2 rowMax = ImVec2(childRightX, rowStart.y + rowHeight);

  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImGuiCol_TableRowBg));

  ImGui::PushID(cmd_index);

  ImDrawListSplitter splitter;
  splitter.Split(dl, 2);
  splitter.SetCurrentChannel(dl, 1);

  const int hotkeyCount = cmd->getHotkeyCount();
  const dag::ConstSpan<const EditorCommandHotkey> defaultHotkeys = cmd->getDefaultHotkeys();

  const ImGuiHoveredFlags hoverFlags =
    ImGuiHoveredFlags_RootAndChildWindows |
    (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ? ImGuiHoveredFlags_AllowWhenBlockedByActiveItem : 0);
  const bool mouseInRow = !groupResetPopupOpen && ImGui::IsWindowHovered(hoverFlags) && ImGui::IsMouseHoveringRect(rowMin, rowMax);
  bool anyWidgetHovered = false;

  const float textY = rowStart.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;
  ImGui::SetCursorScreenPos(ImVec2(rowStart.x + kCommandTextInset, textY));
  ImGui::TextUnformatted(lastIdSegment(id));

  ImGui::PushClipRect(rowMin, rowMax, true);
  const float buttonsY = rowStart.y + (rowHeight - buttonH) * 0.5f;
  ImGui::SetCursorScreenPos(ImVec2(childLeftX + kChildPadding + col1StartX, buttonsY));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + 6.0f, ImGui::GetStyle().FramePadding.y));
  for (int i = 0; i < hotkeyCount; ++i)
  {
    ImGui::PushID(i);
    const EditorCommandHotkey &hotkey = cmd->getHotkeys()[i];
    bool isDefault = false;
    for (const EditorCommandHotkey &dh : defaultHotkeys)
      if (dh == hotkey)
      {
        isDefault = true;
        break;
      }
    if (!isDefault)
      ImGui::PushStyleColor(ImGuiCol_Button,
        PropPanel::getOverriddenColor(PropPanel::ColorOverride::KEYBOARD_SHORTCUTS_ITEM_NON_DEFAULT));
    if (ImGui::Button(hotkey.getKeyChordAsText()))
    {
      activeEditCmdIndex = cmd_index;
      ec_create_single_hotkey_editor(id, i);
    }
    if (!isDefault)
      ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
    {
      anyWidgetHovered = true;
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::SameLine();
    ImGui::PopID();
  }
  if (ImGui::Button("+"))
  {
    activeEditCmdIndex = cmd_index;
    ec_create_single_hotkey_editor(id, -1);
  }
  if (ImGui::IsItemHovered())
  {
    anyWidgetHovered = true;
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }
  ImGui::PopStyleVar();
  ImGui::PopClipRect();

  bool rowResetClicked = false;
  bool rowDeleteClicked = false;
  if (mouseInRow)
  {
    const float kActionBtnInset = 3.0f;
    const float actionBtnSize = eastl::max(rowHeight - kActionBtnInset * 2.0f, 14.0f);
    const ImVec2 actionIconSize(actionBtnSize - ImGui::GetStyle().FramePadding.x * 2.0f,
      actionBtnSize - ImGui::GetStyle().FramePadding.y * 2.0f);
    const float actionGap = 2.0f;
    const float actionY = rowStart.y + kActionBtnInset;
    const float actionsTotalW = actionBtnSize * 2.0f + actionGap;
    float actionX = rowMax.x - kActionBtnInset - actionsTotalW;

    ImGui::SetCursorScreenPos(ImVec2(actionX, actionY));
    const bool resetDisabled = cmd->isUsingDefaultHotkeys();
    if (resetDisabled)
      ImGui::BeginDisabled();
    rowResetClicked = ImGui::ImageButton("##rowReset", PropPanel::get_im_texture_id_from_icon_id(resetIconId), actionIconSize);
    if (resetDisabled)
      ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
      anyWidgetHovered = true;
      ImGui::SetTooltip("Reset %s to defaults", lastIdSegment(id));
    }
    actionX += actionBtnSize + actionGap;

    ImGui::SetCursorScreenPos(ImVec2(actionX, actionY));
    const bool deleteDisabled = hotkeyCount == 0;
    if (deleteDisabled)
      ImGui::BeginDisabled();
    rowDeleteClicked = ImGui::ImageButton("##rowDelete", PropPanel::get_im_texture_id_from_icon_id(deleteIconId), actionIconSize);
    if (deleteDisabled)
      ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
      anyWidgetHovered = true;
      ImGui::SetTooltip("Remove all shortcuts for %s", lastIdSegment(id));
    }
  }

  const bool rowClicked = !anyWidgetHovered && mouseInRow && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

  const bool rowHovered = (mouseInRow && !anyWidgetHovered) || (activeEditCmdIndex == cmd_index);
  if (rowHovered)
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  splitter.SetCurrentChannel(dl, 0);
  if (rowHovered)
    dl->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
  splitter.Merge(dl);

  const ImU32 rowSepCol = ImGui::GetColorU32(ImGuiCol_Separator);
  const float sepY = rowMax.y + 0.5f;
  dl->AddLine(ImVec2(rowMin.x, sepY), ImVec2(rowMax.x, sepY), rowSepCol);

  if (rowResetClicked)
  {
    ec_editor_commands.resetToDefaultHotkeys(id);
    recomputeNonDefault(root);
    filterDirty = true;
  }
  else if (rowDeleteClicked)
  {
    ec_editor_commands.removeAllHotkeys(id);
    recomputeNonDefault(root);
    filterDirty = true;
  }
  else if (rowClicked)
  {
    activeEditCmdIndex = cmd_index;
    ec_create_single_hotkey_editor(id, hotkeyCount > 0 ? 0 : -1);
  }

  ImGui::PopID();

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + rowHeight + kRowGapPx));
  ImGui::Dummy(ImVec2(0.0f, 0.0f));
  ImGui::PopStyleVar();
}

void KeyboardShortcutsPanel::State::update()
{
  if (cache.empty())
  {
    searchIconId = PropPanel::load_icon("search");
    clearIconId = PropPanel::load_icon("close_editor");
    resetIconId = PropPanel::load_icon("reset");
    deleteIconId = PropPanel::load_icon("delete");
    buildCache();
    buildTree();
    recomputeNonDefault(root);
    filterDirty = true;
  }

  if (checkAndUpdateShortcuts())
  {
    recomputeNonDefault(root);
    filterDirty = true;
  }

  groupResetPopupOpen = ImGui::IsPopupOpen("Reset Group##confirm");
  if (!ec_is_single_hotkey_editor_open())
    activeEditCmdIndex = -1;

  ImGui::SetNextItemWidth(-FLT_MIN);
  if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##search", "Search commands or shortcuts...", searchText, searchIconId,
        clearIconId))
  {
    searchTokens.clear();
    tokenizeSearch(searchText.c_str(), searchTokens);
    filterDirty = true;
  }

  if (filterDirty)
  {
    rebuildFilter(root);
    filterDirty = false;
  }

  ImGui::Spacing();
  ImGui::Dummy(ImVec2(0.0f, 4.0f));

  const float avail = ImGui::GetContentRegionAvail().x;
  const float usable = avail - kChildPadding * 2.0f;
  float col0Width = floorf(usable * 0.5f);
  const float minCol0 = ImGui::CalcTextSize("XXXXXXXXXXX").x;
  if (col0Width < minCol0)
    col0Width = minCol0;
  col1StartX = col0Width;

  const float scrollbarSize = ImGui::GetStyle().ScrollbarSize;
  const float headerWidth = avail - scrollbarSize - kScrollDividerWidth;
  {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float kHeaderExtraPad = 6.0f;
    const float kLabelExtraLeftPad = 8.0f;
    const float hRowH = ImGui::GetFrameHeight() + kHeaderExtraPad * 2.0f;
    const ImVec2 hMin = ImGui::GetCursorScreenPos();
    const ImVec2 hMax = ImVec2(hMin.x + headerWidth, hMin.y + hRowH);
    const ImU32 bg = ImGui::GetColorU32(ImGuiCol_TableHeaderBg);
    if ((bg & IM_COL32_A_MASK) != 0)
      dl->AddRectFilled(hMin, hMax, bg);
    const ImU32 border = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);
    dl->AddRect(hMin, hMax, border);

    const float textY = hMin.y + (hRowH - ImGui::GetTextLineHeight()) * 0.5f;
    dl->AddText(ImVec2(hMin.x + kChildPadding + kCommandTextInset, textY), ImGui::GetColorU32(ImGuiCol_Text), "Command");
    const float dividerX = hMin.x + kChildPadding + col1StartX - kHeaderDividerShiftLeft;
    dl->AddLine(ImVec2(dividerX, hMin.y), ImVec2(dividerX, hMax.y), border);
    dl->AddText(ImVec2(hMin.x + kChildPadding + col1StartX + kLabelExtraLeftPad, textY), ImGui::GetColorU32(ImGuiCol_Text),
      "Shortcut");

    ImGui::Dummy(ImVec2(headerWidth, hRowH));
  }

  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::BeginChild("##shortcutsScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
  ImGui::PopStyleVar();

  const ImVec2 scrollChildWndMin = ImGui::GetWindowPos();
  const ImVec2 scrollChildWndSize = ImGui::GetWindowSize();

  const ImVec2 childOrigin = ImGui::GetCursorScreenPos();
  childLeftX = childOrigin.x;
  childRightX = childOrigin.x + ImGui::GetContentRegionAvail().x - kScrollDividerWidth;

  ImGui::Dummy(ImVec2(0, kChildPadding));
  ImGui::Indent(kChildPadding);

  for (GroupNode &top : root.children)
    renderGroup(top);
  for (int idx : root.bindings)
  {
    if (!searchTokens.empty() && !cache[idx].matches)
      continue;
    renderBinding(idx);
  }

  ImGui::Unindent(kChildPadding);
  ImGui::EndChild();
  ImGui::PopStyleColor();

  {
    const float dividerRight = scrollChildWndMin.x + scrollChildWndSize.x - ImGui::GetStyle().ScrollbarSize;
    const float dividerLeft = dividerRight - kScrollDividerWidth;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(dividerLeft, scrollChildWndMin.y),
      ImVec2(dividerRight, scrollChildWndMin.y + scrollChildWndSize.y), ImGui::GetColorU32(ImGuiCol_WindowBg));
  }

  if (openGroupResetPopup)
  {
    ImGui::OpenPopup("Reset Group##confirm");
    openGroupResetPopup = false;
  }
  if (groupResetPopupOpen)
  {
    ImGuiViewport *vp = ImGui::GetWindowViewport();
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  }
  if (ImGui::BeginPopup("Reset Group##confirm", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
  {
    {
      ImDrawList *dl = ImGui::GetWindowDrawList();
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      const float padX = ImGui::GetStyle().WindowPadding.x;
      const float padY = ImGui::GetStyle().WindowPadding.y;
      const float titleH = ImGui::GetTextLineHeight() + padY;
      const ImVec2 titleMin = {cursor.x - padX, cursor.y - padY};
      const ImVec2 titleMax = {titleMin.x + ImGui::GetWindowWidth(), titleMin.y + titleH};
      dl->AddRectFilled(titleMin, titleMax, IM_COL32(0, 0, 0, 255));
      dl->AddText({cursor.x, cursor.y - padY * 0.5f}, IM_COL32(255, 255, 255, 255), "Reset Group");
      ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, titleH - padY));
    }
    ImGui::Spacing();
    if (pendingResetGroup)
      ImGui::Text("Reset all shortcuts in \"%s\" to defaults?", pendingResetGroup->name.c_str());
    ImGui::Spacing();
    ImGui::Separator();

    const float btnW = floorf((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
    if (ImGui::Button("Confirm", ImVec2(btnW, 0.0f)))
    {
      if (pendingResetGroup)
      {
        resetGroupRecursive(*pendingResetGroup);
        pendingResetGroup = nullptr;
        recomputeNonDefault(root);
        filterDirty = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(btnW, 0.0f)))
    {
      pendingResetGroup = nullptr;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}


KeyboardShortcutsPanel::KeyboardShortcutsPanel() : state(new State())
{
  PropPanel::focus_helper.requestFocus(&state->searchInputFocusId);
}

KeyboardShortcutsPanel::~KeyboardShortcutsPanel() { delete state; }

void KeyboardShortcutsPanel::updateImgui() { state->update(); }
