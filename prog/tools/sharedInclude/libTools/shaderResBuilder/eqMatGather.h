// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  equal material gather
************************************************************************/

#include <3d/dag_materialData.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_tab.h>


/************************************************************************
  struct EqualMaterial
************************************************************************/
struct EqualMaterial
{
private:
  inline static bool safe_strcmp(const char *s1, const char *s2)
  {
    // fix NULL error
    if (!s1 || !s2)
      return s1 == s2;
    else
      return strcmp(s1, s2) == 0;
  }

public:
  Ptr<MaterialData> src;
  PtrTab<MaterialData> eqmats;

  inline EqualMaterial() : eqmats(midmem) {}

  // add material, if it equal to this material & not in list
  inline bool addEqual(MaterialData *m)
  {
    if (checkEqual(m))
      return true;
    // return false; //////// DEBUG!!! - off 'smart' compare nafig (compare only pointers)

    if ((src->flags == m->flags) && (src->mat == m->mat) && safe_strcmp(src->matScript, m->matScript) &&
        safe_strcmp(src->className, m->className))
    {
      for (int i = 0; i < MAXMATTEXNUM; i++)
        if (src->mtex[i] != m->mtex[i])
          return false;

      eqmats.push_back(m);
      return true;
    }

    return false;
  }

  // check for equality
  inline bool checkEqual(MaterialData *m) const
  {
    if (m == src)
      return true;

    for (int i = 0; i < eqmats.size(); i++)
      if (eqmats[i] == m)
        return true;

    return false;
  }
};


/*********************************
 *
 * class EqualMaterialGather
 *
 *********************************/
class EqualMaterialGather
{
public:
  Tab<EqualMaterial> equalMats;

  // ctor/dtor
  EqualMaterialGather() : equalMats(tmpmem) {}
  ~EqualMaterialGather() {}

  // add material, if it equal to this material & not in list. return index of exists material
  inline int addEqual(MaterialData *m)
  {
    if (!m)
      return -1;
    for (int i = 0; i < equalMats.size(); i++)
    {
      if (equalMats[i].addEqual(m))
        return i;
    }

    return -1;
  }

  // check for equality. return equal material index or -1, if not found
  inline int checkEqual(MaterialData *m) const
  {
    if (!m)
      return -1;
    for (int i = 0; i < equalMats.size(); i++)
    {
      if (equalMats[i].checkEqual(m))
        return i;
    }

    return -1;
  }

}; // class EqualMaterialGather
