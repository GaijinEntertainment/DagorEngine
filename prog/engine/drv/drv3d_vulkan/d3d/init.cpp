// Copyright (C) Gaijin Games KFT.  All rights reserved.

// driver init and shutdown implementation
#include <drv/3d/dag_driver.h>
#include "driver.h"
#include "vulkan_instance.h"
#include "vulkan_device.h"
#include "physical_device_set.h"
#include "device_queue.h"
#include "globals.h"
#include <EASTL/sort.h>
#include "driver_config.h"
#include "os.h"
#include "pipeline_cache_file.h"
#include "bindless.h"
#include "render_state_system.h"
#include "pipeline_cache.h"
#include "shader/program_database.h"
#include "renderDoc_capture_module.h"
#include "debug_callbacks.h"
#include "resource_manager.h"
#include "backend.h"
#include "device_context.h"
#include "device_memory_report.h"
#include "buffer_alignment.h"
#include "execution_markers.h"
#include "raytrace_scratch_buffer.h"
#include "dummy_resources.h"
#include "sampler_cache.h"
#include <3d/tql.h>
#include "external_resource_pools.h"
#include <gpuConfig.h>
#include <workCycle/dag_workCycle.h>
#include "device_featureset.h"
#include "frontend.h"
#include "execution_timings.h"
#include "global_lock.h"
#include <destroyEvent.h>
#include <osApiWrappers/dag_direct.h>
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

namespace
{

bool isInitialized = false;
bool initVideoDone = false;

Driver3dInitCallback *initCallback;
VulkanSurfaceKHRHandle surface;

// init method has lots of shared data, but multiple logic parts, keep data in shared struct with different methods
struct InitCtx
{
  Tab<const char *> enabledInstanceLayers;
  eastl::vector<VkLayerProperties> layerList;

  Tab<PhysicalDeviceSet> physicalDeviceSets;
  uint32_t usePhysicalDeviceIndex = -1;


  const DataBlock *videoCfg = nullptr;
  const DataBlock *vkCfg = nullptr;
  const DataBlock *extensionCfg = nullptr;
  const DataBlock *layersCfg = nullptr;

  const char *error = nullptr;
  void fail(const char *msg) { error = msg; }

  void fillPhysicalDeviceSets()
  {
    Tab<VulkanPhysicalDeviceHandle> physicalDevices = Globals::VK::inst.getAllDevices();
    physicalDeviceSets.clear();
    for (int i = 0; i < physicalDevices.size(); ++i)
    {
      PhysicalDeviceSet pd{};
      pd.init(Globals::VK::inst, physicalDevices[i]);
      physicalDeviceSets.push_back(pd);
    }

    eastl::sort(physicalDeviceSets.begin(), physicalDeviceSets.end(),
      [preferIGpu = videoCfg->getBool("preferiGPU", false)](const PhysicalDeviceSet &l, const PhysicalDeviceSet &r) {
        if (preferIGpu && l.properties.deviceType != r.properties.deviceType)
          return l.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        return r.score < l.score;
      });
  }

  void dumpDevicesInfo()
  {
    String devicesDump(32 + physicalDeviceSets.size() * 64, "vulkan: %u devices available\n", physicalDeviceSets.size());

    for (auto &&device : physicalDeviceSets)
      devicesDump.aprintf(64, "vulkan: %u : %s, %s [score %u]\n", &device - &physicalDeviceSets[0], device.properties.deviceName,
        PhysicalDeviceSet::nameOf(device.properties.deviceType), device.score);
    devicesDump.pop_back();
    debug(devicesDump);

    for (auto &&device : physicalDeviceSets)
      device.print(&device - &physicalDeviceSets[0]);
  }

  void selectDeviceIndex()
  {
    usePhysicalDeviceIndex = vkCfg->getInt("usePhysicalDeviceIndex", -1);
    if (usePhysicalDeviceIndex != -1)
    {
      if (usePhysicalDeviceIndex >= physicalDeviceSets.size())
      {
        debug("vulkan: configuration asked for non existent device index %u, fallback to auto", usePhysicalDeviceIndex);
        usePhysicalDeviceIndex = -1;
      }
      else
        debug("vulkan: using config supplied device %u", usePhysicalDeviceIndex);
    }
    else
      debug("vulkan: selecting device by internal score order");
  }

  void dumpInstanceExts(const Tab<const char *> &ext_set)
  {
    String usedExtsDump(256, "vulkan: used instance extensions [");
    for (auto &&name : ext_set)
      usedExtsDump += String(32, "%s, ", name);
    usedExtsDump.chop(2);
    usedExtsDump += "]";
    debug(usedExtsDump);
  }

