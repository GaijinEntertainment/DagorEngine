#include <consoleWindow/dag_consoleWindow.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <imgui/imgui.h>

#include <visualConsole/dag_visualConsole.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderCommon.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_color.h>

static const int RUN_BUTTON_WIDTH = 70;
static const int CLEAR_BUTTON_WIDTH = RUN_BUTTON_WIDTH;
static eastl::string console_output_text;
static bool console_output_updated = false;

static void init_favourites(eastl::vector<eastl::string> &favourites, const DataBlock *blk, const char *param_name)
{
  favourites.clear();
  int nid = blk->getNameId(param_name);
  for (int i = 0; i < blk->paramCount(); i++)
    if (blk->getParamNameId(i) == nid && blk->getParamType(i) == DataBlock::TYPE_STRING)
      favourites.push_back(blk->getStr(i));
}

static void save_favourites(const eastl::vector<eastl::string> &favourites, DataBlock *blk, const char *param_name)
{
  blk->removeParam(param_name);
  for (const eastl::string &fav : favourites)
    blk->addStr(param_name, fav.c_str());
}

static void swap_favourites(eastl::vector<eastl::string> &favourites, int i, int j, int *edit_index, bool &favourites_changed)
{
  if (i < 0 || i >= favourites.size() || j < 0 || j >= favourites.size())
    return;
  eastl::swap(favourites[i], favourites[j]);
  if (edit_index != nullptr)
  {
    if (*edit_index == i)
      *edit_index = j;
    else if (*edit_index == j)
      *edit_index = i;
  }
  favourites_changed = true;
}

enum EchoMode
{
  NO_ECHO = 0,
  ECHO = 1
};
enum HistoryMode
{
  DONT_ADD_TO_HISTORY = 0,
  ADD_TO_HISTORY = 1
};

static void execute_console_command(const char *command, EchoMode echo, HistoryMode add_to_history)
{
  if (echo)
    console::print_d(command);
  if (add_to_history)
    console::add_history_command(command);
  console::command(command);
}

static void console_output_listener(const char *message)
{
  if (!console_output_text.empty())
    console_output_text += '\n';
  eastl::string msg(message);
  eastl_size_t lastNotNewLine = msg.find_last_not_of('\n');
  if (lastNotNewLine != eastl::string::npos)
    console_output_text.append(msg.begin(), msg.begin() + msg.find_last_not_of('\n') + 1);
  console_output_updated = true;
}

static void ensure_console_listener_registered()
{
  static bool listenerRegistered = false;
  if (!listenerRegistered)
  {
    console::register_console_listener(console_output_listener);
    listenerRegistered = true;
  }
}

static void console_output_area()
{
  static bool autoScroll = true;
  const char *consoleOutputLabel = "##consoleoutput";
  ImGui::BeginChild(consoleOutputLabel, ImVec2(0, 150));
  ImGui::TextUnformatted(console_output_text.c_str());
  if (console_output_updated)
  {
    if (autoScroll)
      ImGui::SetScrollHereY(1.0f);
    console_output_updated = false;
  }
  if (ImGui::BeginPopupContextItem(consoleOutputLabel))
  {
    if (ImGui::MenuItem("Clear"))
    {
      console_output_text.clear();
      console_output_updated = true;
    }
    if (ImGui::MenuItem("Copy"))
      ImGui::SetClipboardText(console_output_text.c_str());
    ImGui::MenuItem("Auto-scroll", nullptr, &autoScroll);
    ImGui::EndPopup();
  }
  ImGui::EndChild();
}

