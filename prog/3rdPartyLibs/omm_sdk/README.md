# omm_sdk

NVIDIA Opacity Micro-Map (OMM) SDK: bakes and encodes opacity states of
micro-triangles to accelerate raytracing of alpha-tested/alpha-blended
geometry (e.g. foliage) on D3D12 and Vulkan.

Upstream: NVIDIA-RTX/OMM (formerly NVIDIAGameWorks/Opacity-MicroMap-SDK),
https://github.com/NVIDIA-RTX/OMM
Vendored version: 1.9.1.0 (see src/version.h)

Local modifications not present upstream:

- shaders/omm_work_setup_cs.cs.hlsl, shaders/omm_work_setup_gfx.cs.hlsl:
  skip OMM rasterization for triangles with an enormous or non-finite
  texel-space UV footprint, which otherwise hangs gpu.

Licensed under the NVIDIA RTX SDKs proprietary license (see LICENSE.txt).
