#include <3d/dag_drv3d.h>
#include "driver.h"
#include "texture.h"

namespace d3d
{
HRESULT DagorCreateTexture2D(D3D11_TEXTURE2D_DESC &desc, D3D11_SUBRESOURCE_DATA *initial_data, BaseTex &bt, BaseTex::D3DTextures &tex)
{
  // unsafe (no crtitical section) by contract
  return dx_device->CreateTexture2D(&desc, initial_data, &tex.tex2D);
}
HRESULT DagorCreateTexture3D(D3D11_TEXTURE3D_DESC &desc, D3D11_SUBRESOURCE_DATA *initial_data, BaseTex &bt, BaseTex::D3DTextures &tex)
{
  // unsafe (no crtitical section) by contract
  return dx_device->CreateTexture3D(&desc, initial_data, &tex.tex3D);
}
void release_texture_esram(BaseTex::D3DTextures &) {}
void free_placement_memory(BaseTex::D3DTextures &) {}
void collect_garbage() {}
HRESULT DagorCreateBuffer(D3D11_BUFFER_DESC &desc, ID3D11Buffer *&outBuf, drv3d_dx11::GenericBuffer &buf)
{
  // unsafe by contract
  if (buf.bufFlags & SBCF_ZEROMEM)
  {
    D3D11_SUBRESOURCE_DATA data;
    eastl::vector<unsigned char> zeroes(buf.bufSize, 0);
    data.pSysMem = zeroes.data();
    data.SysMemPitch = data.SysMemSlicePitch = 0;
    return dx_device->CreateBuffer(&desc, &data, &outBuf);
  }
  else
    return dx_device->CreateBuffer(&desc, NULL, &outBuf);
}
void esram_close() {}
void esram_init() {}
}; // namespace d3d