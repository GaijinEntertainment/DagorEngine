#include <util/dag_console.h>
#include "texMgrData.h"

using texmgr_internal::RMGR;

static TEXTUREID context_tid = BAD_TEXTUREID;
static void verbose_tex_info(int idx)
{
  const char *nm = RMGR.getName(idx);
  TextureInfo ti;
  int texmem_sz = 0;
  if (RMGR.texDesc[idx].dim.maxLev > 0)
  {
    ti.w = RMGR.texDesc[idx].dim.w;
    ti.h = RMGR.texDesc[idx].dim.h;
    ti.d = RMGR.texDesc[idx].dim.d;
    ti.mipLevels = RMGR.texDesc[idx].dim.l;
    texmem_sz = RMGR.getTexMemSize4K(idx) * 4;
  }
  else if (BaseTexture *t = RMGR.baseTexture(idx))
  {
    t->getinfo(ti);
    texmem_sz = tql::sizeInKb(t->ressize());
  }
  else
    ti.w = ti.h = ti.d = 0;

  console::print_d(
    "%d: %dx%dx%d,L%d, desc=0x%04X max=%d(%d/%d) ld=%d rd=%d req=%d  tm=%X  "
    "TQ=%d:%d BQ=%d:%d HQ=%d:%d UHQ=%d:%d rc=%d bt.rc=%d bt.id=%d ql=%d(%d) stubIdx=%d gpu=%dK(+%dK) bd=%dK lfu=%u%c(%s)",
    idx, ti.w, ti.h, ti.d, ti.mipLevels, RMGR.levDesc[idx], RMGR.texDesc[idx].dim.maxLev, RMGR.resQS[idx].getQLev(),
    RMGR.resQS[idx].getMaxLev(), RMGR.resQS[idx].getLdLev(), RMGR.resQS[idx].getRdLev(), RMGR.resQS[idx].getMaxReqLev(),
    RMGR.getTagMask(idx), RMGR.texDesc[idx].packRecIdx[TQL_thumb].pack,
    RMGR.uint16_to_int(RMGR.texDesc[idx].packRecIdx[TQL_thumb].rec), RMGR.texDesc[idx].packRecIdx[TQL_base].pack,
    RMGR.uint16_to_int(RMGR.texDesc[idx].packRecIdx[TQL_base].rec), RMGR.texDesc[idx].packRecIdx[TQL_high].pack,
    RMGR.uint16_to_int(RMGR.texDesc[idx].packRecIdx[TQL_high].rec), RMGR.texDesc[idx].packRecIdx[TQL_uhq].pack,
    RMGR.uint16_to_int(RMGR.texDesc[idx].packRecIdx[TQL_uhq].rec), RMGR.getRefCount(idx), RMGR.getBaseTexUsedCount(idx),
    RMGR.pairedBaseTexId[idx] ? RMGR.pairedBaseTexId[idx].index() : -1, RMGR.resQS[idx].getCurQL(), RMGR.resQS[idx].getMaxQL(),
    RMGR.texDesc[idx].dim.stubIdx, texmem_sz, RMGR.getTexAddMemSizeNeeded4K(idx) * 4, RMGR.getTexBaseDataSize(idx) >> 10,
    RMGR.getResLFU(idx), strlen(nm) > 30 ? '\n' : ' ', nm);
}
static TEXTUREID resolve_tid(const char *nm)
{
  if (strcmp(nm, "@") == 0)
    return context_tid;

  TEXTUREID tid = get_managed_texture_id(nm);
  if (tid != BAD_TEXTUREID)
    return tid;
  if (strncmp(nm, "0x", 2) == 0)
    tid = TEXTUREID(strtoul(nm + 2, nullptr, 16));
  return tid;
}
static unsigned resolve_level(int idx, const char *lev)
{
  if (strcmp(lev, "/tq") == 0)
    return RMGR.getLevDesc(idx, TQL_thumb);
  if (strcmp(lev, "/bq") == 0)
    return RMGR.getLevDesc(idx, TQL_base);
  if (strcmp(lev, "/hq") == 0)
    return RMGR.getLevDesc(idx, TQL_high);
  if (strcmp(lev, "/uhq") == 0)
    return RMGR.getLevDesc(idx, TQL_uhq);
  return atoi(lev);
}

