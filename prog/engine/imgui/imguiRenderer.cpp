// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "imguiRenderer.h"

#include <imgui/imgui.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <shaders/dag_overrideStates.h>
#include <image/dag_texPixel.h>
#include <memory/dag_memBase.h>

constexpr int NUM_VERTEX_CHANNELS = 3;

static int imgui_mvp_VarIds[4];
static int imgui_texVarId = -1;

DagImGuiRenderer::DagImGuiRenderer()
{
  // Init input layout
  static CompiledShaderChannelId channels[NUM_VERTEX_CHANNELS] = {
    {SCTYPE_FLOAT2, SCUSAGE_POS, 0, 0},
    {SCTYPE_FLOAT2, SCUSAGE_TC, 0, 0},
    {SCTYPE_E3DCOLOR, SCUSAGE_VCOL, 0, 0},
  };
  vDecl = dynrender::addShaderVdecl(channels, NUM_VERTEX_CHANNELS);
  vStride = dynrender::getStride(channels, NUM_VERTEX_CHANNELS);
  G_ASSERT(vDecl != BAD_VDECL);

  // Init shader
  renderer.init("imgui", channels, NUM_VERTEX_CHANNELS, "imgui", true);

  // Init RenderState
  shaders::OverrideState os;
  os.set(shaders::OverrideState::SCISSOR_ENABLED);
  overrideStateId = shaders::overrides::create(os);

  // Init shadervars
  for (int i = 0; i < 4; i++)
    imgui_mvp_VarIds[i] = ::get_shader_variable_id(String(128, "imgui_mvp_%d", i), true);
  imgui_texVarId = ::get_shader_variable_id("imgui_tex", true);
  ShaderGlobal::set_sampler(get_shader_variable_id("imgui_tex_samplerstate", true), d3d::request_sampler({}));
}

void DagImGuiRenderer::setBackendFlags(ImGuiIO &io)
{
  io.BackendRendererName = "imgui_impl_dagor";

  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

void DagImGuiRenderer::createAndSetFontTexture(ImGuiIO &io)
{
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  TexImage32 *img = TexImage32::create(width, height, tmpmem);
  memcpy(img->getPixels(), pixels, width * height * 4);
  fontTex = dag::create_tex(img, width, height, TEXFMT_R8G8B8A8, 1, "imgui_font");
  memfree(img, tmpmem);
  io.Fonts->TexID = (ImTextureID)(uintptr_t) unsigned(fontTex.getTexId());
}

void DagImGuiRenderer::render(ImDrawData *draw_data)
{
  G_ASSERT(vDecl != BAD_VDECL);
  G_ASSERT(renderer.shader.get());
  G_ASSERT(fontTex.getTex2D());

  // Avoid rendering when minimized
  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  // Create and grow vertex/index buffers if needed
  if (!vb.getBuf() || vbSize < draw_data->TotalVtxCount)
  {
    vb = dag::create_vb(sizeof(ImDrawVert) * (draw_data->TotalVtxCount + 5000), SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_vb");
    G_ASSERT(vb.getBuf());
    vbSize = draw_data->TotalVtxCount + 5000;
  }
  if (!ib.getBuf() || ibSize < draw_data->TotalIdxCount)
  {
    ib = dag::create_ib(sizeof(ImDrawIdx) * (draw_data->TotalIdxCount + 10000), SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_ib");
    G_ASSERT(ib.getBuf());
    ibSize = draw_data->TotalIdxCount + 10000;
  }

  // Upload vertex/index data into a single contiguous GPU buffer
  ImDrawVert *vbdata = nullptr;
  ImDrawIdx *ibdata = nullptr;
  bool vbLockSuccess =
    vb.getBuf()->lock(0, draw_data->TotalVtxCount * sizeof(ImDrawVert), (void **)&vbdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  G_ASSERT(vbLockSuccess && vbdata != nullptr);
  G_UNUSED(vbLockSuccess);
  bool ibLockSuccess =
    ib.getBuf()->lock(0, draw_data->TotalIdxCount * sizeof(ImDrawIdx), (void **)&ibdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  G_ASSERT(ibLockSuccess && ibdata != nullptr);
  G_UNUSED(ibLockSuccess);

  bool hasItemsToRender = false;

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList *cmdList = draw_data->CmdLists[n];
    memcpy(vbdata, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(ibdata, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    vbdata += cmdList->VtxBuffer.Size;
    ibdata += cmdList->IdxBuffer.Size;
    hasItemsToRender |= cmdList->CmdBuffer.Size > 0;
  }
  vb.getBuf()->unlock();
  ib.getBuf()->unlock();

  if (!hasItemsToRender)
    return;

  const float L = draw_data->DisplayPos.x;
  const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  const float T = draw_data->DisplayPos.y;
  const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  const float mvp[4][4] = {
    {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
    {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
    {0.0f, 0.0f, 0.5f, 0.0f},
    {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
  };
  for (int i = 0; i < 4; i++)
    ShaderGlobal::set_color4(imgui_mvp_VarIds[i], mvp[i][0], mvp[i][1], mvp[i][2], mvp[i][3]);

  d3d::setview(0, 0, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, 1.0f);
  d3d::setvdecl(vDecl);
  d3d::setvsrc(0, vb.getBuf(), vStride);
  d3d::setind(ib.getBuf());
  shaders::overrides::set(overrideStateId);

  int globalIdxOffset = 0;
  int globalVtxOffset = 0;
  ImVec2 clipOff = draw_data->DisplayPos;
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList *cmdList = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd *pcmd = &cmdList->CmdBuffer[cmd_i];
      if (pcmd->UserCallback != nullptr)
      {
        // User callback, registered via ImDrawList::AddCallback()
        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render
        // state.)
        if (pcmd->UserCallback != ImDrawCallback_ResetRenderState)
          pcmd->UserCallback(cmdList, pcmd);
      }
      else
      {
        RectInt scissorRect;
        scissorRect.left = pcmd->ClipRect.x - clipOff.x;
        scissorRect.top = pcmd->ClipRect.y - clipOff.y;
        scissorRect.right = pcmd->ClipRect.z - clipOff.x;
        scissorRect.bottom = pcmd->ClipRect.w - clipOff.y;
        const int w = scissorRect.right - scissorRect.left;
        const int h = scissorRect.bottom - scissorRect.top;
        if (w > 0 && h > 0)
        {
          d3d::setscissor(scissorRect.left, scissorRect.top, w, h);

          TEXTUREID tid = (TEXTUREID)(intptr_t)pcmd->TextureId;
          if (get_managed_texture_refcount(tid) < 1)
            tid = BAD_TEXTUREID;
          ShaderGlobal::set_texture(imgui_texVarId, tid);
          renderer.shader->setStates();

          d3d::drawind(PRIM_TRILIST, pcmd->IdxOffset + globalIdxOffset, pcmd->ElemCount / 3, pcmd->VtxOffset + globalVtxOffset);
        }
      }
    }
    globalIdxOffset += cmdList->IdxBuffer.Size;
    globalVtxOffset += cmdList->VtxBuffer.Size;
  }

  shaders::overrides::reset();
}