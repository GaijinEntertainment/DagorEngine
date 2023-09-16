#pragma once

#include <3d/dag_drv3d.h>


namespace res_act_generic
{

void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);


void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline);
void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline);

} // namespace res_act_generic


#define IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC()                                                                         \
  void d3d::activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline) \
  {                                                                                                                                   \
    res_act_generic::activate_buffer(buf, action, value, gpu_pipeline);                                                               \
  }                                                                                                                                   \
  void d3d::activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value,                      \
    GpuPipeline gpu_pipeline)                                                                                                         \
  {                                                                                                                                   \
    res_act_generic::activate_texture(tex, action, value, gpu_pipeline);                                                              \
  }                                                                                                                                   \
  void d3d::deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline)                                                                 \
  {                                                                                                                                   \
    res_act_generic::deactivate_buffer(buf, gpu_pipeline);                                                                            \
  }                                                                                                                                   \
  void d3d::deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline)                                                            \
  {                                                                                                                                   \
    res_act_generic::deactivate_texture(tex, gpu_pipeline);                                                                           \
  }
