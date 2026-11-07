// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omm.h>
#include <omm.h>

namespace render::omm
{

bool init_cpu(CpuContext &ctx)
{
  shutdown_cpu(ctx);

  ommBakerCreationDesc bakerDesc = ommBakerCreationDescDefault();
  bakerDesc.type = ommBakerType_CPU;
  bakerDesc.messageInterface.messageCallback = [](ommMessageSeverity severity, const char *message, void *userArg) {
    switch (severity)
    {
      case ommMessageSeverity_Info: logdbg("omm: info: %s", message); break;
      case ommMessageSeverity_PerfWarning: logdbg("omm: perf: %s", message); break;
      case ommMessageSeverity_Error: logdbg("omm: error: %s", message); break;
      case ommMessageSeverity_Fatal: logdbg("omm: fatal: %s", message); break;
    }
  };
  ommBaker baker = nullptr;
  if (ommCreateBaker(&bakerDesc, &baker) != ommResult_SUCCESS)
    return false;
  ctx.baker = baker;
  return true;
}

void shutdown_cpu(CpuContext &ctx)
{
  ommBaker baker = static_cast<ommBaker>(ctx.baker);
  if (ctx.texture && baker)
    ommCpuDestroyTexture(baker, static_cast<ommCpuTexture>(ctx.texture));
  if (baker)
    ommDestroyBaker(baker);
  ctx = {};
}

static ommCpuTexture create_sdk_texture(CpuContext &ctx, const char *key, uint32_t w, uint32_t h, const uint8_t *alpha)
{
  ommBaker baker = static_cast<ommBaker>(ctx.baker);
  if (ctx.texture && ctx.textureKey == key)
    return static_cast<ommCpuTexture>(ctx.texture);

  ommCpuTextureMipDesc mip = {};
  mip.width = w;
  mip.height = h;
  mip.rowPitch = w;
  mip.textureData = alpha;

  ommCpuTextureDesc desc = ommCpuTextureDescDefault();
  desc.format = ommCpuTextureFormat_UNORM8;
  desc.mips = &mip;
  desc.mipCount = 1;

  ommCpuTexture texture = nullptr;
  if (ommCpuCreateTexture(baker, &desc, &texture) != ommResult_SUCCESS)
    return nullptr;
  if (ctx.texture)
    ommCpuDestroyTexture(baker, static_cast<ommCpuTexture>(ctx.texture));
  ctx.textureKey = key;
  ctx.texture = texture;
  return texture;
}

static OpacityState to_dagor_opacity_state(int32_t omm_index)
{
  switch (omm_index)
  {
    case ommSpecialIndex_FullyTransparent: return OpacityState::Transparent;
    case ommSpecialIndex_FullyOpaque: return OpacityState::Opaque;
    case ommSpecialIndex_FullyUnknownTransparent: return OpacityState::UnknownTransparent;
    case ommSpecialIndex_FullyUnknownOpaque: return OpacityState::UnknownOpaque;
  }
  return OpacityState::UnknownOpaque;
}

bool cpu_bake_alpha_stats(CpuContext &ctx, const char *tex_key, uint32_t w, uint32_t h, const uint8_t *alpha,
  const float *texcoords_uv, const uint32_t *indices, uint32_t index_count, CpuBakeStats &stats,
  dag::Span<OpacityState> out_triangle_states)
{
  stats = {};
  if (!ctx.baker || !tex_key || !w || !h || !alpha || !texcoords_uv || !indices || index_count < 3 || index_count % 3)
    return false;

  ommCpuTexture texture = create_sdk_texture(ctx, tex_key, w, h, alpha);
  if (!texture)
    return false;

  BakeInput input;
  input.globalFormat = Format::OC1_2_State;

  ommCpuBakeInputDesc sdk_config = ommCpuBakeInputDescDefault();
  sdk_config.texture = texture;
  sdk_config.alphaMode = ommAlphaMode_Test;
  sdk_config.runtimeSamplerDesc.addressingMode = ommTextureAddressMode_Wrap;
  sdk_config.runtimeSamplerDesc.filter = ommTextureFilterMode_Linear;
  sdk_config.texCoordFormat = ommTexCoordFormat_UV32_FLOAT;
  sdk_config.texCoords = texcoords_uv;
  sdk_config.indexFormat = ommIndexFormat_UINT_32;
  sdk_config.indexBuffer = indices;
  sdk_config.indexCount = index_count;
  sdk_config.alphaCutoff = input.alphaCutoff;
  sdk_config.alphaCutoffLessEqual = static_cast<ommOpacityState>(input.alphaCutoffLessEqual);
  sdk_config.alphaCutoffGreater = static_cast<ommOpacityState>(input.alphaCutoffGreater);
  sdk_config.dynamicSubdivisionScale = input.dynamicSubdivisionScale;
  sdk_config.format = static_cast<ommFormat>(input.globalFormat);
  sdk_config.maxSubdivisionLevel = input.maxSubdivisionLevel;
  sdk_config.maxWorkloadSize = uint64_t(1) << 28;

  ommCpuBakeResult result = nullptr;
  if (ommCpuBake(static_cast<ommBaker>(ctx.baker), &sdk_config, &result) != ommResult_SUCCESS)
    return false;

  const ommCpuBakeResultDesc *desc = nullptr;
  const bool gotDesc = ommCpuGetBakeResultDesc(result, &desc) == ommResult_SUCCESS && desc;
  if (gotDesc)
  {
    stats.triangles = index_count / 3;
    stats.descCount = desc->descArrayCount;
    for (uint32_t i = 0; i < desc->indexCount; i++)
    {
      int32_t idx = desc->indexFormat == ommIndexFormat_UINT_16 ? int32_t(((const int16_t *)desc->indexBuffer)[i])
                                                                : ((const int32_t *)desc->indexBuffer)[i];
      switch (idx)
      {
        case ommSpecialIndex_FullyOpaque: stats.totalFullyOpaqueCount++; break;
        case ommSpecialIndex_FullyTransparent: stats.totalFullyTransparentCount++; break;
        case ommSpecialIndex_FullyUnknownOpaque:
        case ommSpecialIndex_FullyUnknownTransparent: stats.totalFullyUnknownCount++; break;
        default: break;
      }
      if (i < out_triangle_states.size())
        out_triangle_states[i] = to_dagor_opacity_state(idx);
    }
  }

  ommCpuDestroyBakeResult(result);
  return gotDesc;
}

} // namespace render::omm
