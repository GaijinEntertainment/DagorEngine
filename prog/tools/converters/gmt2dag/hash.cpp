// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Hash.cpp : Implementation of the Hash
//
////////////////////////////////////////////////////////////////////////////////

#include <crtdbg.h>
#include <limits.h>

#include "hash.h"

#define CHECK(x) _ASSERTE(x)


//
// Hash Implementation
////////////////////////////////////////////////////////////////////////////////

class CHashNode
{

public:
  int miCode;
  CHashElement *mpElements;

  CHashNode *mpSubNodes[2];
};


class CHashImplementation
{

public:
  CHashImplementation(int (*pGetHashCodeFunction)(CHashElement *), bool (*pIsSameFunction)(CHashElement *, CHashElement *),
    void (*pDeleteFunction)(CHashElement *));

  ~CHashImplementation();


  bool Add(CHashElement *);

  CHashElement *Find(CHashElement *);


private:
  int (*mpGetHashCodeFunction)(CHashElement *);
  bool (*mpIsSameFunction)(CHashElement *, CHashElement *);
  void (*mpDeleteFunction)(CHashElement *);

  CHashNode *mpRoot;


  void RecurseDeleteNodes(CHashNode *pNode);
};


////////////////////////////////////////////////////////////////////////////////
// CHash implementation
////////////////////////////////////////////////////////////////////////////////

CHash::CHash(int (*pGetHashCodeFunction)(CHashElement *), bool (*pIsSameFunction)(CHashElement *, CHashElement *),
  void (*pDeleteFunction)(CHashElement *))
{
  mpImplementation = new CHashImplementation(pGetHashCodeFunction, pIsSameFunction, pDeleteFunction);
}

CHash::~CHash()
{
  if (mpImplementation)
  {
    delete mpImplementation;
  }
}

bool CHash::IsValid() { return mpImplementation != NULL; }

bool CHash::Add(CHashElement *pElement) { return mpImplementation->Add(pElement); }

CHashElement *CHash::Find(CHashElement *pElement) { return mpImplementation->Find(pElement); }


////////////////////////////////////////////////////////////////////////////////
// CHashImplementation constructor and destructor
////////////////////////////////////////////////////////////////////////////////

CHashImplementation::CHashImplementation(int (*pGetHashCodeFunction)(CHashElement *),
  bool (*pIsSameFunction)(CHashElement *, CHashElement *), void (*pDeleteFunction)(CHashElement *))
{
  mpGetHashCodeFunction = pGetHashCodeFunction;
  mpIsSameFunction = pIsSameFunction;
  mpDeleteFunction = pDeleteFunction;

  mpRoot = NULL;
}

CHashImplementation::~CHashImplementation()
{
  if (mpRoot)
  {
    RecurseDeleteNodes(mpRoot);
  }
}

void CHashImplementation::RecurseDeleteNodes(CHashNode *pNode)
{
  if (pNode->mpSubNodes[0])
  {
    RecurseDeleteNodes(pNode->mpSubNodes[0]);
  }

  if (pNode->mpSubNodes[1])
  {
    RecurseDeleteNodes(pNode->mpSubNodes[1]);
  }


  CHashElement *pElement = pNode->mpElements;

  do
  {
    CHashElement *pNextElement = (CHashElement *)pElement->mpData;

    mpDeleteFunction(pElement);

    pElement = pNextElement;
  } while (pElement);


  delete pNode;
}


////////////////////////////////////////////////////////////////////////////////
// Add
////////////////////////////////////////////////////////////////////////////////

bool CHashImplementation::Add(CHashElement *pObject)
{
  CHECK(pObject);


  int iCode = mpGetHashCodeFunction(pObject);


  CHashNode **ppNode = &mpRoot;
  int iLevel = 0;

  while (*ppNode)
  {
    if ((*ppNode)->miCode == iCode)
    {
      CHashNode *pNode = *ppNode;

      CHECK(pNode->mpElements);

      pObject->mpData = pNode->mpElements;

      pNode->mpElements = pObject;

      return true;
    }


    CHECK(UINT_MAX == (1i64 << 32) - 1);
    CHECK(iLevel <= 31);

    ppNode = &(*ppNode)->mpSubNodes[(iCode >> iLevel) & 1];
    iLevel++;
  }


  *ppNode = new CHashNode;

  if (!*ppNode)
  {
    return false;
  }


  CHashNode *pNode = *ppNode;

  pNode->miCode = iCode;

  pNode->mpSubNodes[0] = NULL;
  pNode->mpSubNodes[1] = NULL;


  pNode->mpElements = pObject;

  pObject->mpData = NULL;

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Find
////////////////////////////////////////////////////////////////////////////////

CHashElement *CHashImplementation::Find(CHashElement *pSample)
{
  CHECK(pSample);

  int iCode = mpGetHashCodeFunction(pSample);

  CHashNode *pNode = mpRoot;
  int iLevel = 0;

  while (pNode)
  {
    if (pNode->miCode == iCode)
    {
      CHashElement *pElement = pNode->mpElements;

      do
      {
        if (mpIsSameFunction(pSample, pElement))
        {
          return pElement;
        }

        pElement = (CHashElement *)pElement->mpData;
      } while (pElement);

      return NULL;
    }
    else
    {
      CHECK(UINT_MAX == (1i64 << 32) - 1);
      CHECK(iLevel <= 31);

      pNode = pNode->mpSubNodes[(iCode >> iLevel) & 1];

      iLevel++;
    }
  }

  return NULL;
}
