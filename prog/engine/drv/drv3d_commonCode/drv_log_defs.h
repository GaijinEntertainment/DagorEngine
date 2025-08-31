// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define D3D_ERROR(...) logmessage(_MAKE4C('D3DE'), __VA_ARGS__)

#define D3D_CONTRACT_ERROR(fmt, ...) logmessage(_MAKE4C('D3DE'), "Incorrect D3D API usage\n" fmt, ##__VA_ARGS__)
