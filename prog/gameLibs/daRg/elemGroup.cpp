// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "elemGroup.h"

#include <daRg/dag_element.h>


namespace darg
{


ElemGroup::~ElemGroup()
{
  for (auto e : children)
    e->onElemGroupRelease();
}


void ElemGroup::addChild(Element *elem) { children.insert(elem); }


void ElemGroup::removeChild(Element *elem) { children.erase(elem); }


void ElemGroup::setStateFlags(int f)
{
  for (auto e : children)
    e->setElemStateFlags(f);
}


void ElemGroup::clearStateFlags(int f)
{
  for (auto e : children)
    e->clearElemStateFlags(f);
}


} // namespace darg
