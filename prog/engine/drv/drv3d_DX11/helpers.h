// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct IDXGIAdapter;
struct ID3D11Device;

IDXGIAdapter *get_active_adapter(ID3D11Device *dx_device);