  void fillEnabledInstanceLayers()
  {
    layerList =
      build_instance_layer_list(Globals::VK::loader, layersCfg, Globals::cfg.debugLevel, Globals::cfg.bits.enableRenderDocLayer);

    enabledInstanceLayers.clear();
    enabledInstanceLayers.reserve(layerList.size());
    for (auto &&layer : layerList)
      enabledInstanceLayers.push_back(layer.layerName);
  }

  void initInstance()
  {
    Tab<const char *> instanceExtensionSet =
      build_instance_extension_list(Globals::VK::loader, extensionCfg, Globals::cfg.debugLevel, initCallback);
    dumpInstanceExts(instanceExtensionSet);

    if (!has_all_required_instance_extension(instanceExtensionSet, [](const char *name) //
          { D3D_ERROR("vulkan: Missing required instance extension %s", name); }))
      return fail("vulkan: One ore more required instance extensions are missing");

    fillEnabledInstanceLayers();

    VkApplicationInfo appInfo = create_app_info(vkCfg);
    debug("vulkan: Reporting to Vulkan: pAppName=%s appVersion=0x%08X pEngineName=%s engineVersion=0x%08X "
          "apiVersion=0x%08X...",
      appInfo.pApplicationName, appInfo.applicationVersion, appInfo.pEngineName, appInfo.engineVersion, appInfo.apiVersion);

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr};
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionSet.data();
    instanceCreateInfo.enabledExtensionCount = instanceExtensionSet.size();

    auto tryInitInstance = [&]() {
      instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
      instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
      return Globals::VK::inst.init(Globals::VK::loader, instanceCreateInfo);
    };

    if (!tryInitInstance())
      return fail("vulkan: Failed to create Vulkan instance");

    if (Globals::VK::inst.getAllDevices().empty())
    {
      Globals::VK::inst.shutdown();
      debug("vulkan: No devices found, trying with VK_LAYER_NV_optimus");
      enabledInstanceLayers.push_back("VK_LAYER_NV_optimus");

      if (!tryInitInstance())
        return fail("vulkan: Failed to create Vulkan instance with optimus retry");

      if (Globals::VK::inst.getAllDevices().empty())
        return fail("vulkan: No Vulkan compatible devices found");
    }

    // check if all extensions that are required where loaded correctly
    // usually this should never happen as the loaded exposes those
    if (!instance_has_all_required_extensions(Globals::VK::inst, [](const char *name) //
          { D3D_ERROR("vulkan: Missing required instance extension %s", name); }))
      return fail("vulkan: Some required instance extensions where not loaded (broken loader)");

    if (Globals::VK::inst.hasExtension<DisplayKHR>())
    {
      debug("vulkan: disabled VK_KHR_display due to crashes on some systems");
      Globals::VK::inst.disableExtension<DisplayKHR>();
    }
  }

  void loadVkLib()
  {
    const char *lib_names[] = {
      vkCfg->getStr("driverName", VULKAN_LIB_NAME),
#if _TARGET_ANDROID
      "libvulkan.so",
      "libvulkan.so.1",
#endif
    };

    bool lib_loaded = false;
    for (const char *lib_nm : lib_names)
    {
      lib_loaded = Globals::VK::loader.load(lib_nm, vkCfg->getBool("validateLoaderLibrary", true));
      if (lib_loaded)
        break;
      Globals::VK::loader.unload();
    }
    if (!lib_loaded)
      return fail("vulkan: failed to load vulkan dynamic library");
  }

  void initConfigs()
  {
    videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
    vkCfg = ::dgs_get_settings()->getBlockByNameEx("vulkan");
    extensionCfg = vkCfg->getBlockByName("extensions");
    layersCfg = vkCfg->getBlockByName("layers");
    Globals::cfg.fillConfigBits(vkCfg);
  }

  void createOutputWindow(void *hinst, main_wnd_f *wnd_proc, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd,
    void *hicon, const char *title)
  {
    Globals::window.set(hinst, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, *(void **)&wnd_proc);
    Globals::window.getRenderWindowSettings(initCallback);

    if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
      os_set_display_mode(Globals::window.settings.resolutionX, Globals::window.settings.resolutionY);
    else
      Globals::window.updateRefreshRateFromCurrentDisplayMode();

    if (!Globals::window.setRenderWindowParams())
      return fail("vulkan: Failed to create output window");
    mainwnd = Globals::window.getMainWindow();
  }

  void initSurface()
  {
    surface = init_window_surface(Globals::VK::inst);
    if (is_null(surface))
      return fail("vulkan: Unable to create surface definition of the output window");
  }

