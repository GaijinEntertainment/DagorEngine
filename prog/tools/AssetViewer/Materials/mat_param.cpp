// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mat_param.h"
#include "mat_paramDesc.h"
#include "mat_util.h"

#include <ioSys/dag_dataBlock.h>
#include <3d/dag_materialData.h>
#include <util/dag_globDef.h>


const char *MatParam::getParamName() const { return paramDescr.getName(); }


int MatTexture::getSlot() const { return paramDescr.getSlot(); }


void MatTripleInt::setScript(MaterialRec &mat)
{
  if (useDef)
    ::erase_script_param(mat.script, getParamName());
  else
    ::set_script_param_int(mat.script, getParamName(), val);
}


void MatTripleReal::setScript(MaterialRec &mat)
{
  if (useDef)
    ::erase_script_param(mat.script, getParamName());
  else
    ::set_script_param_real(mat.script, getParamName(), val);
}


void MatE3dColor::setScript(MaterialRec &mat)
{
  int slot = paramDescr.getSlot();

  if (useDef)
  {
    mat.defMatCol |= (1 << slot);
    switch (slot)
    {
      case 0: mat.mat.diff = Color4(1, 1, 1, 1); break;
      case 1: mat.mat.amb = Color4(1, 1, 1, 1); break;
      case 2: mat.mat.spec = Color4(1, 1, 1, 1); break;
      case 3: mat.mat.emis = Color4(0, 0, 0, 0); break;
    }
  }
  else
  {
    mat.defMatCol &= ~(1 << slot);
    switch (slot)
    {
      case 0: mat.mat.diff = color4(val); break;
      case 1: mat.mat.amb = color4(val); break;
      case 2: mat.mat.spec = color4(val); break;
      case 3: mat.mat.emis = color4(val); break;
    }
  }
}


void MatCombo::setScript(MaterialRec &mat) { ::set_script_param(mat.script, getParamName(), val); }


void Mat2Sided::setScript(MaterialRec &mat)
{
  switch (val)
  {
    case TWOSIDED_NONE:
      mat.flags &= ~MATF_2SIDED;
      ::erase_script_param(mat.script, "real_two_sided");
      break;

    case TWOSIDED:
      mat.flags |= MATF_2SIDED;
      ::erase_script_param(mat.script, "real_two_sided");
      break;

    case TWOSIDED_REAL:
      mat.flags |= MATF_2SIDED;
      ::set_script_param(mat.script, "real_two_sided", "yes");
      break;
  }
}
