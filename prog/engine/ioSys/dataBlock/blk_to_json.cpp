#include "blk_shared.h"
#include <ioSys/dag_chainedMemIo.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <ctype.h>
#include <stdio.h> // sprintf

namespace
{
struct JsonCtx
{
  int level;
  int maxParPerLn, maxBlkPerLn;
  bool allowUnquoted;
  bool oneLineBlk;
};
} // namespace

static void writeIndent(IGenSave &cb, int n)
{
  static const char *space8 = "        ";
  for (; n >= 8; n -= 8)
    cb.write(space8, 8);
  if (n > 0)
    cb.write(space8, n);
}

static void writeString(IGenSave &cb, const char *s)
{
  if (!s || !*s)
    return;
  int l = i_strlen(s);
  cb.write(s, l);
}

static void writeJsonStrQuoted(IGenSave &cb, const char *nm)
{
  cb.write("\"", 1);
  for (const char *p = nm; *p; p++)
    if (*p == '\"')
      cb.write("\\\"", 2);
    else if (*p == '\n')
      cb.write("\\\n", 2);
    else if (*p == '\\')
      cb.write("\\\\", 2);
    else
      cb.write(p, 1);
  cb.write("\"", 1);
}
static void writeJsonIdent(IGenSave &cb, const char *nm, bool allow_unquoted)
{
  if (!allow_unquoted)
    return writeJsonStrQuoted(cb, nm);
  if (!isalpha(*nm) && *nm != '_')
    return writeJsonStrQuoted(cb, nm);
  for (const char *p = nm + 1; *p; p++)
    if (!isalnum(*p) && *p != '_')
      return writeJsonStrQuoted(cb, nm);
  if (strcmp(nm, "true") == 0 || strcmp(nm, "false") == 0 || strcmp(nm, "null") == 0)
    return writeJsonStrQuoted(cb, nm);
  cb.write(nm, i_strlen(nm));
}
static int getAvgParamCount(const DataBlock *b)
{
  int cnt = b->paramCount();
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == b->TYPE_MATRIX)
      cnt += 3;
  return cnt;
}
static bool isSubBlkCountLower(const DataBlock *b, int max_param_per_line, int max_block_per_line)
{
  max_param_per_line -= getAvgParamCount(b);
  max_block_per_line -= b->blockCount();
  if (max_param_per_line < 0 || max_block_per_line < 0)
    return false;
  for (int i = 0; i < b->blockCount(); i++)
    if (!isSubBlkCountLower(b->getBlock(i), max_param_per_line, max_block_per_line))
      return false;
  return true;
}

