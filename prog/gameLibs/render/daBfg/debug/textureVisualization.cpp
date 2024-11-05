// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textureVisualization.h"
#include <frontend/internalRegistry.h>
#include <gui/dag_imguiUtil.h>
#include <render/daBfg/bfg.h>
#include <util/dag_console.h>
#include <drv/3d/dag_texture.h>


static dabfg::NodeHandle debugTextureCopyNode;
static UniqueTex copiedTexture;

static const ImVec4 DISABLED_TEXT_COLOR = ImGuiStyle().Colors[ImGuiCol_TextDisabled];

struct AutoShowTexSettings
{
  float fgDebugTexBrightness = 0.f;
};
struct ManualShowTexSettings
{
  eastl::string showTexArgs = "";
};
using ShowTexSettings = eastl::variant<AutoShowTexSettings, ManualShowTexSettings>;
static ShowTexSettings showTexSettings;

static int copiedTextureChannelCount = 0;
static bool selectedResourceIsATexture = true;
static bool runShowTex = false;
static eastl::optional<Selection> savedSelection;


dag::Vector<dabfg::NodeNameId, framemem_allocator> filter_out_debug_node(eastl::span<const dabfg::NodeNameId> exec_order,
  const dabfg::InternalRegistry &registry)
{
  dag::Vector<dabfg::NodeNameId, framemem_allocator> result;
  result.assign(exec_order.begin(), exec_order.end());
  auto debugNodeNameId = registry.knownNames.getNameId<dabfg::NodeNameId>(registry.knownNames.root(), "debug_texture_copy_node");
  erase_item_by_value(result, debugNodeNameId);
  return result;
}

void close_visualization_texture() { copiedTexture.close(); }

void clear_visualization_node() { debugTextureCopyNode = {}; }

bool deselect_button(const char *label)
{
  if (!savedSelection)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, DISABLED_TEXT_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DISABLED_TEXT_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, DISABLED_TEXT_COLOR);
  }
  else
  {
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.7f, 0.8f));
  }
  bool buttonPressed = ImGui::Button(label);
  ImGui::PopStyleColor(3);
  return buttonPressed;
}

static UniqueTex create_empty_debug_tex(int fmt, int w, int h) { return dag::create_tex(NULL, w, h, fmt, 1, "copied_fg_debug_tex"); }

static const char *get_selected_resource_name(dabfg::ResNameId nameId, const dabfg::InternalRegistry &registry)
{
  return nameId == dabfg::ResNameId::Invalid ? "None" : registry.knownNames.getName(nameId);
}

// Calculated assuming that fgDebugTexBrightness of 1 corresponds to range limit of 1,
// and fgDebugTexBrightness of 10 corresponds to range limit of 0.001.
static float brightness_to_range_limit(float brightness) { return pow(10, (1 - brightness) / 3); }

static void update_shown_texture()
{
  if (!savedSelection)
  {
    console::command("render.show_tex");
    return;
  }

  String showTexCommand(0, "render.show_tex copied_fg_debug_tex ");
  if (auto manualSettings = eastl::get_if<ManualShowTexSettings>(&showTexSettings))
  {
    showTexCommand.aprintf(0, "%s", manualSettings->showTexArgs);
    console::command(showTexCommand);
  }
  else if (auto autoSettings = eastl::get_if<AutoShowTexSettings>(&showTexSettings))
  {
    if (copiedTextureChannelCount == 1)
    {
      if (autoSettings->fgDebugTexBrightness < 0.0001f)
        autoSettings->fgDebugTexBrightness = 3.2f;
      showTexCommand.aprintf(0, "rrr 0.0 %f", brightness_to_range_limit(autoSettings->fgDebugTexBrightness));
      console::command(showTexCommand);
    }
    else
    {
      if (autoSettings->fgDebugTexBrightness < 0.0001f)
        autoSettings->fgDebugTexBrightness = 1.f;
      showTexCommand.aprintf(0, "rgb 0.0 %f", brightness_to_range_limit(autoSettings->fgDebugTexBrightness));
      console::command(showTexCommand);
    }
  }
}

void fg_texture_visualization_imgui_line(const dabfg::InternalRegistry &registry)
{
  ImGui::SameLine();

  if (!selectedResourceIsATexture)
  {
    ImGui::TextColored((ImVec4)ImColor::HSV(0.f, 1.f, 1.f), "Selected resource is not a texture, so it cannot be displayed.");
    return;
  }

  if (!savedSelection)
  {
    ImGui::Text("Right click on an edge and select a texture to visualize it's contents at a precise time point!");
    return;
  }

  String selectedTextureNotification(0, "Selected texture: %s", get_selected_resource_name(savedSelection->what, registry));

  if (auto prec = eastl::get_if<PreciseTimePoint>(&savedSelection->when))
    selectedTextureNotification.aprintf(0, "; sampled after: %s", registry.knownNames.getName(prec->precedingNode));
  else
    selectedTextureNotification.aprintf(0, "; sampled as-if read by a node");

  ImGui::Text("%s", selectedTextureNotification.c_str());

  ImGui::SameLine(0.f, ImGui::GetWindowWidth() * 0.015f);
  ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);
  ImGuiInputTextFlags input_text_flags =
    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

  static eastl::string consoleInput = "";
  if (ImGuiDagor::InputTextWithHint("##show_tex_command_input", "'render.show_tex <tex_name>' args", &consoleInput, input_text_flags))
  {
    if (consoleInput.empty())
      showTexSettings = AutoShowTexSettings{};
    else
      showTexSettings = ManualShowTexSettings{consoleInput};
    runShowTex = true;
  }

  ImGui::SameLine();
  String hint(0,
    "Refer to 'render.show_tex' command documentation when typing command arguments.\n\n"
    "Assume 'render.show_tex %s' has already been typed.\n\n"
    "'Brightness' slider will be ignored if input is not empty. Clear console input to use the slider.",
    get_selected_resource_name(savedSelection->what, registry));
  ImGuiDagor::HelpMarker(hint);

  if (auto autoSettings = eastl::get_if<AutoShowTexSettings>(&showTexSettings))
  {
    ImGui::SameLine(0.f, ImGui::GetWindowWidth() * 0.015f);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.1f);
    if (ImGui::SliderFloat("Brightness", &autoSettings->fgDebugTexBrightness, 1.f, 10.f, "%.1f"))
      runShowTex = true;
  }
}

