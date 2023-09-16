//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes: DirectInput
#ifdef UNICODE
struct IDirectInputW;
struct IDirectInput7W;
struct IDirectInput8W;
struct IDirectInputDevice7W;
struct IDirectInputDevice8W;
#define IDirectInput        IDirectInputW
#define IDirectInput7       IDirectInput7W
#define IDirectInput8       IDirectInput8W
#define IDirectInputDevice7 IDirectInputDevice7W
#define IDirectInputDevice8 IDirectInputDevice8W
#else
struct IDirectInputA;
struct IDirectInput7A;
struct IDirectInput8A;
struct IDirectInputDevice7A;
struct IDirectInputDevice8A;
#define IDirectInput        IDirectInputA
#define IDirectInput7       IDirectInput7A
#define IDirectInput8       IDirectInput8A
#define IDirectInputDevice7 IDirectInputDevice7A
#define IDirectInputDevice8 IDirectInputDevice8A
#endif
struct IDirectInputEffect;
