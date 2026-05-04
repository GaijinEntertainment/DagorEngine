// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <netImguiServer/NetImguiServer_App.h>
#include <netImguiServer/NetImguiServer_RemoteClient.h>

#include <netImgui/source/NetImgui_Shared.h>

#include <windows.h>
#include <tchar.h>
#include <shlobj_core.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <imgui/imgui.h>

#include <gui/dag_imgui.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_d3dResource.h>
#include <3d/dag_texMgr.h>
#include <util/dag_string.h>
#include <image/dag_texPixel.h>


namespace NetImguiServer::App
{

//=================================================================================================
// HAL STARTUP
// Additional initialisation that are platform specific
//=================================================================================================
bool HAL_Startup(const char *) { return true; }

//=================================================================================================
// HAL SHUTDOWN
// Prepare for shutdown of application, with platform specific code
//=================================================================================================
void HAL_Shutdown() {}

//=================================================================================================
// HAL GET SOCKET INFO
// Take a platform specific socket (based on the NetImguiNetworkXXX.cpp implementation) and
// fetch informations about the client IP connected
//=================================================================================================
bool HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo *pClientSocket, char *pOutHostname, size_t HostNameLen, int &outPort)
{
  sockaddr socketAdr;
  int sizeSocket(sizeof(sockaddr));
  SOCKET *pClientSocketWin = reinterpret_cast<SOCKET *>(pClientSocket);
  if (getsockname(*pClientSocketWin, &socketAdr, &sizeSocket) == 0)
  {
    char zPortBuffer[32];
    if (getnameinfo(&socketAdr, sizeSocket, pOutHostname, static_cast<DWORD>(HostNameLen), zPortBuffer, sizeof(zPortBuffer),
          NI_NUMERICSERV) == 0)
    {
      outPort = atoi(zPortBuffer);
      return true;
    }
  }
  return false;
}

//=================================================================================================
// HAL GET USER SETTING FOLDER
// Request the directory where to the 'shared config' clients should be saved
// Return 'nullptr' to disable this feature
//=================================================================================================
const char *HAL_GetUserSettingFolder()
{
  static char sUserSettingFolder[1024] = {};
  if (sUserSettingFolder[0] == 0)
  {
    WCHAR *UserPath;
    HRESULT Ret = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &UserPath);
    if (SUCCEEDED(Ret))
    {
      sprintf_s(sUserSettingFolder, "%ws\\NetImgui", UserPath); // convert from wchar to char
      DWORD ftyp = GetFileAttributesA(sUserSettingFolder);
      if (ftyp != INVALID_FILE_ATTRIBUTES && (ftyp & FILE_ATTRIBUTE_DIRECTORY))
      {
        return sUserSettingFolder;
      }
      else if (ftyp == INVALID_FILE_ATTRIBUTES && CreateDirectoryA(sUserSettingFolder, nullptr))
      {
        return sUserSettingFolder;
      }
    }
    sUserSettingFolder[0] = 0;
    return nullptr;
  }
  return sUserSettingFolder;
}

//=================================================================================================
// HAL GET CLIPBOARD UPDATED
// Detect when clipboard had a content change and we should refetch it on the Server and
// forward it to the Clients
//
// Note: We rely on Dear ImGui for Clipboard Get/Set but want to avoid constantly reading then
// converting it to a UTF8 text. If the Server platform doesn't support tracking change,
// return true. If the Server platform doesn't support any clipboard, return false;
//=================================================================================================
bool HAL_GetClipboardUpdated()
{
  static DWORD sClipboardSequence(0);
  DWORD clipboardSequence = GetClipboardSequenceNumber();
  if (sClipboardSequence != clipboardSequence)
  {
    sClipboardSequence = clipboardSequence;
    return true;
  }
  return false;
}

//=================================================================================================
// HAL RENDER DRAW DATA
//=================================================================================================
void HAL_RenderDrawData(ImDrawData *pDrawData)
{
  if (pDrawData)
    imgui_render_drawdata_to_texture(pDrawData, d3d::get_backbuffer_tex());
}

//=================================================================================================
// HAL RENDER DRAW DATA
// The drawing of remote clients is handled normally by the standard rendering backend,
// but output is redirected to an allocated client texture  instead default swapchain
//=================================================================================================
void HAL_RenderDrawData(RemoteClient::Client &client, ImDrawData *pDrawData)
{
  if (client.mpHAL_AreaRT)
  {
    {
      void *mainBackend = ImGui::GetIO().BackendRendererUserData;
      NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
      ImGui::GetIO().BackendRendererUserData = mainBackend; // Appropriate the existing renderer backend, for this client rendering
      imgui_render_drawdata_to_texture(ImGui::GetDrawData(), reinterpret_cast<BaseTexture *>(client.mpHAL_AreaRT));
      ImGui::GetIO().BackendRendererUserData = nullptr; // Restore it to null
    }
    if (pDrawData)
    {
      imgui_render_drawdata_to_texture(pDrawData, reinterpret_cast<BaseTexture *>(client.mpHAL_AreaRT));
    }
  }
}

//=================================================================================================
// HAL CREATE RENDER TARGET
// Allocate RenderTargetView / TextureView for each connected remote client.
// The drawing of their menu content will be outputed in it, then displayed normally
// inside our own 'NetImGui application' Imgui drawing
//=================================================================================================

bool HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void *&pOutRT, ImTextureData &OutTexture)
{
  HAL_DestroyRenderTarget(pOutRT, OutTexture);

  BaseTexture *createdTexture = d3d::create_tex(nullptr, Width, Height, TEXCF_RTARGET, 1, RESTAG_IMGUI);

  if (createdTexture)
  {
    pOutRT = reinterpret_cast<void *>(createdTexture);
    OutTexture.SetTexID(createdTexture);
    OutTexture.SetStatus(ImTextureStatus_OK);
    return true;
  }

  return false;
}

//=================================================================================================
// HAL DESTROY RENDER TARGET
// Free up allocated resources tried to a RenderTarget
//=================================================================================================
void HAL_DestroyRenderTarget(void *&pOutRT, ImTextureData &OutTexture)
{
  if (pOutRT)
  {
    destroy_d3dres(reinterpret_cast<BaseTexture *>(pOutRT));
    pOutRT = nullptr;
    OutTexture.SetTexID(nullptr);
    OutTexture.SetStatus(ImTextureStatus_Destroyed);
  }
}

} // namespace NetImguiServer::App