static void console_area()
{
  ensure_console_listener_registered();
  console_output_area();

  ImGui::Separator();

  static eastl::string cmdText;
  static bool cmdTextUpdated = true;
  static console::CommandList cmdList;
  static int selectedCommand = -1;
  static bool scrollToSelectedCommand = false;
  static bool applySelectedCommand = false;

  // Console input text with "Run" button
  {
    ImGui::SetNextItemWidth(-RUN_BUTTON_WIDTH - 2);
    const ImGuiInputTextFlags commandInputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways |
                                                      ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    bool enterOnCommandInputText =
      ImGuiDagor::InputTextWithHint("##Command", "Command", &cmdText, commandInputTextFlags, [](ImGuiInputTextCallbackData *data) {
        if (data->EventFlag & ImGuiInputTextFlags_CallbackCompletion)
        {
          if (!cmdList.empty())
          {
            selectedCommand += ImGui::GetIO().KeyShift ? -1 : 1;
            selectedCommand = clamp(selectedCommand, 0, (int)cmdList.size());
            scrollToSelectedCommand = true;
          }
        }
        if (data->EventFlag & ImGuiInputTextFlags_CallbackHistory)
        {
          if (data->EventKey == ImGuiKey_UpArrow)
          {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, console::get_prev_command());
            cmdTextUpdated = true;
          }
          else if (data->EventKey == ImGuiKey_DownArrow)
          {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, console::get_next_command());
            cmdTextUpdated = true;
          }
        }
        if ((data->EventFlag & ImGuiInputTextFlags_CallbackAlways) && applySelectedCommand)
        {
          // Need to apply completion from callback, otherwise caret will not go to the end of the completed text
          data->DeleteChars(0, data->BufTextLen);
          data->InsertChars(0, cmdList[selectedCommand].str());
          cmdTextUpdated = true;
          applySelectedCommand = false;
        }
        return 0;
      });
    bool execute = false;
    if (enterOnCommandInputText)
    {
      if (selectedCommand >= 0 && selectedCommand < cmdList.size())
        applySelectedCommand = true;
      else
        execute = true;
    }
    ImGui::SetItemDefaultFocus(); // Auto-focus command InputText on window apparition
    if (enterOnCommandInputText)
      ImGui::SetKeyboardFocusHere(-1); // Reclaim focus for command InputText after hitting enter
    cmdTextUpdated |= ImGui::IsItemEdited();

    ImGui::SameLine(0, 2);
    if (ImGui::Button("Run", ImVec2(RUN_BUTTON_WIDTH, 0)) || execute)
    {
      execute_console_command(cmdText.c_str(), ECHO, ADD_TO_HISTORY);
      cmdText.clear();
      cmdTextUpdated = true;
    }

    if (cmdTextUpdated)
    {
      clear_and_shrink(cmdList);
      console::get_filtered_command_list(cmdList, String(cmdText.c_str()));
      selectedCommand = -1;
      cmdTextUpdated = false;
    }
  }

  ImGui::Separator();

  // Listbox for autocomplete candidates
  if (!cmdText.empty() && !cmdList.empty() && ImGui::BeginListBox("##Matching commands", ImVec2(-FLT_MIN, 0)))
  {
    for (int i = 0; i < cmdList.size(); i++)
    {
      bool isSelected = selectedCommand == i;
      if (ImGui::Selectable(cmdList[i].str(), isSelected))
      {
        selectedCommand = i;
        cmdText = cmdList[selectedCommand].str();
        cmdTextUpdated = true;
      }
      if (isSelected && scrollToSelectedCommand)
      {
        ImGui::SetScrollHereY();
        scrollToSelectedCommand = false;
      }
    }
    ImGui::EndListBox();
  }
}

static void console_favourites(eastl::vector<eastl::string> &favourite_commands, bool &favourites_changed)
{
  if (ImGui::CollapsingHeader("Favourites", ImGuiTreeNodeFlags_DefaultOpen))
  {
    int indexToRemove = -1;
    static int editIndex = -1;
    static eastl::string uniqueLabel;
    for (int i = 0; i < favourite_commands.size(); i++)
    {
      const eastl::string &cmd = favourite_commands[i];

      if (editIndex == i)
      {
        uniqueLabel.sprintf("##editfav%i", i);
        if (ImGuiDagor::InputText(uniqueLabel.c_str(), &favourite_commands[i], ImGuiInputTextFlags_EnterReturnsTrue))
          editIndex = -1;
        if (ImGui::IsItemEdited())
          favourites_changed = true;
      }
      else
        ImGui::TextUnformatted(cmd.c_str());

      ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 215); // Align buttons to the right
      uniqueLabel.sprintf("Run##fav%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
        execute_console_command(cmd.c_str(), ECHO, ADD_TO_HISTORY);

      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("Unpin##%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
        indexToRemove = i; // We can only click on one button per frame anyway

      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("Duplicate##%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
      {
        favourite_commands.insert(favourite_commands.begin() + i + 1, cmd);
        favourites_changed = true;
      }

      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("Edit##%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
        editIndex = i;

      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("##MoveUp%d", i);
      if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Up))
        swap_favourites(favourite_commands, i, i - 1, &editIndex, favourites_changed);
      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("##MoveDown%d", i);
      if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Down))
        swap_favourites(favourite_commands, i, i + 1, &editIndex, favourites_changed);
    }
    if (indexToRemove >= 0)
    {
      favourite_commands.erase(favourite_commands.begin() + indexToRemove);
      if (indexToRemove <= editIndex)
        editIndex--;
      favourites_changed = true;
    }
  }
}

