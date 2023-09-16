#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

struct RaytraceTopAccelerationStructure
{
  id<MTLAccelerationStructure> instanceAccelerationStructure;
};

struct RaytraceBottomAccelerationStructure
{
  id<MTLAccelerationStructure> primitiveAccelerationStructure;
  NSUInteger index = -1;
};