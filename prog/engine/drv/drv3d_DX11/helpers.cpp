// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "helpers.h"
#include "driver.h"

IDXGIAdapter1 *get_active_adapter(ID3D11Device *dx_device)
{
  G_ASSERT_RETURN(dx_device, nullptr);
  IDXGIDevice *dxgiDevice = nullptr;
  IDXGIAdapter1 *dxgiAdapter = nullptr;
  HRESULT hr = dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
  if (SUCCEEDED(hr))
  {
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&dxgiAdapter);
    if (FAILED(hr))
      dxgiAdapter = nullptr;
    dxgiDevice->Release();
  }
  return dxgiAdapter;
}
