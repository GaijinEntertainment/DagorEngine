// Copyright (C) Gaijin Games KFT.  All rights reserved.

// driver commands implementation
#include <drv/3d/dag_commands.h>
#include <util/dag_stdint.h>
#include "driver.h"
#include "globals.h"
#include "frontend.h"
#include "physical_device_set.h"
#include "global_lock.h"
#include "renderDoc_capture_module.h"
#include "execution_timings.h"
#include "shader/program_database.h"
#include "driver_config.h"
#include <perfMon/dag_statDrv.h>
#include "util/scoped_timer.h"
#include "pipeline/manager.h"
#include "timestamp_queries.h"
#include "pipeline_cache.h"
#include "pipeline_cache_file.h"
#include <osApiWrappers/dag_miscApi.h>
#include "device_queue.h"
#include <3d/dag_amdFsr.h>
#include "texture.h"
#include "swapchain.h"
#include "frontend_pod_state.h"
#include "resource_upload_limit.h"
#include "device_context.h"
#include "timeline_latency.h"
#include "command.h"

#if DAGOR_DBGLEVEL > 0
#include "3d/dag_resourceDump.h"
#include "external_resource_pools.h"
#endif

namespace
{
uint64_t lastGpuTimeStamp = 0;
Tab<drv3d_vulkan::PipelineManager::AsyncMask> asyncCompileStack;
} // namespace

using namespace drv3d_vulkan;

