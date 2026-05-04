// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define HAS_MULTIPLE_VIEWPORTS _TARGET_PC_WIN | _TARGET_PC_MACOSX

#include "imguiRenderer.h"

#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <3d/dag_render.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderVar.h>
#include <image/dag_texPixel.h>
#include <memory/dag_memBase.h>

#include <EASTL/fixed_string.h>

#if HAS_MULTIPLE_VIEWPORTS
#include <drv/3d/dag_platform_pc.h>
#endif

constexpr int NUM_VERTEX_CHANNELS = 3;
constexpr size_t VERTEX_BUF_ADDITIONAL_SIZE = 5000;
constexpr size_t INDEX_BUF_ADDITIONAL_SIZE = 10000;

static int imgui_mvp_VarIds[4];
static int imgui_texVarId = -1;
static int imgui_texture_name_unique_id = 0;

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
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

#if HAS_MULTIPLE_VIEWPORTS
  if (d3d::pcwin::can_render_to_window())
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
#endif
}

void DagImGuiRenderer::render(ImGuiPlatformIO &platform_io)
{
  G_ASSERT(vDecl != BAD_VDECL);
  G_ASSERT(renderer.shader.get());

  uint32_t totalVtx = 0, totalIdx = 0;
  for (auto viewport : platform_io.Viewports)
  {
    if ((viewport->Flags & ImGuiViewportFlags_IsMinimized) || !viewport->DrawData)
      continue;

    auto draw_data = viewport->DrawData;
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
      continue;

    if (draw_data->Textures)
      for (ImTextureData *td : *draw_data->Textures)
        if (td->Status != ImTextureStatus_OK)
          updateTextureData(*td);

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
        d3d::set_render_target({}, DepthAccess::RW, {{rt, 0, 0}});
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
      ShaderGlobal::set_float4(imgui_mvp_VarIds[i], mvp[i][0], mvp[i][1], mvp[i][2], mvp[i][3]);

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

  if (!draw_data || !rt)
    return;

  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  if (draw_data->Textures)
    for (ImTextureData *td : *draw_data->Textures)
      if (td->Status != ImTextureStatus_OK)
        updateTextureData(*td);

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

  d3d::set_render_target({}, DepthAccess::RW, {{rt, 0, 0}});

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
    ShaderGlobal::set_float4(imgui_mvp_VarIds[i], mvp[i][0], mvp[i][1], mvp[i][2], mvp[i][3]);

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
    vb = dag::create_vb(sizeof(ImDrawVert) * vbSize, SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_vb", RESTAG_IMGUI);
    G_ASSERT(vb.getBuf());
  }
  if (!ib.getBuf() || ibSize < idx_count)
  {
    ibSize = idx_count + INDEX_BUF_ADDITIONAL_SIZE;
    ib = dag::create_ib(sizeof(ImDrawIdx) * ibSize, SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, "imgui_ib", RESTAG_IMGUI);
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
  if (vbdata == nullptr || ibdata == nullptr)
    return false;
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
      else if (pcmd->ElemCount > 0)
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

          BaseTexture *tPtr = reinterpret_cast<BaseTexture *>(pcmd->GetTexID());

          ShaderGlobal::set_texture(imgui_texVarId, tPtr);

          renderer.shader->setStates();

          d3d::drawind(PRIM_TRILIST, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3, pcmd->VtxOffset + global_vtx_offset);

          ShaderGlobal::set_texture(imgui_texVarId, nullptr);
        }
      }
    }
    global_idx_offset += cmdList->IdxBuffer.Size;
    global_vtx_offset += cmdList->VtxBuffer.Size;
  }
}

void DagImGuiRenderer::afterDeviceReset() { releaseAllTextureData(); }

void DagImGuiRenderer::updateTextureData(ImTextureData &td)
{
  if (td.Status == ImTextureStatus_WantCreate)
  {
    G_ASSERT(td.TexID == ImTextureID_Invalid);
    G_ASSERT(td.BackendUserData == nullptr);
    G_ASSERT(td.Format == ImTextureFormat_RGBA32);

    ++imgui_texture_name_unique_id;
    using TmpName = eastl::fixed_string<char, 32>;
    const TmpName name(TmpName::CtorSprintf{}, "imgui_tex_%d", imgui_texture_name_unique_id);

    // Create and upload new texture to graphics system
    TexImage32 *img = TexImage32::create(td.Width, td.Height, tmpmem);
    memcpy(img->getPixels(), td.GetPixels(), td.Width * td.Height * 4);
    Texture *texture = d3d::create_tex(img, td.Width, td.Height, TEXFMT_R8G8B8A8, 1, name.c_str(), RESTAG_IMGUI);
    memfree(img, tmpmem);

    if (texture)
    {
      const TEXTUREID textureId = register_managed_tex(name.c_str(), texture);

      // Store identifiers
      td.SetTexID(ImGuiDagor::EncodeTexturePtr(texture));
      td.SetStatus(ImTextureStatus_OK);
      td.BackendUserData = (void *)(uintptr_t)(((unsigned)textureId));
    }
    else
    {
      logerr("Failed to create ImGui texture \"%s\" (%dx%d).", name.c_str(), td.Width, td.Height);
    }
  }
  else if (td.Status == ImTextureStatus_WantUpdates)
  {
    if (Texture *texture = (Texture *)td.GetTexID())
    {
      const ImTextureRect &ur = td.UpdateRect;
      if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex_region(texture, 0, 0, ur.x, ur.y, 0, ur.w, ur.h, 1))
      {
        const uint8_t *src = (const uint8_t *)td.GetPixelsAt(ur.x, ur.y);
        const int srcPitch = td.GetPitch();
        uint8_t *dst = (uint8_t *)d3d::get_update_buffer_addr_for_write(rub);
        const size_t dstPitch = d3d::get_update_buffer_pitch(rub);
        const int bytesPerLineToCopy = ur.w * td.BytesPerPixel;
        for (int y = 0; y < ur.h; ++y, src += srcPitch, dst += dstPitch)
          memcpy(dst, src, bytesPerLineToCopy);

        d3d::update_texture_and_release_update_buffer(rub);
        td.SetStatus(ImTextureStatus_OK);
      }
    }
  }
  else if (td.Status == ImTextureStatus_WantDestroy && td.UnusedFrames > 0)
  {
    releaseTextureData(td);
  }
}

void DagImGuiRenderer::releaseTextureData(ImTextureData &td)
{
  if (!td.BackendUserData)
    return;

  Texture *texture = (Texture *)td.GetTexID();
  TEXTUREID textureId = (TEXTUREID)((uintptr_t)td.BackendUserData);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(textureId, texture);

  // Clear identifiers and mark as destroyed.
  td.SetTexID(ImTextureID_Invalid);
  td.SetStatus(ImTextureStatus_Destroyed);
  td.BackendUserData = nullptr;
}

void DagImGuiRenderer::releaseAllTextureData()
{
  if (!ImGui::GetCurrentContext())
    return;

  for (ImTextureData *td : ImGui::GetPlatformIO().Textures)
    if (td->RefCount == 1)
      releaseTextureData(*td);
}
