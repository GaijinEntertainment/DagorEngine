// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "element.h"
#include "fileCache.h"
#include "fileStructure.h"

Element::Element(FileStructure *aStructure) : theStructure(aStructure) {}

Element::Element(const Element &anOther) : theStructure(anOther.theStructure) {}

Element::~Element() {}

Element &Element::operator=(const Element &anOther)
{
  theStructure = anOther.theStructure;
  return *this;
}

FileStructure *Element::getStructure() { return theStructure; }

FileCache *Element::getCache() { return theStructure->getCache(); }