static bool texmgr_dbgcontrol_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("tex", "list", 1, 10)
  {
    int min_rc = 0;
    const char *name_substr = nullptr;
    bool brief = true, only_bt = false, only_reading = false;
    TEXTUREID only_tid = BAD_TEXTUREID;

    for (int i = 1; i < argc; i++)
      if (strncmp(argv[i], "/rc:", 4) == 0)
        min_rc = atoi(argv[i] + 4);
      else if (strcmp(argv[i], "/verbose") == 0)
        brief = false;
      else if (strcmp(argv[i], "/rd") == 0)
        only_reading = true;
      else if (strcmp(argv[i], "/bt") == 0)
        only_bt = true;
      else if (strncmp(argv[i], "/tex:", 5) == 0)
        only_tid = resolve_tid(argv[i] + 5);
      else if (argv[i][0] != '/')
        name_substr = argv[i];

    console::print_d("managed textures list: minRefCount=%d substr=%s %s %s onlyTid=0x%x", min_rc, name_substr,
      only_bt ? "onlyBT" : "", only_reading ? "onlyReading" : "", only_tid);
    for (unsigned idx = 0, ie = RMGR.getRelaxedIndexCount(); idx < ie; idx++)
      if (RMGR.getRefCount(idx) >= min_rc)
      {
        if (name_substr && !strstr(RMGR.getName(idx), name_substr))
          continue;
        if (only_reading && !RMGR.resQS[idx].isReading())
          continue;
        if (only_bt && !RMGR.getBaseTexUsedCount(idx) && !RMGR.hasTexBaseData(idx))
          continue;
        if (only_tid != BAD_TEXTUREID && RMGR.toId(idx) != only_tid)
          continue;

        D3dResource *res = RMGR.getD3dResRelaxed(idx);
        TextureInfo ti;
        if (RMGR.texDesc[idx].dim.maxLev > 0)
        {
          ti.w = RMGR.texDesc[idx].dim.w;
          ti.h = RMGR.texDesc[idx].dim.h;
          ti.d = RMGR.texDesc[idx].dim.d;
          ti.mipLevels = RMGR.texDesc[idx].dim.l;
        }
        else if (BaseTexture *t = RMGR.baseTexture(idx))
          t->getinfo(ti);
        else
          ti.w = ti.h = ti.d = 0;

        if (brief)
          console::print_d("%d: %dx%dx%d,L%d rc=%d tid=0x%x (%s) %p sz=%dK", idx, ti.w, ti.h, ti.d, ti.mipLevels,
            RMGR.getRefCount(idx), RMGR.toId(idx), RMGR.getName(idx), res, res ? tql::sizeInKb(res->ressize()) : 0);
        else
          verbose_tex_info(idx);
      }
    if (argc == 1)
      console::print("usage: tex.list [/rc:MIN_RC] [/verbose] [name_substr]");
  }
  CONSOLE_CHECK_NAME("tex", "downgrade", 2, 3)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      int idx = tid.index();
      int lev = min(RMGR.texDesc[idx].dim.maxLev, min(RMGR.resQS[idx].getQLev(), RMGR.resQS[idx].getMaxLev()));
      BaseTexture *t = RMGR.baseTexture(idx);
      lev = min(lev, (int)interlocked_acquire_load(RMGR.resQS[idx].maxReqLev));
      if (argc == 3)
        lev = resolve_level(idx, argv[2]);

      verbose_tex_info(idx);
      console::print_d("downgrading 0x%x(%s) tex=%p to lev=%d", tid, RMGR.getName(idx), t, lev);
      if (t && RMGR.downgradeTexQuality(idx, *t, lev))
        verbose_tex_info(idx);
      else
        console::print_d("cannot downgrade!");
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  CONSOLE_CHECK_NAME("tex", "setMaxLev", 2, 3)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      int idx = tid.index();
      int lev = RMGR.resQS[idx].getQLev();
      if (argc == 3)
        lev = resolve_level(idx, argv[2]);

      verbose_tex_info(idx);
      console::print_d("setMaxLev=%d for 0x%x(%s)", lev, tid, RMGR.getName(idx));
      RMGR.changeTexMaxLev(idx, lev);
      verbose_tex_info(idx);
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  CONSOLE_CHECK_NAME("tex", "set", 2, 2)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      context_tid = tid;
      console::print_d("set (%s)=0x%x as active texture (use @ to refer at it)", argv[1], tid);
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  CONSOLE_CHECK_NAME("tex", "addRef", 2, 2)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      RMGR.incRefCount(tid.index());
      console::print_d("add ref for (%s)=0x%x -> rc=%d", RMGR.getName(tid.index()), tid, RMGR.getRefCount(tid));
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  CONSOLE_CHECK_NAME("tex", "delRef", 2, 2)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      if (RMGR.decRefCount(tid.index()) == 0)
        RMGR.incReadyForDiscardTex(tid.index());
      console::print_d("del ref for (%s)=0x%x -> rc=%d", RMGR.getName(tid.index()), tid, RMGR.getRefCount(tid));
      verbose_tex_info(tid.index());
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  CONSOLE_CHECK_NAME("tex", "markLFU", 2, 3)
  {
    TEXTUREID tid = resolve_tid(argv[1]);
    if (tid != BAD_TEXTUREID)
    {
      int req_lev = argc < 3 ? 15 : resolve_level(tid.index(), argv[2]);
      RMGR.markResLFU(tid, req_lev);
      console::print_d("mark (%s)=0x%x used (reqLev=%d)", RMGR.getName(tid.index()), tid, req_lev);
      verbose_tex_info(tid.index());
    }
    else
      console::print_d("texture (%s) not registered", argv[1]);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(texmgr_dbgcontrol_console_handler);
