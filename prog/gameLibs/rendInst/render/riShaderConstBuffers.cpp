// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/riShaderConstBuffers.h>
#include "render/riShaderConstBuffers.h"
#include "render/genRender.h"
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>


namespace rendinst::render
{

Sbuffer *perDrawCB = nullptr;

static carray<Sbuffer *, INST_BINS> instancesCB = {0};

void endRenderInstancing()
{
  d3d::set_buffer(STAGE_VS, instancingTexRegNo, 0);
  d3d::set_const_buffer(STAGE_VS, perinstBuffNo, nullptr);
  d3d::set_const_buffer(STAGE_VS, instanceBuffNo, nullptr);
  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
}

void startRenderInstancing()
{
#if !(_TARGET_C1 | _TARGET_C2)
  d3d::set_const_buffer(STAGE_VS, perinstBuffNo, rendinst::render::perDrawCB); // fixme: set only once
#endif
}

bool setPerDrawData(const UniqueBuf &buf)
{
  static ShaderVariableInfo perDrawDataVarId("perDrawInstanceData");
  if (ShaderGlobal::get_buffer(perDrawDataVarId) != buf.getBufId())
    return ShaderGlobal::set_buffer(perDrawDataVarId, buf);

  return false;
}

void init_instances_tb()
{
  for (int i = 0; i < instancesCB.size(); ++i)
    instancesCB[i] = d3d::buffers::create_one_frame_cb(MIN_INST_COUNT << i, "perInstanceData");
  // instancesTB[i] = d3d::create_sbuffer(16, MIN_INST_COUNT<<i,
  //     SBCF_DYNAMIC|SBCF_BIND_SHADER_RES|SBCF_CPU_ACCESS_WRITE, TEXFMT_A32B32G32R32F, "perInstanceData");
  // on my 1070 ConstBuffer is proven to work faster than TB. We can actually handle a lot of instances in one CB. It may be worth
  // switching to CB only instancing... however, in synthetic tests both TB and SB were faster than CB when fetch 3 float4. I guess,
  // the difference is that we sample just ONE float4 for trees, and cache prefetching doesn't help us (like it does in TB/SB)
  // unfortunately, that mean we have to keep one more interval
}

void close_instances_tb()
{
  for (int i = 0; i < instancesCB.size(); ++i)
    del_d3dres(instancesCB[i]);
}

RiShaderConstBuffers::RiShaderConstBuffers()
{
  memset(&consts, 0, sizeof(consts));
  consts.rendinst_opacity = v_make_vec4f(0.f, 1.f, 0.f, 0.f);
  consts.bbox[0] = v_zero(); // no change
  consts.bbox[1] = V_C_ONE;  // no change
  consts.bbox[2] = v_cast_vec4f(v_make_vec4i(0, 3, 0, 0));
}

void RiShaderConstBuffers::setInstancing(uint32_t ofs, uint32_t stride, uint32_t flags, uint32_t impostor_data_offset)
{
  // flags 1: useCbufferParams 2: hashVal
  consts.bbox[2] = v_cast_vec4f(v_make_vec4i(ofs, stride, flags, impostor_data_offset));
}

void RiShaderConstBuffers::setBBoxZero()
{
  consts.bbox[0] = v_zero();
  consts.bbox[1] = V_C_UNIT_0001;
}
void RiShaderConstBuffers::setBBoxNoChange()
{
  consts.bbox[0] = v_zero();
  consts.bbox[1] = V_C_ONE;
}

void RiShaderConstBuffers::setBBox(vec4f p[2])
{
  consts.bbox[0] = p[0];
  consts.bbox[1] = p[1];
}

void RiShaderConstBuffers::setOpacity(float p0, float p1, float p2, float p3)
{
  consts.rendinst_opacity = v_make_vec4f(p0, p1, p2, p3);
}

void RiShaderConstBuffers::setBoundingBox(const vec4f &bounding_box)
{
  consts.rendinst_bbox__cross_dissolve_range = v_perm_xyzd(bounding_box, consts.rendinst_bbox__cross_dissolve_range);
}

void RiShaderConstBuffers::setCrossDissolveRange(float crossDissolveRange)
{
  consts.rendinst_bbox__cross_dissolve_range = v_perm_xyzd(consts.rendinst_bbox__cross_dissolve_range, v_splats(crossDissolveRange));
}

void RiShaderConstBuffers::setBoundingSphere(float p0, float p1, float sphereRadius, float cylinderRadius, float sphereCenterY)
{
  const float a0 = rendinst_ao_mul / (sphereRadius + 0.0001f);

  consts.bounding_sphere = v_make_vec4f(p0, p1, sphereRadius, sphereCenterY);
  vec4f aoMul_cylRad = v_make_vec4f(a0, cylinderRadius, 0.f, 0.f);
  consts.ao_mul__inv_height = v_perm_xycd(aoMul_cylRad, consts.ao_mul__inv_height);
}

void RiShaderConstBuffers::setShadowImpostorSizes(const SizePerRotationArr &sizes)
{
  int s = 0;
  for (auto &shadow_impostor_size : consts.optional_data.shadow_impostor_sizes)
  {
    // 2 width+height per register
    shadow_impostor_size = v_make_vec4f(sizes[s].x, sizes[s].y, sizes[s + 1].x, sizes[s + 1].y);
    s += 2;
  }
}

void RiShaderConstBuffers::setRadiusFade(float radius, float drown_scale)
{
  vec4f radius_drownScale = v_make_vec4f(0., 0., radius, drown_scale);
  consts.ao_mul__inv_height = v_perm_xycd(consts.ao_mul__inv_height, radius_drownScale);
}

void RiShaderConstBuffers::setInteractionParams(float softness, float rendinst_height, float center_x, float center_z)
{
  consts.rendinst_interaction_params = v_make_vec4f(softness, rendinst_height, center_x, center_z);
}

void RiShaderConstBuffers::flushPerDraw() const
{
#if _TARGET_C1 | _TARGET_C2

#else
  if (perDrawCB)
  {
    rendinst::render::perDrawCB->updateDataWithLock(0, sizeof(consts), &consts, VBLOCK_DISCARD);
    d3d::set_const_buffer(STAGE_VS, perinstBuffNo, rendinst::render::perDrawCB);
  }
  else
  {
    d3d::set_vs_const(50, (float *)&consts, sizeof(consts) / sizeof(vec4f)); // 40 is hardcoded
    int *instConstI = (int *)&consts.bbox[2];
    Point4 instConstF(instConstI[0], instConstI[1], instConstI[2], instConstI[3]);
    d3d::set_vs_const(52, &instConstF.x, 1);
  }
#endif
}

void RiShaderConstBuffers::setInstancePositions(const float *data, int vec4_count)
{
#if _TARGET_C1 | _TARGET_C2

#else
  G_ASSERT_RETURN(vec4_count > 0, );
  G_ASSERT(vec4_count <= MAX_INSTANCES);
  vec4_count = min(vec4_count, (int)MAX_INSTANCES);

  const int bin = d3d::get_driver_code().is(d3d::metal)
                    ? rendinst::render::instancesCB.size() - 1 // because of metal validator has to select cb size that matches the
                                                               // shader
                    : min<int>(get_bigger_log2(1 + (vec4_count - 1) / MIN_INST_COUNT), rendinst::render::instancesCB.size() - 1);

  rendinst::render::instancesCB[bin]->updateDataWithLock(0, vec4_count * sizeof(vec4f), data, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  // d3d::set_buffer(STAGE_VS, instancingTexRegNo, rendinst::render::instancesCB[bin]);
  // on my 1070 ConstBuffer is proven to work faster than TB. We can actually handle a lot of instances in one CB. It may be worth
  // switching to CB only instancing... however, in synthetic tests both TB and SB were faster than CB when fetch 3 float4. I guess,
  // the difference is that we sample just ONE float4 for trees, and cache prefetching doesn't help us (like it does in TB/SB)
  d3d::set_const_buffer(STAGE_VS, instanceBuffNo, rendinst::render::instancesCB[bin]);
#endif
}

void RiShaderConstBuffers::setPLODRadius(float radius)
{
  consts.optional_data.plod_data.radius = v_make_vec4f(radius, 0.0f, 0.0f, 0.0f);
}

} // namespace rendinst::render
