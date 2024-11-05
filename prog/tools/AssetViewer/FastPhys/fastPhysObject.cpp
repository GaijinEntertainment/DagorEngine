// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edclipper.h>
#include <libTools/fastPhysData/fp_edpoint.h>

#include "fastPhysEd.h"
#include "fastPhysObject.h"
#include "fastPhysBone.h"
#include "fastPhysClipper.h"
#include "fastPhysPoint.h"

//------------------------------------------------------------------

E3DCOLOR IFPObject::frozenColor(120, 120, 120);

IFPObject::IFPObject(FpdObject *obj, FastPhysEditor &editor) : mObject(obj), mFPEditor(editor)
{
  setName(mObject->getName());
  objEditor = &editor;
}


IFPObject::~IFPObject() {}


FastPhysEditor *IFPObject::getObjEditor() const { return &mFPEditor; }


IFPObject *IFPObject::createFPObject(FpdObject *obj, FastPhysEditor &editor)
{
  if (obj->isSubOf(FpdBone::HUID))
    return new FPObjectBone(obj, editor);
  else if (obj->isSubOf(FpdClipper::HUID))
    return new FPObjectClipper(obj, editor);
  else if (obj->isSubOf(FpdPoint::HUID))
    return new FPObjectPoint(obj, editor);

  return NULL;
}

//------------------------------------------------------------------
