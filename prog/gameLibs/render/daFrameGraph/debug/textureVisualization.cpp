// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textureVisualization.h"
#include <common/genericPoint.h>
#include <frontend/internalRegistry.h>
#include <frontend/dependencyData.h>
#include <gui/dag_imguiUtil.h>
#include <gui/dag_imgui.h>
#include <render/daFrameGraph/daFG.h>
#include <runtime/runtime.h>
#include <backend/resourceScheduling/resourceLifetimes.h>
#include <util/dag_console.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>
#include <render/texDebug.h>
#include <generic/dag_enumerate.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/unordered_set.h>
#include <EASTL/unordered_map.h>

static dafg::NodeHandle debugTextureCopyNode;
static UniqueTex copiedTexture;
static String copiedTextureName;

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

static TextureInfo copiedTextureInfo;
static eastl::optional<dafg::AutoResolutionData> copiedTextureResolution;
static IdIndexedMapping<dafg::AutoResTypeNameId, IPoint2> prevAppliedResolutions;
static int copiedTextureChannelCount = 0;
static bool selectedResourceIsATexture = true;
static bool externalTexturesOutdated = false;
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

void clear_visualization_node()
{
  if (debugTextureCopyNode.valid() && dafg::Runtime::isInitialized())
    dafg::root().unmarkResourceExternallyConsumed(copiedTextureName);
  debugTextureCopyNode = {};
}

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

static String debug_copy_tex_name(const char *texture_name, bool history)
{
  String textureName({}, ".debug_%s%s_copy", texture_name, history ? "_history" : "");
  textureName.replaceAll("/", "_");
  return textureName;
}

static UniqueTex create_debug_copy_tex(const TextureInfo &info, const char *name)
{
  return dag::create_tex(nullptr, info.w, info.h, info.cflg, info.mipLevels, name, RESTAG_DEBUG);
}

static uint32_t debug_copy_tex_flags(uint32_t src_flags)
{
  return (src_flags & ~static_cast<uint32_t>(TEXCF_NO_STATE_TRACKING)) | TEXCF_RTARGET;
}

static eastl::optional<TextureInfo> get_physical_tex_info(const BaseTexture *tex)
{
  if (!tex)
    return eastl::nullopt;

  TextureInfo info{};
  tex->getinfo(info);
  if (info.type != D3DResourceType::TEX)
    return eastl::nullopt;

  info.cflg = debug_copy_tex_flags(info.cflg);
  return info;
}

static eastl::optional<IPoint2> get_applied_resolution(const dafg::AutoResolutionData &resolution, bool history,
  const dafg::InternalRegistry &registry)
{
  const auto autoResId = dafg::Runtime::get().getNameResolver().resolve(resolution.id);
  if (autoResId == dafg::AutoResTypeNameId::Invalid)
    return eastl::nullopt;

  const auto &autoResType = registry.autoResTypes[autoResId];
  const auto *values = eastl::get_if<dafg::ResolutionValues<IPoint2>>(&autoResType.values);
  if (!values)
    return eastl::nullopt;

  const bool historyHasPrevResolution =
    history && autoResType.dynamicResolutionCountdown == dafg::SCHEDULE_FRAME_WINDOW - 1 && prevAppliedResolutions.isMapped(autoResId);
  const IPoint2 resolutionValue = historyHasPrevResolution ? prevAppliedResolutions[autoResId] : values->lastAppliedDynamicResolution;
  return dafg::scale_by(resolutionValue, resolution.multiplier);
}

static eastl::optional<IPoint2> get_creation_resolution(const dafg::CreatedResourceData &created_res_data,
  const dafg::Texture2dCreateInfo &creation_info, bool history, const dafg::InternalRegistry &registry)
{
  if (created_res_data.resolution)
    return get_applied_resolution(*created_res_data.resolution, history, registry);

  if (const auto *resolution = eastl::get_if<IPoint2>(&creation_info.resolution))
    return *resolution;

  return eastl::nullopt;
}

