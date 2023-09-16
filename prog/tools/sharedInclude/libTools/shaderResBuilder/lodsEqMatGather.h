// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __LODSEQMATGATHER_H
#define __LODSEQMATGATHER_H
#pragma once


#include "eqMatGather.h"
#include <shaders/dag_shaders.h>


/*********************************
 *
 * class LodsEqualMaterialGather
 *
 *********************************/
class LodsEqualMaterialGather
{
private:
  PtrTab<ShaderMaterial> sceneMats;

public:
  EqualMaterialGather eqGather;

  // ctor/dtor
  LodsEqualMaterialGather() : sceneMats(midmem) {}

  virtual ~LodsEqualMaterialGather() {}

  // add material, if it equal to this material & not in list
  inline ShaderMaterial *addEqual(MaterialData *m)
  {
    if (m->className.empty())
      return NULL;
    int index = eqGather.addEqual(m);

    //    int index = -1;
    //    for (int i = 0; i < eqGather.equalMats.size(); i++)
    //    {
    //      if (eqGather.equalMats[i].src == m)
    //      {
    //        index = i; break;
    //      }
    //    }

    if (index < 0)
    {
      index = sceneMats.size();
      sceneMats.push_back(new_shader_material(*m, true));
      append_items(eqGather.equalMats, 1);
      eqGather.equalMats[index].src = m;
      G_ASSERT(sceneMats.size() == eqGather.equalMats.size());
    }

    return sceneMats[index];
  }

  // copy this to material saver
  inline void copyToMatSaver(ShaderMaterialsSaver &matSaver, ShaderTexturesSaver &texSaver)
  {
    int i;
    for (i = 0; i < sceneMats.size(); i++)
    {
      matSaver.addMaterial(sceneMats[i], texSaver, eqGather.equalMats[i].src);
    }
  }
}; // class LodsEqualMaterialGather
//

#endif
