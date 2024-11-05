// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include.h"
#include "fileCache.h"
#include "fileStructure.h"
#include "compileState.h"
#include "os.h"

Include::Include(FileStructure *aStructure, const std::string &aFilename, const std::string &aFromFilename, bool aSystem) :
  Element(aStructure), Filename(aFilename), FromFilename(aFromFilename), System(aSystem)
{}

Include::Include(const Include &anOther) :
  Element(anOther), Filename(anOther.Filename), FromFilename(anOther.FromFilename), System(anOther.System)
{}

Include::~Include() {}

Include &Include::operator=(const Include &anOther)
{
  if (this != &anOther)
  {
    Element::operator=(*this);
    Filename = anOther.Filename;
    FromFilename = anOther.FromFilename;
    System = anOther.System;
  }
  return *this;
}

Element *Include::copy() const { return new Include(*this); }

void Include::getDependencies(CompileState *aState)
{
  if (Filename.length() > 0)
  {
    /*		std::string Fullname;
        if (Filename[0] == cPathSep)
          Fullname = Filename;
        else
          Fullname = getStructure()->getPath()+"/"+Filename;
        aState->addDependencies(Fullname); */
    FileStructure *S = getCache()->update(getStructure()->getPath(), Filename, System, FromFilename);
    if (S /* && (std::string(S->getFileName(),0,4) != "/usr") */)
    {
      aState->addDependencies(S->getFileName());
      S->getDependencies(aState);
    }
  }
}
