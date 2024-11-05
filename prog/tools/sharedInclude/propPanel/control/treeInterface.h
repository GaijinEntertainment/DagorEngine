//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

class ContainerPropertyControl;
class IMenu;
class ITreeInterface;

class ITreeControlEventHandler
{
public:
  virtual bool onTreeContextMenu(ContainerPropertyControl &tree, int pcb_id, ITreeInterface &tree_interface) = 0;
};

// NOTE: ImGui porting: this and its usage is totally new compared to the original.
class ITreeInterface
{
public:
  virtual ~ITreeInterface() {}

  virtual IMenu &createContextMenu() = 0;
};

} // namespace PropPanel