static eastl::optional<TextureInfo> get_debug_copy_tex_info(const dafg::CreatedResourceData &created_res_data, bool history,
  const dafg::InternalRegistry &registry)
{
  if (const auto *provider = eastl::get_if<dafg::ExternalResourceProvider>(&created_res_data.creationInfo))
  {
    if (!*provider)
      return eastl::nullopt;

    const dafg::ExternalResource extRes = (*provider)(dafg::multiplexing::Index{});
    const auto *tex = eastl::get_if<ManagedTexView>(&extRes);
    return tex ? get_physical_tex_info(tex->getBaseTex()) : eastl::nullopt;
  }

  if (eastl::holds_alternative<dafg::DriverDeferredTexture>(created_res_data.creationInfo))
  {
    d3d::GpuAutoLock gpuLock;
    return get_physical_tex_info(d3d::get_backbuffer_tex());
  }

  const auto *creationInfo = eastl::get_if<dafg::Texture2dCreateInfo>(&created_res_data.creationInfo);
  if (!creationInfo)
    return eastl::nullopt;

  const auto resolution = get_creation_resolution(created_res_data, *creationInfo, history, registry);
  if (!resolution)
    return eastl::nullopt;

  TextureInfo info{};
  info.type = D3DResourceType::TEX;
  info.cflg = debug_copy_tex_flags(creationInfo->creationFlags);
  info.mipLevels = creationInfo->mipLevels;
  info.w = resolution->x;
  info.h = resolution->y;
  return info;
}

static bool fit_debug_copy_tex(UniqueTex &texture, TextureInfo &info, const eastl::optional<dafg::AutoResolutionData> &resolution,
  bool history, const dafg::InternalRegistry &registry)
{
  if (!texture || !resolution)
    return false;

  const auto applied = get_applied_resolution(*resolution, history, registry);
  if (!applied || *applied == IPoint2(info.w, info.h))
    return false;

  info.w = applied->x;
  info.h = applied->y;
  const String name(texture->getTexName());
  texture.close();
  texture = create_debug_copy_tex(info, name);
  return true;
}

static const char *get_selected_resource_name(dafg::ResNameId nameId, const dafg::InternalRegistry &registry)
{
  return nameId == dafg::ResNameId::Invalid ? "None" : registry.knownNames.getName(nameId);
}

static bool is_texture_array(const dafg::CreatedResourceData &created_res_data)
{
  auto info = eastl::get_if<dafg::Texture2dCreateInfo>(&created_res_data.creationInfo);
  return info && info->arrayLayers > 1;
}

