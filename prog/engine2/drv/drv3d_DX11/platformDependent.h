#pragma once

namespace drv3d_dx11
{
class GenericBuffer;
class BaseTex;
} // namespace drv3d_dx11

namespace d3d
{
extern void esram_init();
extern void esram_close();
extern void resummarize_htile(BaseTexture *tex);
extern void release_texture_esram(BaseTex::D3DTextures &tex);
extern void free_placement_memory(BaseTex::D3DTextures &tex);
extern void collect_garbage();
HRESULT DagorCreateTexture2D(D3D11_TEXTURE2D_DESC &desc, D3D11_SUBRESOURCE_DATA *initial_data, drv3d_dx11::BaseTex &bt,
  BaseTex::D3DTextures &tex);
HRESULT DagorCreateTexture3D(D3D11_TEXTURE3D_DESC &desc, D3D11_SUBRESOURCE_DATA *initial_data, drv3d_dx11::BaseTex &bt,
  BaseTex::D3DTextures &tex);
HRESULT DagorCreateBuffer(D3D11_BUFFER_DESC &desc, ID3D11Buffer *&outBuf, drv3d_dx11::GenericBuffer &buf);
} // namespace d3d
