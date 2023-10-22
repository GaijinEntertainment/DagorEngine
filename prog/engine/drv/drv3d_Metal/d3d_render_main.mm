
#if _TARGET_PC_MACOSX
#import <AppKit/NSWindow.h>
#import <AppKit/NSScreen.h>
#import <AppKit/NSCursor.h>
#include <3d/dag_drv3d_pc.h>
#else
#import <UIKit/UIKit.h>
#include <humanInput/dag_hiCreate.h>
#endif

#include <CoreFoundation/CoreFoundation.h>

#include <3d/dag_drv3dCmd.h>
#include <3d/tql.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>

#include <osApiWrappers/dag_miscApi.h>
#include "../drv3d_commonCode/gpuConfig.h"

#include <macosx/macWnd.h>

#include "render.h"
#include "d3d_initialSettings.h"
#include "d3d_config.h"
#define IMMEDIATE_CB_NAMESPACE namespace drv3d_metal
#include <immediateConstStub.h>

using namespace drv3d_metal;

extern bool get_metal_settings_resolution(int &width, int &height, bool& is_retina, int def_width, int def_height, bool &out_is_auto);
extern int get_retina_mode();
extern bool get_allow_intel4000();

extern Tab<drv3d_metal::Texture*> tmp_depth;

extern bool metal_use_queries;

//load d3d initial settings with metal specific
bool get_initial_settings(D3dInitialSettings &initSetts)
{
  bool isAutoResoultion;
  int width, height;
  IPoint2 &scr = initSetts.resolution;
  get_metal_settings_resolution(width, height, initSetts.allowRetinaRes, scr.x, scr.y, isAutoResoultion);
  initSetts.resolution.set(width, height);

  return true;
}

bool isMetalAvailable()
{
#if _TARGET_PC_MACOSX
  NSDictionary *systemVersionDictionary = [NSDictionary dictionaryWithContentsOfFile :
                       @"/System/Library/CoreServices/SystemVersion.plist"];

  NSString *systemVersion = [systemVersionDictionary objectForKey : @"ProductVersion"];
  const char* str = [systemVersion UTF8String];

  int major = atof(str);

  if (major < 10)
  {
    debug("major version is below 10, Metal is not avalable");
    return false;
  }

  if (major == 10)
  {
    int minor = atoi(strstr(str, ".") + 1);

    if (minor < 13)
    {
      debug("major is 10 but minor version is below 12, Metal is not avalable");
      return false;
    }
  }

  if (([MTLCopyAllDevices() count] == 0))
  {
    debug("There is no Metal device available.");
    return false;
  }
  else
  {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    bool res = true;
    if ([[device name] isEqualToString:@"Intel HD Graphics 4000"] && !get_allow_intel4000())
    {
      res = false;
    }

    [device release];

    if (!res)
    {
      debug("There is no Metal device available.");
      return false;
    }
  }
#endif

  debug("Metal device is available.");

  return true;
}

void* screenBitmap;

#if _TARGET_PC_MACOSX
NSWindow* mainwnd;
extern NSWindow* macosx_create_dagor_window(const char* title, int scr_w, int scr_h, NSView* drawView, NSRect rect);
#else
extern UIWindow* createIOSDagorWindow(UIView* drawView, float scale);
#endif

bool d3d::init_driver()
{
  if (isMetalAvailable())
    return true;
  return false;
}

void d3d::release_driver()
{
  for (int i = 0; i < tmp_depth.size(); i++)
  {
    tmp_depth[i]->release();
  }

  tmp_depth.clear();

  TEXQL_SHUTDOWN_TEX();

  // we just need pointers
  Tab<BaseTexture*> texStub(tql::texStub.size());
  for (size_t i = 0; i < tql::texStub.size(); ++i)
    texStub[i] = tql::texStub[i];
  tql::termTexStubs();
  tql::texStub = eastl::move(texStub);
  render.release();
  tql::texStub.clear();
}

bool d3d::is_inited()
{
  return render.isInited();
}

// for debugging
void d3d::beginEvent(const char * name)
{
  render.beginEvent(name);
}

// for debugging
void d3d::endEvent()
{
  render.endEvent();
}

