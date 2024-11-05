// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "helpers.h"
#include "driver.h"

IDXGIAdapter *get_active_adapter(ID3D11Device *dx_device)
{
  IDXGIDevice *dxgiDevice = nullptr;
  IDXGIAdapter *dxgiAdapter = nullptr;
  HRESULT hr = dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
  if (SUCCEEDED(hr))
  {
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&dxgiAdapter);
    if (FAILED(hr))
      dxgiAdapter = nullptr;
    dxgiDevice->Release();
  }
  return dxgiAdapter;
}