static void console_history(eastl::vector<eastl::string> &favourite_commands, bool &favourites_changed)
{
  if (ImGui::CollapsingHeader("History", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::BeginChild("HistoryPanel");
    static eastl::string uniqueLabel;
    for (int i = 0; i < console::get_command_history().size(); i++)
    {
      const SimpleString &cmd = console::get_command_history()[i];
      ImGui::TextUnformatted(cmd.c_str());
      ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 65); // Align buttons to the right
      uniqueLabel.sprintf("Run##hist%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
        execute_console_command(cmd.c_str(), ECHO, DONT_ADD_TO_HISTORY);
      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("Pin##%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
      {
        favourite_commands.push_back(cmd.c_str());
        favourites_changed = true;
      }
    }
    ImGui::EndChild();
  }
}

static void console_script(DataBlock *blk)
{
  if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static eastl::string scriptText;
    const char *consoleScriptTextParamName = "consoleScriptText";
    scriptText = blk->getStr(consoleScriptTextParamName, "");
    ImGui::SetNextItemWidth(-1);
    if (ImGuiDagor::InputTextMultiline("##scriptText", &scriptText))
    {
      blk->setStr(consoleScriptTextParamName, scriptText.c_str());
      imgui_save_blk();
    }
    if (ImGui::Button("Run script"))
    {
      eastl::string cmd;
      eastl_size_t prev = 0, pos = 0;
      while ((pos = scriptText.find('\n', prev)) != eastl::string::npos)
      {
        cmd = scriptText.substr(prev, pos - prev);
        cmd.rtrim();
        if (cmd.size() > 0 && cmd.find("//") != 0)
          execute_console_command(cmd.c_str(), ECHO, ADD_TO_HISTORY);
        prev = pos + 1;
      }
      cmd = scriptText.substr(prev);
      cmd.rtrim();
      if (cmd.size() > 0 && cmd.find("//") != 0)
        execute_console_command(cmd.c_str(), ECHO, ADD_TO_HISTORY);
    }
  }
}

static void console_commands_tab(DataBlock *blk)
{
  console_area();

  static eastl::vector<eastl::string> favouriteCommands;
  const char *favouriteParamName = "favouriteConsoleCommand";
  init_favourites(favouriteCommands, blk, favouriteParamName);
  bool favouritesChanged = false;

  console_favourites(favouriteCommands, favouritesChanged);
  console_script(blk);
  console_history(favouriteCommands, favouritesChanged);

  if (favouritesChanged)
  {
    save_favourites(favouriteCommands, blk, favouriteParamName);
    imgui_save_blk();
  }
}

static void convar_favourites(eastl::vector<eastl::string> &favourite_convars, bool &favourites_changed)
{
  static eastl::string uniqueLabel;

  if (ImGui::CollapsingHeader("Favourites", ImGuiTreeNodeFlags_DefaultOpen))
  {
    int indexToRemove = -1;
    for (int i = 0; i < favourite_convars.size(); i++)
    {
      ConVarIterator cit;
      while (ConVarBase *cv = cit.nextVar())
      {
        if (strcmp(cv->getName(), favourite_convars[i].c_str()) == 0)
        {
          uniqueLabel.sprintf("Unpin##%d", i);
          if (ImGui::Button(uniqueLabel.c_str()))
            indexToRemove = i;

          ImGui::SameLine(0, 2);
          uniqueLabel.sprintf("Revert##fav%d", i);
          if (ImGui::Button(uniqueLabel.c_str()))
            cv->revert();

          ImGui::SameLine(0, 2);
          uniqueLabel.sprintf("##MoveUp%d", i);
          if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Up))
            swap_favourites(favourite_convars, i, i - 1, nullptr, favourites_changed);
          ImGui::SameLine(0, 2);
          uniqueLabel.sprintf("##MoveDown%d", i);
          if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Down))
            swap_favourites(favourite_convars, i, i + 1, nullptr, favourites_changed);

          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionMax().x * 0.6f - ImGui::GetCursorPosX());
          uniqueLabel.sprintf("%s##fav", cv->getName());
          cv->imguiWidget(uniqueLabel.c_str());

          break;
        }
      }
    }
    if (indexToRemove >= 0)
    {
      favourite_convars.erase(favourite_convars.begin() + indexToRemove);
      favourites_changed = true;
    }
  }
}