  void verifyDeviceCreated()
  {
    if (Globals::VK::dev.isValid())
    {
      debug("vulkan: device created");
      if (videoCfg->getBool("preferiGPU", false) && Globals::VK::phy.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        logwarn("vulkan: Despite the preferiGPU flag being enabled, the dedicated GPU is used!");
      return;
    }
    VULKAN_LOG_CALL(Globals::VK::inst.vkDestroySurfaceKHR(Globals::VK::inst.get(), surface, nullptr));
    return fail("vulkan: Unable to create any device");
  }

  void loadPrimaryPipeCache()
  {
    PipelineCacheInFile pipelineCacheFile;
    Globals::pipeCache.load(pipelineCacheFile);
  }

  struct DeviceCreateCache
  {
    StaticTab<const char *, VulkanDevice::ExtensionCount> enabledDeviceExtensions;

    StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> qci;
    StaticTab<VkDeviceQueueGlobalPriorityCreateInfoKHR, uint32_t(DeviceQueueType::COUNT)> qgpri;
    VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr};

    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDeviceFeatures2KHR featuresExt = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR, nullptr, {}};
    DeviceQueueGroup::Info queueGroupInfo = {};
    carray<float, uint32_t(DeviceQueueType::COUNT)> prio;
    Tab<const char *> usedDeviceExtension;
    const DataBlock *extensionCfg = nullptr;

    PhysicalDeviceSet *set = nullptr;

    const char *skip = nullptr;
    void reject(const char *msg) { skip = msg; }

    void init(const DataBlock *extension_cfg, const Tab<const char *> &enabled_instance_layers)
    {
      extensionCfg = extension_cfg;

      // build static data and pointer links
      for (int i = 0; i < prio.size(); ++i)
        prio[i] = 1.f;

      dci.flags = 0;
      dci.enabledLayerCount = static_cast<uint32_t>(enabled_instance_layers.size());
      dci.ppEnabledLayerNames = enabled_instance_layers.data();
    }

    void setupFeatures()
    {
      features = set->features;
      if (!check_and_update_device_features(features))
        return reject("does not support the required features");

      if (features.robustBufferAccess)
      {
        features.robustBufferAccess = Globals::cfg.bits.robustBufferAccess;
        if (features.robustBufferAccess)
          debug("vulkan: enabled robustBufferAccess");
      }
      // enable features that live in extended descriptions
      set->enableExtendedFeatures(dci, featuresExt);
    }

#if VK_KHR_global_priority
    void setupQueuesGlobalPriority()
    {
      qgpri.clear();
      if (!set->hasGlobalPriority || !Globals::cfg.bits.highPriorityQueues)
        return;
      debug("vulkan: using high priority queues");
      qgpri.resize(qci.size());
      for (VkDeviceQueueGlobalPriorityCreateInfoKHR &qprio : qgpri)
      {
        size_t idx = &qprio - qgpri.data();
        qprio.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR;
        qprio.pNext = nullptr;
        qprio.globalPriority = VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR;
        for (uint32_t i = 0; i < set->queuesGlobalPriorityProperties[idx].priorityCount; ++i)
          if (set->queuesGlobalPriorityProperties[idx].priorities[i] == VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR)
            qprio.globalPriority = VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR;
        chain_structs(qci[idx], qprio);
      }
    }
#else
    void setupQueuesGlobalPriority() {}
#endif

    void setupQueues()
    {
      if (!queueGroupInfo.init(Globals::VK::inst, set->device, surface, set->queueFamilyProperties))
        return reject("does not support the required queue types");
      qci = build_queue_create_info(queueGroupInfo, prio.data());
      dci.pQueueCreateInfos = qci.data();
      dci.queueCreateInfoCount = qci.size();
      setupQueuesGlobalPriority();
    }