using UpdateGpuDriverConfigFunc = void (*)(GpuDriverConfig &);
extern UpdateGpuDriverConfigFunc update_gpu_driver_config;
static void update_metal_gpu_driver_config(GpuDriverConfig &gpu_driver_config)
{
#if _TARGET_IOS
  NSArray *ver = [[[UIDevice currentDevice] systemVersion] componentsSeparatedByString:@"."];

  gpu_driver_config.driverVersion[0] = ver.count > 0 ? [[ver objectAtIndex:0] intValue] : 0;
  gpu_driver_config.driverVersion[1] = ver.count > 1 ? [[ver objectAtIndex:1] intValue] : 0;
  gpu_driver_config.driverVersion[2] = ver.count > 2 ? [[ver objectAtIndex:2] intValue] : 0;
  gpu_driver_config.driverVersion[3] = ver.count > 3 ? [[ver objectAtIndex:3] intValue] : 0;
#else
  G_UNUSED(gpu_driver_config);
#endif
}

bool d3d::init_video(void* hinst,
           main_wnd_f* mwf,
           const char* wcname,
           int ncmdshow,
           void*& mainwnd_,
           void* renderwnd,
           void* hicon,
           const char* title,
           Driver3dInitCallback* cb)
{
  float scale = 1.0f;

#if _TARGET_PC_MACOSX
  D3dInitialSettings initSetts(0, 0);
  render.scr_bpp = 32;
  bool inwin = dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE;

  NSRect mainDisplayRect = inwin ? [[NSScreen mainScreen] visibleFrame] : [[NSScreen mainScreen] frame];
  initSetts.resolution.x = mainDisplayRect.size.width;
  initSetts.resolution.y = mainDisplayRect.size.height;

  get_initial_settings(initSetts);
  scale = get_retina_mode() > 0 ? [[NSScreen mainScreen] backingScaleFactor] : 1;
  float rt_scale = scale;

  render.scr_wd = initSetts.resolution.x;
  render.scr_ht = initSetts.resolution.y;
  render.wnd_wd = render.scr_wd * scale;
  render.wnd_ht = render.scr_ht * scale;

  if (inwin)
  {
    mainDisplayRect = NSMakeRect((mainDisplayRect.size.width - render.scr_wd)/2,
                                 (mainDisplayRect.size.height- render.scr_ht)/2,
                                  render.scr_wd, render.scr_ht);
  }

  NSRect viewRect = NSMakeRect(0.0, 0.0, mainDisplayRect.size.width, mainDisplayRect.size.height);
  render.mainview = [[MetalView alloc] initWithFrame:viewRect andScale:scale];

  [render.mainview setWidth : render.scr_wd setHeight: render.scr_ht];
  [render.mainview setVSync : initSetts.vsync];

  render.scr_wd *= rt_scale;
  render.scr_ht *= rt_scale;

  debug("[METAL_INIT] src_wd=%d scr_ht=%d wnd_wd=%d wnd_ht=%d scale=%.3f rt_scale=%.3f vsync=%s fullscreen=%s",
    render.scr_wd, render.scr_ht, render.wnd_wd, render.wnd_ht, scale, rt_scale,
    initSetts.vsync ? "vsync-on" : "no-vsync", inwin ? "false" : "true");
  debug("[METAL_INIT] display rect = (%d %d %d %d)",
    mainDisplayRect.origin.x, mainDisplayRect.origin.y, mainDisplayRect.size.width, mainDisplayRect.size.height);

#else

  if ([[UIScreen mainScreen] respondsToSelector:@selector(nativeScale)])
  {
    scale = get_retina_mode() > 0 ? [[UIScreen mainScreen] nativeScale] : 1;
  }
  float rt_scale = scale;

  render.scr_wd = [[UIScreen mainScreen] bounds].size.width;
  render.scr_ht = [[UIScreen mainScreen] bounds].size.height;
  render.wnd_wd = [[UIScreen mainScreen] bounds].size.width  * scale;
  render.wnd_ht = [[UIScreen mainScreen] bounds].size.height * scale;
  render.scr_bpp = 32;

  D3dInitialSettings initSetts(render.scr_wd, render.scr_ht);
  get_initial_settings(initSetts);

  render.scr_wd *= rt_scale;
  render.scr_ht *= rt_scale;

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 90000
  CGRect rect = [[UIScreen mainScreen] bounds];
#else
  CGRect rect = [[UIScreen mainScreen] applicationFrame];
#endif
  render.mainview = [[MetalView alloc] initWithFrame:rect andScale:scale];

  UIWindow* mainwnd = createIOSDagorWindow(render.mainview, scale);
  mainwnd_ = mainwnd;
#endif

  render.device = render.mainview.device;

  d3d::set_render_target();

  render.init();

  int *mtl;
  render.getSupportedMTLVersion(&mtl);
  debug("[METAL_INIT] supported metal version %c %c %c %c", mtl[0], mtl[1], mtl[2], mtl[3]);
  debug("[METAL_INIT] readWriteTextureTier1 %d", render.caps.readWriteTextureTier1);
  debug("[METAL_INIT] readWriteTextureTier2 %d", render.caps.readWriteTextureTier2);
  debug("[METAL_INIT] samplerHaveCmprFun %d", render.caps.samplerHaveCmprFun);
  debug("[METAL_INIT] drawIndxWithBaseVertes %d", render.caps.drawIndxWithBaseVertes);

  tql::initTexStubs();

#if _TARGET_PC_MACOSX
  mainwnd = macosx_create_dagor_window(title, render.wnd_wd, render.wnd_ht, render.mainview, mainDisplayRect);
  mainwnd_ = mainwnd;
#endif
  update_gpu_driver_config = update_metal_gpu_driver_config;

  render.inited = true;

  return true;
}

