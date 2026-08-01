// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/material.h>
#include <daFracture/core/destrMesh.h>

namespace frx
{

DestrContextRenderMaterialsHolder::DestrContextRenderMaterialsHolder() = default;
DestrContextRenderMaterialsHolder::~DestrContextRenderMaterialsHolder() = default;


ShaderMatChannels ShaderMatChannels::create(ShaderMaterial *sh)
{
  struct GatherChanCB : ShaderMatChannels, ShaderChannelsEnumCB
  {
    void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int) override
    {
      G_UNUSED(u);
      G_UNUSED(ui);
      if (vbu == SCUSAGE_POS && vbui == 0)
        posChIdx = channels.size();
      if (vbu == SCUSAGE_TC && vbui == 0)
        tcChIdx = channels.size();
      if (vbu == SCUSAGE_NORM && vbui == 0)
        normChIdx = channels.size();
      ChannelDesc &d = channels.push_back();
      d.offset = vStride;
      d.type = t;
      d.usage = vbu;
      d.usageIdx = vbui;
      d.mod = mod;
      channel_size(t, d.size);
      vStride += d.size;
    }
  } ccb;
  int flags = 0;
  if (!sh->enum_channels(ccb, flags))
    G_ASSERT(0 && "can't enum material channels");
  return ShaderMatChannels(eastl::move(ccb));
}

uint16_t add_render_material(DestrContext &ctx, ShaderMaterial *mat, RenderMatElem &&elem, bool allow_reuse)
{
  int materialId = -1;
  G_ASSERT(ctx.renderMats.size() == ctx.materials.size());
  if (allow_reuse)
    for (int i = 0; i < ctx.materials.size(); i++)
      if (ctx.renderMats[i].shMat == mat && ctx.renderMats[i].elem.stage == elem.stage &&
          ctx.renderMats[i].elem.shElem == elem.shElem && ctx.renderMats[i].elem.instData == elem.instData)
      {
        materialId = i;
        break;
      }
  if (materialId == -1)
  {
    materialId = ctx.materials.size();
    auto &rMat = ctx.renderMats.push_back();
    rMat.shMat = mat;
    rMat.elem = eastl::move(elem);
    rMat.channels = ShaderMatChannels::create(mat);
    G_ASSERT(rMat.channels.posChIdx >= 0);

    const int totalChannels = int(rMat.channels.channels.size());
    const int preservedChannels =
      int(rMat.channels.posChIdx >= 0) + int(rMat.channels.normChIdx >= 0) + int(rMat.channels.tcChIdx >= 0);
    if (totalChannels > preservedChannels)
      logerr("daFracture: material has %d vertex channels, but only pos/norm/tc (%d) are preserved; the rest will be uninitialized",
        totalChannels, preservedChannels);

    auto &dMat = ctx.materials.push_back();
    dMat.isSolid = ShaderMesh::Stage(rMat.elem.stage) <= ShaderMesh::STG_atest;
  }
  return uint16_t(materialId);
}

} // namespace frx