    void setupExtensions()
    {
      adjustDeviceExtensionsByConfig();
      dci.enabledExtensionCount = fill_device_extension_list(usedDeviceExtension, enabledDeviceExtensions, set->extensions);
      usedDeviceExtension.resize(dci.enabledExtensionCount);
      usedDeviceExtension.shrink_to_fit();

      auto injectExtensions = [&](const char *injection) {
        char *desiredRendererExtensions = const_cast<char *>(injection);
        while (desiredRendererExtensions && *desiredRendererExtensions)
        {
          char *currentExtension = desiredRendererExtensions;

          char *separator = strchr(desiredRendererExtensions, ' ');
          if (separator && *separator)
          {
            *separator = 0;
            desiredRendererExtensions = separator + 1;
          }
          else
            desiredRendererExtensions = separator;

          bool dupe = eastl::find_if(usedDeviceExtension.cbegin(), usedDeviceExtension.cend(), [currentExtension](const char *ext) {
            return strcmp(ext, currentExtension) == 0;
          }) != usedDeviceExtension.cend();

          if (!dupe)
            usedDeviceExtension.push_back(currentExtension);
        }

        dci.enabledExtensionCount = usedDeviceExtension.size();
      };
      if (initCallback)
        injectExtensions(initCallback->desiredRendererDeviceExtensions());

      eastl::string lateOsSpecificExts = os_get_additional_ext_requirements(set->device, set->extensions);
      if (!lateOsSpecificExts.empty())
        injectExtensions(lateOsSpecificExts.data());

      dci.ppEnabledExtensionNames = usedDeviceExtension.data();

      if (!has_all_required_extension(make_span_const(usedDeviceExtension).first(dci.enabledExtensionCount)))
        return reject("not all required extensions are supported");
    }

    void setupDepthStencilFormatRemap()
    {
      auto dsFormat = get_default_depth_stencil_format(Globals::VK::inst, set->device);
      if (VK_FORMAT_UNDEFINED == dsFormat)
        return reject("no suitable depth stencil format supported for depth "
                      "stencil attachments");
      FormatStore::setRemapDepth24BitsToDepth32Bits(dsFormat == VK_FORMAT_D32_SFLOAT_S8_UINT);
    }

    void disableExtensionsByDataBlock(const DataBlock *ext_disable_block, const char *block_type)
    {
      if (ext_disable_block)
      {
        auto newEnd = eastl::end(enabledDeviceExtensions);
        for (int i = 0; i < ext_disable_block->paramCount(); ++i)
        {
          if (!ext_disable_block->getBool(i))
          {
            const char *extName = ext_disable_block->getParamName(i);
            debug("vulkan: extension %s disabled by %s", extName, block_type);
            newEnd = eastl::remove_if(eastl::begin(enabledDeviceExtensions), newEnd,
              [pName = extName](const char *name) { return 0 == strcmp(pName, name); });
            set->disableExtensionByName(extName);
          }
        }
        auto cnt = eastl::end(enabledDeviceExtensions) - newEnd;
        erase_items(enabledDeviceExtensions, enabledDeviceExtensions.size() - cnt, cnt);
      }
    }

    void adjustDeviceExtensionsByConfig()
    {
      enabledDeviceExtensions.clear();
      enabledDeviceExtensions = make_span_const(VulkanDevice::ExtensionNames, VulkanDevice::ExtensionCount);
      disableExtensionsByDataBlock(extensionCfg, "global config");
      disableExtensionsByDataBlock(Globals::cfg.getPerDriverPropertyBlock("extensions"), "per driver config");
      // re-init extensions on set to update features supported
      set->initExt(Globals::VK::inst);
    }

    bool trySetup(PhysicalDeviceSet &device, int index)
    {
      // update structs chain data & enabled features as we reusing them for multiple devices
      dci.pNext = nullptr;
      dci.pEnabledFeatures = &features;
      featuresExt.features = {};
      featuresExt.pNext = nullptr;
      set = &device;
      // allow to work with external code like per device driver config
      Globals::VK::phy = device;
      skip = nullptr;

      debug("vulkan: considering %u: %s", index, set->properties.deviceName);

#define REJECT_CHECK(x)                                                                      \
  {                                                                                          \
    x;                                                                                       \
    if (skip)                                                                                \
    {                                                                                        \
      logwarn("vulkan: rejecting device [%s] due to: %s", set->properties.deviceName, skip); \
      return false;                                                                          \
    }                                                                                        \
  }
      REJECT_CHECK(setupDepthStencilFormatRemap());
      REJECT_CHECK(setupExtensions());
      REJECT_CHECK(setupFeatures());
      REJECT_CHECK(setupQueues());
#undef REJECT_CHECK
      return true;
    }

    void dumpDeviceExtensions()
    {
      String enabledExtDump("vulkan: enabled extensions\nvulkan: ");
      size_t extDumpRawLength = 0;
      Globals::VK::dev.iterateEnabledExtensionNames([&enabledExtDump, &extDumpRawLength](const char *name) {
        size_t extNameLen = strlen(name);
        bool newline = (extDumpRawLength / LOG_DUMP_STR_SIZE) < ((extNameLen + extDumpRawLength) / LOG_DUMP_STR_SIZE);
        extDumpRawLength += extNameLen;
        if (newline)
          enabledExtDump += "\nvulkan: ";
        enabledExtDump += name;
        enabledExtDump += ", ";
      });
      enabledExtDump.chop(2);
      debug(enabledExtDump);
    }

