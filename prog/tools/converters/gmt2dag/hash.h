// Hash.h: a hash-based storage
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

class CHashElement;
class CHashImplementation;


//
// Hash
////////////////////////////////////////////////////////////////////////////////

class CHash // Entries must inherit from CHashElement
{

public:
  CHash(int (*pGetHashCodeFunction)(CHashElement *), bool (*pIsSameFunction)(CHashElement *, CHashElement *),
    void (*pDeleteFunction)(CHashElement *));

  ~CHash();

  bool IsValid();

  bool Add(CHashElement *);

  CHashElement *Find(CHashElement *);


private:
  CHashImplementation *mpImplementation;
};


//
// Hash Element
////////////////////////////////////////////////////////////////////////////////

class CHashElement
{

public:
  void *mpData;
};
