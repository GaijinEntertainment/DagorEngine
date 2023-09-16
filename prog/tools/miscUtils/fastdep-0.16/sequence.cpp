#include "sequence.h"

Sequence::Sequence(FileStructure *aStructure) : Element(aStructure) {}

Sequence::Sequence(const Sequence &anOther) : Element(anOther)
{
  for (unsigned int i = 0; i < anOther.Seq.size(); ++i)
    Seq.push_back(anOther.Seq[i]->copy());
}

Sequence::~Sequence()
{
  for (unsigned int i = 0; i < Seq.size(); ++i)
    delete Seq[i];
}

Sequence &Sequence::operator=(const Sequence &anOther)
{
  if (this != &anOther)
  {
    Element::operator=(*this);
    for (unsigned int i = 0; i < Seq.size(); ++i)
      delete Seq[i];
    Seq.erase(Seq.begin(), Seq.end());
    for (unsigned int j = 0; j < anOther.Seq.size(); ++j)
      Seq.push_back(anOther.Seq[j]->copy());
  }
  return *this;
}

Element *Sequence::copy() const { return new Sequence(*this); }

void Sequence::add(Element *anElem) { Seq.push_back(anElem); }

void Sequence::getDependencies(CompileState *aState)
{
  for (unsigned int i = 0; i < Seq.size(); ++i)
    Seq[i]->getDependencies(aState);
}
