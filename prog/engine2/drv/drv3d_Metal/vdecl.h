
#pragma once

#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

namespace drv3d_metal
{
class VDecl
{
public:
  MTLVertexDescriptor *vertexDescriptor;

  int num_location;

  struct Location
  {
    int offset;
    int vsdr;
    int size;
    int stream;
  };

  Location locations[16];

  int num_streams;

  uint64_t hash;

  VDecl(VSDTYPE *d);
  void release();
};
} // namespace drv3d_metal