static void convar_all(eastl::vector<eastl::string> &favourite_convars, bool &favourites_changed)
{
  static eastl::string uniqueLabel;

  if (ImGui::CollapsingHeader("All", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static eastl::string convarSearchText;
    ImGui::SetNextItemWidth(-CLEAR_BUTTON_WIDTH - 2);
    ImGuiDagor::InputTextWithHint("##Search", "Search", &convarSearchText);
    ImGui::SameLine(0, 2);
    if (ImGui::Button("Clear", ImVec2(CLEAR_BUTTON_WIDTH, 0)))
      convarSearchText.clear();
    ImGui::Separator();

    ImGui::BeginChild("AllConVarPanel");

    ConVarIterator cit;
    while (ConVarBase *cv = cit.nextVar())
    {
      if (strstr(cv->getName(), convarSearchText.c_str()))
      {
        uniqueLabel.sprintf("Pin##%s", cv->getName());
        if (ImGui::Button(uniqueLabel.c_str()))
        {
          if (eastl::find(favourite_convars.begin(), favourite_convars.end(), cv->getName()) == favourite_convars.end())
          {
            favourite_convars.push_back(cv->getName());
            favourites_changed = true;
          }
        }

        ImGui::SameLine(0, 2);
        uniqueLabel.sprintf("Revert##all%s", cv->getName());
        if (ImGui::Button(uniqueLabel.c_str()))
          cv->revert();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionMax().x * 0.6f - ImGui::GetCursorPosX());
        uniqueLabel.sprintf("%s##all", cv->getName());
        cv->imguiWidget(uniqueLabel.c_str());
      }
    }

    ImGui::EndChild();
  }
}

static void console_variables_tab(DataBlock *blk)
{
  static eastl::vector<eastl::string> favouriteConVars;
  const char *favouriteParamName = "favouriteConsoleVariable";
  init_favourites(favouriteConVars, blk, favouriteParamName);
  bool favouritesChanged = false;

  convar_favourites(favouriteConVars, favouritesChanged);
  convar_all(favouriteConVars, favouritesChanged);

  if (favouritesChanged)
  {
    save_favourites(favouriteConVars, blk, favouriteParamName);
    imgui_save_blk();
  }
}

static void shadervar_widget(const char *var_name, const char *widget_label)
{
  const int varId = VariableMap::getVariableId(var_name);

  switch (ShaderGlobal::get_var_type(varId))
  {
    case SHVT_INT:
    {
      int i = ShaderGlobal::get_int(varId);
      if (ImGui::InputInt(widget_label, &i))
        ShaderGlobal::set_int(varId, i);
      break;
    }
    case SHVT_REAL:
    {
      float r = ShaderGlobal::get_real(varId);
      if (ImGui::DragFloat(widget_label, &r))
        ShaderGlobal::set_real(varId, r);
      break;
    }
    case SHVT_COLOR4:
    {
      Color4 c = ShaderGlobal::get_color4(varId);
      if (ImGui::DragFloat4(widget_label, &c.r))
        ShaderGlobal::set_color4(varId, c);
      break;
    }
    case SHVT_TEXTURE:
    {
      TEXTUREID tid = ShaderGlobal::get_tex(varId);
      const char *texName = get_managed_texture_name(tid);
      float currentItemWidth = ImGui::CalcItemWidth();
      float currentCursorPos = ImGui::GetCursorPos().x;
      eastl::string showButtonLabel;
      showButtonLabel.sprintf("Show##%s", widget_label);
      if (ImGui::Button(showButtonLabel.c_str()))
      {
        eastl::string showTexCmd;
        showTexCmd.sprintf("render.show_tex %s", texName);
        execute_console_command(showTexCmd.c_str(), NO_ECHO, DONT_ADD_TO_HISTORY);
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(currentItemWidth - (ImGui::GetCursorPos().x - currentCursorPos));
      ImGui::LabelText(widget_label, "[texture, name: %s, TEXTUREID: 0x%x]", texName, unsigned(tid));
      break;
    }
    case SHVT_BUFFER:
    {
      D3DRESID bid = ShaderGlobal::get_buf(varId);
      const char *bufName = get_managed_res_name(bid);
      ImGui::LabelText(widget_label, "[buffer, name: %s, D3DRESID: 0x%x]", bufName, unsigned(bid));
      break;
    }
    case SHVT_INT4:
    {
      // Couldn't find int4 getter in ShaderGlobal::
      ImGui::LabelText(widget_label, "[int4]");
      break;
    }
    default: break;
  }
}

static void shadervar_favourites(eastl::vector<eastl::string> &favourite_shvars, bool &favourites_changed)
{
  static eastl::string uniqueLabel;

  if (ImGui::CollapsingHeader("Favourites", ImGuiTreeNodeFlags_DefaultOpen))
  {
    int indexToRemove = -1;
    for (int i = 0; i < favourite_shvars.size(); i++)
    {
      uniqueLabel.sprintf("Unpin##%d", i);
      if (ImGui::Button(uniqueLabel.c_str()))
        indexToRemove = i;

      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("##MoveUp%d", i);
      if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Up))
        swap_favourites(favourite_shvars, i, i - 1, nullptr, favourites_changed);
      ImGui::SameLine(0, 2);
      uniqueLabel.sprintf("##MoveDown%d", i);
      if (ImGui::ArrowButton(uniqueLabel.c_str(), ImGuiDir_Down))
        swap_favourites(favourite_shvars, i, i + 1, nullptr, favourites_changed);

      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionMax().x * 0.6f - ImGui::GetCursorPosX());
      uniqueLabel.sprintf("%s##fav", favourite_shvars[i].c_str());
      shadervar_widget(favourite_shvars[i].c_str(), uniqueLabel.c_str());
    }
    if (indexToRemove >= 0)
    {
      favourite_shvars.erase(favourite_shvars.begin() + indexToRemove);
      favourites_changed = true;
    }
  }
}

