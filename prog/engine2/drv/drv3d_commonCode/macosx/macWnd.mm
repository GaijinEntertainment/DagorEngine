#import <AppKit/NSWindow.h>
#import <AppKit/NSScreen.h>
#import <AppKit/NSCursor.h>
#include <CoreFoundation/CoreFoundation.h>
#import <QuartzCore/CAMetalLayer.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_drv3d_pc.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>

@interface DagView : NSView
@end

extern NSWindow* macosx_create_dagor_window(const char* title, int scr_w, int scr_h, NSView* drawView, NSRect rect);



@implementation DagView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer { return YES; }

-(BOOL) wantsLayer { return YES; }

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
-(CALayer*) makeBackingLayer { return [self.class.layerClass layer]; }

@end



namespace mac_wnd
{
  static NSWindow *mainwnd = NULL;
  static DagView *mainview = NULL;

  void init()
  {
  }

  bool init_window(const char *title, int width, int height)
  {
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
    if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
    {
      mainDisplayRect = NSMakeRect((mainDisplayRect.size.width - width) / 2, (mainDisplayRect.size.height - height) / 2, width, height);
    }

    NSRect viewRect = NSMakeRect(0.0, 0.0, mainDisplayRect.size.width, mainDisplayRect.size.height);
    mainview = [[DagView alloc] initWithFrame:viewRect];
    mainwnd = macosx_create_dagor_window(title, width, height, mainview, mainDisplayRect);
    mainview.needsDisplay = true;
    // explicitly call the display method to be ready for immediate using (for example in vulkan surface)
    [mainwnd display];

    return true;
  }

  void destroy_window()
  {
    mainview = NULL;
    mainwnd = NULL;
  }

  void get_display_size(int &width, int &height)
  {
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
    width = mainDisplayRect.size.width;
    height = mainDisplayRect.size.height;
  }

  void get_video_modes_list(Tab<String> &list)
  {
    list.clear();
    CFArrayRef modes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, NULL);
    int modeCount = CFArrayGetCount(modes);

    float retina_scale = [[NSScreen mainScreen] backingScaleFactor];
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
    int scr_w = mainDisplayRect.size.width;
    int scr_h = mainDisplayRect.size.height;

    list.push_back(String(64, "%d x %d", scr_w, scr_h));
    if (retina_scale > 1.f)
      list.push_back(String(64, "%d x %d :R", scr_w, scr_h));

    for (int i = 0; i < modeCount; ++i)
    {
      CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
      int height = CGDisplayModeGetHeight(mode);
      int width = CGDisplayModeGetWidth(mode);

      String newItem = String(64, "%d x %d", width, height);
      if (width > scr_w || height > scr_h || find_value_idx(list, newItem) >= 0)
        continue;

      list.push_back(newItem);
      if (retina_scale > 1.f)
        list.push_back(String(64, "%d x %d :R", width, height));
    }

    CFRelease(modes);
  }

  void *get_main_window()
  {
    return mainwnd;
  }

  void *get_main_view()
  {
    return mainview;
  }
}
