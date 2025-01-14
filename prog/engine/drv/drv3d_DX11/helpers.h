// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct IDXGIAdapter1;
struct ID3D11Device;

IDXGIAdapter1 *get_active_adapter(ID3D11Device *dx_device);