    bool tryInitDevice()
    {
      initDevice();
      if (!skip)
        return true;
      logwarn("vulkan: rejecting device [%s] due to: %s", set->properties.deviceName, skip);
      if (Globals::VK::dev.isValid())
        Globals::VK::dev.shutdown();
      return false;
    }

    void initMemory()
    {
      Globals::Mem::pool.init(Globals::VK::phy.memoryProperties);
#if VK_EXT_memory_budget
      if (Globals::VK::phy.memoryBudgetInfoAvailable)
        Globals::Mem::pool.applyBudget(Globals::VK::phy.memoryBudgetInfo);
#endif
      Globals::Mem::res.init(Globals::VK::phy);
    }

    void initCfg()
    {
      Globals::cfg.fillDeviceBits();
      Globals::cfg.configurePerDeviceDriverFeatures();
      LogicAddress::setAttachmentNoStoreSupport(Globals::cfg.has.attachmentNoStoreOp);
      PipelineBarrier::dbgUseSplitSubmits =
        dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx("debug")->getBool("pipelineBarrierUseSplitSubmits", false);
    }

    bool dropLastStructInFeatureChain()
    {
      VkBaseInStructure *chain = reinterpret_cast<VkBaseInStructure *>(&featuresExt);
      while (chain->pNext)
      {
        if (!chain->pNext->pNext)
        {
          logwarn("vulkan: dropped feature struct %u", chain->pNext->sType);
          chain->pNext = nullptr;
          return true;
        }
        chain = (VkBaseInStructure *)chain->pNext;
      }
      return false;
    }

