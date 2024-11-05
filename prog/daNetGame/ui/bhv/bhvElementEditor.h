// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <daInput/input_api.h>
#include "ui/scriptStrings.h"

struct ElementEditorState;

class BhvElementEditor : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    elementEditorState,
    elementMaxScale,
    elementMinScale,
    elementScaleRatio);

  BhvElementEditor();

  virtual void onAttach(darg::Element *) override;
  virtual void onDetach(darg::Element *, DetachMode) override;

  virtual int touchEvent(darg::ElementTree *,
    darg::Element *,
    darg::InputEvent /*event*/,
    HumanInput::IGenPointing * /*pnt*/,
    int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/,
    int /*accum_res*/) override;

private:
  void touchBegin(ElementEditorState *state, darg::Element *elem, int idx, int flags, Point2 pos);
  void touchMove(ElementEditorState *state, int idx, Point2 pos);
  void touchEnd(ElementEditorState *state, darg::Element *elem, int idx);
  void onElementEdited(darg::Element *elem, Point2 translate, Point2 scale);
};

extern BhvElementEditor bhv_element_editor;
