//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>

template <class T>
class FTab : public Tab<T>
{
public:
  FTab() : Tab<T>(framemem_ptr()) {}
};
