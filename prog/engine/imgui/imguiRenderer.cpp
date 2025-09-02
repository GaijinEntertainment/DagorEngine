// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define HAS_MULTIPLE_VIEWPORTS _TARGET_PC_WIN | _TARGET_PC_MACOSX

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

#if HAS_MULTIPLE_VIEWPORTS
#include <drv/3d/dag_platform_pc.h>
#endif

constexpr int NUM_VERTEX_CHANNELS = 3;
constexpr size_t VERTEX_BUF_ADDITIONAL_SIZE = 5000;
constexpr size_t INDEX_BUF_ADDITIONAL_SIZE = 10000;

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

#if HAS_MULTIPLE_VIEWPORTS
  if (d3d::pcwin::can_render_to_window())
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
#endif
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

void DagImGuiRenderer::render(ImGuiPlatformIO &platform_io)
{
  G_ASSERT(vDecl != BAD_VDECL);
  G_ASSERT(renderer.shader.get());
  G_ASSERT(fontTex.getTex2D());

  uint32_t totalVtx = 0, totalIdx = 0;
  for (auto viewport : platform_io.Viewports)
  {
    if ((viewport->Flags & ImGuiViewportFlags_IsMinimized) || !viewport->DrawData)
      continue;

    auto draw_data = viewport->DrawData;
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
      continue;

    totalVtx += draw_data->TotalVtxCount;
    totalIdx += draw_data->TotalIdxCount;
  }

  // Create and grow vertex/index buffers if needed
  resizeBuffers(totalVtx, totalIdx);


  // Upload vertex/index data into a single contiguous GPU buffer
  ImDrawVert *vbdata = nullptr;
  ImDrawIdx *ibdata = nullptr;
  lockBuffers(vbdata, totalVtx, ibdata, totalIdx);

  bool hasItemsToRender = false;

  for (auto viewport : platform_io.Viewports)
  {
    if ((viewport->Flags & ImGuiViewportFlags_IsMinimized) || !viewport->DrawData)
      continue;

    auto draw_data = viewport->DrawData;
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
      continue;

    hasItemsToRender |= copyDrawData(draw_data, vbdata, ibdata);
  }

  unlockBuffers();

  if (!hasItemsToRender)
    return;

  // Render draw datas to corressponding viewports
  d3d::setvdecl(vDecl);
  d3d::setvsrc(0, vb.getBuf(), vStride);
  d3d::setind(ib.getBuf());
  shaders::overrides::set(overrideStateId);

  auto mainViewport = platform_io.Viewports[0];

  int globalIdxOffset = 0;
  int globalVtxOffset = 0;
  for (auto viewport : platform_io.Viewports)
  {
    if ((viewport->Flags & ImGuiViewportFlags_IsMinimized) || !viewport->DrawData)
      continue;

    auto draw_data = viewport->DrawData;
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
      continue;

    if (viewport != mainViewport)
    {
#if HAS_MULTIPLE_VIEWPORTS
      if (auto rt = d3d::pcwin::get_swapchain_for_window(viewport->PlatformHandle))
        d3d::set_render_target(0, rt, 0);
      else
        continue;
#endif
    }

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
    if (viewport != mainViewport)
    {
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(120, 120, 120, 255), 0, 0);
    }

    processDrawDataToRT(draw_data, globalIdxOffset, globalVtxOffset);

    if (viewport != mainViewport)
    {
#if HAS_MULTIPLE_VIEWPORTS
      d3d::pcwin::present_to_window(viewport->PlatformHandle);
#endif
    }
  }

  shaders::overrides::reset();
}

