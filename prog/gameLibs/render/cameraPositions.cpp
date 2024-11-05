// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_string.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <render/cameraPositions.h>

inline const char *get_camera_position_blk_name(const char *slot_name) { return slot_name != nullptr ? slot_name : "default"; }

void save_camera_position(const char *file_name, const char *slot_name, const TMatrix &camera_itm)
{
  const char *slot = get_camera_position_blk_name(slot_name);

  DataBlock cameraPositions;
  if (dd_file_exist(file_name))
    cameraPositions.load(file_name);

  DataBlock *slotBlk = cameraPositions.addBlock(slot);
  slotBlk->clearData();
  slotBlk->setTm("tm", camera_itm);

  cameraPositions.saveToTextFile(file_name);
}

bool load_camera_position(const char *file_name, const char *slot_name, TMatrix &camTm)
{
  const char *slot = get_camera_position_blk_name(slot_name);

  DataBlock cameraPositions;
  if (dd_file_exist(file_name))
    cameraPositions.load(file_name);

  const DataBlock *slotBlk = cameraPositions.getBlockByName(slot);
  if (!slotBlk)
    return false;
  if (slotBlk->paramExists("tm"))
    camTm = slotBlk->getTm("tm", TMatrix::IDENT);
  else if (slotBlk->paramExists("tm0"))
  {
    camTm.setcol(0, slotBlk->getPoint3("tm0", Point3(1.f, 0.f, 0.f)));
    camTm.setcol(1, slotBlk->getPoint3("tm1", Point3(0.f, 1.f, 0.f)));
    camTm.setcol(2, slotBlk->getPoint3("tm2", Point3(0.f, 0.f, 1.f)));
    camTm.setcol(3, slotBlk->getPoint3("tm3", ZERO<Point3>()));
  }
  else
    return false;
  return true;
}
