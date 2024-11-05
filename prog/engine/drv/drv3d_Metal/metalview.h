// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#if _TARGET_IOS | _TARGET_TVOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#if _TARGET_IOS | _TARGET_TVOS
@interface MetalView : UIView
#else
@interface MetalView : NSView <NSWindowDelegate>
#endif
{
  CGColorSpaceRef defaultColorSpace;
}

@property(nonatomic, readonly) id<MTLDevice> device;

@property(nonatomic) MTLPixelFormat depthPixelFormat;
@property(nonatomic) MTLPixelFormat stencilPixelFormat;
@property(nonatomic) NSUInteger sampleCount;

@property(nonatomic, readonly) bool hdrEnabled;
@property(nonatomic, readonly) bool int10HDRBuffer;

- (id)initWithFrame:(CGRect)frame andScale:(float)retinaScale;

- (void)setGamma:(float)gamma;
- (void)setVSync:(bool)enable;
- (void)setWidth:(int)scr_wd setHeight:(int)scr_ht;

- (id<MTLTexture>)getBackBuffer;
- (id<MTLTexture>)getsRGBBackBuffer;

- (id<MTLTexture>)getSavedBackBuffer;

- (void)acquireDrawable;
- (void)presentDrawable:(id<MTLCommandBuffer>)commandBuffer;

- (MTLPixelFormat)getLayerPixelFormat;
- (bool)isHDRAvailable;
- (bool)isHDREnabled;
- (void)setHDR:(bool)enable;

@end