    bool dropFirstFeatureInFeatureStruct()
    {
#define VULKAN_FEATURE_OPTIONAL(name)                                \
  {                                                                  \
    if (!dci.pEnabledFeatures)                                       \
    {                                                                \
      if (featuresExt.features.name)                                 \
      {                                                              \
        featuresExt.features.name = VK_FALSE;                        \
        logwarn("vulkan: drop feature %s from ext features", #name); \
        return true;                                                 \
      }                                                              \
    }                                                                \
    else                                                             \
    {                                                                \
      if (features.name)                                             \
      {                                                              \
        features.name = VK_FALSE;                                    \
        logwarn("vulkan: drop feature %s from features", #name);     \
        return true;                                                 \
      }                                                              \
    }                                                                \
  }                                                                  \
  VULKAN_FEATURESET_OPTIONAL_LIST
#undef VULKAN_FEATURE_OPTIONAL
      return false;
    }

    bool initDeviceWithReducedFeatures()
    {
      if (!Globals::VK::dev.init(&Globals::VK::inst, Globals::VK::phy.device, dci))
        return false;
      logerr("vulkan: created device without some features, while expected to be working AS-IS!!! workaround this driver bug via "
             "config!!!");
      return true;
    }

    bool featureDowngradeStep()
    {
      if (dropLastStructInFeatureChain())
        return true;
      if (dropFirstFeatureInFeatureStruct())
        return true;

      return false;
    }

    // some devices provide invalid support of features, try to disable features till device are able to be created
    bool initDeviceWithFeatureDowngrade()
    {
      for (;;)
      {
        if (!featureDowngradeStep())
          return false;
        if (initDeviceWithReducedFeatures())
          return true;
      }
      // unreachable
      return false;
    }

#if VK_KHR_global_priority
    bool initDeviceWithoutQueuePriority()
    {
      if (!set->hasGlobalPriority || !Globals::cfg.bits.highPriorityQueues)
        return false;
      logwarn("vulkan: retrying init without queue priority");
      for (VkDeviceQueueCreateInfo &i : qci)
        i.pNext = nullptr;
      return Globals::VK::dev.init(&Globals::VK::inst, Globals::VK::phy.device, dci);
    }
#else
    bool initDeviceWithoutQueuePriority() { return false; }
#endif

    void initDevice()
    {
      // may be modified with extension disable logic, so need to update it
      Globals::VK::phy = *set;
      if (!Globals::VK::dev.init(&Globals::VK::inst, Globals::VK::phy.device, dci)) //-V1051
      {
        if (!initDeviceWithoutQueuePriority())
        {
          if (d3d::get_last_error_code() == (uint32_t)VK_ERROR_FEATURE_NOT_PRESENT)
          {
            logwarn("vulkan: retrying init with iterative feature downgrade");
            if (!initDeviceWithFeatureDowngrade())
              return reject("create or proc addr getting failed at non present feature but feature downgrade did not helped");
          }
          else
            return reject("create or proc addr getting failed");
        }
      }
      dumpDeviceExtensions();
      if (!device_has_all_required_extensions(Globals::VK::dev,
            [](const char *name) { logwarn("vulkan: Missing device extension %s", name); }))
        return reject("some required extensions where not loaded (broken driver)");
      Globals::VK::queue = DeviceQueueGroup(Globals::VK::dev, queueGroupInfo);
      initCfg();
      initMemory();

      SwapchainMode swapchainMode;
      swapchainMode.recreateRequest = 0;
      swapchainMode.surface = surface;
      swapchainMode.enableSrgb = false;
      swapchainMode.extent.width = Globals::window.settings.resolutionX;
      swapchainMode.extent.height = Globals::window.settings.resolutionY;
      swapchainMode.setPresentModeFromConfig();
      swapchainMode.fullscreen =
        dgs_get_window_mode() == WindowMode::WINDOWED_NO_BORDER || dgs_get_window_mode() == WindowMode::WINDOWED_FULLSCREEN;
      if (!Globals::swapchain.init(swapchainMode))
        return reject("unable to create swapchain");
    }
  };
  DeviceCreateCache dcc = {};

  void tryInitDeviceLoop()
  {
    dcc.init(extensionCfg, enabledInstanceLayers);
    if (usePhysicalDeviceIndex != -1)
      tryInitDevice(usePhysicalDeviceIndex);
    else
      for (const PhysicalDeviceSet &device : physicalDeviceSets)
      {
        if (tryInitDevice(&device - &physicalDeviceSets[0]))
          return;
      }
  }

  bool tryInitDevice(int index)
  {
    if (!dcc.trySetup(physicalDeviceSets[index], index))
      return false;

    DeviceMemoryReport::CallbackDesc deviceMemoryReportCb;
    if (Globals::Mem::report.init(*dcc.set, deviceMemoryReportCb))
      chain_structs(dcc.dci, deviceMemoryReportCb);

    return dcc.tryInitDevice();
  }

  void initTimelines()
  {
    Globals::timelines.init();
    Frontend::replay.start();
    Backend::gpuJob.start();
  }

  void initDeviceIndependentSystems()
  {
    initTimelines();
    shaders::RenderState emptyRS{};
    Backend::renderStateSystem.setRenderStateData((shaders::DriverRenderStateId)0, emptyRS);
    Frontend::State::pipe.reset();
    Backend::pipelineCompiler.init();
    Frontend::timings.init(ref_time_ticks());
    Globals::ctx.initMode(Globals::cfg.bits.useThreadedExecution ? ExecutionMode::THREADED : ExecutionMode::DEFERRED);
  }

  void initBufferRelatedSystems()
  {
    Globals::VK::bufAlign.init();
    Globals::ctx.initTempBuffersConfiguration();
    Backend::immediateConstBuffers.init();
    if (Globals::cfg.bits.commandMarkers)
      Globals::gpuExecMarkers.init();
  }
};

InitCtx ictx;

} // namespace

void shutdown_device()
{
  if (!Globals::VK::dev.isValid())
    return;

  // shutdown surface if we exited prior window destruction
  SwapchainMode newMode(Globals::swapchain.getMode());
  newMode.surface = VulkanNullHandle();
  Globals::swapchain.setMode(newMode);

  if (Globals::cfg.bits.commandMarkers)
    Globals::gpuExecMarkers.shutdown();
  Globals::dummyResources.shutdown(Globals::ctx);
  Globals::rtScratchBuffer.shutdown();
  Globals::surveys.shutdownDataBuffers();
  Globals::ctx.shutdown();

  VkResult res = Globals::VK::dev.vkDeviceWaitIdle(Globals::VK::dev.get());
  if (VULKAN_FAIL(res))
    D3D_ERROR("vulkan: Device::shutdown vkDeviceWaitIdle failed with %08lX", res);

  Globals::samplers.shutdown();
  Globals::timelines.shutdown();
  Globals::pipelines.unloadAll(Globals::VK::dev);
  Globals::pipeCache.shutdown();
  Globals::passes.unloadAll(Globals::VK::dev);
  Globals::fences.resetAll(Globals::VK::dev);
  Globals::Mem::res.shutdown();
  Globals::rtSizeQueryPool.shutdownPools();
  Globals::surveys.shutdownPools();
  Globals::timestamps.shutdown();
  BuiltinPipelineBarrierCache::clear();
  Backend::cb.shutdown();
  Globals::VK::dev.shutdown();
  Globals::VK::bufAlign.shutdown();
}

