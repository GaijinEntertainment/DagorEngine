// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import "metalview.h"
#include "render.h"

#include <drv/3d/dag_commands.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>

#include <osApiWrappers/dag_miscApi.h>
#include "drv_utils.h"

extern bool get_lowpower_mode();

@implementation MetalView
{
@private
  CAMetalLayer *metalLayer;

  BOOL layerSizeDidUpdate;

  id <MTLTexture>      backTex;
  id <MTLTexture>      SRGBTex;
  id <CAMetalDrawable> currentDrawable;
  id <MTLTexture>      savedBackTex;

  int layer_width;
  int layer_height;
}

+ (Class)layerClass
{
  return [CAMetalLayer class];
}

- (void)initCommon:(float)retinaScale
{
#if _TARGET_IOS | _TARGET_TVOS
  self.userInteractionEnabled = NO;
  self.opaque      = YES;
  self.backgroundColor = nil;
  self.contentScaleFactor = [UIScreen mainScreen].nativeScale;
  self.layer.contentsScale = retinaScale;
  metalLayer = (CAMetalLayer *)self.layer;
  metalLayer.maximumDrawableCount = 3; // drv3d_metal::Render::MAX_FRAMES_TO_RENDER;
#else
  self.wantsLayer = YES;
  self.layer = metalLayer = [CAMetalLayer layer];
  self.layer.contentsScale = retinaScale;
#endif

#if _TARGET_IOS | _TARGET_TVOS
  _device = MTLCreateSystemDefaultDevice();
#else
  _device = nil;
  debug("[METAL_INIT] lowpower device is requested %s", get_lowpower_mode() ? "true" : "false");
  if (get_lowpower_mode())
  {
    NSArray<id<MTLDevice>> * devices = MTLCopyAllDevices();
    for (id<MTLDevice> dev in devices)
    {
      debug("[METAL_INIT] checking device %s, lowpower is %s", [dev.name UTF8String], dev.lowPower ? "true" : "false");
      if (dev.lowPower == YES)
        _device = [dev retain];
    }
  }
  if (_device == nil)
    _device = MTLCreateSystemDefaultDevice();
  debug("[METAL_INIT] selected device %s, lowpower is %s", [_device.name UTF8String], _device.lowPower ? "true" : "false");
#endif

  metalLayer.device      = _device;
  metalLayer.pixelFormat   = MTLPixelFormatBGRA8Unorm;
  metalLayer.framebufferOnly = NO;
  defaultColorSpace = metalLayer.colorspace;
  if ([self isHDREnabled] && [self isHDRAvailable])
  {
    [self setHDR: true];
  }
  layer_width = 800;
  layer_height = 600;

  layerSizeDidUpdate = false;

  [self setVSync:false];

  backTex = nil;
  currentDrawable = nil;
  savedBackTex = nil;
}

-(void)setGamma : (float)gamma
{
#if _TARGET_IOS | _TARGET_TVOS

  debug("metalLayer.colorspace not supported on iOS/tvOS");

#else

  if (metalLayer.colorspace)
  {
    CGColorSpaceRelease(metalLayer.colorspace);
  }

  gamma = fmax(0.0f, gamma);
  gamma = fmin(2.0f, gamma);

  gamma = (2.0f - gamma) * 1.5f + 0.7f;

  CGFloat white[3] = { 0.95, 1, 1.089 };
  CGFloat black[3] = { 0, 0, 0 };
  CGFloat gammav[3] = { gamma, gamma, gamma };
  CGFloat matrix[9] = { 0.449, 0.244, 0.024,
                        0.316, 0.672, 0.141,
                        0.184, 0.083, 0.923 };

  metalLayer.colorspace = CGColorSpaceCreateCalibratedRGB(white, black, gammav, matrix);
#endif
}

-(void)setVSync : (bool)enable
{
#if _TARGET_IOS | _TARGET_TVOS

  debug("setVSync not supported on iOS/tvOS");

#else
  metalLayer.displaySyncEnabled = enable;
#endif
}

-(void)setWidth : (int)scr_wd setHeight : (int)scr_ht;
{
  layer_width = scr_wd;
  layer_height = scr_ht;
}

#if _TARGET_IOS | _TARGET_TVOS
- (void)didMoveToWindow
{
  self.contentScaleFactor = self.window.screen.nativeScale;
  self.layer.contentsScale = (self.window.screen.nativeScale);
}
#endif

- (id)initWithFrame:(CGRect)frame andScale:(float)retinaScale
{
  self = [super initWithFrame:frame];

	if(self)
  {
    [self initCommon:retinaScale];
  }

  return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];

  if(self)
  {
    [self initCommon:1.0f];
  }
  return self;
}

- (void)releaseTextures
{
  SRGBTex   = nil;
}

#if _TARGET_PC_MACOSX
- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize : newSize];
    layerSizeDidUpdate = YES;
}

