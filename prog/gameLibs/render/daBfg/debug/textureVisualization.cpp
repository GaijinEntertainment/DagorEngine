#include "textureVisualization.h"
#include <frontend/internalRegistry.h>
#include <gui/dag_imguiUtil.h>
#include <render/daBfg/bfg.h>
#include <util/dag_console.h>


static const ImVec4 DISABLED_TEXT_COLOR = ImGuiStyle().Colors[ImGuiCol_TextDisabled];
static float fgDebugTexBrightness = 0.f;
static eastl::string consoleInput = "";
static eastl::string showTexArgs = "";
static int copiedTextureChannelCount = 0;
static bool selectedResourceIsATexture = true;
static bool runShowTex = false;
static dabfg::ResNameId savedResId = dabfg::ResNameId::Invalid;
static dabfg::NodeNameId savedPrecedingNode = dabfg::NodeNameId::Invalid;


static bool imgui_disableable_button(const char *label, bool condition_for_disabling)
{
  if (condition_for_disabling)
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

// Calculated assuming that fgDebugTexBrightness of 1 corresponds to range limit of 1,
// and fgDebugTexBrightness of 10 corresponds to range limit of 0.001.
static float brightness_to_range_limit() { return pow(10, (1 - fgDebugTexBrightness) / 3); }

static UniqueTex create_empty_debug_tex(int fmt, int w, int h) { return dag::create_tex(NULL, w, h, fmt, 1, "copied_fg_debug_tex"); }

static const char *get_selected_resource_name(dabfg::ResNameId nameId, const dabfg::InternalRegistry &registry)
{
  return nameId == dabfg::ResNameId::Invalid ? "None" : registry.knownNames.getName(nameId);
}

static void display_fg_debug_tex()
{
  String showTexCommand(0, "render.show_tex copied_fg_debug_tex ");
  if (!showTexArgs.empty())
  {
    showTexCommand.aprintf(0, "%s", showTexArgs);
    console::command(showTexCommand);
  }
  else if (copiedTextureChannelCount == 1)
  {
    if (!fgDebugTexBrightness)
      fgDebugTexBrightness = 3.2f;
    showTexCommand.aprintf(0, "rrr 0.0 %f", brightness_to_range_limit());
    console::command(showTexCommand);
  }
  else
  {
    if (!fgDebugTexBrightness)
      fgDebugTexBrightness = 1.f;
    showTexCommand.aprintf(0, "rgb 0.0 %f", brightness_to_range_limit());
    console::command(showTexCommand);
  }
}

static void hide_fg_debug_tex(bool resetSettings)
{
  if (fgDebugTexBrightness == 0.f)
    return;
  console::command("render.show_tex");
  if (resetSettings)
  {
    consoleInput = "";
    showTexArgs = "";
    fgDebugTexBrightness = 0.f;
  }
}

void fg_texture_visualization_imgui_line(dabfg::ResNameId &selectedResId, const dabfg::InternalRegistry &registry,
  dabfg::NodeNameId selectedPrecedingNode)
{
  if (imgui_disableable_button("Deselect", selectedResId == dabfg::ResNameId::Invalid) ||
      selectedPrecedingNode == dabfg::NodeNameId::Invalid)
  {
    selectedResId = dabfg::ResNameId::Invalid;
    runShowTex = true;
  }

  ImGui::SameLine();
  if (!selectedResourceIsATexture && selectedResId != dabfg::ResNameId::Invalid)
    ImGui::TextColored((ImVec4)ImColor::HSV(0.f, 1.f, 1.f), "Selected resource is not a texture, so it cannot be displayed.");
  else
  {
    String selectedTextureNotification(0, "Selected texture: %s", get_selected_resource_name(selectedResId, registry));
    if (selectedResId != dabfg::ResNameId::Invalid)
      selectedTextureNotification.aprintf(0, "; sampled after: %s", registry.knownNames.getName(savedPrecedingNode));
    ImGui::Text("%s", selectedTextureNotification.c_str());
  }

  if (selectedResId != dabfg::ResNameId::Invalid && selectedResourceIsATexture)
  {
    ImGui::SameLine(0.f, ImGui::GetWindowWidth() * 0.015f);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);
    ImGuiInputTextFlags input_text_flags =
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGuiDagor::InputTextWithHint("", "'render.show_tex <tex_name>' args", &consoleInput, input_text_flags))
    {
      showTexArgs = consoleInput;
      runShowTex = true;
    }

    ImGui::SameLine();
    String hint(0,
      "Refer to 'render.show_tex' command documentation when typing command arguments.\n\n"
      "Assume 'render.show_tex %s' has already been typed.\n\n"
      "'Brightness' slider will be ignored if input is not empty. Clear console input to use the slider.",
      get_selected_resource_name(selectedResId, registry));
    ImGuiDagor::HelpMarker(hint);

    if (fgDebugTexBrightness && showTexArgs.empty())
    {
      ImGui::SameLine(0.f, ImGui::GetWindowWidth() * 0.015f);
      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.1f);
      if (ImGui::SliderFloat("Brightness", &fgDebugTexBrightness, 1.f, 10.f, "%.1f"))
        runShowTex = true;
    }
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