static void update_texdebug_texture()
{
  if (!copiedTexture)
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
  if (!copiedTexture)
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

static bool is_overlay_shown() { return overlayMode || imgui_get_state() == ImGuiState::OFF; }

static void update_shown_texture() { is_overlay_shown() ? update_overlay_texture() : update_texdebug_texture(); }

void fg_texture_visualization_imgui_line(const dafg::InternalRegistry &registry)
{
  ImGui::SameLine();

  if (!selectedResourceIsATexture)
  {
    ImGui::TextColored((ImVec4)ImColor::HSV(0.f, 1.f, 1.f),
      "Selected resource is not a texture, or is a texture array, so it cannot be displayed.");
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
    update_shown_texture();
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
      update_shown_texture();
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
// However, texture visualization may be rendered in the node and it should be able to create debug nodes on the fly.
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

static dafg::NodeHandle make_debug_texture_copy_node(const Selection &selection, dafg::InternalRegistry &internal_registry,
  const UniqueTex &texture, const char *copy_name)
{
  const String nodeName(0, "%s_node", copy_name);
  const String textureName(copy_name);

  dafg::root().markResourceExternallyConsumed(textureName);

  return dafg::register_node(nodeName, DAFG_PP_NODE_SRC,
    [selection, &internal_registry, nodeName, &texture, textureName](dafg::Registry registry) {
      const auto ourId = internal_registry.knownNames.getNameId<dafg::NodeNameId>(internal_registry.knownNames.root(), nodeName);

      const auto fullSelectedResName = internal_registry.knownNames.getName(selection.what);
      const auto resNs = reconstruct_namespace_request(registry, fullSelectedResName);
      const auto shortSelectedResName = internal_registry.knownNames.getShortName(selection.what);

      registry.registerTexture(textureName, [&texture](auto) { return ManagedTexView(texture); });

      if (selection.history)
        resNs.readTextureHistory(shortSelectedResName).copyTo(textureName);
      else
        resNs.read(shortSelectedResName).texture().copyTo(textureName);

      // Order and trick with readResources are not required for history copy
      if (auto prec = eastl::get_if<PreciseTimePoint>(&selection.when); prec && !selection.history)
      {
        // Must be done this way in order to work with namespaces, user-facing
        // API does not permit explicit ordering with nodes in different namespaces.
        internal_registry.nodes[ourId].precedingNodeIds.insert(prec->precedingNode);
        internal_registry.nodes[ourId].followingNodeIds.insert(prec->followingNode);
        // Since we  want to read resources from anywhere within a framegraph when doing precise selection,
        // we need to clear readResources of this node in order to prevent possible cycles.
        internal_registry.nodes[ourId].readResources.clear();
      }
    });
}

static bool operator==(const ReadTimePoint &, const ReadTimePoint &) { return true; }

static bool operator==(const PreciseTimePoint &lhs, const PreciseTimePoint &rhs)
{
  return lhs.precedingNode == rhs.precedingNode && lhs.followingNode == rhs.followingNode;
}

static bool operator==(const Selection &lhs, const Selection &rhs)
{
  return lhs.when == rhs.when && lhs.what == rhs.what && lhs.history == rhs.history;
}

void update_fg_debug_tex(const eastl::optional<Selection> &selection, dafg::InternalRegistry &registry,
  const dafg::DependencyData &dep_data)
{
  const bool newSelection = selection != savedSelection;
  if (newSelection || (selection && !copiedTexture))
  {
    DebugNodeRegisterScope debugRegistration;
    if (selection)
    {
      const auto &createdResData = registry.resources[dep_data.renamingRepresentatives[selection->what]].createdResData;

      if (createdResData->type != dafg::ResourceType::Texture || is_texture_array(*createdResData))
      {
        selectedResourceIsATexture = false;
        return;
      }
      selectedResourceIsATexture = true;

      const char *selectedResName = get_selected_resource_name(selection->what, registry);

      savedSelection = selection;
      close_visualization_texture();
      clear_visualization_node();

      const auto copyInfo = get_debug_copy_tex_info(*createdResData, selection->history, registry);
      if (!copyInfo)
      {
        if (newSelection)
        {
          logerr("daFG: cannot copy texture '%s' for visualization, it is not a plain 2d texture or it is not provided.",
            selectedResName);
          update_shown_texture();
        }
        return;
      }

      copiedTextureName = debug_copy_tex_name(selectedResName, selection->history);
      copiedTexture = create_debug_copy_tex(*copyInfo, copiedTextureName);
      copiedTextureInfo = *copyInfo;
      copiedTextureResolution = createdResData->resolution;
      copiedTextureChannelCount = get_tex_format_desc(copyInfo->cflg).channelsCount();
      debugTextureCopyNode = make_debug_texture_copy_node(*selection, registry, copiedTexture, copiedTextureName);
      update_shown_texture();
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
  update_shown_texture();
}

class IntermediateTextureProvider : public texdebug::ExternalTextureProvider
{
public:
  IntermediateTextureProvider(Selection selection, dafg::InternalRegistry &registry, const TextureInfo &tex_info,
    const eastl::optional<dafg::AutoResolutionData> &resolution, const char *name) :
    registry(&registry), selection(selection), textureInfo(tex_info), resolution(resolution), name(name)
  {}
  IntermediateTextureProvider(const IntermediateTextureProvider &) = delete;
  IntermediateTextureProvider &operator=(const IntermediateTextureProvider &) = delete;
  IntermediateTextureProvider(IntermediateTextureProvider &&other) noexcept
  {
    registry = other.registry;
    readerNode = eastl::move(other.readerNode);
    selection = eastl::move(other.selection);
    texture = eastl::move(other.texture);
    textureInfo = eastl::move(other.textureInfo);
    resolution = eastl::move(other.resolution);
    name = eastl::move(other.name);
  };
  IntermediateTextureProvider &operator=(IntermediateTextureProvider &&other) noexcept
  {
    registry = other.registry;
    readerNode = eastl::move(other.readerNode);
    selection = eastl::move(other.selection);
    texture = eastl::move(other.texture);
    textureInfo = eastl::move(other.textureInfo);
    resolution = eastl::move(other.resolution);
    name = eastl::move(other.name);
    return *this;
  }
  ~IntermediateTextureProvider() override = default;

  void init() override
  {
    close();
    if (!refreshDescription())
      return;

    DebugNodeRegisterScope debugRegistration;
    const String copyName = getCopyName();
    texture = create_debug_copy_tex(textureInfo, copyName);
    readerNode = make_debug_texture_copy_node(selection, *registry, texture, copyName);
  }

  void close() override
  {
    DebugNodeRegisterScope debugRegistration;
    if (readerNode.valid())
      dafg::root().unmarkResourceExternallyConsumed(getCopyName());
    readerNode = {};
    if (texture)
      texture.close();
  }

  TextureInfo getTextureInfo() override
  {
    TextureInfo info = textureInfo;
    if (texture)
      texture->getinfo(info);

    return info;
  }

  void fitToAppliedResolution() { fit_debug_copy_tex(texture, textureInfo, resolution, selection.history, *registry); }

  TEXTUREID getTextureId() const override { return texture ? texture.getTexId() : BAD_TEXTUREID; }
  const char *getName() const override { return name; }
  bool isUsed() const { return readerNode.valid(); }
  Selection getSelection() const { return selection; }

private:
  String getCopyName() const { return debug_copy_tex_name(name, selection.history); }

  bool refreshDescription()
  {
    const auto &depData = dafg::Runtime::get().getDependencyData();
    const auto &createdResData = registry->resources[depData.renamingRepresentatives[selection.what]].createdResData;
    const auto info = createdResData ? get_debug_copy_tex_info(*createdResData, selection.history, *registry) : eastl::nullopt;
    if (!info)
      return false;

    textureInfo = *info;
    resolution = createdResData->resolution;
    return true;
  }

  dafg::InternalRegistry *registry;
  dafg::NodeHandle readerNode{};
  Selection selection;
  UniqueTex texture{};
  TextureInfo textureInfo;
  eastl::optional<dafg::AutoResolutionData> resolution;
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
      const bool wasUsed = providerIt->second.isUsed();
      providerIt->second.close();
      providerIt->second = eastl::move(provider);
      if (wasUsed)
        providerIt->second.init();
    }
  }
  else
    providerIt = texture_providers.emplace(provider.getName(), eastl::move(provider)).first;
}

template <typename T>
using FramememUnorderedSet = eastl::unordered_set<T, eastl::hash<T>, eastl::equal_to<T>, framemem_allocator>;

template <typename K, typename V>
using FramememUnorderedMap = eastl::unordered_map<K, V, eastl::hash<K>, eastl::equal_to<K>, framemem_allocator>;

static void rebuild_external_texture_providers(dafg::InternalRegistry &registry, const dafg::DependencyData &dep_data)
{
  FRAMEMEM_REGION;
  FramememUnorderedSet<TextureProviderKey> unregisterList;
  dag::Vector<TextureProviderKey, framemem_allocator> registerList;
  FramememUnorderedMap<TextureProviderKey, IntermediateTextureProvider> latestProviders;
  for (auto &[key, provider] : texture_providers)
    unregisterList.insert(key);

  for (const auto [resId, lifetime] : dep_data.resourceLifetimes.enumerate())
  {
    const auto &createdResData = registry.resources[dep_data.renamingRepresentatives[resId]].createdResData;

    if (!createdResData.has_value() || createdResData->type != dafg::ResourceType::Texture || is_texture_array(*createdResData))
      continue;

    if (lifetime.consumedBy == dafg::NodeNameId::Invalid && lifetime.readers.empty())
      continue;

    const auto info = get_debug_copy_tex_info(*createdResData, false, registry);
    if (!info)
      continue;

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
        latestProviders.insert_or_assign(TextureProviderKey(name.c_str()),
          IntermediateTextureProvider(Selection{PreciseTimePoint{from, to}, resId}, registry, *info, createdResData->resolution,
            name));
      };

      addNewProvider(fromName, "out");
      addNewProvider(toName, "in");
    }
  }

  for (auto &[key, provider] : latestProviders)
  {
    add_texture_provider(eastl::move(provider));
    registerList.push_back(key);
    unregisterList.erase(key);
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

void update_external_texture_visualization() { externalTexturesOutdated = true; }

void apply_external_texture_visualization()
{
  if (!eastl::exchange(externalTexturesOutdated, false))
    return;

  auto &runtime = dafg::Runtime::get();
  rebuild_external_texture_providers(runtime.getInternalRegistry(), runtime.getDependencyData());
}

void clear_external_texture_providers()
{
  for (auto &[_, provider] : texture_providers)
  {
    texdebug::unregister_external_texture(provider);
    provider.close();
  }
  texture_providers.clear();
  externalTexturesOutdated = false;
}

void update_debug_copy_textures()
{
  const auto &registry = dafg::Runtime::get().getInternalRegistry();

  const bool manualHistory = savedSelection && savedSelection->history;
  if (fit_debug_copy_tex(copiedTexture, copiedTextureInfo, copiedTextureResolution, manualHistory, registry) && is_overlay_shown())
    update_overlay_texture();

  for (auto &[_, provider] : texture_providers)
    provider.fitToAppliedResolution();

  for (const auto [autoResId, autoResType] : registry.autoResTypes.enumerate())
    if (const auto *values = eastl::get_if<dafg::ResolutionValues<IPoint2>>(&autoResType.values))
      prevAppliedResolutions.set(autoResId, values->lastAppliedDynamicResolution);
}
