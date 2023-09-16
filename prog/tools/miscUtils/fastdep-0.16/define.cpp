#include "define.h"
#include "compileState.h"

Define::Define(FileStructure *aStructure, const std::string &aMacro) : Element(aStructure), MacroName(aMacro) {}

Define::Define(FileStructure *aStructure, const std::string &aMacro, const std::string &aContent) :
  Element(aStructure), MacroName(aMacro), Content(aContent)
{}

Define::Define(const Define &anOther) : Element(anOther), MacroName(anOther.MacroName), Content(anOther.Content) {}

Define::~Define() {}

Define &Define::operator=(const Define &anOther)
{
  if (this != &anOther)
  {
    Element::operator=(*this);
    MacroName = anOther.MacroName;
    Content = anOther.Content;
  }
  return *this;
}

Element *Define::copy() const { return new Define(*this); }

void Define::getDependencies(CompileState *aState) { aState->define(MacroName, Content); }

std::string Define::getContent() const { return Content; }
