// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "element.h"

#include <string>
#include <vector>

class CompileState;
class FileStructure;

class Sequence : public Element
{
public:
  Sequence(FileStructure *aStructure);
  Sequence(const Sequence &anOther);
  ~Sequence() override;

  Sequence &operator=(const Sequence &anOther);

  void add(Element *anElement);
  Element *copy() const override;
  void getDependencies(CompileState *aState) override;

private:
  std::vector<Element *> Seq;
};
