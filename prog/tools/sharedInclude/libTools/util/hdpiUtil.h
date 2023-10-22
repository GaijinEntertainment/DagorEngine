//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_progGlobals.h>

namespace hdpi
{
enum class Px : int // special strong type to separate legacy (96dpi-based) coords and actual coords in pixels
{
  ZERO = 0
};

//! returns Px scaled from legacy 96dpi-based coord to current dpi
inline Px _pxScaled(int units_96dpi) { return (Px)(units_96dpi * win32_system_dpi / 96); }
//! returns Px that is reinterpreted from actual pixels coord
inline Px _pxActual(int px) { return (Px)px; }
//! explicit cast to to actual pixels from Px
inline int _px(Px px) { return int(px); }
//! helper that scales legacy 96dpi-based coord into actual pixels (use with care since in converts int->int without type checking)
inline int _pxS(int units_96dpi) { return _px(_pxScaled(units_96dpi)); }

inline Px operator+(Px a, Px b) { return _pxActual(_px(a) + _px(b)); }
inline Px operator-(Px a, Px b) { return _pxActual(_px(a) - _px(b)); }
inline Px operator*(Px a, int b) { return _pxActual(_px(a) * b); }
inline Px operator/(Px a, int b) { return _pxActual(_px(a) / b); }
inline Px &operator+=(Px &a, Px b) { return a = a + b; }
inline Px &operator-=(Px &a, Px b) { return a = a - b; }
} // namespace hdpi
