// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_indent.h"
#include "../c_constants.h"

CIndent::CIndent(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w) :

  PropertyControlBase(id, event_handler, parent, x, y, w, 0)
{}


PropertyContainerControlBase *CIndent::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createIndent(id, new_line);
  return NULL;
}
