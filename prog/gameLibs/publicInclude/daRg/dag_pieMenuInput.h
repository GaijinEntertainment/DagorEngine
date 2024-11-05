//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_math2d.h>
namespace Sqrat
{
class Table;
}

// This probably will be useful for other tasks besides PieMenu behavior
// But it will be renamed to correspond to actual design as soon as it changes

namespace darg
{

class PieMenuInputAdapter
{
public:
  virtual Point2 getAxisValue(const Sqrat::Table &comp_desc) = 0;
};


void set_pie_menu_input_adapter(PieMenuInputAdapter *adapter);

}; // namespace darg
