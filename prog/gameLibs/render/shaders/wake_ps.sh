include "shader_global.sh"
include "viewVecVS.sh"
include "waveWorks.sh"
include "water_heightmap.sh"

int wfx_count = 0;
int wfx_offset = 0;
int wfx_limit = 0;
float wfx_ftime = 0;

int wfx_emitter_type = 0;
interval wfx_emitter_type: emitter_flow < 1, emitter_tail;

float4 wfx_normals_pixel_size = (1.0 / 512.0, 1.0 / 512.0, 0.0, 0.0);

texture wfx_diffuse;
texture wfx_distortion;
texture wfx_gradient;
texture wfx_hmap;
texture wfx_normals;

texture heightmap_tex;

macro USE_PARTICLE_SYSTEM(stage)
  (stage) { cnt_ofs_lim_dt@f4 = (wfx_count, wfx_offset, wfx_limit, wfx_ftime); }

  hlsl(stage) {
    #include "../wakePs/wakePs.hlsli"

    float2x2 make_rot2d(float in_angle)
    {
      float rotSin, rotCos;
      sincos(frac(in_angle / (2 * PI)) * 2 * PI, rotSin, rotCos);
      return float2x2(
        float2(rotCos, rotSin),
        float2(-rotSin, rotCos)
      );
    }
  }
endmacro

shader wfx_gen_emitter
{
  hlsl(cs) {
    #include "../wakePs/wakePs.hlsli"

    uint cmdCount: register(c0);
    GPUEmitterGen emitterGen[MAX_EMITTER_GEN_COMMANDS]: register(c1);
    RWStructuredBuffer<GPUEmitter> emitterBuffer: register(u0);

    [numthreads(EMITTER_GEN_WARP, 1, 1)]
    void main(uint3 dtId: SV_DispatchThreadID)
    {
      uint cmdId = dtId.x;
      if (cmdId >= cmdCount)
        return;
      emitterBuffer[emitterGen[cmdId].emitterId] = emitterGen[cmdId].emitter;
    }
  }

  compile("cs_5_0", "main");
}

shader wfx_emit
{
  USE_PARTICLE_SYSTEM(cs)

  (cs) { water_level@f1 = (water_level); }

  hlsl(cs) {
    GPUEmitterDynamic emitterDynData[MAX_EMITTER_GEN_COMMANDS]: register(c2);
    StructuredBuffer<GPUEmitter> emitterBuffer: register(t0);
    Texture2D<float4> random_buffer: register(t1);
    RWStructuredBuffer<GPUParticle> particleBuffer: register(u1);

    [numthreads(EMIT_WARP, 1, 1)]
    void main(uint3 dtId: SV_DispatchThreadID)
    {
      uint cmdId = dtId.x;
      if (cmdId >= cnt_ofs_lim_dt.x)
        return;

      uint emitterNo = cmdId / EMIT_PER_FRAME_LIMIT;
      uint partNo = cmdId % EMIT_PER_FRAME_LIMIT;
      GPUEmitterDynamic emitterDyn = emitterDynData[emitterNo];
      uint id, curOffset, numPart;
      UNPACK_ID_OFFSET_NUM(emitterDyn.id_coff, id, curOffset, numPart);
      if (partNo >= numPart)
        return;

      uint partId = (uint)emitterDyn.startSlot + (partNo + curOffset) % (uint)emitterDyn.numSlots;
      float4 rndV0 = random_buffer[int2((partId + id * 5) % RANDOM_BUFFER_RESOLUTION_X, 0)] * 2.0 - 1.0;
      float4 rndV1 = random_buffer[int2((partId + id * 5) % RANDOM_BUFFER_RESOLUTION_X, 1)] * 2.0 - 1.0;

      GPUEmitter emitter = emitterBuffer[id];

      GPUParticle particle;
      particle.emitterId = id;
      particle.lifeTime = emitter.lifeTime + emitter.lifeSpread * rndV0.x;
      particle.timeLeft = particle.lifeTime + emitter.lifeDelay;
      particle.pos = float3(emitterDyn.pos + emitter.posSpread * rndV1.xy, water_level);

      particle.vel = emitterDyn.vel + emitter.velSpread * rndV0.z;
      particle.vel0 = particle.vel;
      particle.dir = float3(mul(emitterDyn.dir, make_rot2d(emitter.dirSpread * rndV0.y)), 0.0);
      particle.radiusVar = emitter.radiusSpread * rndV1.z;
      particle.rot = emitterDyn.rot + emitter.rotSpread * rndV0.w;
      particle.alpha = emitterDyn.alpha;
      particle.scaleZ = emitterDyn.scaleZ;
      particle.unused = 0;

      particleBuffer[partId] = particle;
    }
  }

  compile("cs_5_0", "main");
}

