#include <libTools/util/binDumpUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
// #include <debug/dag_debug.h>

// StrCollector
void mkbindump::StrCollector::writeStrings(BinDumpSaveCB &cwr)
{
  clear_and_resize(ofs, str.nameCount());
  iterate_names_in_lexical_order(str, [&, this](int id, const char *name) {
    int len = i_strlen(name);
    ofs[id] = cwr.tell();
    cwr.writeRaw(name, len + 1);
  });
}
void mkbindump::StrCollector::writeStringsEx(BinDumpSaveCB &cwr, dag::Span<StrCollector> wr_str)
{
  clear_and_resize(ofs, str.nameCount());
  iterate_names_in_lexical_order(str, [&, this](int m_id, const char *m_name) {
    bool found = false;
    for (int j = 0; j < wr_str.size(); j++)
    {
      int id = wr_str[j].str.getNameId(m_name);
      if (id != -1)
      {
        ofs[m_id] = wr_str[j].ofs[id];
        found = true;
      }
    }

    if (!found)
    {
      int len = i_strlen(m_name);
      ofs[m_id] = cwr.tell();
      cwr.writeRaw(m_name, len + 1);
    }
  });
}

// RoNameMapBuilder
void mkbindump::RoNameMapBuilder::prepare(StrCollector &direct_namemap)
{
  strNames = &direct_namemap;
  noIdxRemap = false;
  map.reset();

  iterate_names_in_lexical_order(direct_namemap.getMap(), [&, this](int, const char *name) { map.addNameId(name); });

  map.prepareXmap();
}

void mkbindump::RoNameMapBuilder::writeHdr(BinDumpSaveCB &cwr)
{
  ptStr.reserveTab(cwr);
  if (noIdxRemap)
    ptIdx.reserveTab(cwr);
}

void mkbindump::RoNameMapBuilder::writeMap(BinDumpSaveCB &cwr)
{
  cwr.align8();
  ptStr.startData(cwr, map.nameCount());

  Tab<int> sorted_ids;
  gather_ids_in_lexical_order(sorted_ids, map);
  iterate_names_in_order(map, sorted_ids, [&, this](int, const char *name) { cwr.writePtr64e(strNames->getOffset(name)); });
  ptStr.finishTab(cwr);

  if (noIdxRemap)
  {
    ptIdx.startData(cwr, map.nameCount());
    iterate_names_in_order(map, sorted_ids, [&](int id, const char *) { cwr.writeInt16e(id); });
    ptIdx.finishTab(cwr);
    cwr.align8();
  }
}

// RoDataBlockBuilder
void mkbindump::RoDataBlockBuilder::prepare(const DataBlock &blk, bool write_be)
{
  varNames.reset();
  valStr.reset();
  realStor.clear();
  intStor.clear();
  blocks.clear();
  params.clear();

  BlockRec &br = blocks.push_back();
  br.name = NULL;
  br.paramNum = blk.paramCount();
  br.blockNum = blk.blockCount();
  enumDataBlock(blk, 0, write_be);

  nameMap.prepare(varNames);
}

void mkbindump::RoDataBlockBuilder::writeHdr(BinDumpSaveCB &cwr)
{
  blocks[0].ptParam.reserveTab(cwr); // PatchableTab<Param> params;
  blocks[0].ptBlock.reserveTab(cwr); // PatchableTab<RoDataBlock> blocks;

  nameMapRefPos = cwr.tell();
  cwr.writePtr64e(0);  // PatchablePtr<RoNameMap> nameMap;
  cwr.writeInt32e(-1); // int nameId;
  cwr.writeInt32e(0);  // int _resv;
}
void mkbindump::RoDataBlockBuilder::writeData(BinDumpSaveCB &cwr)
{
  nameMapOfs = cwr.tell();
  cwr.writePtr32eAt(nameMapOfs, nameMapRefPos);

  nameMap.writeHdr(cwr);
  varNames.writeStrings(cwr);
  nameMap.writeMap(cwr);
  cwr.align8();
  cwr.writeStorage32ex(realStor);
  cwr.align8();
  cwr.writeStorage32ex(intStor);
  valStr.writeStringsEx(cwr, dag::Span<StrCollector>(&varNames, 1));
  cwr.align8();

  blocks[0].ptBlock.startData(cwr, blocks[0].blockNum);
  enumWriteBlocks(cwr, 0);

  for (int bi = 0, pidx = 0; bi < blocks.size(); bi++)
  {
    blocks[bi].ptParam.startData(cwr, blocks[bi].paramNum);
    for (int pi = blocks[bi].stParam; pi < blocks[bi].stParam + blocks[bi].paramNum; pi++)
    {
      switch (params[pi].type)
      {
        case DataBlock::TYPE_STRING: cwr.writeInt32e(valStr.getOffset(params[pi].sval) - nameMapOfs); break;

        case DataBlock::TYPE_E3DCOLOR: cwr.writeE3dColorE(params[pi].val); break;

        case DataBlock::TYPE_INT: cwr.writeInt32e(params[pi].val); break;

        case DataBlock::TYPE_BOOL:
          cwr.writeInt8e(params[pi].val);
          cwr.writeZeroes(3);
          break;

        case DataBlock::TYPE_REAL: cwr.writeReal(params[pi].rval); break;

        case DataBlock::TYPE_POINT2:
        case DataBlock::TYPE_POINT3:
        case DataBlock::TYPE_POINT4:
        case DataBlock::TYPE_MATRIX: cwr.writeInt32e(realStor.indexToOffset(params[pi].val) - nameMapOfs); break;

        case DataBlock::TYPE_IPOINT2:
        case DataBlock::TYPE_IPOINT3:
        case DataBlock::TYPE_INT64: cwr.writeInt32e(intStor.indexToOffset(params[pi].val) - nameMapOfs); break;


        default: cwr.writeInt32e(0);
      }
      cwr.writeInt16e(nameMap.getFinalNameId(params[pi].name));
      cwr.writeInt16e(params[pi].type);
    }

    blocks[bi].ptParam.finishTab(cwr);
    blocks[bi].ptBlock.finishTab(cwr);
  }
}

