#include "renderDoc_capture_module.h"

#include <RenderDoc/renderdoc_app.h>

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_dynLib.h>

namespace drv3d_vulkan
{
RenderDocCaptureModule::RenderDocCaptureModule() { loadRenderDocAPI(); };

RenderDocCaptureModule::~RenderDocCaptureModule()
{
  if (docLib)
    os_dll_close(docLib);
};

void RenderDocCaptureModule::triggerCapture(uint32_t count)
{
  // (RenderDoc docs)
  // Capture the next frame on whichever window and API is currently considered active

  // (Tests)
  // (Seems like there is a 1-2 frame delay on phones)
  if (docAPI)
  {
#if _TARGET_ANDROID
    // On some android devices MultiFrameCapture causes a crash
    // but capture of just 1 frame works correctly
    G_UNUSED(count);
    docAPI->TriggerCapture();
#else
    docAPI->TriggerMultiFrameCapture(count);
#endif
  }
}

void RenderDocCaptureModule::beginCapture()
{
  if (docAPI)
    docAPI->StartFrameCapture(nullptr, nullptr);
}

void RenderDocCaptureModule::endCapture()
{
  if (docAPI)
    docAPI->EndFrameCapture(nullptr, nullptr);
}

const char *RenderDocCaptureModule::getLibName()
{
#if _TARGET_ANDROID
  // To get access to this lib just add following line to jamfile
  // AndroidRenderDocLayer = yes ;
  return "libVkLayer_GLES_RenderDoc.so";

#elif _TARGET_PC_LINUX
  // Wasn't tested on any device. Supposed to work exactly like android.
  return "librenderdoc.so";

#elif _TARGET_PC_WIN
  // Just drop it next to executable
  return "renderdoc.dll";

#else
  return "";
#endif
}

void RenderDocCaptureModule::loadRenderDocAPI()
{
  debug("vulkan: Trying to initialize RenderDocAPI");

  const char *libName = getLibName();

  docLib = os_dll_load(libName);
  if (!docLib)
  {
    debug("vulkan: Failed to load %s", libName);
    return;
  }
  debug("vulkan: Managed to load %s", libName);

  pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(os_dll_get_symbol(docLib, "RENDERDOC_GetAPI"));
  if (RENDERDOC_GetAPI)
    debug("vulkan: RenderDoc entry point function located");
  else
  {
    debug("vulkan: Unable to locate RenderDoc entry point function");
    return;
  }

  if (RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void **)&docAPI))
    debug("vulkan: RenderDocAPI initialized");
  else
  {
    debug("vulkan: Failed to initialize RenderDocAPI");
    return;
  }

  // Game crashes if api is active and handler is unloaded but renderdoc is not attached to the game (only for PC with Vulkan driver)
  // If we start game WITH renderdoc we disable handler to get all errors
  // If we start game WITHOUT renderdoc we keep handler loaded as we don't need crashes caused by API
  // (some crashes still happen, probably because renderdoc is not perfect, but loaded handler significantly lowers crash rate)
  if (docAPI->IsTargetControlConnected())
    docAPI->UnloadCrashHandler();
};
} // namespace drv3d_vulkan