shader wfx_update
{
  USE_PARTICLE_SYSTEM(cs)
  ENABLE_ASSERT(cs)
  hlsl(cs) {
    StructuredBuffer<GPUEmitter> emitterBuffer: register(t0);
    RWStructuredBuffer<GPUParticle> particleBuffer: register(u1);
    RWStructuredBuffer<GPUParticleRender> particleRenderBuffer: register(u2);
    RWByteAddressBuffer indirectBuffer: register(u3);

    [numthreads(UPDATE_WARP, 1, 1)]
    void main(uint3 dtId: SV_DispatchThreadID)
    {
      uint cmdId = dtId.x;
      if (cmdId >= cnt_ofs_lim_dt.x)
        return;

      uint partId = cnt_ofs_lim_dt.y + cmdId;
      GPUParticle particle = structuredBufferAt(particleBuffer, partId);
      if (particle.timeLeft <= 0)
        return;

      GPUEmitter emitter = structuredBufferAt(emitterBuffer, particle.emitterId);
      float tAlpha = saturate(1.0 - particle.timeLeft / particle.lifeTime);
      particle.timeLeft -= cnt_ofs_lim_dt.w;

      particle.vel = max(particle.vel - pow2(particle.vel) * cnt_ofs_lim_dt.w * emitter.velRes, particle.vel0 * 0.35);
      particle.pos += particle.dir * particle.vel * cnt_ofs_lim_dt.w * (emitter.activeFlags & MOD_VELOCITY_FLAG ? 1 : 0);

      structuredBufferAt(particleBuffer, partId) = particle;

      GPUParticleRender particleRender;
      particleRender.pos = particle.pos;

      float2x2 m = make_rot2d(particle.rot);
      particleRender.dirX = m[0];
      particleRender.dirY = m[1];

      particleRender.scale.xy = (emitter.radius + particle.radiusVar) * lerp(1, emitter.radiusScale, tAlpha) * emitter.scaleXY;
      particleRender.scale.z = particle.scaleZ;
      particleRender.color = lerp(emitter.startColor, emitter.endColor, tAlpha);
      particleRender.color.a *= particle.alpha;
      particleRender.tAlpha = tAlpha * (particle.timeLeft > particle.lifeTime ? 0.0 : 1.0);

      int numFrames = emitter.uvNumFrames.x * emitter.uvNumFrames.y;
      int animFrame = clamp(int(tAlpha * numFrames), 0, numFrames - 1);
      particleRender.fAlpha = saturate(tAlpha * numFrames - animFrame);

      #define CALC_UV(texUV, frame) \
        texUV = float4(1.0, 1.0, (frame) % emitter.uvNumFrames.x, (frame) / emitter.uvNumFrames.y) / emitter.uvNumFrames.xyxy; texUV *= emitter.uv.xyxy; texUV.zw += emitter.uv.zw;
      CALC_UV(particleRender.uv, animFrame);
      CALC_UV(particleRender.uv2, min(animFrame + 1, numFrames - 1));

      uint idx;
      indirectBuffer.InterlockedAdd(1 * 4, 1u, idx);
      structuredBufferAt(particleRenderBuffer, idx) = particleRender;
    }
  }

  compile("cs_5_0", "main");
}

shader wfx_clear
{
  ENABLE_ASSERT(cs)
  USE_PARTICLE_SYSTEM(cs)

  hlsl(cs) {
    RWByteAddressBuffer indirectBuffer: register(u3);

    [numthreads(1, 1, 1)]
    void main()
    {
      storeBuffer(indirectBuffer, 1 * 4, 0);
    }
  }

  compile("cs_5_0", "main");
}