static void exportJsonText(IGenSave &cb, const DataBlock &blk, JsonCtx &jctx)
{
  char buf[256];
  bool oneline_par = jctx.oneLineBlk ? true : getAvgParamCount(&blk) <= jctx.maxParPerLn;
  bool oneline_blk = jctx.oneLineBlk ? true : isSubBlkCountLower(&blk, jctx.maxParPerLn, jctx.maxBlkPerLn);
  Tab<unsigned> arrmap;

  cb.write(oneline_blk ? "{ " : "{\n", 2);

  // build array map for params
  arrmap.resize(blk.paramCount() * 2);
  mem_set_ff(arrmap);
  for (uint32_t i = 0, base = 0, ie = blk.paramCount(); i < ie; i++)
  {
    if (!arrmap[i + arrmap.size() / 2])
      continue;
    int nid = blk.getParamNameId(i), ord = 0;
    for (uint32_t j = i + 1; j < ie; j++)
    {
      if (!arrmap[j + arrmap.size() / 2])
        continue;
      if (nid == blk.getParamNameId(j))
      {
        ord++;
        arrmap[base + ord] = (j << 16) | ord;
        arrmap[j + arrmap.size() / 2] = 0;
      }
    }
    arrmap[base] = (i << 16) | (ord ? 0 : 0xFFFF);
    base += ord + 1;
  }
  arrmap.resize(blk.paramCount());

  // write params
  for (uint32_t i = 0, ie = blk.paramCount(); i < ie; i++)
  {
    uint32_t idx = arrmap[i] >> 16;
    int ord = arrmap[i] & 0xFFFF;
    if (!oneline_blk && (i == 0 || !oneline_par))
      writeIndent(cb, jctx.level);

    if (ord == 0xFFFF || ord == 0)
    {
      writeJsonIdent(cb, blk.getParamName(idx), jctx.allowUnquoted);
      cb.write(":", 1);
    }
    if (ord == 0)
    {
      jctx.level++;
      if (oneline_par)
        cb.write("[", 1);
      else
      {
        cb.write("[\n", 2);
        writeIndent(cb, jctx.level);
      }
    }

    switch (blk.getParamType(idx))
    {
      case DataBlock::TYPE_STRING: writeJsonStrQuoted(cb, blk.getStr(idx)); break;
      case DataBlock::TYPE_BOOL: writeString(cb, blk.getBool(idx) ? "true" : "false"); break;
      case DataBlock::TYPE_INT:
        snprintf(buf, sizeof(buf), "%d", blk.getInt(idx));
        writeString(cb, buf);
        break;
      case DataBlock::TYPE_REAL:
        snprintf(buf, sizeof(buf), "%g", blk.getReal(idx));
        writeString(cb, buf);
        break;
      case DataBlock::TYPE_POINT2:
      {
        Point2 cd = blk.getPoint2(idx);
        snprintf(buf, sizeof(buf), "[%g, %g]", cd[0], cd[1]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_POINT3:
      {
        Point3 cd = blk.getPoint3(idx);
        snprintf(buf, sizeof(buf), "[%g, %g, %g]", cd[0], cd[1], cd[2]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_POINT4:
      {
        Point4 cd = blk.getPoint4(idx);
        snprintf(buf, sizeof(buf), "[%g, %g, %g, %g]", cd[0], cd[1], cd[2], cd[3]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_IPOINT2:
      {
        IPoint2 cdi = blk.getIPoint2(idx);
        snprintf(buf, sizeof(buf), "[%d, %d]", cdi[0], cdi[1]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_IPOINT3:
      {
        IPoint3 cdi = blk.getIPoint3(idx);
        snprintf(buf, sizeof(buf), "[%d, %d, %d]", cdi[0], cdi[1], cdi[2]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_E3DCOLOR:
      {
        E3DCOLOR c = blk.getE3dcolor(idx);
        snprintf(buf, sizeof(buf), "[%d, %d, %d, %d]", c.r, c.g, c.b, c.a);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_MATRIX:
      {
        TMatrix tm = blk.getTm(idx);
        const float *m = tm.array;
        snprintf(buf, sizeof(buf), "[%g, %g, %g,  %g, %g, %g,  %g, %g, %g,  %g, %g, %g]", m[0 * 3 + 0], m[0 * 3 + 1], m[0 * 3 + 2],
          m[1 * 3 + 0], m[1 * 3 + 1], m[1 * 3 + 2], m[2 * 3 + 0], m[2 * 3 + 1], m[2 * 3 + 2], m[3 * 3 + 0], m[3 * 3 + 1],
          m[3 * 3 + 2]);
        writeString(cb, buf);
        break;
      }
      case DataBlock::TYPE_INT64:
        snprintf(buf, sizeof(buf), "%lld", (long long int)blk.getInt64(idx));
        writeString(cb, buf);
        break;

      default: G_ASSERT(0);
    }

    if (ord != 0xFFFF && ord != 0 && (i + 1 == arrmap.size() || (arrmap[i + 1] & 0xFFFF) != ord + 1))
    {
      jctx.level--;
      if (!oneline_par)
      {
        cb.write("\n", 1);
        writeIndent(cb, jctx.level);
      }
      cb.write("]", 1);
    }
    if (i + 1 < blk.paramCount() + blk.blockCount())
      cb.write(oneline_par ? ", " : ",\n", 2);
  }

  if (!oneline_blk && blk.paramCount())
    cb.write("\n\n", (oneline_par && blk.blockCount() > 1) ? 2 : 1);

  // build array map for blocks
  arrmap.resize(blk.blockCount() * 2);
  mem_set_ff(arrmap);
  for (int i = 0, base = 0; i < blk.blockCount(); i++)
  {
    if (!arrmap[i + arrmap.size() / 2])
      continue;
    int nid = blk.getBlock(i)->getNameId(), ord = 0;
    for (int j = i + 1; j < blk.blockCount(); j++)
    {
      if (!arrmap[j + arrmap.size() / 2])
        continue;
      if (nid == blk.getBlock(j)->getNameId())
      {
        ord++;
        arrmap[base + ord] = (j << 16) | ord;
        arrmap[j + arrmap.size() / 2] = 0;
      }
    }
    arrmap[base] = (i << 16) | (ord ? 0 : 0xFFFF);
    base += ord + 1;
  }
  arrmap.resize(blk.blockCount());

  // write blocks
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &b = *blk.getBlock(arrmap[i] >> 16);
    int ord = arrmap[i] & 0xFFFF;
    if (!oneline_blk)
      writeIndent(cb, jctx.level);

    if (ord == 0xFFFF || ord == 0)
    {
      writeJsonIdent(cb, b.getBlockName(), jctx.allowUnquoted);
      cb.write(":", 1);
    }
    if (ord == 0)
    {
      jctx.level++;
      cb.write("[\n", oneline_blk ? 1 : 2);
      if (!oneline_blk)
        writeIndent(cb, jctx.level);
    }

    if (b.paramCount() + b.blockCount() == 0)
      cb.write("{}", 2);
    else
    {
      jctx.level++;
      jctx.oneLineBlk = oneline_blk;
      exportJsonText(cb, b, jctx);
      jctx.oneLineBlk = oneline_blk;
      jctx.level--;
    }

    if (ord != 0xFFFF && ord != 0 && (i + 1 == arrmap.size() || (arrmap[i + 1] & 0xFFFF) != ord + 1))
    {
      jctx.level--;
      if (!oneline_blk)
      {
        cb.write("\n", 1);
        writeIndent(cb, jctx.level);
      }
      cb.write("]", 1);
    }
    if (i + 1 < blk.blockCount())
      cb.write(",", 1);

    cb.write(oneline_blk ? " " : "\n", 1);
  }

  if (!oneline_blk && jctx.level > 1)
    writeIndent(cb, jctx.level - 1);
  cb.write("}", 1);
}

bool dblk::export_to_json_text_stream(const DataBlock &blk, IGenSave &cwr, bool allow_unq, int max_ppl, int max_bpl)
{
  JsonCtx jctx = {0, max_ppl, max_bpl, allow_unq, false};

  DAGOR_TRY { exportJsonText(cwr, blk, jctx); }
  DAGOR_CATCH(IGenSave::SaveException) { return false; }

  return true;
}

#define EXPORT_PULL dll_pull_iosys_datablock_to_json
#include <supp/exportPull.h>