static void shadervar_all(eastl::vector<eastl::string> &favourite_shvars, bool &favourites_changed)
{
  static eastl::string uniqueLabel;

  if (ImGui::CollapsingHeader("All", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static eastl::string shaderVarSearchText;
    ImGui::SetNextItemWidth(-CLEAR_BUTTON_WIDTH - 2);
    ImGuiDagor::InputTextWithHint("##Search", "Search", &shaderVarSearchText);
    ImGui::SameLine(0, 2);
    if (ImGui::Button("Clear", ImVec2(CLEAR_BUTTON_WIDTH, 0)))
      shaderVarSearchText.clear();
    ImGui::Separator();

    ImGui::BeginChild("AllShaderVarPanel");

    for (int id = 0, ie = VariableMap::getGlobalVariablesCount(); id < ie; id++)
    {
      const char *varName = VariableMap::getGlobalVariableName(id);
      if (!varName || !strstr(varName, shaderVarSearchText.c_str()))
        continue;

      uniqueLabel.sprintf("Pin##%d", id);
      if (ImGui::Button(uniqueLabel.c_str()))
      {
        if (eastl::find(favourite_shvars.begin(), favourite_shvars.end(), varName) == favourite_shvars.end())
        {
          favourite_shvars.push_back(varName);
          favourites_changed = true;
        }
      }

      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionMax().x * 0.6f - ImGui::GetCursorPosX());
      uniqueLabel.sprintf("%s##all", varName);
      shadervar_widget(varName, uniqueLabel.c_str());
    }

    ImGui::EndChild();
  }
}

static void shader_variables_tab(DataBlock *blk)
{
  static eastl::vector<eastl::string> favouriteShaderVars;
  const char *favouriteParamName = "favouriteShaderVariable";
  init_favourites(favouriteShaderVars, blk, favouriteParamName);
  bool favouritesChanged = false;

  shadervar_favourites(favouriteShaderVars, favouritesChanged);
  shadervar_all(favouriteShaderVars, favouritesChanged);

  if (favouritesChanged)
  {
    save_favourites(favouriteShaderVars, blk, favouriteParamName);
    imgui_save_blk();
  }
}

void console::imgui_window()
{
  DataBlock *blk = imgui_get_blk()->addBlock("ConsoleWindow");

  if (ImGui::BeginTabBar("ConsoleWindowTabBar"))
  {
    if (ImGui::BeginTabItem("Console commands"))
    {
      console_commands_tab(blk);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Console variables"))
    {
      console_variables_tab(blk);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Shader variables"))
    {
      shader_variables_tab(blk);
      ImGui::EndTabItem();
    }
  }
  ImGui::EndTabBar();
}