shader wfx_render_height, wfx_render_height_distorted, wfx_render_foam, wfx_render_foam_distorted, wfx_render_foam_mask
{
  if ((shader == wfx_render_foam) || (shader == wfx_render_foam_distorted))
  {
    blend_src = one;
    blend_dst = isa;
    blend_asrc = zero;
    blend_adst = isa;
  }
  else if(shader == wfx_render_foam_mask)
  {
    blend_src = idc;
    blend_dst = one;
  }
  else
  {
    blend_src = one;
    blend_dst = one;
  }

  z_test = true;
  if (shader == wfx_render_foam_mask) {
    z_write = true;
  } else {
    z_write = false;
  }
  cull_mode = none;

  USE_PARTICLE_SYSTEM(vs)

  if ((shader == wfx_render_foam_distorted) || (shader == wfx_render_height_distorted) || (shader == wfx_render_foam_mask))
  {
    INIT_WATER_GRADIENTS()
    USE_WATER_GRADIENTS(1)

    (ps) {
      wfx_diffuse@smp2d = wfx_diffuse;
      wfx_distortion@smp2d = wfx_distortion;
      wfx_gradient@smp2d = wfx_gradient;
    }
    hlsl(ps) {
      #define water_foam_distortion_vector_params float4(1.0, 1.0, 1.0, 1.0)
      #define water_foam_distortion_color_params float4(2.0, 1.0, 0.0, 0.0)
      #define water_foam_distortion_opacity_params float4(1.0, 1.0, 1.0, 1.0)
    }
  }
  else
  {
    (ps) { wfx_diffuse@smp2d = wfx_diffuse; }
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 color: TEXCOORD0;
      float4 tc_scale_falpha: TEXCOORD1;
##if shader == wfx_render_foam || shader == wfx_render_foam_distorted || shader == wfx_render_height_distorted
      float2 tc2: TEXCOORD2;
##endif
##if shader == wfx_render_foam_distorted || shader == wfx_render_height_distorted
      float3 worldPos: TEXCOORD3;
##endif
    };
  }

  hlsl(ps) {
    float4 wfx_ps(VsOutput input) : SV_Target
    {
      float4 color = input.color;
      float2 tc = input.tc_scale_falpha.xy;
##if shader == wfx_render_foam_distorted || shader == wfx_render_height_distorted
      float3 worldPos = input.worldPos;

      float worldPosDistance = 1.0;
      float4 nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23, nvsf_tex_coord_cascade45, nvsf_blendfactors0123, nvsf_blendfactors4567;
      get_blend_factor(worldPos.xzy, worldPosDistance, nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23,
                      nvsf_tex_coord_cascade45, nvsf_blendfactors0123, nvsf_blendfactors4567);

      float nvsf_foam_turbulent_energy, nvsf_foam_surface_folding, nvsf_foam_wave_hats;
      float3 nvsf_normal;
      float fadeNormal = 1;
      get_gradients(nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23, nvsf_tex_coord_cascade45, nvsf_blendfactors0123,
            nvsf_blendfactors4567, fadeNormal, nvsf_foam_turbulent_energy, nvsf_foam_surface_folding, nvsf_normal, nvsf_foam_wave_hats);

      float strength1 = water_foam_distortion_vector_params.x * lerp(1.0, nvsf_foam_wave_hats, water_foam_distortion_vector_params.y);
      float strength2 = water_foam_distortion_vector_params.z * lerp(1.0, nvsf_foam_wave_hats, water_foam_distortion_vector_params.w);

      float4 distortion = tex2D(wfx_distortion, tc);
      float4 distortion2 = tex2D(wfx_distortion, input.tc2);
      float2 tc3 = worldPos.xz;
      tc3 += (distortion2.rg - 0.5) * strength1;
      tc3 += (tex2D(wfx_diffuse, worldPos.xz * water_foam_distortion_color_params.x).rg - 0.5) * strength2;
      float intensity = tex2D(wfx_diffuse, tc3).a * distortion.b * water_foam_distortion_color_params.y;
      float opacity = distortion.a;
##endif
##if shader == wfx_render_foam
      color.a *= lerp(tex2D(wfx_diffuse, tc).g, tex2D(wfx_diffuse, input.tc2).g, input.tc_scale_falpha.w);
      color.rgb *= color.a;
##elif shader == wfx_render_foam_distorted
      opacity *= water_foam_distortion_opacity_params.x * lerp(1.0, nvsf_foam_wave_hats, water_foam_distortion_opacity_params.y);
      color.rgb *= tex2D(wfx_gradient, float2(intensity, 0.0)).rgb * opacity;
      color.a *= intensity * opacity;
##elif shader == wfx_render_height_distorted
      opacity *= water_foam_distortion_opacity_params.z * lerp(1.0, nvsf_foam_wave_hats, water_foam_distortion_opacity_params.w);
      color.a *= intensity * opacity;
      color.rgb *= color.a;
##elif shader == wfx_render_height
      color.a *= (tex2D(wfx_diffuse, tc).r * 2.0 - 1.0) * input.tc_scale_falpha.z;
      color.rgb *= color.a;
##else // shader == wfx_render_foam_mask
      color.r = color.a*input.tc_scale_falpha.z*(1-tex2D(wfx_distortion, tc).b);
##endif
      return color;
    }
  }

  (vs) {
    globtm@f44 = globtm;
  };

  INIT_WATER_HEIGHTMAP_BASE(vs, @smp2d)
  USE_WATER_HEIGHTMAP(vs)

  hlsl(vs) {
    StructuredBuffer<GPUParticleRender> particleRenderBuffer: register(t2);

    VsOutput wfx_vs(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
    {
      uint vPosId = vertexId % VERTICES_PER_PARTICLE;
      uint partId = instanceId;

      GPUParticleRender particle = particleRenderBuffer[partId];
      float2 vPos = float2(vPosId == 0 || vPosId == 1 ? -1.0 : 1.0, vPosId == 0 || vPosId == 3 ? -1.0 : 1.0);
      float2 tc = vPos * 0.5 + 0.5;
      vPos.xy *= particle.scale.xy;
      vPos = vPos.x * particle.dirX + vPos.y * particle.dirY;
##if shader == wfx_render_foam_distorted || shader == wfx_render_height_distorted
      float2 tc2 = vPos * 0.5 + 0.5;
##endif

      float3 worldPos = particle.pos.xzy + float3(vPos.x, 0.0, vPos.y);
      get_water_height(worldPos.xz, worldPos.y);

      VsOutput output;
      output.pos = mul(float4(worldPos, 1.0), globtm);
      output.color = particle.color;
      output.tc_scale_falpha = float4(tc * particle.uv.xy + particle.uv.zw, particle.scale.z, particle.fAlpha);
##if shader == wfx_render_foam
      output.color.a *= saturate(particle.tAlpha / 0.05);
      output.tc2 = tc * particle.uv2.xy + particle.uv2.zw;
##elif shader == wfx_render_foam_distorted
      output.color.a *= saturate(particle.tAlpha / 0.05);
      output.tc2 = tc2;
      output.worldPos = worldPos;
##elif shader == wfx_render_height_distorted
      output.color.a *= saturate(particle.tAlpha / 0.05) - saturate((particle.tAlpha - 0.8) / 0.2);
      output.tc2 = tc2;
      output.worldPos = worldPos;
##elif shader == wfx_render_height
      output.color.a *= saturate(particle.tAlpha / 0.05) - saturate((particle.tAlpha - 0.8) / 0.2);
##else // shader == wfx_render_foam_mask
      output.color.a *= saturate(particle.tAlpha / 0.05) * 0.5;
##endif
##if shader == wfx_render_foam_distorted || shader == wfx_render_height_distorted
      output.color.rgb *= saturate(1.0 - abs(particle.tAlpha * 2.0 - 1.0));
##endif
      return output;
    }
  }

  compile("target_ps", "wfx_ps");
  compile("target_vs", "wfx_vs");
}

