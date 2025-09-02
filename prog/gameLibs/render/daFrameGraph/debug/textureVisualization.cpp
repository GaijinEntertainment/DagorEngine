// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textureVisualization.h"
#include <frontend/internalRegistry.h>
#include <frontend/dependencyData.h>
#include <gui/dag_imguiUtil.h>
#include <gui/dag_imgui.h>
#include <render/daFrameGraph/daFG.h>
#include <runtime/runtime.h>
#include <util/dag_console.h>
#include <drv/3d/dag_texture.h>
#include <render/texDebug.h>
#include <generic/dag_enumerate.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/unordered_set.h>

static dafg::NodeHandle debugTextureCopyNode;
static UniqueTex copiedTexture;

static bool overlayMode = false;

static const ImVec4 DISABLED_TEXT_COLOR = ImGuiStyle().Colors[ImGuiCol_TextDisabled];

class IntermediateTextureProvider;
using TextureProviderKey = eastl::string;
static ska::flat_hash_map<TextureProviderKey, IntermediateTextureProvider> texture_providers;

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


dag::Vector<dafg::NodeNameId, framemem_allocator> filter_out_debug_node(eastl::span<const dafg::NodeNameId> exec_order,
  const dafg::InternalRegistry &registry)
{
  dag::Vector<dafg::NodeNameId, framemem_allocator> result;
  result.assign(exec_order.begin(), exec_order.end());
  auto debugNodeNameId = registry.knownNames.getNameId<dafg::NodeNameId>(registry.knownNames.root(), "debug_texture_copy_node");
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

void overlay_checkbox(const char *label)
{
  ImGui::Checkbox(label, &overlayMode);
  ImGui::SameLine();
  String hint(0, "If enabled, the selected texture will be displayed as overlay (similar to render.show_tex).\n"
                 "If disabled, Texture Debugger window will be opened for the selected texture.");
  ImGuiDagor::HelpMarker(hint);
}

static UniqueTex create_empty_debug_tex(int fmt, int w, int h, int miplevels, const char *name)
{
  return dag::create_tex(nullptr, w, h, fmt, miplevels, name);
}

static const char *get_selected_resource_name(dafg::ResNameId nameId, const dafg::InternalRegistry &registry)
{
  return nameId == dafg::ResNameId::Invalid ? "None" : registry.knownNames.getName(nameId);
}

static void update_texdebug_texture()
{
  if (!savedSelection)
  {
    texdebug::select_texture("");
    return;
  }

  texdebug::select_texture(copiedTexture->getTexName());
}

// Calculated assuming that fgDebugTexBrightness of 1 corresponds to range limit of 1,
// and fgDebugTexBrightness of 10 corresponds to range limit of 0.001.
static float brightness_to_range_limit(float brightness) { return pow(10, (1 - brightness) / 3); }

static void update_overlay_texture()
{
  if (!savedSelection)
  {
    console::command("render.show_tex");
    return;
  }

  String showTexCommand(0, "render.show_tex ");
  showTexCommand.aprintf(0, "%s ", copiedTexture->getTexName());
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

static void update_shown_texture()
{
  bool imguiOFF = imgui_get_state() == ImGuiState::OFF;
  overlayMode || imguiOFF ? update_overlay_texture() : update_texdebug_texture();
}

void fg_texture_visualization_imgui_line(const dafg::InternalRegistry &registry)
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

auto reconstruct_namespace_request(dafg::Registry reg, const char *full_path)
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

// Normally it's not allowed to modify framegraph structure when it's being executed.
// Modifies are deferred in fact, it's needed to prevent user mistakes.
// However, texture visulization may be rendered in the node and it should be able to create debug nodes on the fly.
// So need to workaround graph change lock.
class DebugNodeRegisterScope
{
public:
  explicit DebugNodeRegisterScope()
  {
    G_ASSERT(dafg::Runtime::isInitialized());

    auto &tracker = dafg::Runtime::get().getNodeTracker();
    needToLock = tracker.isLocked();
    tracker.unlock();
  }
  ~DebugNodeRegisterScope()
  {
    auto &tracker = dafg::Runtime::get().getNodeTracker();
    G_ASSERT(!tracker.isLocked());
    if (needToLock)
      tracker.lock();
  }

private:
  bool needToLock = false;
};

template <bool debug_focus = true>
static dafg::NodeHandle makeDebugTextureCopyNode(const Selection &selection, dafg::InternalRegistry &internalRegistry,
  UniqueTex &texture, const char *texture_name)
{
  String nodeName({}, "debug_%s_copy_node", texture_name);
  nodeName.replaceAll("/", "_");
  String textureName({}, "debug_%s_copy", texture_name);
  textureName.replaceAll("/", "_");
  return dafg::register_node(nodeName, DAFG_PP_NODE_SRC,
    [selection, &internalRegistry, nodeName, &texture, textureName](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);

      const auto ourId = internalRegistry.knownNames.getNameId<dafg::NodeNameId>(internalRegistry.knownNames.root(), nodeName);

      const auto fullSelectedResName = internalRegistry.knownNames.getName(selection.what);
      const auto resNs = reconstruct_namespace_request(registry, fullSelectedResName);
      const auto shortSelectedResName = internalRegistry.knownNames.getShortName(selection.what);

      dafg::VirtualResourceHandle<const BaseTexture, true, false> hndl =
        resNs.read(shortSelectedResName).texture().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();


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

      return [hndl, fullSelectedResName, &texture, textureName]() {
        if (hndl.get())
        {
          TextureInfo srcInfo;
          hndl.view()->getinfo(srcInfo);

          if (!texture)
          {
            texture = create_empty_debug_tex(srcInfo.cflg | TEXCF_RTARGET, srcInfo.w, srcInfo.h, srcInfo.mipLevels, textureName);
            copiedTextureChannelCount = get_tex_format_desc(srcInfo.cflg).channelsCount();
            if constexpr (debug_focus)
              runShowTex = true;
          }
          else
          {
            TextureInfo dstInfo;
            texture->getinfo(dstInfo);
            G_ASSERTF_RETURN(dstInfo.cflg == (srcInfo.cflg | TEXCF_RTARGET), ,
              "Source and debug texture formats are different. Debug texture should be recreated.");
          }

          texture.getBaseTex()->update(hndl.view().getBaseTex());
        }
        else
          logerr("daFG: <%s> texture handle returned NULL - no copy was created", fullSelectedResName);

        if constexpr (debug_focus)
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

void update_fg_debug_tex(const eastl::optional<Selection> &selection, dafg::InternalRegistry &registry)
{
  const bool changed = selection != savedSelection;
  if (changed)
  {
    DebugNodeRegisterScope debugRegistration;
    runShowTex = true;
    if (selection)
    {
      if (registry.resources[selection->what].type != dafg::ResourceType::Texture)
      {
        selectedResourceIsATexture = false;
        return;
      }
      selectedResourceIsATexture = true;
      savedSelection = selection;
      close_visualization_texture();
      debugTextureCopyNode =
        makeDebugTextureCopyNode(*selection, registry, copiedTexture, get_selected_resource_name(selection->what, registry));
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

class IntermediateTextureProvider : public texdebug::ExternalTextureProvider
{
public:
  IntermediateTextureProvider(Selection selection, dafg::InternalRegistry &registry, const TextureInfo &tex_info, const char *name) :
    registry(&registry), selection(selection), textureInfo(tex_info), name(name)
  {}
  IntermediateTextureProvider(const IntermediateTextureProvider &) = delete;
  IntermediateTextureProvider &operator=(const IntermediateTextureProvider &) = delete;
  IntermediateTextureProvider(IntermediateTextureProvider &&other)
  {
    registry = other.registry;
    readerNode = eastl::move(other.readerNode);
    selection = eastl::move(other.selection);
    texture = eastl::move(other.texture);
    textureInfo = eastl::move(other.textureInfo);
    name = eastl::move(other.name);
  };
  IntermediateTextureProvider &operator=(IntermediateTextureProvider &&other)
  {
    registry = other.registry;
    readerNode = eastl::move(other.readerNode);
    selection = eastl::move(other.selection);
    texture = eastl::move(other.texture);
    textureInfo = eastl::move(other.textureInfo);
    name = eastl::move(other.name);
    return *this;
  }
  ~IntermediateTextureProvider() override = default;

  void init() override
  {
    close();
    DebugNodeRegisterScope debugRegistration;
    readerNode = makeDebugTextureCopyNode<false>(selection, *registry, texture, name);
  }

  void close() override
  {
    DebugNodeRegisterScope debugRegistration;
    readerNode = {};
    if (texture)
      texture.close();
  }

  TextureInfo getTextureInfo() override
  {
    if (texture)
      texture->getinfo(textureInfo);

    return textureInfo;
  }

  TEXTUREID getTextureId() const override { return texture ? texture.getTexId() : BAD_TEXTUREID; }
  const char *getName() const override { return name; }
  bool isUsed() const { return readerNode.valid(); }
  Selection getSelection() const { return selection; }

private:
  dafg::InternalRegistry *registry;
  dafg::NodeHandle readerNode{};
  Selection selection;
  UniqueTex texture{};
  TextureInfo textureInfo;
  String name;
};

static bool is_same_selection(const Selection &lhs, const Selection &rhs) { return lhs.when == rhs.when && lhs.what == rhs.what; }

static void add_texture_provider(IntermediateTextureProvider &&provider)
{
  auto providerIt = texture_providers.find(provider.getName());
  if (providerIt != texture_providers.end())
  {
    if (!is_same_selection(provider.getSelection(), providerIt->second.getSelection()))
    {
      if (providerIt->second.isUsed())
        provider.init();
      providerIt->second = eastl::move(provider);
    }
  }
  else
    providerIt = texture_providers.emplace(provider.getName(), eastl::move(provider)).first;
}

template <typename T>
using FramememUnorderedSet = eastl::unordered_set<T, eastl::hash<T>, eastl::equal_to<T>, framemem_allocator>;

void update_external_texture_visualization(dafg::InternalRegistry &registry, const dafg::DependencyData &dep_data)
{
  FRAMEMEM_REGION;
  FramememUnorderedSet<TextureProviderKey> unregisterList;
  dag::Vector<TextureProviderKey, framemem_allocator> registerList;
  for (auto &[key, provider] : texture_providers)
    unregisterList.insert(key);

  for (const auto [resId, lifetime] : dep_data.resourceLifetimes.enumerate())
  {
    if (registry.resources[resId].type != dafg::ResourceType::Texture)
      continue;

    if (lifetime.consumedBy == dafg::NodeNameId::Invalid && lifetime.readers.empty())
      continue;

    TextureInfo info{};
    if (auto resInfo = eastl::get_if<dafg::Texture2dCreateInfo>(&registry.resources[resId].creationInfo))
    {
      info.type = D3DResourceType::TEX;
      info.cflg = resInfo->creationFlags;
    }
    else if (auto resInfo = eastl::get_if<dafg::Texture3dCreateInfo>(&registry.resources[resId].creationInfo))
    {
      info.type = D3DResourceType::VOLTEX;
      info.cflg = resInfo->creationFlags;
    }

    dag::Vector<dafg::NodeNameId, framemem_allocator> resourceUsageChain;
    resourceUsageChain.reserve(lifetime.modificationChain.size() + 2 /*producer and consumer*/);
    resourceUsageChain.push_back(lifetime.introducedBy);
    for (const auto &mod : lifetime.modificationChain)
      resourceUsageChain.push_back(mod);
    resourceUsageChain.push_back(lifetime.consumedBy != dafg::NodeNameId::Invalid ? lifetime.consumedBy : lifetime.readers.front());

    for (size_t idx = 0; idx < resourceUsageChain.size() - 1; ++idx)
    {
      const auto from = resourceUsageChain[idx];
      const auto to = resourceUsageChain[idx + 1];
      const char *fromName = registry.knownNames.getShortName(from);
      const char *toName = registry.knownNames.getShortName(to);
      const char *resourceName = registry.knownNames.getShortName(resId);

      const auto addNewProvider = [&, resId = resId](const char *node, const char *dir) {
        String name(0, "%s/%s/%s", node, dir, resourceName);
        add_texture_provider(IntermediateTextureProvider(Selection{PreciseTimePoint{from, to}, resId}, registry, info, name));
        registerList.push_back(name.c_str());
        unregisterList.erase(name.c_str());
      };

      addNewProvider(fromName, "out");
      addNewProvider(toName, "in");
    }
  }

  for (const auto &providerName : unregisterList)
  {
    auto providerIt = texture_providers.find(providerName);
    G_ASSERT(providerIt != texture_providers.end());
    texdebug::unregister_external_texture(providerIt->second);
    providerIt->second.close();
  }

  for (const auto &providerNames : registerList)
  {
    auto providerIt = texture_providers.find(providerNames);
    G_ASSERT(providerIt != texture_providers.end());
    texdebug::register_external_texture(providerIt->second);
  }
}