void d3d::get_screen_size(int &w, int &h)
{
  w = render.wnd_wd;
  h = render.wnd_ht;
}

bool g_ios_pause_rendering = false;
bool d3d::update_screen(bool app_active)
{
  if (g_ios_pause_rendering)
    return true;
  @autoreleasepool
  {
    render.endFrame();
  }

  drv3d_metal::clear_textures_pool_garbage();
  drv3d_metal::clear_buffers_pool_garbage();

  if (::dgs_on_swap_callback)
  {
    ::dgs_on_swap_callback();
  }
  return true;
}

void d3d::window_destroyed(void* hw)
{
#if _TARGET_PC_MACOSX
  if (hw == mainwnd && hw)
  {
    mainwnd = NULL;
  }
#endif
}

#if _TARGET_PC_MACOSX
bool d3d::get_vsync_enabled()
{
  return render.cur_vsync;
}

bool d3d::enable_vsync(bool enable)
{
  [render.mainview setVSync: enable];
  render.cur_vsync = enable;

  return true;
}

void d3d::get_video_modes_list(Tab<String> &list)
{
  mac_wnd::get_video_modes_list(list);
}
#else
bool d3d::get_vsync_enabled() { return true; }
bool d3d::enable_vsync(bool) { return true; }
#endif

/// capture screen to buffer.fast, but not guaranteed
/// many captures can be followed by only one end_fast_capture_screen()
void *d3d::fast_capture_screen(int &width, int &height, int &stride_bytes, int &format)
{
  render.aquareOwnerShip();
  render.save_backBuffer = true;
  render.flush(true);
  render.releaseOwnerShip();

  id<MTLTexture> backBuffer = [render.mainview getSavedBackBuffer];

  width = (int)[backBuffer width];
  height = (int)[backBuffer height];
  stride_bytes = width * 4;
  int selfturesize = stride_bytes * height;

  screenBitmap = malloc(selfturesize);

  [backBuffer getBytes:screenBitmap bytesPerRow:stride_bytes fromRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0];

  format = CAPFMT_X8R8G8B8;

  uint8_t* data = (uint8_t*)screenBitmap;

#if _TARGET_IOS | _TARGET_TVOS
  for (int i = 0; i < height; i++)
  {
    uint8_t* p = data + stride_bytes * i;

    for (int j = 0; j < width; j++)
    {
      uint8_t tmp[4];
      memcpy(tmp, p, 4);

      p[0] = tmp[3];
      p[1] = tmp[2];
      p[2] = tmp[1];
      p[3] = tmp[0];

      p += 4;
    }
  }
#endif

  return data;
}

void d3d::end_fast_capture_screen()
{
  free(screenBitmap);
}
