// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "element.h"

#include <string>

class FileStructure;

class Define : public Element
{
public:
  Define(FileStructure *aStructure, const std::string &aMacro);
  Define(FileStructure *aStructure, const std::string &aMacro, const std::string &aContent);
  Define(const Define &anOther);
  ~Define() override;

  Define &operator=(const Define &anOther);

  Element *copy() const override;
  void getDependencies(CompileState *aState) override;

  std::string getContent() const;

private:
  std::string MacroName;
  std::string Content;
};