bool d3d::is_inited() { return isInitialized && initVideoDone; }

bool d3d::init_driver()
{
  if (d3d::is_inited())
  {
    D3D_ERROR("Driver is already created");
    return false;
  }
  isInitialized = true;
  return true;
}

void d3d::release_driver()
{
  TEXQL_SHUTDOWN_TEX();
  tql::termTexStubs();
  isInitialized = false;

  if (!initVideoDone)
    return;

  debug("vulkan: releaseAll");

  // reset pipeline state so nothing is still referenced on shutdown
  PipelineState &pipeState = Frontend::State::pipe;
  pipeState.reset();
  pipeState.makeDirty();

  Globals::Res::buf.freeWithLeakCheck(
    [](GenericBufferInterface *i) { debug("vulkan: generic buffer %p %s leaked", i, i->getBufName()); });
  Globals::Res::tex.freeWithLeakCheck([](TextureInterfaceBase *i) { debug("vulkan: texture %p %s leaked", i, i->getTexName()); });

  // otherwise we can crash on validation tracking optimized code in backend
  // if pending work contains some pipe rebinds
  Globals::ctx.processAllPendingWork();
  Globals::shaderProgramDatabase.clear(Globals::ctx);
  shutdown_device();

  Globals::Dbg::callbacks.shutdown();
  Globals::VK::inst.shutdown();
  Globals::VK::loader.unload();
  Globals::window.closeWindow();
  Globals::shaderProgramDatabase.shutdown();
  Globals::Dbg::rdoc.shutdown();
  initVideoDone = false;
}

void dumpStateSizes()
{
#if DAGOR_DBGLEVEL > 0
  String stateSizeLogLine("vulkan: ");
  stateSizeLogLine += String(64, "pipeline state size %u", sizeof(PipelineState));
  stateSizeLogLine += " | ";
  stateSizeLogLine += String(64, "execution state size %u", sizeof(ExecutionState));
  debug(stateSizeLogLine);
#endif
}

void waitForAppWindow()
{
#if _TARGET_ANDROID
  // On Android window is created by OS and set on APP_CMD_INIT_WINDOW.
  // Window can be temporarily destroyed due to losing app focus.
  // Therefore we are waiting either until we get a window to work with
  // or app terminate.
  const bool initialWindowIsNull = !::win32_get_main_wnd();
  if (initialWindowIsNull)
  {
    debug("vulkan: Android window is null, waiting for new window");
    while (!::win32_get_main_wnd())
    {
      dagor_idle_cycle(false);
      sleep_msec(250);
    }
    debug("vulkan: Android window obtained, continuing initialization");
  }
#endif
}

void correctStreamingDriverReserve()
{
  // vulkan have limited memory swapping support now
  // going over quota is most of time an out of memory crash
  // put overdraft in driver reserve to decrease total quota
  // while keeping overdraft correctly working
  DataBlock *writableCfg = const_cast<DataBlock *>(::dgs_get_settings());
  if (writableCfg && !writableCfg->isEmpty())
  {
    const DataBlock *texStreamingCfg = writableCfg->getBlockByNameEx("texStreaming");
    int overdraftMB = texStreamingCfg->getInt("maxQuotaOverdraftMB", 256);
    int driverReserveKB = texStreamingCfg->getInt("driverReserveKB", overdraftMB << 10);

    writableCfg->addBlock("texStreaming")->setInt("driverReserveKB", driverReserveKB);

    debug("vulkan: Override texStreaming/driverReserveKB:i = %@", driverReserveKB);
  }
  else
    debug("vulkan: Cannot override texStreaming/driverReserveKB. Settings is not inited yet.");
}

void update_vulkan_gpu_driver_config(GpuDriverConfig &gpu_driver_config)
{
  auto &deviceProps = Globals::VK::phy;

  gpu_driver_config.primaryVendor = deviceProps.vendorId;
  gpu_driver_config.deviceId = deviceProps.properties.deviceID;
  gpu_driver_config.integrated = deviceProps.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
  for (int i = 0; i < 4; ++i)
    gpu_driver_config.driverVersion[i] = deviceProps.driverVersionDecoded[i];
}

#define ERR_CHECK(x)                                  \
  {                                                   \
    x;                                                \
    if (ictx.error)                                   \
    {                                                 \
      D3D_ERROR(ictx.error);                          \
      set_last_error(VK_ERROR_INITIALIZATION_FAILED); \
      return false;                                   \
    }                                                 \
  }

