//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

struct SrcObjsToPlace
{
  // source data
  SimpleString resName;
  Tab<TMatrix> tm;
  DataBlock addBlk;

  // temporary storage for build
  int tmpOfs, nameOfs;
  mkbindump::PatchTabRef tmsPt;
  mkbindump::RoDataBlockBuilder roBlk;
  // can be used to optimize name comparison
  int nameId;

  SrcObjsToPlace() : tm(tmpmem) {}
};

inline void writeObjsToPlaceDump(mkbindump::BinDumpSaveCB &cwr, dag::Span<SrcObjsToPlace> objs, int type)
{
  mkbindump::PatchTabRef objs_pt;

  int total_cnt = 0;
  for (int i = 0; i < objs.size(); i++)
    total_cnt += objs[i].tm.size();
  if (!total_cnt)
    return;

  cwr.beginTaggedBlock(_MAKE4C('Obj'));
  cwr.setOrigin();

  // struct ObjectsToPlace
  cwr.writeFourCC(type);   // unsigned typeFourCc;
  objs_pt.reserveTab(cwr); // PatchableTab<ObjRec> objs;

  for (int i = 0; i < objs.size(); i++)
  {
    if (!objs[i].tm.size())
      continue;
    objs[i].nameOfs = cwr.tell();
    cwr.writeRaw((char *)objs[i].resName, i_strlen(objs[i].resName) + 1);
  }
  cwr.align8();

  objs_pt.startData(cwr, objs.size());
  for (int i = 0; i < objs.size(); i++)
  {
    if (!objs[i].tm.size())
      continue;
    objs[i].roBlk.prepare(objs[i].addBlk, cwr.getTarget());

    // struct ObjectsToPlace::ObjRec
    cwr.writePtr64e(objs[i].nameOfs); // PatchablePtr<const char> resName;
    objs[i].tmsPt.reserveTab(cwr);    // PatchableTab<TMatrix> tm;
    objs[i].roBlk.writeHdr(cwr);      // RoDataBlock addData;
  };
  objs_pt.finishTab(cwr);

  // write TM array and additional DataBlocks
  for (int i = 0; i < objs.size(); i++)
  {
    if (!objs[i].tm.size())
      continue;
    cwr.align8();
    objs[i].tmsPt.startData(cwr, objs[i].tm.size());
    cwr.writeTabData32ex(objs[i].tm);
    objs[i].roBlk.writeData(cwr);
    objs[i].tmsPt.finishTab(cwr);
  }

  cwr.popOrigin();
  cwr.align8();
  cwr.endBlock();
}
