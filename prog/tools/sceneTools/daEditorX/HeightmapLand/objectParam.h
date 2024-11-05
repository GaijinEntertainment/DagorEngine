// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_ptrTab.h>
#include <EditorCore/ec_rendEdObject.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class ObjectParam
{
public:
  void fillParams(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> selection);

  bool onPPChange(PropPanel::ContainerPropertyControl &panel, int pid, dag::ConstSpan<RenderableEditableObject *> selection);
};
