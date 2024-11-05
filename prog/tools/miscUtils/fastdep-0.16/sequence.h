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
  virtual ~Sequence();

  Sequence &operator=(const Sequence &anOther);

  void add(Element *anElement);
  virtual Element *copy() const;
  virtual void getDependencies(CompileState *aState);

private:
  std::vector<Element *> Seq;
};
