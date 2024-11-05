// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "element.h"

#include <string>

class FileStructure;
class CompileState;

class Include : public Element
{
public:
  Include(FileStructure *aStructure, const std::string &aFilename, const std::string &aFromFilename, bool aSystem);
  Include(const Include &anOther);
  virtual ~Include();

  Include &operator=(const Include &anOther);

  virtual Element *copy() const;
  virtual void getDependencies(CompileState *aState);

private:
  std::string Filename, FromFilename;
  bool System;
};
