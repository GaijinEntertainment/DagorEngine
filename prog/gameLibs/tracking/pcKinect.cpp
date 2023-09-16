#if !_TARGET_PC_WIN
#error PC Win32/win64 only.
#endif

#include <windows.h>
#include <tracking/pcKinect.h>
#include <debug/dag_debug.h>


Kinect::Kinect() : currentSkeletonNo(0xFFFFFFFF), currentTrackingId(0xFFFFFFFF), connected(false), initialized(false)
{
  // if (XNuiGetHardwareStatus() & XNUI_HARDWARE_STATUS_READY)
  init();
}


void Kinect::init()
{
#if SUPPORT_PC_KINECT
  G_ASSERT(!initialized);
  initialized = true;

  HRESULT hres = NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);
  debug("NuiInitialize hres = %x", hres);
  if (SUCCEEDED(hres))
  {
    initialized = true;
    hres = NuiSkeletonTrackingEnable(NULL, 0);
    debug("NuiSkeletonTrackingEnable hres = %x", hres);
    G_ASSERT(SUCCEEDED(hres));
  }
#endif
}


Kinect::~Kinect()
{
#if SUPPORT_PC_KINECT
  if (initialized)
    NuiShutdown();
#endif
}


void Kinect::update()
{
#if SUPPORT_PC_KINECT
  if (!initialized)
  {
    // if (XNuiGetHardwareStatus() & XNUI_HARDWARE_STATUS_READY)
    //   init();
    // else
    //   return;
    return;
  }

  HRESULT hres = NuiSkeletonGetNextFrame(0, &skeletonFrame);
  connected = (hres != E_NUI_DEVICE_NOT_CONNECTED);

  if (SUCCEEDED(hres))
  {
    hres = NuiTransformSmooth(&skeletonFrame, NULL);
    currentSkeletonNo = 0xFFFFFFFF;
    for (unsigned int skelNo = 0; skelNo < NUI_SKELETON_COUNT; skelNo++)
    {
      if (skeletonFrame.SkeletonData[skelNo].eTrackingState != NUI_SKELETON_NOT_TRACKED)
      {
        currentSkeletonNo = skelNo;
        if (skeletonFrame.SkeletonData[skelNo].dwTrackingID == currentTrackingId)
          break;
      }
    }

    if (currentSkeletonNo != 0xFFFFFFFF)
      currentTrackingId = skeletonFrame.SkeletonData[currentSkeletonNo].dwTrackingID;
  }
#endif
}


NUI_SKELETON_DATA *Kinect::getSkeleton()
{
#if SUPPORT_PC_KINECT
  if (!initialized || !connected || currentSkeletonNo == 0xFFFFFFFF ||
      skeletonFrame.SkeletonData[currentSkeletonNo].eTrackingState == NUI_SKELETON_NOT_TRACKED)
  {
    return NULL;
  }

  return &skeletonFrame.SkeletonData[currentSkeletonNo];
#else
  return NULL;
#endif
}
