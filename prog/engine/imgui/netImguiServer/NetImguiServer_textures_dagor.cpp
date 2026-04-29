// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <netImguiServer/NetImguiServer_App.h>
#include <netImguiServer/NetImguiServer_RemoteClient.h>

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_texture.h>
#include <3d/dag_lockTexture.h>

#include <zstd.h>


namespace NetImguiServer
{
namespace App
{

//=================================================================================================
// This contains a sample of letting User create their own texture format, handling it on both
// the NetImgui Client and the NetImgui Server side
//
// User are entirely free to manage custom texture content as they wish as long as it generates
// a valid 'ServerTexture.mpHAL_Texture' GPU texture object for the Imgui Backend
//
// Sample was kept simple by reusing HAL_CreateTexture / HAL_DestroyTexture when updating
// the texture, but User is free to create valid texture without these functions
//
//-------------------------------------------------------------------------------------------------
// Return true if the command was handled
//=================================================================================================
bool CreateTexture_Custom(ServerTexture &serverTexture, const NetImgui::Internal::CmdTexture &cmdTexture, uint32_t customDataSize)
{
  auto eTexFmt = static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
  if (eTexFmt == NetImgui::eTexFormat::kTexFmtCustom)
  {
    constexpr auto FORMAT_SIZE = sizeof(TEXFMT_MASK);

    const uint8_t *compressedData = (uint8_t *)cmdTexture.mpTextureData.Get() + FORMAT_SIZE;
    const uint64_t compressedDataSize = customDataSize - FORMAT_SIZE;
    const uint64_t originalDataSize = ZSTD_getFrameContentSize(compressedData, compressedDataSize);
    if (originalDataSize == ZSTD_CONTENTSIZE_ERROR || originalDataSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
      logerr("zstd : impossible to unpack compressed image (size error)");
      return false;
    }

    const uint16_t width = cmdTexture.mWidth;
    const uint16_t height = cmdTexture.mHeight;
    const uint32_t textureFormat = *(uint32_t *)cmdTexture.mpTextureData.Get();

    BaseTexture *createdTexture = d3d::create_tex(nullptr, width, height, textureFormat, 1, RESTAG_IMGUI);
    if (createdTexture)
    {
      if (auto lockedTex = lock_texture<ImageRawBytes>(createdTexture, 0, TEXLOCK_WRITE))
      {
        const uint64_t createdDataSize = lockedTex.getByteStride() * lockedTex.getHeightInElems();
        if (createdDataSize != originalDataSize)
        {
          logerr("Impossible to create texture : created texture data size (%d) differs from original texture data size (%d)",
            createdDataSize, originalDataSize);
        }
        else
        {
          const uint64_t decompressedDataSize = ZSTD_decompress(lockedTex.get(), originalDataSize, compressedData, compressedDataSize);
          if (ZSTD_isError(decompressedDataSize))
          {
            logerr("zstd : error while decompressing data");
          }
          else
          {
            destroy_d3dres(reinterpret_cast<BaseTexture *>(serverTexture.mTexData.GetTexID()));
            serverTexture.mTexData.SetTexID(createdTexture);
            serverTexture.mTexData.SetStatus(ImTextureStatus_OK);
            return true;
          }
        }
      }

      destroy_d3dres(createdTexture);
    }
  }

  return false;
}

//=================================================================================================
// Return true if the command was handled
bool DestroyTexture_Custom(ServerTexture &serverTexture, const NetImgui::Internal::CmdTexture &cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
  IM_UNUSED(customDataSize);

  if (cmdTexture.mFormat == NetImgui::eTexFormat::kTexFmtCustom)
  {
    destroy_d3dres(reinterpret_cast<BaseTexture *>(serverTexture.mTexData.GetTexID()));
    serverTexture.mTexData.SetTexID(nullptr);
    serverTexture.mTexData.SetStatus(ImTextureStatus_Destroyed);
    return true;
  }

  return false;
}

} // namespace App
} // namespace NetImguiServer