int d3d::driver_command(Drv3dCommand command, void *par1, void *par2, [[maybe_unused]] void *par3)
{
  switch (command)
  {
    case Drv3dCommand::SET_APP_INFO:
    {
      const char *appName = static_cast<const char *>(par1);
      const uint32_t *appVer = static_cast<const uint32_t *>(par2);
      drv3d_vulkan::set_app_info(appName, *appVer);
    }
      return 1;
    case Drv3dCommand::DEBUG_MESSAGE:
      Globals::ctx.writeDebugMessage(static_cast<const char *>(par1), reinterpret_cast<intptr_t>(par2),
        reinterpret_cast<intptr_t>(par3));
      return 1;
    case Drv3dCommand::GET_TIMINGS:
      *reinterpret_cast<Drv3dTimings *>(par1) = Frontend::timings.get(reinterpret_cast<uintptr_t>(par2));
      return FRAME_TIMING_HISTORY_LENGTH;
    case Drv3dCommand::GET_SHADER_CACHE_UUID:
      if (par1)
      {
        auto &properties = Globals::VK::phy.properties;
        G_STATIC_ASSERT(sizeof(uint64_t) * 2 == sizeof(properties.pipelineCacheUUID));
        memcpy(par1, properties.pipelineCacheUUID, sizeof(properties.pipelineCacheUUID));
        return 1;
      }
      break;
    case Drv3dCommand::AFTERMATH_MARKER: Globals::ctx.placeAftermathMarker((const char *)par1); break;
    case Drv3dCommand::SET_VS_DEBUG_INFO:
    case Drv3dCommand::SET_PS_DEBUG_INFO:
      Globals::shaderProgramDatabase.setShaderDebugName(ShaderID(*(int *)par1), (const char *)par2);
      break;
    case Drv3dCommand::D3D_FLUSH:
      // driver can merge multiple queue submits
      // and GPU will timeout itself if workload is too long
      // so use wait instead of simple flush
      {
        TIME_PROFILE(vulkan_d3d_flush);
        int64_t flushTime = 0;
        {
          ScopedTimer timer(flushTime);
          Globals::ctx.wait();
        }
        if (par1)
          debug("vulkan: flush %u:%s taken %u us", (uintptr_t)par2, (const char *)par1, flushTime);
      }
      break;
    case Drv3dCommand::COMPILE_PIPELINE:
    {
      G_ASSERTF(Globals::lock.isAcquired(), "pipeline compilation requires gpu lock");
      const ShaderStage pipelineType = (ShaderStage)(uintptr_t)par1;

      switch (pipelineType)
      {
        case STAGE_PS:
        {
          const VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan((uintptr_t)par2);
          Globals::ctx.compileGraphicsPipeline(topology);
          break;
        }
        case STAGE_CS: Globals::ctx.compileComputePipeline(); break;
        default:
        {
          G_ASSERTF(false, "Unsupported pipeline type(%u) for compilation", (unsigned)pipelineType);
          break;
        }
      }

      break;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN:
    {
      VERIFY_GLOBAL_LOCK_ACQUIRED();

      PipelineManager::AsyncMask mask = PipelineManager::ASYNC_MASK_NONE;
      if (par1 == nullptr)
        mask = par2 == nullptr ? PipelineManager::ASYNC_MASK_ALL : PipelineManager::ASYNC_MASK_RENDER;

      CmdPipelineCompilationTimeBudget cmd{mask};
      Globals::ctx.dispatchCommand(cmd);
      asyncCompileStack.push_back(mask);
      return 1;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END:
    {
      VERIFY_GLOBAL_LOCK_ACQUIRED();

      asyncCompileStack.pop_back();
      PipelineManager::AsyncMask mask = asyncCompileStack.size() ? asyncCompileStack.back() : PipelineManager::ASYNC_MASK_NONE;

      CmdPipelineCompilationTimeBudget cmd{mask};
      Globals::ctx.dispatchCommand(cmd);
      return 1;
    }
    case Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH:
      if (par1)
      {
        uint32_t *frameLatency = static_cast<uint32_t *>(par1);
        *frameLatency = TimelineLatency::replayToGPUCompletion;
      }
      return Globals::ctx.getPiplineCompilationQueueLength();
      break;
    case Drv3dCommand::ACQUIRE_OWNERSHIP: Globals::lock.acquire(); break;
    case Drv3dCommand::RELEASE_OWNERSHIP: Globals::lock.release(); break;

    // TriggerMultiFrameCapture causes crashes on some android devices but 1 frame capture works correctly
    // so free command slot it used for that
    case Drv3dCommand::PIX_GPU_CAPTURE_NEXT_FRAMES: Globals::Dbg::rdoc.triggerCapture(reinterpret_cast<uintptr_t>(par3)); break;

    // Must be used in conjuction with Drv3dCommand::PIX_GPU_END_CAPTURE
    case Drv3dCommand::PIX_GPU_BEGIN_CAPTURE: Globals::Dbg::rdoc.beginCapture(); break;

    // Must be used in conjuction with Drv3dCommand::PIX_GPU_BEGIN_CAPTURE
    case Drv3dCommand::PIX_GPU_END_CAPTURE: Globals::Dbg::rdoc.endCapture(); break;
    case Drv3dCommand::TIMESTAMPFREQ:
      if (Globals::cfg.has.gpuTimestamps)
      {
        *reinterpret_cast<uint64_t *>(par1) = uint64_t(1000000000.0 / Globals::VK::phy.properties.limits.timestampPeriod);
        return 1;
      }
      break;
    case Drv3dCommand::TIMESTAMPISSUE:
    {
      G_ASSERTF(par1, "vulkan: par1 for Drv3dCommand::TIMESTAMPISSUE must be not null");
      TimestampQueryId &qId = *(reinterpret_cast<TimestampQueryId *>(par1));
      qId = Globals::cfg.has.gpuTimestamps ? Globals::ctx.insertTimestamp() : -1;
      return 1;
    }
    break;
    case Drv3dCommand::TIMESTAMPGET:
    {
      uint64_t gpuTimeStamp = Globals::ctx.getTimestampResult(reinterpret_cast<TimestampQueryId>(par1));
      if (!gpuTimeStamp)
        return 0;

      *reinterpret_cast<uint64_t *>(par2) = gpuTimeStamp;
// cache last timestamp result to fake timestamp calibration if it is not available
#if VK_EXT_calibrated_timestamps
      if (!Globals::VK::dev.hasExtension<CalibratedTimestampsEXT>())
#endif
      {
        lastGpuTimeStamp = gpuTimeStamp;
      }
      return 1;
    }
    break;
    case Drv3dCommand::TIMECLOCKCALIBRATION:
    {
      uint64_t pTimestamps[2] = {0, 0};
#if VK_EXT_calibrated_timestamps
      if (Globals::VK::dev.hasExtension<CalibratedTimestampsEXT>())
      {
        VkCalibratedTimestampInfoEXT timestampsInfos[2];
        timestampsInfos[0].sType = timestampsInfos[1].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
        timestampsInfos[0].pNext = timestampsInfos[1].pNext = NULL;
        timestampsInfos[0].timeDomain = VK_TIME_DOMAIN_DEVICE_EXT;

#if _TARGET_PC_WIN
        timestampsInfos[1].timeDomain = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
#else
        timestampsInfos[1].timeDomain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT;
#endif

        uint64_t pMaxDeviation[2];

        VULKAN_LOG_CALL(
          Globals::VK::dev.vkGetCalibratedTimestampsEXT(Globals::VK::dev.get(), 2, timestampsInfos, pTimestamps, pMaxDeviation));

        // report fake CPU timestamp in case of missing result for CPU time domain
        if (pTimestamps[1] == 0)
          pTimestamps[1] = ref_time_ticks();
      }
      else
#endif
      {
        // report fake CPU & GPU timestamp if calibrated timestamps missing fully
        pTimestamps[1] = ref_time_ticks();
        pTimestamps[0] = lastGpuTimeStamp;
      }

      *reinterpret_cast<uint64_t *>(par1) = pTimestamps[1]; // cpu
      *reinterpret_cast<uint64_t *>(par2) = pTimestamps[0];

#if _TARGET_PC_WIN
      if (par3)
        *(int *)par3 = DRV3D_CPU_FREQ_TYPE_QPC;
#else
      // Make notice that CPU Timestamp is same as dagor's `ref_time_ticks` function on switch, android and linux
      // ref_time_ticks is same as profile_ref_ticks on Switch/iOS
      // however, to clarify we use explicit DRV3D_CPU_FREQ_NSEC, rather than DRV3D_CPU_FREQ_REF (but it will be practically same!)
      if (par3)
        *(int *)par3 = DRV3D_CPU_FREQ_NSEC;
#endif

      return 1;
      break;
    }
    case Drv3dCommand::RELEASE_QUERY:
      if (par1)
      {
        TimestampQueryId &qId = *(reinterpret_cast<TimestampQueryId *>(par1));
        qId = -1;
      }
      break;
    case Drv3dCommand::GET_VENDOR: return drv3d_vulkan::Globals::VK::phy.vendorId; break;
    case Drv3dCommand::GET_FRAMERATE_LIMITING_FACTOR:
    {
      return Globals::ctx.getFramerateLimitingFactor();
    }
    case Drv3dCommand::LOAD_PIPELINE_CACHE:
    {
      PipelineCacheInFile cache((const char *)par1);
      Globals::pipeCache.load(cache);
      break;
    }
    case Drv3dCommand::SAVE_PIPELINE_CACHE:
    {
      TIME_PROFILE(vulkan_save_pipeline_cache);
      Globals::lock.acquire();
      Globals::ctx.wait();
      if (Globals::ctx.getPiplineCompilationQueueLength())
      {
        TIME_PROFILE(vulkan_save_pipeline_cache_async_compile_wait);
        // code is made in a way that everything pending should be processed and it should always reach 0
        // but better safe than sorry, with logwarn we will see if game stucks on such case
        logwarn("vulkan: waiting for async compile on pipe cache save");
        spin_wait([]() { return Globals::ctx.getPiplineCompilationQueueLength() != 0; });
      }
      Globals::pipeCache.storeAsync();
      Globals::lock.release();
      break;
    }
    case Drv3dCommand::GET_VIDEO_MEMORY_BUDGET:
    {
      uint32_t totalMemory = Globals::VK::phy.getAvailableVideoMemoryKb();
      if (par1)
        *reinterpret_cast<uint32_t *>(par1) = totalMemory;
      if (par2 || par3)
      {
        uint32_t availableMemory = Globals::Mem::pool.getCurrentAvailableDeviceKb();
        availableMemory = min<uint32_t>(totalMemory, availableMemory);
        if (availableMemory == 0)
          return 0;
        if (par2)
          *reinterpret_cast<uint32_t *>(par2) = availableMemory;
        if (par3)
          *reinterpret_cast<uint32_t *>(par3) = totalMemory - availableMemory; // it is always positive
      }
      return totalMemory;
    }

    case Drv3dCommand::GET_INSTANCE: *(VkInstance *)par1 = drv3d_vulkan::Globals::VK::dev.getInstance().get(); return 1;
    case Drv3dCommand::GET_PHYSICAL_DEVICE: *(VkPhysicalDevice *)par1 = drv3d_vulkan::Globals::VK::phy.device; return 1;
    case Drv3dCommand::GET_QUEUE_FAMILY_INDEX:
      *(uint32_t *)par1 = drv3d_vulkan::Globals::VK::queue[DeviceQueueType::GRAPHICS].getFamily();
      return 1;
    case Drv3dCommand::GET_QUEUE_INDEX:
      *(uint32_t *)par1 = drv3d_vulkan::Globals::VK::queue[DeviceQueueType::GRAPHICS].getIndex();
      return 1;
    case Drv3dCommand::EXECUTE_FSR: Globals::ctx.executeFSR((amd::FSR *)par1, *(const amd::FSR::UpscalingArgs *)par2); return 1;
    case Drv3dCommand::MAKE_TEXTURE:
    {
      const Drv3dMakeTextureParams *makeParams = (const Drv3dMakeTextureParams *)par1;
      *(Texture **)par2 = drv3d_vulkan::BaseTex::wrapVKImage(*(VkImage *)makeParams->tex, makeParams->currentState, makeParams->w,
        makeParams->h, makeParams->layers, makeParams->mips, makeParams->name, makeParams->flg);
      return 1;
    }
    case Drv3dCommand::REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS:
      drv3d_vulkan::Globals::ctx.registerFrameEventsCallback(static_cast<FrameEvents *>(par1), par2);
      return 1;

    case Drv3dCommand::PROCESS_APP_INACTIVE_UPDATE:
    {
      // keep finishing frames when app is minimized/inactive
      // as app can generate resource creation/deletion even if minimized
      // and we must process them (by finishing frames), otherwise we will run out of memory
      // also not finishing frames can cause swapchain to bug out either by not updating on restore,
      // or hanging somewhere inside system (on android)
      Globals::lock.acquire();
      // keep pre rotated state to avoid glitching system preview
      drv3d_vulkan::Globals::ctx.holdPreRotateStateForOneFrame();
      d3d::update_screen();
      Globals::lock.release();
      return 1;
    }
    case Drv3dCommand::PRE_ROTATE_PASS:
    {
      uintptr_t v = (uintptr_t)par1;
      Globals::ctx.startPreRotate((uint32_t)v);
      return Globals::swapchain.getPreRotationAngle();
    }
    case Drv3dCommand::PROCESS_PENDING_RESOURCE_UPDATED:
    {
      // flush draws only if we under global lock, otherwise just use extra memory
      // as it is quite unsafe to flushDraws outside of global lock
      if (Globals::lock.isAcquired())
      {
        TIME_PROFILE(vulkan_wait_for_rub_quota);
        Globals::ctx.flushDraws();
      }
      else
        Frontend::resUploadLimit.reset();
      return 1;
    }
    case Drv3dCommand::GET_WORKER_CPU_CORE:
    {
      drv3d_vulkan::Globals::ctx.getWorkerCpuCore((int *)par1, (int *)par2);
      return 1;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILATION_FEEDBACK_BEGIN:
    {
      G_ASSERTF(Globals::lock.isAcquired(), "vulkan: pipeline compile feedback begin must be under GPU lock");
      uint32_t *feedbackData = (uint32_t *)par1;
      CmdAsyncPipeFeedbackPtr cmd{feedbackData};
      Globals::ctx.dispatchCommand(cmd);

      const uint32_t feedbackDataContent = interlocked_relaxed_load(*feedbackData);
      // 16b+16b processed & current skip counts
      const bool pipeSkippedInRange = (feedbackDataContent >> 16) != (feedbackDataContent & 0xFFFF);
      const bool otherPipeSkippedInRange = Frontend::State::pod.summaryAsyncPipelineCompilationFeedback > 0;
      if (pipeSkippedInRange)
      {
        interlocked_and(*feedbackData, 0x0000FFFF);
        interlocked_or(*feedbackData, (feedbackDataContent << 16) & 0xFFFF0000);
        ++Frontend::State::pod.summaryAsyncPipelineCompilationFeedback;
      }
      return (otherPipeSkippedInRange ? 0x4 : 0) | (pipeSkippedInRange ? 0x2 : 0) | 0x1;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILATION_FEEDBACK_END:
    {
      G_ASSERTF(Globals::lock.isAcquired(), "vulkan: pipeline compile feedback end must be under GPU lock");
      CmdAsyncPipeFeedbackPtr cmd{nullptr};
      Globals::ctx.dispatchCommand(cmd);
      return 1;
    }
    case Drv3dCommand::GET_MONITORS:
    {
      Tab<String> &monitorList = *reinterpret_cast<Tab<String> *>(par1);
      clear_and_shrink(monitorList);

#if VK_KHR_display
      if (Globals::VK::phy.displays.empty())
        return 0;

      monitorList.reserve(Globals::VK::phy.displays.size());
      for (VkDisplayPropertiesKHR &iter : Globals::VK::phy.displays)
      {
        monitorList.push_back(
          String(64, "%s", iter.displayName ? iter.displayName : String(16, "DISPLAY%u", &iter - Globals::VK::phy.displays.begin())));
      }
      return monitorList.size() > 0;
#endif
      return 0;
    }
    case Drv3dCommand::GET_VSYNC_REFRESH_RATE:
    {
      return Globals::window.refreshRate;
    }
    case Drv3dCommand::DELAY_SYNC: Globals::ctx.dispatchCommand<CmdDelaySyncCompletion>({true}); return 0;
    case Drv3dCommand::CONTINUE_SYNC: Globals::ctx.dispatchCommand<CmdDelaySyncCompletion>({false}); return 0;
    case Drv3dCommand::GET_RESOURCE_STATISTICS:
    {
#if DAGOR_DBGLEVEL > 0
      Tab<ResourceDumpInfo> &dumpInfo = *static_cast<Tab<ResourceDumpInfo> *>(par1);

      WinAutoLock lk(Globals::Mem::mutex);

      // Textures
      Globals::Res::tex.iterateAllocated([&](TextureInterfaceBase *tex) {
        resource_dump_types::TextureTypes type = resource_dump_types::TextureTypes::TEX2D;
        switch (tex->resType)
        {
          case RES3D_TEX: type = resource_dump_types::TextureTypes::TEX2D; break;
          case RES3D_CUBETEX: type = resource_dump_types::TextureTypes::CUBETEX; break;
          case RES3D_VOLTEX: type = resource_dump_types::TextureTypes::VOLTEX; break;
          case RES3D_ARRTEX: type = resource_dump_types::TextureTypes::ARRTEX; break;
          case RES3D_CUBEARRTEX: type = resource_dump_types::TextureTypes::CUBEARRTEX; break;
        }
        bool color = true;
        switch (tex->getFormat().asTexFlags())
        {
          case TEXFMT_DEPTH16:
          case TEXFMT_DEPTH24:
          case TEXFMT_DEPTH32:
          case TEXFMT_DEPTH32_S8: color = false; break;
          default: break;
        }
        Image *img = tex->getDeviceImage();
        uint64_t heapID = (uint64_t)-1;
        uint64_t heapOffset = (uint64_t)-1;
        if (img && img->getMemoryId() != -1)
        {
          heapID = (uint64_t)img->getMemoryId();
          heapOffset = img->getMemory().offset;
        }

        dumpInfo.emplace_back(ResourceDumpTexture({(uint64_t)-1, heapID, heapOffset, tex->width, tex->height, tex->level_count(),
          (tex->resType == RES3D_VOLTEX || tex->resType == RES3D_CUBEARRTEX) ? tex->depth : -1,
          (tex->resType == RES3D_TEX || tex->resType == RES3D_VOLTEX) ? -1 : (img ? img->getArrayLayers() : -1), tex->ressize(),
          tex->cflg, tex->getFormat().asTexFlags(), type, color, tex->getResName(), ""}));
      });

      // Buffers
      Globals::Res::buf.iterateAllocated([&](GenericBufferInterface *buff) {
        const BufferRef &buffRef = buff->getBufferRef();
        const Buffer *bufferPtr = buffRef.buffer;

        uint64_t gpuAddress = (uint64_t)-1;
        if (bufferPtr && !bufferPtr->isFrameMem() &&
            (bufferPtr->getDescription().memoryClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER))
        {
          gpuAddress = bufferPtr->devOffsetLoc(0);
        }

        uint64_t cpuAddress = (uint64_t)-1;
        if (bufferPtr && !bufferPtr->isFrameMem() && (int)bufferPtr->getDescription().memoryClass > 1 &&
            (int)bufferPtr->getDescription().memoryClass < 6)
        {
          cpuAddress = bufferPtr->memOffsetLoc(0);
        }

        uint64_t heapID = (uint64_t)-1;
        uint64_t heapOffset = (uint64_t)-1;
        if (bufferPtr && bufferPtr->getMemoryId() != -1)
        {
          heapID = (uint64_t)bufferPtr->getMemoryId();
          heapOffset = bufferPtr->getMemory().offset;
        }

        dumpInfo.emplace_back(ResourceDumpBuffer({gpuAddress, heapID, heapOffset, (uint64_t)-1, cpuAddress, buff->ressize(), -1,
          buff->getFlags(), -1, bufferPtr ? (int)bufferPtr->getCurrentDiscardOffset() : -1,
          bufferPtr ? (int)bufferPtr->getDiscardBlocks() : -1, -1, buff->getResName(), ""}));
      });

      // Ray Acceleration Structures
#if D3D_HAS_RAY_TRACING

      Globals::Mem::res.iterateAllocated<RaytraceAccelerationStructure>([&](RaytraceAccelerationStructure *rayAS) {
        dumpInfo.emplace_back(ResourceDumpRayTrace(
          {(uint64_t)-1, (uint64_t)-1, (uint64_t)-1, (int)rayAS->getDescription().size, rayAS->getDescription().isTopLevel, true}));
      });
#endif
      return 1;
#else
      return 0;
#endif
    }
    case Drv3dCommand::CHANGE_QUEUE:
    {
      d3d_command_change_queue(static_cast<GpuPipeline>((intptr_t)par1));
      return 1;
    }
    default: break;
  };

  return 0;
}
