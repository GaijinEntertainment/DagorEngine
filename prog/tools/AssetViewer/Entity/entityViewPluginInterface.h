// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class IGenViewportWnd;
class IObjEntity;

class IEntityViewPluginInterface
{
public:
  virtual bool isMouseOverSelectedCompositeSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity *main_entity) = 0;
};
