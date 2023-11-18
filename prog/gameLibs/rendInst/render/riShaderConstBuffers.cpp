#include <rendInst/riShaderConstBuffers.h>
#include "render/riShaderConstBuffers.h"
#include "render/genRender.h"


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
  perDraw[0] = v_make_vec4f(0.f, 1.f, 0.f, 0.f);
  for (int i = 1; i < countof(perDraw); ++i)
    perDraw[i] = v_zero();
  bbox[0] = v_zero(); // no change
  bbox[1] = V_C_ONE;  // no change
  bbox[2] = v_cast_vec4f(v_make_vec4i(0, 3, 0, 0));
}

void RiShaderConstBuffers::setInstancing(uint32_t ofs, uint32_t stride, uint32_t impostor_data_offset, bool global_per_draw_buffer)
{
  bbox[2] = v_cast_vec4f(v_make_vec4i(ofs, stride, global_per_draw_buffer ? 1 : 0, impostor_data_offset));
}

void RiShaderConstBuffers::setBBoxZero()
{
  bbox[0] = v_zero();
  bbox[1] = V_C_UNIT_0001;
}
void RiShaderConstBuffers::setBBoxNoChange()
{
  bbox[0] = v_zero();
  bbox[1] = V_C_ONE;
}

void RiShaderConstBuffers::setBBox(vec4f p[2])
{
  bbox[0] = p[0];
  bbox[1] = p[1];
}

void RiShaderConstBuffers::setOpacity(float p0, float p1, float p2, float p3) { perDraw[0] = v_make_vec4f(p0, p1, p2, p3); }

void RiShaderConstBuffers::setBoundingBox(const vec4f &bounding_box) { perDraw[7] = bounding_box; }

void RiShaderConstBuffers::setBoundingSphere(float p0, float p1, float sphereRadius, float cylinderRadius, float sphereCenterY)
{
  const float a0 = rendinst_ao_mul / (sphereRadius + 0.0001f);

  perDraw[1] = v_make_vec4f(p0, p1, sphereRadius, sphereCenterY);
  vec4f aoMul_cylRad = v_make_vec4f(a0, cylinderRadius, 0.f, 0.f);
  perDraw[2] = v_perm_xycd(aoMul_cylRad, perDraw[2]);
}

void RiShaderConstBuffers::setImpostorMultiWidths(float widths[], float heights[])
{
  int s = 0;
  for (int i = 0; i < 4; ++i)
  {
    // 2 slices width+height per reg
    perDraw[3 + i] = v_make_vec4f(widths[s], heights[s], widths[s + 1], heights[s + 1]);
    s += 2;
  }
}

void RiShaderConstBuffers::setRadiusFade(float radius, float drown_scale)
{
  vec4f radius_drownScale = v_make_vec4f(0., 0., radius, drown_scale);
  perDraw[2] = v_perm_xycd(perDraw[2], radius_drownScale);
}

void RiShaderConstBuffers::setInteractionParams(float hardness, float rendinst_height, float center_x, float center_z)
{
  perDraw[10] = v_make_vec4f(hardness, rendinst_height, center_x, center_z);
}

void RiShaderConstBuffers::setRandomColors(const E3DCOLOR *colors)
{
#if _TARGET_SIMD_SSE >= 4 // Note: _mm_cvtepu8_epi32 is SSE4.1
  __m128 m255 = _mm_set1_ps(float(UCHAR_MAX));
  perDraw[8] = _mm_div_ps(_mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_cvtsi32_si128(colors[0].u))), m255);
  perDraw[9] = _mm_div_ps(_mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_cvtsi32_si128(colors[1].u))), m255);
  perDraw[8] = _mm_shuffle_ps(perDraw[8], perDraw[8], 0xC6); // Swizzle BGRA.
  perDraw[9] = _mm_shuffle_ps(perDraw[9], perDraw[9], 0xC6);
#else
  *(Color4 *)(char *)&perDraw[8] = color4(colors[0]);
  *(Color4 *)(char *)&perDraw[9] = color4(colors[1]);
#endif
}

void RiShaderConstBuffers::flushPerDraw()
{
#if _TARGET_C1 | _TARGET_C2

#else
  if (perDrawCB)
  {
    bool res = rendinst::render::perDrawCB->updateDataWithLock(0, sizeof(consts), consts, VBLOCK_DISCARD);
    /*G_ASSERT(res);*/ (void)res;
    d3d::set_const_buffer(STAGE_VS, perinstBuffNo, rendinst::render::perDrawCB);
  }
  else
  {
    d3d::set_vs_const(50, (float *)consts, sizeof(consts) / sizeof(vec4f)); // 40 is hardcoded
    int *instConstI = (int *)&bbox[2];
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

} // namespace rendinst::render
