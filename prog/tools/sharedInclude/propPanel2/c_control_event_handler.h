//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>

typedef class PropertyContainerControlBase PropPanel2;

class ControlEventHandler
{
public:
  virtual ~ControlEventHandler() {}

  virtual long onChanging(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
    return 0;
  }
  virtual void onChange(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
  virtual void onChangeFinished(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
  virtual void onClick(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
  virtual void onDoubleClick(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
  virtual long onKeyDown(int pcb_id, PropPanel2 *panel, unsigned v_key)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
    G_UNUSED(v_key);
    return 0;
  }
  virtual void onResize(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
  virtual void onPostEvent(int pcb_id, PropPanel2 *panel)
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
  }
};
