#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

struct RaytraceTopAccelerationStructure
{
  API_AVAILABLE(ios(15.0), macos(11.0)) id<MTLAccelerationStructure> instanceAccelerationStructure;
  RaytraceBuildFlags createFlags;
};

struct RaytraceBottomAccelerationStructure
{
  API_AVAILABLE(ios(15.0), macos(11.0)) id<MTLAccelerationStructure> primitiveAccelerationStructure;
  NSUInteger index = -1;
  RaytraceBuildFlags createFlags;
};
