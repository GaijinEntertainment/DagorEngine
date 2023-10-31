#pragma once

#include <generic/dag_ptrTab.h>
#include <EditorCore/ec_rendEdObject.h>

class PropertyContainerControlBase;

class ObjectParam
{
public:
  void fillParams(PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> selection);

  bool onPPChange(PropPanel2 &panel, int pid, dag::ConstSpan<RenderableEditableObject *> selection);
};