void mkbindump::RoDataBlockBuilder::enumWriteBlocks(BinDumpSaveCB &cwr, int bi)
{
  BlockRec &b = blocks[bi];
  int cbi = b.stBlock;

  for (int j = 0; j < b.blockNum; j++)
  {
    blocks[cbi + j].ptParam.reserveTab(cwr);                       // PatchableTab<Param> params;
    blocks[cbi + j].ptBlock.reserveTab(cwr);                       // PatchableTab<RoDataBlock> blocks;
    cwr.writePtr64e(nameMapOfs);                                   // PatchablePtr<RoNameMap> nameMap;
    cwr.writeInt32e(nameMap.getFinalNameId(blocks[cbi + j].name)); // int nameId;
    cwr.writeInt32e(0);                                            // int _resv;
  }

  for (int j = 0; j < b.blockNum; j++)
  {
    blocks[cbi + j].ptBlock.startData(cwr, blocks[cbi + j].blockNum);
    enumWriteBlocks(cwr, cbi + j);
  }
}

void mkbindump::RoDataBlockBuilder::enumDataBlock(const DataBlock &blk, int bi, bool write_be)
{
  blocks[bi].ptParam.reset();
  blocks[bi].ptBlock.reset();
  blocks[bi].stParam = params.size();
  blocks[bi].stBlock = blocks.size();

  for (int i = 0; i < blk.paramCount(); i++)
  {
    ParamRec &pr = params.push_back();

    pr.name = varNames.addString(blk.getParamName(i));
    pr.type = blk.getParamType(i);
    switch (pr.type)
    {
      case DataBlock::TYPE_STRING: pr.sval = valStr.addString(blk.getStr(i)); break;
      case DataBlock::TYPE_INT: pr.val = blk.getInt(i); break;
      case DataBlock::TYPE_REAL: pr.rval = blk.getReal(i); break;
      case DataBlock::TYPE_POINT2:
      {
        auto v = blk.getPoint2(i);
        realStor.getRef(pr.val, &v.x, 2);
      }
      break;
      case DataBlock::TYPE_POINT3:
      {
        auto v = blk.getPoint3(i);
        realStor.getRef(pr.val, &v.x, 3);
      }
      break;
      case DataBlock::TYPE_POINT4:
      {
        auto v = blk.getPoint4(i);
        realStor.getRef(pr.val, &v.x, 4);
      }
      break;
      case DataBlock::TYPE_IPOINT2:
      {
        auto v = blk.getIPoint2(i);
        intStor.getRef(pr.val, &v.x, 2);
      }
      break;
      case DataBlock::TYPE_IPOINT3:
      {
        auto v = blk.getIPoint3(i);
        intStor.getRef(pr.val, &v.x, 3);
      }
      break;
      case DataBlock::TYPE_BOOL: pr.val = blk.getBool(i); break;
      case DataBlock::TYPE_E3DCOLOR: pr.val = blk.getE3dcolor(i); break;
      case DataBlock::TYPE_MATRIX: realStor.getRef(pr.val, (real *)blk.getTm(i).m, 12); break;
      case DataBlock::TYPE_INT64:
      {
        uint64_t tmp = blk.getInt64(i);
        if (write_be)
          tmp = (tmp << 32) | (tmp >> 32);
        intStor.getRef(pr.val, (int *)&tmp, 2, 4, 2);
      }
      break;
    }
  }

  for (int j = 0; j < blk.blockCount(); j++)
  {
    const DataBlock &blk2 = *blk.getBlock(j);

    BlockRec &b = blocks.push_back();
    b.name = varNames.addString(blk2.getBlockName());
    b.paramNum = blk2.paramCount();
    b.blockNum = blk2.blockCount();
  }

  for (int i = 0; i < blk.blockCount(); i++)
    enumDataBlock(*blk.getBlock(i), blocks[bi].stBlock + i, write_be);
}
