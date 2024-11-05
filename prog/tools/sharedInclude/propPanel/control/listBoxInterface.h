//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

class IMenu;

// NOTE: ImGui porting: this and its usage is totally new compared to the original.
class IListBoxInterface
{
public:
  virtual ~IListBoxInterface() {}

  virtual IMenu &createContextMenu() = 0;
};

} // namespace PropPanel