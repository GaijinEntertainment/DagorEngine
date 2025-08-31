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
  ~Include() override;

  Include &operator=(const Include &anOther);

  Element *copy() const override;
  void getDependencies(CompileState *aState) override;

private:
  std::string Filename, FromFilename;
  bool System;
};