-(void)setBoundsSize:(NSSize)newSize
{
    [super setBoundsSize : newSize];
    layerSizeDidUpdate = YES;
}

-(void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    layerSizeDidUpdate = YES;
}
#endif

#if _TARGET_IOS | _TARGET_TVOS
- (void)setContentScaleFactor:(CGFloat)contentScaleFactor
{
    [super setContentScaleFactor : contentScaleFactor];
    self.layer.contentsScale = (self.contentScaleFactor);
    layerSizeDidUpdate = YES;
}

-(void)layoutSubviews
{
    [super layoutSubviews];
    layerSizeDidUpdate = YES;
}
#endif

- (id<MTLTexture>)getBackBuffer
{
  return backTex;
}

-(id<MTLTexture>)getsRGBBackBuffer
{
  G_ASSERT(SRGBTex);
  return SRGBTex;
}

-(id<MTLTexture>)getSavedBackBuffer
{
  {
    G_ASSERT(backTex != nil);
    BOOL doUpdate = (savedBackTex.width != backTex.width) ||
                    (savedBackTex.height != backTex.height);

    if (!savedBackTex || doUpdate)
    {
      if (savedBackTex)
        drv3d_metal::render.queueTextureForDeletion(savedBackTex);
      MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : metalLayer.pixelFormat
                                                                                      width : backTex.width
                                                                                     height : backTex.height
                                                                                  mipmapped : NO];

      desc.textureType = MTLTextureType2D;

      savedBackTex = [self.device newTextureWithDescriptor : desc];
    }
  }
  return savedBackTex;
}

-(void)acquireDrawable
{
  if (currentDrawable == nil)
  {
    @autoreleasepool
    {
      currentDrawable = [[metalLayer nextDrawable] retain];

      while (!currentDrawable)
      {
        cpu_yield();
        currentDrawable = [[metalLayer nextDrawable] retain];
      }
    }
  }

  @autoreleasepool
  {
    backTex = [currentDrawable.texture retain];
    SRGBTex = [backTex newTextureViewWithPixelFormat : MTLPixelFormatBGRA8Unorm_sRGB];
  }
}

-(void)presentDrawable: (id<MTLCommandBuffer>)commandBuffer
{
#if _TARGET_IOS
  extern int g_ios_fps_limit;
  [commandBuffer presentDrawable:currentDrawable afterMinimumDuration:1.0 / g_ios_fps_limit];
#else
  [commandBuffer presentDrawable:currentDrawable];
#endif

  if (layerSizeDidUpdate)
  {
    CGSize drawableSize = self.bounds.size;

#if _TARGET_IOS | _TARGET_TVOS
    drawableSize.width  *= metalLayer.contentsScale;
    drawableSize.height *= metalLayer.contentsScale;
#else
    drawableSize.width = layer_width * metalLayer.contentsScale;
    drawableSize.height = layer_height * metalLayer.contentsScale;
#endif

    metalLayer.drawableSize = drawableSize;

    layerSizeDidUpdate = NO;
  }

  [currentDrawable release];
  currentDrawable = NULL;

  drv3d_metal::render.queueTextureForDeletion(backTex);
  drv3d_metal::render.queueTextureForDeletion(SRGBTex);
  backTex = NULL;
  SRGBTex = NULL;
}

-(MTLPixelFormat) getLayerPixelFormat
{
  return metalLayer.pixelFormat;
}

-(bool) isHDREnabled
{
  return get_enable_hdr_from_settings([[_device name] UTF8String]);
}

-(bool) isHDRAvailable
{
#if _TARGET_IOS | _TARGET_TVOS
  return true;
#elif _TARGET_PC_MACOSX
  NSScreen *screen = [NSScreen mainScreen];
  return screen.maximumPotentialExtendedDynamicRangeColorComponentValue > 1.f;
#endif
}

-(void) setHDR:(bool)enable
{
  if (_hdrEnabled == enable)
    return;
  if (enable)
  {
    metalLayer.pixelFormat = MTLPixelFormatBGR10A2Unorm;
#if _TARGET_IOS | _TARGET_TVOS
    if ([_device supportsFamily:MTLGPUFamilyApple3])
      metalLayer.pixelFormat = MTLPixelFormatBGR10_XR;
#elif _TARGET_PC_MACOSX
    metalLayer.wantsExtendedDynamicRangeContent = YES;
#endif

#if _TARGET_IOS || MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_11_0
    const CFStringRef name = kCGColorSpaceITUR_2020_PQ_EOTF;
#else
    const CFStringRef name = kCGColorSpaceITUR_2100_PQ;
#endif
    CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(name);
    metalLayer.colorspace = colorspace;
    CGColorSpaceRelease(colorspace);
  }
  else
  {
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.colorspace = defaultColorSpace;
  }
  _int10HDRBuffer = _hdrEnabled = enable;
  DEBUG_CTX("macOS: HDR is %s", enable ? "Enabled" : "disabled");
}
@end
