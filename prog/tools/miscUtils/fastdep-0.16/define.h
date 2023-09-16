#ifndef DEFINE_H_
#define DEFINE_H_

#include "element.h"

#include <string>

class FileStructure;

class Define : public Element
{
public:
  Define(FileStructure *aStructure, const std::string &aMacro);
  Define(FileStructure *aStructure, const std::string &aMacro, const std::string &aContent);
  Define(const Define &anOther);
  virtual ~Define();

  Define &operator=(const Define &anOther);

  virtual Element *copy() const;
  virtual void getDependencies(CompileState *aState);

  std::string getContent() const;

private:
  std::string MacroName;
  std::string Content;
};

#endif