bool d3d::init_video(void *hinst, main_wnd_f *wnd_proc, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb)
{
  G_ASSERTF(!initVideoDone, "vulkan: trying to call init_video when already inited");
  // on some systems we attach to existing window and must wait for it to be available
  waitForAppWindow();
  initCallback = cb;
  ictx.initConfigs();
  dumpStateSizes();

  ERR_CHECK(ictx.loadVkLib())
  ERR_CHECK(ictx.initInstance())
  Globals::Dbg::callbacks.init();

  // create draw window now, so we can determine which device
  // is able to render to the window surface
  ERR_CHECK(ictx.createOutputWindow(hinst, wnd_proc, wcname, ncmdshow, mainwnd, renderwnd, hicon, title));
  ERR_CHECK(ictx.initSurface());
  ictx.fillPhysicalDeviceSets();
  ictx.dumpDevicesInfo();
  ictx.selectDeviceIndex();
  ictx.tryInitDeviceLoop();
  ERR_CHECK(ictx.verifyDeviceCreated());
  ictx.initDeviceIndependentSystems();
  ictx.initBufferRelatedSystems();
  Globals::dummyResources.init();
  // load the driver pipeline cache from disk
  ictx.loadPrimaryPipeCache();

  Globals::cfg.fillExternalCaps(Globals::desc);
  // must be after caps fill
  Globals::bindless.init();
  Globals::shaderProgramDatabase.init(Globals::desc.caps.hasBindless, Globals::ctx);
  if (Globals::cfg.bits.enableRenderDocLayer)
    Globals::Dbg::rdoc.init();
  initVideoDone = true;

  // external engine stuff

  correctStreamingDriverReserve();
  update_gpu_driver_config = update_vulkan_gpu_driver_config;
  // tex streaming need to know that driver is inited
  tql::initTexStubs();
  // state inited to NULL target by default, reset to BB
  d3d::set_render_target();

  if (Globals::VK::phy.hasDeviceFaultFeature)
  {
    // remove old crash if exists
    if (dd_file_exist(Globals::cfg.getFaultVendorDumpFile()))
      dd_erase(Globals::cfg.getFaultVendorDumpFile());
  }

  debug("vulkan: init_video done");
  return true;
}

extern bool dagor_d3d_force_driver_mode_reset;
bool d3d::device_lost(bool *can_reset_now)
{
  if (can_reset_now)
    *can_reset_now = dagor_d3d_force_driver_mode_reset;
  return dagor_d3d_force_driver_mode_reset;
}
static bool device_is_being_reset = false;
bool d3d::is_in_device_reset_now() { return /*device_is_lost != S_OK || */ device_is_being_reset; }

bool d3d::reset_device()
{
  struct RaiiReset
  {
    RaiiReset()
    {
      Globals::lock.acquire();
      device_is_being_reset = true;
    }
    ~RaiiReset()
    {
      device_is_being_reset = false;
      Globals::lock.release();
    }
  } raii_reset;

  debug("vulkan: reset device");

  os_restore_display_mode();
  void *oldWindow = Globals::window.getMainWindow();
  Globals::window.getRenderWindowSettings();

  if (!Globals::window.setRenderWindowParams())
    return false;

  SwapchainMode newMode(Globals::swapchain.getMode());

  VkExtent2D cext;
  cext.width = Globals::window.settings.resolutionX;
  cext.height = Globals::window.settings.resolutionY;
  newMode.extent = cext;
  newMode.setPresentModeFromConfig();

  // linux returns pointer to static object field
  // treat this as always changed instead as always same
#if !_TARGET_PC_LINUX
  // reset changed window, we need to update surface
  if (oldWindow != Globals::window.getMainWindow())
#endif
  {
    newMode.surface = init_window_surface(Globals::VK::inst);
  }

  // empty mode change will be filtered automatically
  Globals::swapchain.setMode(newMode);

  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
    os_set_display_mode(Globals::window.settings.resolutionX, Globals::window.settings.resolutionY);

  dagor_d3d_force_driver_mode_reset = false;
  return true;
}

void d3d::prepare_for_destroy() { on_before_window_destroyed(); }

void d3d::window_destroyed(void *handle)
{
  // may be called after shutdown of device, so make sure we don't crash
  if (Globals::VK::dev.isValid())
  {
    Globals::lock.acquire();

    if (handle == Globals::window.getMainWindow())
    {
      Globals::window.closeWindow();
      SwapchainMode newMode(Globals::swapchain.getMode());
      newMode.surface = VulkanNullHandle();
      Globals::swapchain.setMode(newMode);
    }
    Globals::lock.release();
  }
}