shader wfx_normals
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  blend_src = one;
  blend_dst = one;
  blend_asrc = one;
  blend_adst = one;

  USE_AND_INIT_VIEW_VEC_PS()

  (ps) {
    wfx_hmap@smp2d = wfx_hmap;
    world_view_pos@f4 = world_view_pos;
    wlevel_psize@f3 = (water_level, wfx_normals_pixel_size.x, wfx_normals_pixel_size.y, 0);
  }

  POSTFX_VS_TEXCOORD(1, texcoord)

  hlsl(ps) {
    float4 get_water_position(float2 texcoord, float2 offset, out float3 out_view)
    {
      texcoord = texcoord + offset * wlevel_psize.yz;
      out_view = normalize(lerp_view_vec(texcoord));
      float waterDist = -(world_view_pos.y - wlevel_psize.x) / out_view.y;
      float texCol = tex2Dlod(wfx_hmap, float4(texcoord, 0, 0)).r;
      return float4(out_view * waterDist + float3(0, texCol, 0.0), waterDist);
    }
    float4 wfx_normals_ps(VsOutput input): SV_Target
    {
      #define DIRECTIONS 4
      #define OFFSET_FROM_VIEW_VEC_THRESHOLD 0.125

      float3 viewVec = float3(0, -1, 0);
      float4 center = get_water_position(input.texcoord.xy, float2(0, 0), viewVec);
      float horOffset = 1.0 / max(-viewVec.y, OFFSET_FROM_VIEW_VEC_THRESHOLD);
      float2 offsets[DIRECTIONS] = { float2(horOffset, 0),  float2(0, 1), float2(-horOffset, 0), float2(0, -1) };

      //hyperbola having a value of f0 at the point 0 and f1 at the point x0(max distance to water)
      const float f0 = 10, f1 = 1, x0 = 10;
      const float b = f1 * x0 / (f0 - f1);
      const float a = f0 * b;
      //distortion factor need for smoothing normals when we near with water plane to avoid flickering
      int distortion = int(clamp(a / (abs(center.w) + b), f1, f0));

      float3 vectors[DIRECTIONS];
      uint i;
      UNROLL
      for (i = 0; i < DIRECTIONS; ++i)
          vectors[i] = get_water_position(input.texcoord.xy, offsets[i] * distortion, viewVec).xyz - center.xyz;
      float3 normal = 0;
      UNROLL
      for (i = 0; i < DIRECTIONS; ++i)
          normal += (cross(vectors[i], vectors[(i + 1) % DIRECTIONS]));

      normal = normalize(normal);
      return float4(normal.xz * 0.5, 0.0, 0.0);
    }
  }

  compile("target_ps", "wfx_normals_ps");
}
