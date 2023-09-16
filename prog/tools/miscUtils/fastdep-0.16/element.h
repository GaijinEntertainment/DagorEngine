#ifndef ELEMENT_H_
#define ELEMENT_H_

#include <string>

class CompileState;
class FileCache;
class FileStructure;

class Element
{
public:
  Element(FileStructure *aStructure);
  Element(const Element &anOther);
  virtual ~Element();

  Element &operator=(const Element &anOther);

  virtual Element *copy() const = 0;
  virtual void getDependencies(CompileState *aState) = 0;

  FileStructure *getStructure();
  FileCache *getCache();

private:
  FileStructure *theStructure;
};

#endif