static dabfg::NodeHandle makeDebugTextureCopyNode(dabfg::NodeNameId precedingNode, dabfg::NodeNameId followingNode,
  dabfg::ResNameId selectedResId, UniqueTex &copiedTexture, dabfg::InternalRegistry &internalRegistry)
{
  return dabfg::register_node("debug_texture_copy_node", DABFG_PP_NODE_SRC,
    [precedingNode, followingNode, selectedResId, &copiedTexture, &internalRegistry](dabfg::Registry registry) {
      const auto ourId =
        internalRegistry.knownNames.getNameId<dabfg::NodeNameId>(internalRegistry.knownNames.root(), "debug_texture_copy_node");
      internalRegistry.nodes[ourId].precedingNodeIds.insert(precedingNode);
      internalRegistry.nodes[ourId].followingNodeIds.insert(followingNode);

      const auto fullSelectedResName = internalRegistry.knownNames.getName(selectedResId);
      const auto resNs = reconstruct_namespace_request(registry, fullSelectedResName);
      const auto shortSelectedResName = internalRegistry.knownNames.getShortName(selectedResId);

      dabfg::VirtualResourceHandle<const BaseTexture, true, false> hndl =
        resNs.read(shortSelectedResName).texture().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();

      // Since we  want to read resources from anywhere within a framegraph, we need to clear readResources of this node
      // in order to prevent possible cycles.
      internalRegistry.nodes[ourId].readResources.clear();

      return [hndl, fullSelectedResName, &copiedTexture]() {
        if (hndl.get())
        {
          if (!copiedTexture)
          {
            TextureInfo texInfo;
            hndl.view()->getinfo(texInfo);
            copiedTexture = create_empty_debug_tex(TEXFMT_A16B16G16R16F | TEXCF_RTARGET, texInfo.w, texInfo.h);
            copiedTextureChannelCount = get_tex_format_desc(texInfo.cflg).channelsCount();
          }
          d3d::stretch_rect(hndl.view().getTex2D(), copiedTexture.getTex2D());
        }
        else
          logerr("<%s> texture handle returned NULL - no copy was created", fullSelectedResName);
      };
    });
}

static dabfg::NodeHandle update_debug_texture_copy_node(dabfg::NodeNameId precedingNode, dabfg::NodeNameId followingNode,
  dabfg::ResNameId selectedResId, UniqueTex &copiedTexture, dabfg::InternalRegistry &internalRegistry)
{
  if (precedingNode == dabfg::NodeNameId::Invalid || internalRegistry.resources[selectedResId].type != dabfg::ResourceType::Texture)
  {
    selectedResourceIsATexture = false;
    return {};
  }
  selectedResourceIsATexture = true;
  savedPrecedingNode = precedingNode;
  return makeDebugTextureCopyNode(precedingNode, followingNode, selectedResId, copiedTexture, internalRegistry);
}

void update_fg_debug_tex(dabfg::NodeNameId selectedPrecedingNode, dabfg::NodeNameId *selectedFollowingNode,
  dabfg::ResNameId selectedResId, UniqueTex &copiedTexture, dabfg::InternalRegistry &registry, dabfg::NodeHandle &debugTextureCopyNode)
{
  const bool resetSettings = selectedResId != dabfg::ResNameId::Invalid && savedResId != selectedResId;
  if (selectedFollowingNode && selectedResId != dabfg::ResNameId::Invalid)
  {
    runShowTex = true;
    savedResId = selectedResId;
    hide_fg_debug_tex(resetSettings);
    debugTextureCopyNode =
      update_debug_texture_copy_node(selectedPrecedingNode, *selectedFollowingNode, selectedResId, copiedTexture, registry);
  }

  if (runShowTex && (selectedResId == dabfg::ResNameId::Invalid || !selectedResourceIsATexture))
  {
    debugTextureCopyNode = {};
    runShowTex = false;
    hide_fg_debug_tex(resetSettings);
  }

  if (runShowTex && copiedTexture)
  {
    runShowTex = false;
    display_fg_debug_tex();
  }
}