void DagImGuiRenderer::renderDrawDataToTexture(const ImDrawData *draw_data, BaseTexture *rt)
{
  G_ASSERT(vDecl != BAD_VDECL);
  G_ASSERT(renderer.shader.get());
  G_ASSERT(fontTex.getTex2D());

  if (!draw_data || !rt)
    return;

  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  uint32_t totalVtx = draw_data->TotalVtxCount;
  uint32_t totalIdx = draw_data->TotalIdxCount;

  // Create and grow vertex/index buffers if needed
  resizeBuffers(totalVtx, totalIdx);

  // Upload vertex/index data into a single contiguous GPU buffer
  ImDrawVert *vbdata = nullptr;
  ImDrawIdx *ibdata = nullptr;

  lockBuffers(vbdata, totalVtx, ibdata, totalIdx);

  const bool hasItemsToRender = copyDrawData(draw_data, vbdata, ibdata);

  unlockBuffers();

  if (!hasItemsToRender)
    return;

  // Render passed draw data to passed texture
  d3d::setvdecl(vDecl);
  d3d::setvsrc(0, vb.getBuf(), vStride);
  d3d::setind(ib.getBuf());
  shaders::overrides::set(overrideStateId);

  d3d::set_render_target(0, rt, 0);

  int globalIdxOffset = 0;
  int globalVtxOffset = 0;

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
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(120, 120, 120, 255), 0, 0);

  processDrawDataToRT(draw_data, globalIdxOffset, globalVtxOffset);

  shaders::overrides::reset();
}

void DagImGuiRenderer::resizeBuffers(const uint32_t vtx_count, const uint32_t idx_count)
{
  if (!vb.getBuf() || vbSize < vtx_count)
  {
    vbSize = vtx_count + VERTEX_BUF_ADDITIONAL_SIZE;
    vb = dag::create_vb(sizeof(ImDrawVert) * vbSize, SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_vb");
    G_ASSERT(vb.getBuf());
  }
  if (!ib.getBuf() || ibSize < idx_count)
  {
    ibSize = idx_count + INDEX_BUF_ADDITIONAL_SIZE;
    ib = dag::create_ib(sizeof(ImDrawIdx) * ibSize, SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_ib");
    G_ASSERT(ib.getBuf());
  }
}

void DagImGuiRenderer::lockBuffers(ImDrawVert *&vbdata, const uint32_t vtx_count, ImDrawIdx *&ibdata, const uint32_t idx_count)
{
  bool vbLockSuccess = vb.getBuf()->lock(0, vtx_count * sizeof(ImDrawVert), (void **)&vbdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  G_ASSERT(vbLockSuccess && vbdata != nullptr);
  G_UNUSED(vbLockSuccess);
  bool ibLockSuccess = ib.getBuf()->lock(0, idx_count * sizeof(ImDrawIdx), (void **)&ibdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  G_ASSERT(ibLockSuccess && ibdata != nullptr);
  G_UNUSED(ibLockSuccess);
}

void DagImGuiRenderer::unlockBuffers()
{
  vb.getBuf()->unlock();
  ib.getBuf()->unlock();
}

bool DagImGuiRenderer::copyDrawData(const ImDrawData *draw_data, ImDrawVert *&vbdata, ImDrawIdx *&ibdata)
{
  bool res = false;
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList *cmdList = draw_data->CmdLists[n];
    memcpy(vbdata, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(ibdata, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    vbdata += cmdList->VtxBuffer.Size;
    ibdata += cmdList->IdxBuffer.Size;
    res |= cmdList->CmdBuffer.Size > 0;
  }
  return res;
}

void DagImGuiRenderer::processDrawDataToRT(const ImDrawData *draw_data, int &global_idx_offset, int &global_vtx_offset)
{
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
        scissorRect.left = max(scissorRect.left, 0);
        scissorRect.top = max(scissorRect.top, 0);
        scissorRect.right = min(scissorRect.right, (int)(draw_data->DisplaySize.x));
        scissorRect.bottom = min(scissorRect.bottom, (int)(draw_data->DisplaySize.y));

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

          d3d::drawind(PRIM_TRILIST, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3, pcmd->VtxOffset + global_vtx_offset);
        }
      }
    }
    global_idx_offset += cmdList->IdxBuffer.Size;
    global_vtx_offset += cmdList->VtxBuffer.Size;
  }
}