auto reconstruct_namespace_request(dabfg::Registry reg, const char *full_path)
{
  auto nameSpaceReq = reg.root();
  G_ASSERT(full_path[0] == '/');
  for (auto current = full_path + 1;;)
  {
    const auto next = strchr(current, '/');
    if (next == nullptr)
      break;

    eastl::fixed_string<char, 256> folderName(current, next);
    nameSpaceReq = nameSpaceReq / folderName.c_str();

    current = next + 1;
  }
  return nameSpaceReq;
}

static dabfg::NodeHandle makeDebugTextureCopyNode(const Selection &selection, dabfg::InternalRegistry &internalRegistry)
{
  return dabfg::register_node("debug_texture_copy_node", DABFG_PP_NODE_SRC, [selection, &internalRegistry](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);

    const auto ourId =
      internalRegistry.knownNames.getNameId<dabfg::NodeNameId>(internalRegistry.knownNames.root(), "debug_texture_copy_node");

    const auto fullSelectedResName = internalRegistry.knownNames.getName(selection.what);
    const auto resNs = reconstruct_namespace_request(registry, fullSelectedResName);
    const auto shortSelectedResName = internalRegistry.knownNames.getShortName(selection.what);

    dabfg::VirtualResourceHandle<const BaseTexture, true, false> hndl =
      resNs.read(shortSelectedResName).texture().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();


    if (auto prec = eastl::get_if<PreciseTimePoint>(&selection.when))
    {
      // Must be done this way in order to work with namespaces, user-facing
      // API does not permit explicit ordering with nodes in different namespaces.
      internalRegistry.nodes[ourId].precedingNodeIds.insert(prec->precedingNode);
      internalRegistry.nodes[ourId].followingNodeIds.insert(prec->followingNode);
      // Since we  want to read resources from anywhere within a framegraph when doing precise selection,
      // we need to clear readResources of this node in order to prevent possible cycles.
      internalRegistry.nodes[ourId].readResources.clear();
    }

    return [hndl, fullSelectedResName]() {
      if (hndl.get())
      {
        if (!copiedTexture)
        {
          TextureInfo texInfo;
          hndl.view()->getinfo(texInfo);
          copiedTexture = create_empty_debug_tex(TEXFMT_A16B16G16R16F | TEXCF_RTARGET, texInfo.w, texInfo.h);
          copiedTextureChannelCount = get_tex_format_desc(texInfo.cflg).channelsCount();

          runShowTex = true;
        }
        d3d::stretch_rect(hndl.view().getBaseTex(), copiedTexture.getBaseTex());
      }
      else
        logerr("<%s> texture handle returned NULL - no copy was created", fullSelectedResName);

      if (eastl::exchange(runShowTex, false))
        update_shown_texture();
    };
  });
}

static bool operator==(const ReadTimePoint &, const ReadTimePoint &) { return true; }

static bool operator==(const PreciseTimePoint &lhs, const PreciseTimePoint &rhs)
{
  return lhs.precedingNode == rhs.precedingNode && lhs.followingNode == rhs.followingNode;
}

static bool operator==(const Selection &lhs, const Selection &rhs) { return lhs.when == rhs.when && lhs.what == rhs.what; }

void update_fg_debug_tex(const eastl::optional<Selection> &selection, dabfg::InternalRegistry &registry)
{
  const bool changed = selection != savedSelection;
  if (changed)
  {
    runShowTex = true;
    if (selection)
    {
      if (registry.resources[selection->what].type != dabfg::ResourceType::Texture)
      {
        selectedResourceIsATexture = false;
        return;
      }
      selectedResourceIsATexture = true;
      savedSelection = selection;
      debugTextureCopyNode = makeDebugTextureCopyNode(*selection, registry);
    }
    else
    {
      selectedResourceIsATexture = true;
      savedSelection = {};
      showTexSettings = AutoShowTexSettings{};
      close_visualization_texture();
      clear_visualization_node();
      update_shown_texture();
    }
  }
}

void set_manual_showtex_params(const char *params)
{
  showTexSettings = ManualShowTexSettings{params};
  runShowTex = true;
}
