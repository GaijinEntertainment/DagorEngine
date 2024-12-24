// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_picMgr.h>
#include <3d/dag_dynAtlas.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texPackMgr2.h>
#include <shaders/dag_shaders.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_map.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_urlLike.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastIntList.h>
#include <util/dag_texMetaData.h>
#include <util/dag_delayedAction.h>
#include <math/dag_adjpow2.h>
#include <stdio.h>
#include <startup/dag_globalSettings.h>

// #define PICMGR_DEBUG debug  // trace picture manager execution
#ifndef PICMGR_DEBUG
#define PICMGR_DEBUG(...) ((void)0)
#endif

/** @page PictureManager

.. seealso:: :ref:`image_rasterization`

Picture Sources
---------------

Pictures can be files, resources, http resources, base64 encoded or dynamically rendered characters.

Full image path can be with or without path to **PictureAtlas** (see below) ``[atlasPath#]<image path string>``.

If image path string starts with http or https - it is treated as url and image would be requested from this url.
If image path string doesn't have slashes and endswith ``*`` - it is treated as game built resource and would be loaded from .dxp.bin
files. If image path string starts with ``b64://`` it is supposed to be base64 encoded string file (usually svg). if image path string
starts with ``render`` string - it is supposed to be datablock string, that allow to render in atlas or separately dynamic render
objects, like characters. In other cases image path string should be path to image path (usually png, svg or jpg).


Picture Atlases
---------------


Dagor allow to make dynamic atlases for images.
Such atlases improve rendering performance, cause allow to render several UI objects with one drawprimitive.

To create atlas you need specify it's parameters in configuration file in datablock format.

Picture atlas configuration format::

  tex{
    dynAtlas:b=yes
    rtTex:b=yes
    name:t="myatlas" //for debugging? and for static (obsolete) atlases
    size:ip2=512,512 //size in pixels

    maxAllowedPicSz:ip2=320,320 //max allowed picture size to rasterize.
                                //Will raise a logerr on attempt to rasterize bigger image.
    premultiplyAlpha:b=yes
    margin:i=1 //margin around pictures
    picSrcFolder:t="/ui/uiskin/"
    picReserve:i=128
    refHRes:i=1080 //reference size of vertcal resoultion.
                   //Atlas will proportionally scale on bigger resolutions,
                   //as we usually need pixel perfect rasterization of icons
  }


To specify usage of picture atlas::

  //   "tex/mytex.tga"        - real texture name
  //   "tex/myatlas#mytex" - picture "mytex" in tex/myatlas.blk

*/
namespace PictureManager
{
PictureRenderFactory *PictureRenderFactory::tail = NULL;

typedef DynamicAtlasTexTemplate<true> DynamicPicAtlas;
enum
{
  LINE_TEX_LEN = 1024,
  MAX_MARGIN_WD = 2
};

static FastNameMapEx nameMap;
static CritSecStorage critSec;
static bool last_discard_skipped = false;
static FastIntList missingTexNameId;
static Tab<PictureRenderFactory *> picRenderFactory;
static DelayedAction *volatile pending_render_action = nullptr; //< set from loading thread, reset from main thread

inline int getTexRecIdx(PICTUREID pid) { return unsigned(pid) & 0xFFFF; }
inline int getPicRecIdx(PICTUREID pid) { return (unsigned(pid) >> 16) & 0x3FFF; }
inline PICTUREID makePicId(int texRecIdx, int picRecIdx, bool dyn = false)
{
  return ((picRecIdx << 16) & 0x3FFF0000) | (texRecIdx & 0xFFFF) | (dyn ? 0x40000000 : 0);
}
bool is_name_b64_like(const char *nm) { return nm && strncmp(nm, "b64://", 6) == 0; }
inline bool is_render_pic_name_like(const char *nm)
{
  if (!nm || strncmp(nm, "render", 6) != 0)
    return false;
  const char *fext = dd_get_fname_ext(nm);
  return fext && strcmp(fext, ".render") == 0;
}

static const IPoint2 defMaxAllowedPicSz = IPoint2(4096, 4096);
struct AtlasData
{
  enum
  {
    FMT_ARGB,
    FMT_L8,
    FMT_DXT5,
    FMT_RGBA,
    FMT_none
  };
  DynamicPicAtlas atlas;
  DataBlock blk;
  Tab<int> picNameId;
  String picSrcFolder;
  FastIntList missingPicHash;
  bool dynamic = false, premultiplyAlpha = true, lazyAlloc = false, isRt = false;
  int fmt = FMT_none;
  int texFmt = 0;
  IPoint2 maxAllowedPicSz = defMaxAllowedPicSz;

  String makeAbsFn(const char *nm) const
  {
    if (df_is_fname_url_like(nm) || is_render_pic_name_like(nm) || is_name_b64_like(nm))
      return String(nm);
    return picSrcFolder + nm;
  }
};
struct TexRec
{
  TEXTUREID texId;
  d3d::SamplerHandle smpId;
  int nameId : 24;
  unsigned ownedTex : 1, flags : 7;
  int refCount;
  AtlasData *ad;
  Point2 size;

  TexRec(int name_id) :
    texId(BAD_TEXTUREID),
    smpId(d3d::INVALID_SAMPLER_HANDLE),
    nameId(name_id),
    ownedTex(0),
    flags(0),
    refCount(-1),
    ad(NULL),
    size(0, 0)
  {}
  ~TexRec()
  {
    G_ASSERT_LOG(refCount <= 1, "refCount(%s)=%d{%d}", getName(), refCount, get_managed_texture_refcount(texId));
    if (ad)
      debug("PM: termAtlas(%s) refCount=%d{%d}", getName(), refCount, get_managed_texture_refcount(texId));
    while (refCount > 0)
      delRef();
    delete ad;
    if (ownedTex)
    {
      ShaderGlobal::reset_from_vars(texId);
      evict_managed_tex_id(texId);
    }
  }

  bool initAtlas();
  bool reinitDynAtlas();
  void addRef()
  {
    interlocked_increment(refCount);
    if (texId == BAD_TEXTUREID)
    {
      addPendingTexRef();
      return;
    }
    TEXTUREID _texId = texId;
    interlocked_increment(refCount); // prevent accidental unloading during unlock/relock
    leave_critical_section(critSec); // we assume call under crit sec!
    acquire_managed_tex(_texId);
    enter_critical_section(critSec);
    interlocked_decrement(refCount); // release refcount after relock
  }
  void delRef()
  {
    G_ASSERT_LOG(refCount > 0, "refCount(%s)=%d", getName(), refCount);
    if (texId == BAD_TEXTUREID)
      delPendingTexRef();
    else
      release_managed_tex(texId);
    interlocked_decrement(refCount);
  }

  void addPendingTexRef()
  {
    G_ASSERTF(ad == NULL, "texId=0x%x pendRefCount=%.0f refCount=%d for '%s'", texId, size.x, refCount, getName());
    size.x += 1; // compute pending tex acquires (while texId is not known)
  }
  void delPendingTexRef()
  {
    G_ASSERTF(ad == NULL && size.x > 0, "texId=0x%x pendRefCount=%.0f refCount=%d for '%s'", texId, size.x, refCount, getName());
    size.x -= 1; // compute pending tex acquires (while texId is not known)
  }
  int getPendingTexRef() { return (texId == BAD_TEXTUREID && !ad) ? (int)size.x : 0; }

  const char *getName() const { return nameMap.getName(nameId); }
  Point2 getSize() const { return texId != BAD_TEXTUREID ? size : Point2(1, 1); }
  Point2 picSz(const DynamicPicAtlas::ItemData *d)
  {
    return d->valid() ? Point2((d->u1 - d->u0) * size.x, (d->v1 - d->v0) * size.y) : Point2(d->w, d->h);
  }
  TEXTUREID resolveTexId() { return ad ? texId : add_managed_texture(getName()); }
};

static FixedBlockAllocator texRecAlloc(sizeof(TexRec), 1024);
union TexRecOrFreeListIndex
{
  TexRec *rec;
  int nextFreeIdxX2;
  bool isFreeSlot() { return (nextFreeIdxX2 & 1) != 0; }
  operator TexRec *() { return rec; }
  TexRec *operator->() { return rec; }
};
static Tab<TexRecOrFreeListIndex> texRec;
static eastl::vector<uint8_t> texRecGen;
static int texRecFreeList = -1;
static eastl::vector_map<int, int> nameId2texRecIdx;

template <typename F>
static inline void for_each_tex_rec(F fn)
{
  int i = 0;
  for (auto &tr : texRec)
  {
    if (!tr.isFreeSlot())
      fn(*tr.rec, i);
    ++i;
  }
}

struct AsyncPicLoadJob : public cpujobs::IJob
{
  enum JobType
  {
    JT_picInAtlas,
    JT_texPic
  };
  SimpleString name;
  async_load_done_cb_t done_cb;
  void *done_arg;
  Point2 *outTc0, *outTc1, *outSz, tc0S, tc1S, szS;
  PICTUREID picId;
  TEXTUREID outTexId;
  d3d::SamplerHandle smpId;
  bool premulAlpha = false;
  bool jobDone;
  JobType jtype;
  bool skipAtlasPic = false;
  int reportErr = 100;

  static volatile int numJobsInFlight;
  static volatile bool loadAllowed;

  AsyncPicLoadJob(JobType jt, PICTUREID pic, const char *nm, Point2 *out_tc0, Point2 *out_tc1, Point2 *out_sz, async_load_done_cb_t cb,
    void *arg) :
    jtype(jt),
    name(nm),
    picId(pic),
    done_cb(cb),
    done_arg(arg),
    jobDone(false),
    outTexId(BAD_TEXTUREID),
    smpId(d3d::INVALID_SAMPLER_HANDLE),
    outTc0(out_tc0 ? out_tc0 : &tc0S),
    outTc1(out_tc1 ? out_tc1 : &tc1S),
    outSz(out_sz ? out_sz : &szS)
  {
    outTc0->set(0, 0);
    outTc1->set(0, 0);
    outSz->set(1, 1);
    interlocked_increment(numJobsInFlight);
    interlocked_increment(texRec[getTexRecIdx(picId)]->refCount);
  }

  virtual void doJob()
  {
    if (jtype == JT_picInAtlas)
      loadPicInAtlas();
    else if (jtype == JT_texPic)
      loadTexPic();
  }
  virtual void releaseJob()
  {
    finalizeJob();
    delete this;
  }
  virtual unsigned getJobTag() { return _MAKE4C('PICM'); };

  eastl::pair<TEXTUREID, d3d::SamplerHandle> doJobSync()
  {
    done_cb = NULL;
    doJob();
    finalizeJob();
    return {outTexId, smpId};
  }

protected:
  void loadPicInAtlas();
  void loadTexPic();
  void finalizeJob()
  {
    if (done_cb)
    {
      if (jtype == JT_picInAtlas && reportErr == 100)
      {
        WinAutoLock lock(critSec);
        if (getTexRecIdx(picId) >= texRec.size() || texRec[getTexRecIdx(picId)].isFreeSlot())
        {
          LOGERR_CTX("PM: finalizeJob(%08X) for already discarded rec, done_cb=%p name=%s", picId, (void *)done_cb, name);
          if (!jobDone)
            interlocked_decrement(numJobsInFlight);
          (*done_cb)(BAD_PICTUREID, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, NULL, NULL, NULL, done_arg);
          return;
        }
        outTexId = texRec[getTexRecIdx(picId)]->texId;
        smpId = texRec[getTexRecIdx(picId)]->smpId;
        if (outTexId != BAD_TEXTUREID)
          reportErr = 101; // we can report texture as done and then try reload it later on demand
      }

      if (jobDone || reportErr == 101)
        (*done_cb)(picId, outTexId, smpId, outTc0, outTc1, outSz, done_arg);
      else
        (*done_cb)(reportErr == 100 ? picId : BAD_PICTUREID, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, NULL, NULL, NULL,
          done_arg); // report abort/error
    }

    WinAutoLock lock(critSec);
    if (getTexRecIdx(picId) >= texRec.size() || texRec[getTexRecIdx(picId)].isFreeSlot())
    {
      LOGERR_CTX("PM: finalizeJob(%08X) for already discarded rec, name=%s", picId, name);
      if (!jobDone)
        interlocked_decrement(numJobsInFlight);
      return;
    }
    TexRec &tr = *texRec[getTexRecIdx(picId)];
    if (!jobDone)
    {
      discardUndonePic();
      interlocked_decrement(numJobsInFlight);
    }
    interlocked_decrement(tr.refCount);

    switch (reportErr)
    {
      case 11:
      case 12:
      case 21: DAG_FATAL("PM: internal error #%d: pic=%08X (%s)", reportErr, picId, name); break;
      case 13: DAG_FATAL("PM: load failed, name='%s', pic=%08X (%d pics missing)", name, picId, tr.ad->missingPicHash.size()); break;
      case 22: DAG_FATAL("PM: load failed, name='%s', pic=%08X texId=0x%x", name, picId, outTexId); break;
      case 100:
        debug("PM: async load aborted, name='%s', pic=%08X texId=0x%x", name, picId, outTexId);
        if (done_cb && tr.ad)
          tr.delRef();
        break;
    }
  }
  void discardUndonePic();
  void finalizePic(DynamicPicAtlas::ItemData *d, TexRec &tr);
};
volatile int AsyncPicLoadJob::numJobsInFlight = 0;
volatile bool AsyncPicLoadJob::loadAllowed = false;

struct PictureRenderContext
{
  PictureRenderContext(PictureRenderFactory *_prf, Texture *_rt, int _x0, int _y0, int _w, int _h, const DataBlock &p,
    PICTUREID pic_id, uint8_t gen_) :
    prf(_prf), rt(_rt), x0(_x0), y0(_y0), w(_w), h(_h), props(p), pid(pic_id), gen(gen_)
  {}

  unsigned triedAtFrame = 0;
  PICTUREID pid;
  PictureRenderFactory *prf;
  Texture *rt;
  int x0, y0, w, h;
  DataBlock props;
  uint8_t gen;
};

static bool inited = false;
static int debugAsyncSleepMs = -1;
static int asyncLoadJobMgr = -1;
static bool asyncLoadJobMgrOwned = false;
static bool doFatalOnPictureLoadFailed = false;
static bool searchBlkBeforeTaBin = true;
static bool dynAtlasLazyAllocDef = false;
static Texture *texTransp[AtlasData::FMT_none * 2] = {NULL};
static TEXTUREID substOnePixelTexId = BAD_TEXTUREID;
static d3d::SamplerHandle substOnePixelSmpId = d3d::INVALID_SAMPLER_HANDLE;
} // namespace PictureManager

namespace PictureManager
{
static inline uint32_t str_hash_fnv1(const char *fn) { return (uint32_t)eastl::hash<const char *>()(fn); }

static const char *decodeFileName(const char *file_name, int &out_name_id, unsigned &out_hash)
{
  String name(framemem_ptr());

  if (const char *str = strchr(file_name, '#'))
  {
    name.setSubStr(file_name, str);
    dd_simplify_fname_c(name);
    int len = name.length();

    out_name_id = nameMap.addNameId(name);

    name += str;
    char *nameCStr = name.c_str();

    if (const char *p = name.find("@@", nameCStr + len))
      if (strchr("hvs", p[2]) && p[3] == 's')
        erase_items(name, p - nameCStr, 4);

    dd_simplify_fname_c(nameCStr + len + 1);
    dd_strlwr(nameCStr + len + 1);

    out_hash = PictureManager::str_hash_fnv1(name);
    G_FAST_ASSERT(out_hash != 0);

    return str + 1;
  }

  name = file_name;

  if (strncmp(file_name, "b64://", 6) != 0)
    dd_simplify_fname_c(name);

  out_name_id = nameMap.addNameId(name);
  out_hash = 0;

  return file_name;
}

static int decodeAtlasNameId(const char *file_name)
{
  if (const char *str = strchr(file_name, '#'))
  {
    char name[DAGOR_MAX_PATH];
    SNPRINTF(name, DAGOR_MAX_PATH, "%.*s", int(str - file_name), file_name);
    dd_simplify_fname_c(name);
    return nameMap.addNameId(name);
  }
  return -1;
}
static int createTexRec(int name_id)
{
  auto it = nameId2texRecIdx.find(name_id);
  if (it != nameId2texRecIdx.end())
    return it->second;
  G_FAST_ASSERT(name_id >= 0);
  int idx = texRecFreeList;
  if (idx >= 0)
  {
    G_FAST_ASSERT(texRec[idx].isFreeSlot());
    texRecFreeList = texRec[idx].nextFreeIdxX2 >> 1;
  }
  else
  {
    idx = (int)texRec.size();
    texRec.push_back();
    texRecGen.push_back(0);
  }
  G_ASSERTF(idx < 0x10000, "idx=%d for '%s'", nameMap.getName(name_id));
  texRec[idx].rec = new (texRecAlloc.allocateOneBlock(), _NEW_INPLACE) TexRec(name_id);
  nameId2texRecIdx.insert(it, eastl::make_pair(name_id, idx));
  return idx;
}

inline const DynamicPicAtlas::ItemData *decodePicId(PICTUREID pid, int &tex_idx, bool update_lru = false,
  const uint8_t *pgen = nullptr)
{
  if (pid == -1)
  {
    tex_idx = -1;
    return NULL;
  }
  tex_idx = getTexRecIdx(pid);
  if (tex_idx >= texRec.size() || texRec[tex_idx].isFreeSlot() || (pgen && *pgen != texRecGen[tex_idx]))
  {
    G_ASSERTF(tex_idx < texRec.size(), "pid=%08X tex_idx=%d texRec.size()=%d", pid, tex_idx, texRec.size());
    tex_idx = -1;
    return NULL;
  }
  TexRec &tr = *texRec[tex_idx];
  if (tr.ad)
  {
    int pic_idx = getPicRecIdx(pid);
    if (const DynamicPicAtlas::ItemData *d = tr.ad->atlas.findItemIndexed(pic_idx, update_lru))
      return d;
    tex_idx = -1;
    return NULL;
  }
  return NULL;
}

static bool readAtlasDesc(const char *file_name, DataBlock &out_blk, String &out_tex_name)
{
  TIME_PROFILE(picmgr_readAtlasDesc);

  if (!dd_get_fname_ext(file_name) && strstr(file_name, "::"))
  {
    // parse file_name as dynamic atlas in form "name::<sz.x>:<sz.y>"
    const char *name_end = strstr(file_name, "::");
    const char *sz_y_start = name_end ? strstr(name_end + 2, ":") : nullptr;
    out_blk.clearData();
    if (!sz_y_start)
    {
      logerr("PM: bad name for realtime dynamic atlas: %s (scheme is NAME::X:Y[:{MPLR}*] )", file_name);
      return false;
    }
    int sz_x = atoi(name_end + 2), sz_y = atoi(sz_y_start + 1);
    DataBlock &texBlk = *out_blk.addNewBlock("tex");
    texBlk.setBool("dynAtlas", true);
    texBlk.setIPoint2("size", IPoint2(sz_x, sz_y));

    const char *attr_start = strstr(sz_y_start + 1, ":");
    if (attr_start)
      attr_start++;
    else
      attr_start = "";
    texBlk.setInt("margin", strchr(attr_start, 'M') ? 1 : 0);
    texBlk.setIPoint2("maxAllowedPicSz", IPoint2(sz_x, sz_y));
    texBlk.setBool("premultiplyAlpha", strchr(attr_start, 'P') ? true : false);
    texBlk.setBool("lazyAlloc", strchr(attr_start, 'L') ? true : false);
    texBlk.setBool("rtTex", strchr(attr_start, 'R') ? true : false);
    texBlk.setStr("name", file_name);
    texBlk.setInt("picReserve", 1);
    return true;
  }

  FullFileLoadCB ta_crd(NULL);
  bool ret = false;
  if (!dd_get_fname_ext(file_name) && (!searchBlkBeforeTaBin || !dd_file_exists(String(0, "%s.blk", file_name))))
    ret = ta_crd.open(String(0, "%s.ta.bin", file_name), DF_READ | DF_IGNORE_MISSING);

  if (ret)
  {
    if (ta_crd.readInt() == _MAKE4C('TA.'))
    {
      ta_crd.readInt();
      ret = dblk::load_from_stream(out_blk, ta_crd, dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS, ta_crd.getTargetName());
    }
  }
  else if (dd_get_fname_ext(file_name))
    ret = dblk::load(out_blk, file_name, dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);
  else
    ret = dblk::load(out_blk, String(0, "%s.blk", file_name), dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);

  if (!ret)
  {
    logerr("PM: cannot load atlas blk file '%s'", file_name);
    return false;
  }

  const DataBlock &texBlk = *out_blk.getBlockByNameEx("tex");

  const char *texName = texBlk.getStr("name", NULL);
  if (ta_crd.fileHandle)
    texName = ta_crd.getTargetName();
  else if (!texName || !*texName)
  {
    logerr("PM: bad name=\"%s\" in atlas '%s'", texName, file_name);
    return false;
  }

  if (ta_crd.fileHandle)
    out_tex_name = texName;
  else if (*texName == '/' || *texName == '\\')
  {
    // remove leading slash
    out_tex_name = texName + 1;
  }
  else
  {
    out_tex_name.printf(0, "%.*s%s", int(dd_get_fname(file_name) - file_name), file_name, texName);
    dd_simplify_fname_c(out_tex_name);
  }

  if (texBlk.getBool("dynAtlas", false))
  {
    out_tex_name.printf(0, "$%s", texName);
    const char *fmt_str = texBlk.getStr("format", "ARGB");
    bool bc4_fmt = strstr(fmt_str, "DXT") != NULL;
    bool allow_npot = texBlk.getBool("allowNonPow2Size", false);
    IPoint2 sz = texBlk.getIPoint2("size", IPoint2(0, 0));
    if (sz.x == 0 || sz.y == 0 || (!allow_npot && (!is_pow_of2(sz.x) || !is_pow_of2(sz.y))) ||
        (allow_npot && bc4_fmt && ((sz.x % 4) || (sz.y % 4))))
    {
      logerr("PM: bad texture size=%d,%d for dynamic atlas '%s' (fmt=%s allowNonPow2Size=%s)", sz.x, sz.y, file_name, fmt_str,
        allow_npot ? "yes" : "no");
      return false;
    }

    const char *src_folder = texBlk.getStr("picSrcFolder", "./");
    String abs_path;
    if (*src_folder == '/' || *src_folder == '\\')
      abs_path = src_folder + 1; // remove leading slash
    else
      abs_path.printf(0, "%.*s%s", int(dd_get_fname(file_name) - file_name), file_name, src_folder);
    abs_path += "/";
    dd_simplify_fname_c(abs_path);
    if (DataBlock *b = out_blk.getBlockByName("tex"))
      b->setStr("picSrcFolderAbs", abs_path);
  }
  return true;
}

static bool reloadDiscardedPic(PICTUREID pid, DynamicPicAtlas::ItemData *d)
{
  if (d->scheduleRestoring())
  {
    TexRec &tr = *texRec[getTexRecIdx(pid)];
    const char *pic_nm = nameMap.getName(tr.ad->picNameId[getPicRecIdx(pid)]);
    if (!pic_nm || !*pic_nm)
    {
      logerr("PM: bad nameId=%d for pic=%08X", tr.ad->picNameId[getPicRecIdx(pid)], pid);
      return false;
    }
    String fn = tr.ad->makeAbsFn(pic_nm);
    PICMGR_DEBUG("PM: scheduling reload %s[%d]='%s', pic=%08X (was discarded)", tr.getName(), getPicRecIdx(pid), fn, pid);

    AsyncPicLoadJob *j = new AsyncPicLoadJob(AsyncPicLoadJob::JT_picInAtlas, pid, fn, NULL, NULL, NULL, NULL, NULL);
    j->premulAlpha = tr.ad->premultiplyAlpha;
    if (asyncLoadJobMgr < 0)
    {
      j->doJobSync();
      delete j;
      return true;
    }
    G_VERIFY(cpujobs::add_job(asyncLoadJobMgr, j));
  }
  return false;
}

static void init_transp_tex(unsigned fmt)
{
  if (fmt >= AtlasData::FMT_none)
    return;
  if (texTransp[fmt * 2 + 0])
    return;

  int tr_tex_flg = TEXCF_SYSMEM | TEXCF_DYNAMIC;
  int h_step, bpp, margin;
  if (fmt == AtlasData::FMT_ARGB)
  {
    tr_tex_flg |= TEXFMT_A8R8G8B8;
    h_step = 1;
    bpp = 4;
    margin = MAX_MARGIN_WD;
  }
  else if (fmt == AtlasData::FMT_RGBA)
  {
    tr_tex_flg |= TEXFMT_R8G8B8A8;
    h_step = 1;
    bpp = 4;
    margin = MAX_MARGIN_WD;
  }
  else if (fmt == AtlasData::FMT_L8)
  {
    tr_tex_flg |= TEXFMT_L8;
    h_step = 1;
    bpp = 1;
    margin = MAX_MARGIN_WD;
  }
  else if (fmt == AtlasData::FMT_DXT5)
  {
    tr_tex_flg |= TEXFMT_DXT5;
    h_step = 4;
    bpp = 4;
    margin = 4;
  }
  else
    return;

  texTransp[fmt * 2 + 0] = d3d::create_tex(NULL, margin, LINE_TEX_LEN, tr_tex_flg, 1, String(0, "transpV%d", fmt));
  texTransp[fmt * 2 + 1] = d3d::create_tex(NULL, LINE_TEX_LEN, margin, tr_tex_flg, 1, String(0, "transpH%d", fmt));
  G_ASSERT(texTransp[fmt * 2 + 0]);
  G_ASSERT(texTransp[fmt * 2 + 1]);
  char *data;
  int stride;
  if (texTransp[fmt * 2 + 0]->lockimgEx(&data, stride, 0, TEXLOCK_WRITE))
  {
    for (int i = LINE_TEX_LEN; i > 0; i -= h_step, data += stride)
      memset(data, 0, margin * bpp);
    texTransp[fmt * 2 + 0]->unlockimg();
  }
  if (texTransp[fmt * 2 + 1]->lockimgEx(&data, stride, 0, TEXLOCK_WRITE))
  {
    for (int i = margin; i > 0; i -= h_step, data += stride)
      memset(data, 0, LINE_TEX_LEN * bpp);
    texTransp[fmt * 2 + 1]->unlockimg();
  }
}
static void copy_vert_line(Texture *t, int x0, int y0, int len, int m, unsigned fmt)
{
  if (fmt >= AtlasData::FMT_none)
    return;
  Texture *tt = texTransp[fmt * 2 + 0];
  bool upd_ok = true;
#if DAGOR_DBGLEVEL > 0
  int _x0 = x0, _y0 = y0, _len = len, _y0_ = 0, _len2 = 0;
#endif
  if (x0 < 0)
  {
    m += x0;
    x0 = 0;
  }
  if (y0 < 0)
  {
    len += y0;
    y0 = 0;
  }
  if (m <= 0 || len <= 0)
    return;
#if DAGOR_DBGLEVEL > 0
  _y0_ = y0;
  _len2 = len;
#endif
  for (; len > LINE_TEX_LEN; len -= LINE_TEX_LEN, y0 += LINE_TEX_LEN)
    if (!t->updateSubRegion(tt, 0, 0, 0, 0, m, LINE_TEX_LEN, 1, 0, x0, y0, 0))
      upd_ok = false;
  if (!t->updateSubRegion(tt, 0, 0, 0, 0, m, len, 1, 0, x0, y0, 0))
    upd_ok = false;
#if DAGOR_DBGLEVEL > 0
  if (!upd_ok)
    logerr("copy_vert_line(%p, %d, %d, %d, %d, 0x%X) failed (tt=%p x0=%d y0=%d len=%d)", t, _x0, _y0, _len, m, fmt, tt, _x0, _y0_,
      _len2);
#endif
  G_UNUSED(upd_ok);
}
static void copy_hor_line(Texture *t, int x0, int y0, int len, int m, unsigned fmt)
{
  if (fmt >= AtlasData::FMT_none)
    return;
  bool upd_ok = true;
#if DAGOR_DBGLEVEL > 0
  int _x0 = x0, _y0 = y0, _len = len, _x0_ = 0, _len2 = 0;
#endif
  if (x0 < 0)
  {
    len += x0;
    x0 = 0;
  }
  if (y0 < 0)
  {
    m += y0;
    y0 = 0;
  }
  if (m <= 0 || len <= 0)
    return;
#if DAGOR_DBGLEVEL > 0
  _x0_ = x0;
  _len2 = len;
#endif
  Texture *tt = texTransp[fmt * 2 + 1];
  for (; len > LINE_TEX_LEN; len -= LINE_TEX_LEN, x0 += LINE_TEX_LEN)
    if (!t->updateSubRegionNoOrder(tt, 0, 0, 0, 0, LINE_TEX_LEN, m, 1, 0, x0, y0, 0))
      upd_ok = false;
  if (!t->updateSubRegionNoOrder(tt, 0, 0, 0, 0, len, m, 1, 0, x0, y0, 0))
    upd_ok = false;
#if DAGOR_DBGLEVEL > 0
  if (!upd_ok)
    logerr("copy_hor_line(%p, %d, %d, %d, %d, 0x%X) failed (tt=%p x0=%d y0=%d len=%d)", t, _x0, _y0, _len, m, fmt, tt, _x0_, _y0,
      _len2);
#endif
  G_UNUSED(upd_ok);
}
static void copy_left_top_margin_cb(void *cb_arg, Texture *tex, const DynamicPicAtlas::ItemData &d, int margin)
{
  AtlasData *ad = (AtlasData *)cb_arg;
  copy_vert_line(tex, d.x0 - margin, d.y0 - margin, d.h + 2 * margin, margin, ad->fmt);
  copy_hor_line(tex, d.x0, d.y0 - margin, d.w + margin, margin, ad->fmt);
  if (ad->isRt)
    d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}
#if DAGOR_DBGLEVEL > 0
static void clear_discarded_cb(void *cb_arg, Texture *tex, const DynamicPicAtlas::ItemData &d, int margin)
{
  AtlasData *ad = (AtlasData *)cb_arg;
  for (int y = d.y0, ye = y + d.h + margin; y < ye; y += margin)
    copy_hor_line(tex, d.x0, y, d.w, min(margin, ye - y), ad->fmt);
  if (ad->isRt)
    d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}
#endif

static PictureRenderFactory *match_render_pic(const char *name, DataBlock &rendPicProps, uint16_t &out_w, uint16_t &out_h)
{
  if (!is_render_pic_name_like(name))
    return NULL;
  if (!dblk::load_text(rendPicProps, make_span_const(name, dd_get_fname_ext(name) - name), dblk::ReadFlag::ROBUST))
  {
    logerr("failed to parse render-pic-BLK from %s", name);
    return NULL;
  }
  int w = 0, h = 0;
  const DataBlock &props = *rendPicProps.getBlockByNameEx("render");
  for (int i = 0; i < picRenderFactory.size(); i++)
    if (picRenderFactory[i]->match(props, w, h))
    {
      out_w = uint16_t(w), out_h = uint16_t(h);
      return picRenderFactory[i];
    }
  return NULL;
}

static bool check_pic_still_valid(PICTUREID pid, const uint8_t &gen, int &x0, int &y0)
{
  WinAutoLock lock(critSec);
  int tex_idx;
  if (const DynamicPicAtlas::ItemData *d = decodePicId(pid, tex_idx, /*update_lru*/ false, &gen))
    // treat image as valid if ST_restoring is setted up in dx field, to support first round of render_pic_with_factory
    if (d->valid() || d->dx == DynamicPicAtlas::ItemData::ST_restoring)
    {
      // dynamic atlas could be rearranged (see reArrangeAndAlloc()), so update actual texture coordinates
      x0 = d->x0;
      y0 = d->y0;
      return true;
    }
  return false;
}
#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
static const char *extract_pic_name(PICTUREID pid, uint8_t gen)
{
  WinAutoLock lock(critSec);
  int texIdx = getTexRecIdx(pid);
  if (texIdx < texRec.size() && gen == texRecGen[texIdx])
    return nameMap.getName(texRec[texIdx].rec->ad->picNameId[getPicRecIdx(pid)]);
  return nullptr;
}
#else
static const char *extract_pic_name(PICTUREID, uint8_t) { return nullptr; }
#endif
static void retry_render_pic_with_factory_imm(void *arg)
{
  PictureRenderContext &ctx = *(PictureRenderContext *)arg;
  if (!AsyncPicLoadJob::loadAllowed)
  {
    delete &ctx;
    return;
  }
  if (ctx.triedAtFrame + 10 > dagor_frame_no())
    return add_delayed_callback_buffered(retry_render_pic_with_factory_imm, arg);

  if (check_pic_still_valid(ctx.pid, ctx.gen, ctx.x0, ctx.y0))
  {
    d3d::GpuAutoLock gpu_lock;
    SCOPE_RENDER_TARGET;
    if (!ctx.prf->render(ctx.rt, ctx.x0, ctx.y0, ctx.w, ctx.h, ctx.props, ctx.pid))
    {
      ctx.triedAtFrame = dagor_frame_no();
      add_delayed_callback_buffered(retry_render_pic_with_factory_imm, arg);
      return;
    }
    debug("PM: render succeed at last for pic=%08X(%s)", ctx.pid, extract_pic_name(ctx.pid, ctx.gen));
  }
  else
    logwarn("PM: pic=%08X(%s) was discarded, render retry ceased", ctx.pid, extract_pic_name(ctx.pid, ctx.gen));
  delete &ctx;
}
static void render_pic_with_factory_imm(PictureRenderContext &ctx)
{
  if (!check_pic_still_valid(ctx.pid, ctx.gen, ctx.x0, ctx.y0) || !AsyncPicLoadJob::loadAllowed)
  {
    logwarn("PM: pic=%08X(%s) was discarded, render retry ceased", ctx.pid, extract_pic_name(ctx.pid, ctx.gen));
    return; // try no more
  }

  d3d::GpuAutoLock gpu_lock;
  SCOPE_RENDER_TARGET;
  if (!ctx.prf->render(ctx.rt, ctx.x0, ctx.y0, ctx.w, ctx.h, ctx.props, ctx.pid))
  {
    debug("PM: render failed for pic=%08X(%s), will retry later", ctx.pid, extract_pic_name(ctx.pid, ctx.gen));
    ctx.triedAtFrame = dagor_frame_no();
    add_delayed_callback_buffered(retry_render_pic_with_factory_imm, new PictureRenderContext(ctx));
  }
}
static void render_pic_with_factory(PictureRenderContext &ctx, WinAutoLock *lock)
{
  if (is_main_thread())
    return render_pic_with_factory_imm(ctx);

  struct RenderAction : public DelayedAction
  {
    PictureManager::PictureRenderContext ctx;
    RenderAction(PictureRenderContext &_ctx) : ctx(_ctx) {}
    virtual void performAction()
    {
      render_pic_with_factory_imm(ctx);
      interlocked_release_store_ptr(pending_render_action, (DelayedAction *)nullptr);
    }
  };
  if (lock)
    lock->unlock();
  G_ASSERT(!interlocked_acquire_load_ptr(pending_render_action));
  DelayedAction *act = new RenderAction(ctx);
  interlocked_release_store_ptr(pending_render_action, act);
  add_delayed_action(act);
  do
  {
    sleep_msec(1);
  } while (interlocked_acquire_load_ptr(pending_render_action));
  if (lock)
    lock->lock();
}
} // namespace PictureManager

void PictureManager::init(const DataBlock *params)
{
  if (inited)
    return;

  if (!params)
    params = &DataBlock::emptyBlock;
  asyncLoadJobMgr = params->getInt("asyncLoadJobMgr", -1);
  asyncLoadJobMgrOwned = false;
  debugAsyncSleepMs = params->getInt("debugAsyncSleep", -1);
  doFatalOnPictureLoadFailed = params->getBool("fatalOnPicLoadFailed", false);
  searchBlkBeforeTaBin = params->getBool("searchBlkBeforeTaBin", true);
  dynAtlasLazyAllocDef = params->getBool("dynAtlasLazyAllocDef", false);

  if (asyncLoadJobMgr == -1 && params->getBool("createAsyncLoadJobMgr", false))
  {
    size_t stackSize = (256 << 10) * (1 + sizeof(int) / sizeof(void *)); // More stack on 32 bit for avif load (dav1d)
    asyncLoadJobMgr = cpujobs::create_virtual_job_manager(stackSize, WORKER_THREADS_AFFINITY_MASK, "PicMgr::asyncLoad");
    asyncLoadJobMgrOwned = true;
  }
  AsyncPicLoadJob::loadAllowed = true;

  TexImage32 image[2];
  image[0].w = image[0].h = 1;
  image[0].getPixels()[0].u = params->getE3dcolor("stubPixelColor", E3DCOLOR(0, 0, 0, 0)).u;
  Texture *onePixelTex = d3d::create_tex(image, 1, 1, TEXCF_RGB | TEXCF_LOADONCE | TEXCF_SYSTEXCOPY, 1, "picMgr$stub");
  d3d_err(onePixelTex);
  substOnePixelTexId = register_managed_tex("picMgr$stub", onePixelTex);
  substOnePixelSmpId = d3d::request_sampler({});

  create_critical_section(critSec);
  for (PictureRenderFactory *f = PictureRenderFactory::tail; f; f = f->next)
    register_pic_render_factory(f);
  inited = true;
}

void PictureManager::prepare_to_release(bool final_term)
{
  if (!inited)
    return;

  AsyncPicLoadJob::loadAllowed = false;
  if (asyncLoadJobMgr >= 0)
  {
    cpujobs::remove_jobs_by_tag(asyncLoadJobMgr, _MAKE4C('PICM'));
    while (interlocked_acquire_load(AsyncPicLoadJob::numJobsInFlight))
    {
      sleep_msec(1);
      perform_delayed_actions();
      cpujobs::release_done_jobs();
    }
  }
  perform_delayed_actions();
  if (!final_term)
  {
    enter_critical_section(critSec);
    for (PictureRenderFactory *f = PictureRenderFactory::tail; f; f = f->next)
      f->clearPendReq();
    leave_critical_section(critSec);
    AsyncPicLoadJob::loadAllowed = true;
    return;
  }

  enter_critical_section(critSec);
  for (PictureRenderFactory *f = PictureRenderFactory::tail; f; f = f->next)
    unregister_pic_render_factory(f);
  clear_and_shrink(picRenderFactory);
  leave_critical_section(critSec);
}

void PictureManager::release()
{
  if (!inited)
    return;
  AsyncPicLoadJob::loadAllowed = false;
  for (PictureRenderFactory *f = PictureRenderFactory::tail; f; f = f->next)
    unregister_pic_render_factory(f);

  if (asyncLoadJobMgr >= 0)
  {
    cpujobs::remove_jobs_by_tag(asyncLoadJobMgr, _MAKE4C('PICM'));
    while (interlocked_acquire_load(AsyncPicLoadJob::numJobsInFlight))
    {
      sleep_msec(1);
      perform_delayed_actions();
      cpujobs::release_done_jobs();
    }
  }
  if (asyncLoadJobMgrOwned)
  {
    cpujobs::destroy_virtual_job_manager(asyncLoadJobMgr, false);
    asyncLoadJobMgrOwned = false;
  }
  asyncLoadJobMgr = -1;
  discard_unused_picture();

  enter_critical_section(critSec);
  for_each_tex_rec([](TexRec &tr, int) { eastl::destroy_at(&tr); });
  clear_and_shrink(texRec);
  decltype(texRecGen)().swap(texRecGen);
  texRecFreeList = -1;
  texRecAlloc.clear();
  decltype(nameId2texRecIdx)().swap(nameId2texRecIdx);
  nameMap.reset(false);
  for (int i = 0; i < sizeof(texTransp) / sizeof(texTransp[0]); i++)
    del_d3dres(texTransp[i]);
  ShaderGlobal::reset_from_vars(substOnePixelTexId);
  while (get_managed_texture_refcount(substOnePixelTexId) > 0)
    release_managed_tex(substOnePixelTexId);
  substOnePixelTexId = BAD_TEXTUREID;
  leave_critical_section(critSec);
  destroy_critical_section(critSec);
  inited = false;
}

void PictureManager::register_pic_render_factory(PictureRenderFactory *f)
{
  G_ASSERT(is_main_thread());
  if (find_value_idx(picRenderFactory, f) >= 0)
    return;
  picRenderFactory.push_back(f);
  f->registered();
}
void PictureManager::unregister_pic_render_factory(PictureRenderFactory *f)
{
  G_ASSERT(is_main_thread());
  if (find_value_idx(picRenderFactory, f) < 0)
    return;
  erase_item_by_value(picRenderFactory, f);
  f->unregistered();
}

void PictureManager::per_frame_update()
{
  for (PictureRenderFactory *f = PictureRenderFactory::tail; f; f = f->next)
    f->updatePerFrame();
}

void PictureManager::before_d3d_reset()
{
  if (!inited)
    return;

  if (asyncLoadJobMgr >= 0 && interlocked_acquire_load(AsyncPicLoadJob::numJobsInFlight))
  {
    debug("PM: before d3d reset, wait for %d async jobs", interlocked_acquire_load(AsyncPicLoadJob::numJobsInFlight));
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
    d3d::driver_command(Drv3dCommand::RELEASE_LOADING, (void *)1); // unlockWrite
    cpujobs::remove_jobs_by_tag(asyncLoadJobMgr, _MAKE4C('PICM'));
    while (interlocked_acquire_load(AsyncPicLoadJob::numJobsInFlight))
    {
      sleep_msec(10);
      cpujobs::release_done_jobs();
      if (DelayedAction *act = interlocked_acquire_load_ptr(pending_render_action))
      {
        logwarn("removing pending RenderAction=%p", act);
        remove_delayed_action(act);
        interlocked_release_store_ptr(pending_render_action, (DelayedAction *)nullptr);
      }
    }
    d3d::driver_command(Drv3dCommand::ACQUIRE_LOADING, (void *)1); // lockWrite
    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  }
}
void PictureManager::after_d3d_reset()
{
  if (!inited)
    return;

  debug("PM: after d3d reset");
  discard_unused_picture();

  WinAutoLock lock(critSec);
  for_each_tex_rec([](TexRec &tr, int texIdx) {
    if (tr.ad && tr.ad->dynamic)
    {
      tr.reinitDynAtlas();
      texRecGen[texIdx]++;
    }
  });
}

PICTUREID PictureManager::add_picture(BaseTexture *tex, const char *as_name, bool get_it)
{
  if (!as_name || !*as_name || !tex)
    return BAD_PICTUREID;

  WinAutoLock lock(critSec);
  int tex_rec_idx = createTexRec(nameMap.addNameId(as_name));
  TexRec &tr = *texRec[tex_rec_idx];
  if (tr.ad)
    return BAD_PICTUREID;
  if (tr.refCount != -1)
    return BAD_PICTUREID;
  tr.refCount = 1;
  tr.texId = register_managed_tex(as_name, tex);
  tr.smpId = d3d::request_sampler({});
  tr.ownedTex = 1;
  G_ASSERTF(tr.texId != BAD_TEXTUREID, "tex=%p as_name=%s", tex, as_name);

  TextureInfo ti;
  if (tex->restype() == RES3D_TEX && tex->getinfo(ti))
    tr.size.set(ti.w, ti.h);
  if (get_it)
    tr.addRef();
  return makePicId(tex_rec_idx, 0);
}

bool PictureManager::get_picture_ex(const char *file_name, PICTUREID &out_pic_id, TEXTUREID &out_tex_id,
  d3d::SamplerHandle &out_smp_id, Point2 *out_tc0, Point2 *out_tc1, Point2 *out_sz, async_load_done_cb_t done_cb, void *cb_arg)
{
  out_pic_id = BAD_PICTUREID;
  out_tex_id = BAD_TEXTUREID;
  out_smp_id = d3d::INVALID_SAMPLER_HANDLE;
  if (!file_name || !*file_name)
    return true;

  WinAutoLock lock(critSec);
  AsyncPicLoadJob *j = nullptr;
  int tex_name_id = -1;
  unsigned pic_hash = 0;
  const char *fn = decodeFileName(file_name, tex_name_id, pic_hash);
  G_ASSERTF(fn && *fn, "file_name=<%s> tex_name_id=%d pic_hash=%08X fn=<%s>", file_name, tex_name_id, pic_hash, fn);
  if (pic_hash)
  {
    // atlas case
    int tex_rec_idx = createTexRec(tex_name_id);
    TexRec &tr = *texRec[tex_rec_idx];
    if (!tr.ad && !tr.initAtlas())
    {
      if (doFatalOnPictureLoadFailed)
        DAG_FATAL("PM: atlas '%s' init failed (for %s)", tr.getName(), file_name);
      return done_cb != NULL;
    }

    if (tr.ad)
    {
      const DynamicPicAtlas::ItemData *d = tr.ad->atlas.findItem(pic_hash, !tr.ad->lazyAlloc);
      if (d && d->valid())
      {
        out_pic_id = makePicId(tex_rec_idx, tr.ad->atlas.getItemIdx(d), tr.ad->dynamic);
        out_tex_id = tr.texId;
        out_smp_id = tr.smpId;
        if (out_tc0)
          out_tc0->set(d->u0, d->v0);
        if (out_tc1)
          out_tc1->set(d->u1, d->v1);
        if (out_sz)
          *out_sz = tr.picSz(d);

        tr.addRef();
        PICMGR_DEBUG("PM: get_picture_ex(%08X) refCount(%s)=%d{%d}", out_pic_id, file_name, tr.refCount,
          get_managed_texture_refcount(tr.texId));
        return true;
      }
      if (!tr.ad->dynamic)
      {
        if (tr.ad->missingPicHash.addInt(pic_hash))
        {
          logerr("PM: missing picture '%s' in static atlas '%s'", file_name, tr.getName());
          if (doFatalOnPictureLoadFailed)
            DAG_FATAL("PM: missing picture '%s' in static atlas '%s' (%d pics missing)", file_name, tr.getName(),
              tr.ad->missingPicHash.size());
        }
        return done_cb != NULL; // missing picture in atlas
      }

      if (!d)
      {
        d = tr.ad->atlas.addDirectItemOnly(pic_hash, 0, 0, 0, 0, true);
        G_ASSERTF(tr.ad->atlas.getItemIdx(d) < 0x3FFF, "pic=%d for '%s'", tr.ad->atlas.getItemIdx(d), tr.getName());

        int idx = tr.ad->atlas.getItemIdx(d);
        if (idx == tr.ad->picNameId.size())
          tr.ad->picNameId.push_back(nameMap.addNameId(fn));
        else
        {
          while (tr.ad->picNameId.size() <= idx)
            tr.ad->picNameId.push_back(-1);
          tr.ad->picNameId[idx] = nameMap.addNameId(fn);
        }
      }
      out_pic_id = makePicId(tex_rec_idx, tr.ad->atlas.getItemIdx(d), true);
      out_tex_id = BAD_TEXTUREID;
      out_smp_id = d3d::INVALID_SAMPLER_HANDLE;

      j = new AsyncPicLoadJob(AsyncPicLoadJob::JT_picInAtlas, out_pic_id, tr.ad->makeAbsFn(fn), out_tc0, out_tc1, out_sz, done_cb,
        cb_arg);
      j->premulAlpha = tr.ad->premultiplyAlpha;
      tr.addRef();
      if (!done_cb || asyncLoadJobMgr < 0)
      {
        lock.unlockFinal();
        const auto res = j->doJobSync();
        out_tex_id = res.first;
        out_smp_id = res.second;
        delete j;
        return true;
      }
      j->skipAtlasPic = tr.ad->lazyAlloc;
    }
  }
  else
  {
    // separate picture case
    if (missingTexNameId.hasInt(tex_name_id))
      return done_cb != NULL;
    int tex_rec_idx = createTexRec(tex_name_id);
    TexRec &tr = *texRec[tex_rec_idx];
    if (tr.ad)
    {
      logerr("PM: cannot get atlas '%s' as picture '%s'", tr.getName(), file_name);
      return done_cb != NULL;
    }
    out_pic_id = makePicId(tex_rec_idx, 0);
    out_tex_id = tr.texId;
    out_smp_id = tr.smpId;
    if (tr.refCount != -1 && (tr.texId != BAD_TEXTUREID || tr.size.y > 0))
    {
      tr.addRef();

      if (out_tc0)
        out_tc0->set(0, 0);
      if (out_tc1)
        out_tc1->set(1, 1);
      if (out_sz)
        *out_sz = tr.size;
      PICMGR_DEBUG("PM: get_picture_ex(%08X) refCount(%s)=%d{%d}", out_pic_id, file_name, tr.refCount,
        get_managed_texture_refcount(tr.texId));
      return true;
    }

    if (tr.refCount == -1)
    {
      tr.refCount = 0;
      tr.addPendingTexRef();
    }
    TEXTUREID tex_id = tr.resolveTexId();
    bool add_quard_ref = get_managed_texture_refcount(tex_id) > 0 && D3dResManagerData::getD3dRes(tex_id);
    if (add_quard_ref) // texture is loaded, do sync
    {
#if DAGOR_DBGLEVEL > 0
      int64_t reft = profile_ref_ticks();
      acquire_managed_tex(tex_id);
      if (done_cb && profile_time_usec(reft) > 40) // we expect it lightning fast, it should do only addRef
        logerr("PM: acquire_managed_tex(%s) took %d usec, rc=%d", get_managed_texture_name(tex_id), profile_time_usec(reft),
          get_managed_texture_refcount(tex_id));
#else
      acquire_managed_tex(tex_id);
#endif
      done_cb = NULL;
    }

    j = new AsyncPicLoadJob(AsyncPicLoadJob::JT_texPic, out_pic_id, file_name, out_tc0, out_tc1, out_sz, done_cb, cb_arg);
    if (!done_cb || asyncLoadJobMgr < 0 || tr.texId != BAD_TEXTUREID)
    {
      lock.unlockFinal();
      const auto res = j->doJobSync();
      out_tex_id = res.first;
      out_smp_id = res.second;
      delete j;
      if (add_quard_ref)
        release_managed_tex(tex_id);
      return true;
    }
    G_ASSERT(!add_quard_ref);
  }

  lock.unlockFinal();
  G_VERIFY(cpujobs::add_job(asyncLoadJobMgr, j));
  out_pic_id = BAD_PICTUREID;
  acquire_managed_tex(out_tex_id = substOnePixelTexId);
  out_smp_id = substOnePixelSmpId;

  return false;
}

TEXTUREID PictureManager::get_picture_data(PICTUREID pid, Point2 &out_lt, Point2 &out_rb)
{
  WinAutoLock lock(critSec);
  int tex_idx = -1;
  if (const DynamicPicAtlas::ItemData *d = decodePicId(pid, tex_idx, true))
  {
    if (d->valid() || reloadDiscardedPic(pid, const_cast<DynamicPicAtlas::ItemData *>(d)))
    {
      out_lt.set(d->u0, d->v0);
      out_rb.set(d->u1, d->v1);
    }
    else
    {
      out_lt.set(0, 0);
      out_rb.set(0, 0);
    }
    return texRec[tex_idx]->texId;
  }
  if (tex_idx == -1)
  {
    out_lt = out_rb = Point2(0, 0);
    return BAD_TEXTUREID;
  }
  out_lt.set(0, 0);
  out_rb.set(1, 1);
  return texRec[tex_idx]->texId;
}
bool PictureManager::get_picture_size(PICTUREID pid, Point2 &out_total_texsz, Point2 &out_picsz)
{
  WinAutoLock lock(critSec);
  int tex_idx = -1;
  if (const DynamicPicAtlas::ItemData *d = decodePicId(pid, tex_idx))
  {
    TexRec &tr = *texRec[tex_idx];
    out_total_texsz = tr.size;
    out_picsz = tr.picSz(d);
    if (d->valid())
      reloadDiscardedPic(pid, const_cast<DynamicPicAtlas::ItemData *>(d));
    return true;
  }
  if (tex_idx == -1)
  {
    out_total_texsz = out_picsz = Point2(1, 1);
    return false;
  }
  out_total_texsz = out_picsz = texRec[tex_idx]->getSize();
  return true;
}
Point2 PictureManager::get_picture_pix_size(PICTUREID pid)
{
  WinAutoLock lock(critSec);
  int tex_idx = -1;
  if (const DynamicPicAtlas::ItemData *d = decodePicId(pid, tex_idx))
  {
    if (d->valid())
      reloadDiscardedPic(pid, const_cast<DynamicPicAtlas::ItemData *>(d));
    return texRec[tex_idx]->picSz(d);
  }
  if (tex_idx == -1)
    return Point2(0, 0);
  return texRec[tex_idx]->getSize();
}
void PictureManager::free_picture(PICTUREID pid)
{
  if (pid <= BAD_PICTUREID)
    return;

  WinAutoLock lock(critSec);
  int tex_idx = getTexRecIdx(pid);
  if (tex_idx < texRec.size() && texRec[tex_idx].isFreeSlot())
  {
    logerr("PM: free_picture(%08X) for already discarded rec", pid);
    return;
  }
  if (tex_idx < texRec.size() && texRec[tex_idx]->refCount > 0)
  {
    TexRec &tr = *texRec[tex_idx];
    G_ASSERT_LOG(tr.refCount > 1, "refCount(%s)=%d{%d} pid=%08X", tr.getName(), tr.refCount, get_managed_texture_refcount(tr.texId),
      pid);
    tr.delRef();
    PICMGR_DEBUG("PM: free_picture(%08X) refCount(%s)=%d{%d}", pid, tr.getName(), tr.refCount, get_managed_texture_refcount(tr.texId));
  }
  else
    logerr("PM: free_picture(%08X) failed, texRec.size()=%d refCount=%d", pid, texRec.size(),
      tex_idx < texRec.size() ? texRec[tex_idx]->refCount : -1);
}
bool PictureManager::discard_unused_picture(bool force_lock)
{
  if (!inited)
    return false;

  if (DAGOR_LIKELY(try_enter_critical_section(critSec)))
    ;
  else if (!force_lock && !last_discard_skipped)
  {
    int spins = 1024;
    for (; spins > 0 && !try_enter_critical_section(critSec); --spins)
      cpu_yield();
    if (!spins)
    {
      last_discard_skipped = true;
      return false;
    }
  }
  else
    enter_critical_section(critSec);
  last_discard_skipped = false;

  for_each_tex_rec([](TexRec &tr, int i) {
    if (tr.refCount > 1 || (tr.ad && tr.ad->missingPicHash.size()) || (!tr.ad && tr.texId == BAD_TEXTUREID && tr.size.y < 1))
      return;
    else
    {
      PICMGR_DEBUG("PM: discard_unused_picture(%s) pic=xxxx%04X%s", tr.getName(), i, tr.ownedTex ? " owned" : "");
      G_VERIFY(nameId2texRecIdx.erase(tr.nameId));
      eastl::destroy_at(&tr);
      texRecAlloc.freeOneBlock(&tr);
      texRecGen[i]++;
      texRec[i].nextFreeIdxX2 = (texRecFreeList << 1) | 1;
      texRecFreeList = i;
    }
  });

  leave_critical_section(critSec);
  return true;
}
void PictureManager::add_ref_picture(PICTUREID pid)
{
  if (pid <= BAD_PICTUREID)
    return;

  WinAutoLock lock(critSec);
  int tex_idx = getTexRecIdx(pid);
  if (tex_idx < texRec.size())
  {
    TexRec &tr = *texRec[tex_idx];
    tr.addRef();
    PICMGR_DEBUG("PM: add_ref_picture(%08X) refCount(%s)=%d{%d}", pid, tr.getName(), tr.refCount,
      get_managed_texture_refcount(tr.texId));
  }
  else
    logerr("PM: add_ref_picture(%08X) failed, texRec.size()=%d refCount=%d", pid, texRec.size(),
      tex_idx < texRec.size() ? texRec[tex_idx]->refCount : -1);
}
void PictureManager::get_picture(const char *file_name, PicDesc &out_pic)
{
  if (!PictureManager::get_picture_ex(file_name, out_pic.pic, out_pic.tex, out_pic.smp, &out_pic.tcLt, &out_pic.tcRb))
  {
    PICMGR_DEBUG("PM: get_picture(%s) failed", file_name);
    out_pic.pic = BAD_PICTUREID;
    out_pic.tex = BAD_TEXTUREID;
    out_pic.smp = d3d::INVALID_SAMPLER_HANDLE;
  }
}

bool PictureManager::load_tex_atlas_data(const char *ta_fn, DataBlock &out_ta_blk, String &out_ta_tex_fn)
{
  WinAutoLock lock(critSec);
  return readAtlasDesc(ta_fn, out_ta_blk, out_ta_tex_fn);
}
bool PictureManager::is_picture_in_dynamic_atlas(const char *file_name, bool *out_premul_alpha)
{
  WinAutoLock lock(critSec);
  int tex_name_id = decodeAtlasNameId(file_name);
  if (tex_name_id < 0) // not pic in atlas
    return false;

  auto it = nameId2texRecIdx.find(tex_name_id);
  int tex_rec_idx = (it != nameId2texRecIdx.end()) ? it->second : -1;
  if (tex_rec_idx < 0)
    return false;
  TexRec &tr = *texRec[tex_rec_idx];
  if (!tr.ad)
    return false;
  if (out_premul_alpha)
    *out_premul_alpha = tr.ad->premultiplyAlpha;
  return tr.ad->dynamic;
}

static void get_effective_dyn_atlas_sizes(const DataBlock &texBlk, IPoint2 &sz, IPoint2 &max_pic_sz, const char *file_name)
{
  sz = texBlk.getIPoint2("size", IPoint2(0, 0));
  max_pic_sz = texBlk.getIPoint2("maxAllowedPicSz", PictureManager::defMaxAllowedPicSz);
  if (int ref_hres = texBlk.getInt("refHRes", 0))
  {
    enum
    {
      SZ_ALIGN = 32,
      MAXPICSZ_ALIGN = 16
    };
    int scrw = 0, scrh = 0;
    d3d::get_screen_size(scrw, scrh);
    if (scrh > ref_hres)
    {
      if (is_pow_of2(sz.x) && is_pow_of2(sz.y) && !texBlk.getBool("allowNonPow2sz", false))
      {
        float area0 = float(sz.x) * sz.y, scl = float(scrh) / ref_hres;
        sz.x = get_bigger_pow2(sz.x * scrh / ref_hres);
        sz.y = get_bigger_pow2(sz.y * scrh / ref_hres);
        if (float(sz.x) * sz.y >= area0 * 1.9f * scl * scl)
          sz.y /= 2;
      }
      else
      {
        sz.x = (sz.x * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
        sz.y = (sz.y * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
      }

      max_pic_sz.x = (max_pic_sz.x * scrh / ref_hres + MAXPICSZ_ALIGN - 1) / MAXPICSZ_ALIGN * MAXPICSZ_ALIGN;
      max_pic_sz.y = (max_pic_sz.y * scrh / ref_hres + MAXPICSZ_ALIGN - 1) / MAXPICSZ_ALIGN * MAXPICSZ_ALIGN;
      debug("PM: dynAtlas %s: uses dynamic sz=%d,%d and max_pic_sz=%d,%d due to refHRes:i=%d and screen size %dx%d", file_name, sz.x,
        sz.y, max_pic_sz.x, max_pic_sz.y, ref_hres, scrw, scrh);
    }
  }
  G_UNUSED(file_name);
}
bool PictureManager::TexRec::initAtlas()
{
  if (refCount != -1)
    return false;
  if (ad)
    return true;

  TIME_PROFILE(picmgr_initAtlas);

  ad = new AtlasData;
  refCount = 0;

  const char *file_name = nameMap.getName(nameId);
  DA_PROFILE_TAG(picmgr_initAtlas, file_name);

  String validTexName;
  if (!readAtlasDesc(file_name, ad->blk, validTexName))
    return false;

  const DataBlock &texBlk = *ad->blk.getBlockByNameEx("tex");

  if (texBlk.getBool("dynAtlas", false))
  {
    if (get_managed_texture_id(validTexName) != BAD_TEXTUREID)
    {
      logerr("PM: texture %s already registered, failed to use it for dynamic atlas '%s'", validTexName, file_name);
      return false;
    }
    IPoint2 sz, max_pic_sz;
    get_effective_dyn_atlas_sizes(texBlk, sz, max_pic_sz, file_name);
    const char *fmt_str = texBlk.getBlockByNameEx(get_platform_string_id())->getStr("format", texBlk.getStr("format", "ARGB"));

    int tex_fmt = TEXFMT_A8R8G8B8;
    if (strcmp(fmt_str, "ARGB") == 0)
      tex_fmt = TEXFMT_A8R8G8B8;
    else if (strcmp(fmt_str, "L8") == 0)
      tex_fmt = TEXFMT_L8;
    else if (strcmp(fmt_str, "DXT5") == 0)
      tex_fmt = TEXFMT_DXT5;
    else if (strcmp(fmt_str, "DXT1") == 0)
      tex_fmt = TEXFMT_DXT1;
    else if (strcmp(fmt_str, "RGBA") == 0)
      tex_fmt = TEXFMT_R8G8B8A8;
    else
      logerr("PM: unrecognized format:t=\"%s\" for dynamic atlas '%s'", fmt_str, file_name);

    if ((d3d::get_texformat_usage(tex_fmt) & d3d::USAGE_TEXTURE) != d3d::USAGE_TEXTURE)
    {
      debug("PM: unsupported format:t=\"%s\" for dynamic atlas '%s', fallback to RGBA", fmt_str, file_name);
      tex_fmt = TEXFMT_R8G8B8A8;
      fmt_str = "RGBA";
    }

    int margin = texBlk.getInt("margin", 1);
    if (tex_fmt == TEXFMT_A8R8G8B8)
      margin = clamp(margin, 0, (int)MAX_MARGIN_WD), ad->fmt = ad->FMT_ARGB;
    else if (tex_fmt == TEXFMT_R8G8B8A8)
      margin = clamp(margin, 0, (int)MAX_MARGIN_WD), ad->fmt = ad->FMT_RGBA;
    else if (tex_fmt == TEXFMT_L8)
      margin = clamp(margin, 0, (int)MAX_MARGIN_WD), ad->fmt = ad->FMT_L8;
    else if (tex_fmt == TEXFMT_DXT5)
      margin = margin ? 4 : 0, ad->fmt = ad->FMT_DXT5;
    else if (tex_fmt == TEXFMT_DXT1)
      margin = 0, ad->fmt = ad->FMT_none;

    size.set(sz.x, sz.y);
    ad->dynamic = true;
    ad->texFmt = tex_fmt;
    ad->premultiplyAlpha = texBlk.getBool("premultiplyAlpha", true);
    ad->lazyAlloc = texBlk.getBool("lazyAlloc", dynAtlasLazyAllocDef);
    ad->picSrcFolder = texBlk.getStr("picSrcFolderAbs", NULL);
    ad->maxAllowedPicSz = max_pic_sz;
    if (texBlk.getBool("rtTex", false))
    {
      tex_fmt |= TEXCF_RTARGET;
      ad->isRt = true;
    }
    else
    {
      tex_fmt |= TEXCF_UPDATE_DESTINATION;
    }

    ad->atlas.init(sz, texBlk.getInt("picReserve", 16), margin, texBlk.getStr("name", NULL),
      tex_fmt | TEXCF_CLEAR_ON_CREATE | TEXCF_LINEAR_LAYOUT);
    bool bc_format = ad->fmt == ad->FMT_DXT5 || ad->fmt == ad->FMT_none;
    G_UNUSED(bc_format);
    G_ASSERT(!bc_format || ad->atlas.getMargin() == margin);
    G_ASSERT(!bc_format || ad->atlas.getCornerResvSz() == 4);
    G_ASSERT(!bc_format || ad->atlas.getMarginLtOfs() == 0);
    texId = ad->atlas.tex.first.getId();
    smpId = ad->atlas.tex.second;
    if (ad->maxAllowedPicSz.x > sz.x - ad->atlas.getMargin())
      ad->maxAllowedPicSz.x = sz.x - ad->atlas.getMargin();
    if (ad->maxAllowedPicSz.y > sz.y - ad->atlas.getMargin())
      ad->maxAllowedPicSz.y = sz.y - ad->atlas.getMargin();
    G_ASSERTF(texId != BAD_TEXTUREID, "'%s'", texBlk.getStr("name", NULL));
    addRef();
    debug("PM: initAtlas(%s, %s) dynamic %dx%d refCount=%d{%d}, maxPic=%dx%d fmt=%s(%s) %s", getName(), validTexName.c_str(), sz.x,
      sz.y, refCount, get_managed_texture_refcount(texId), ad->maxAllowedPicSz.x, ad->maxAllowedPicSz.y, fmt_str,
      ad->premultiplyAlpha ? "premul.alpha" : "non-premul.alpha", ad->lazyAlloc ? "[lazy alloc]" : "[regular]");

    init_transp_tex(ad->fmt);
    copy_hor_line(ad->atlas.tex.first.getTex2D(), 0, 0, ad->atlas.getCornerResvSz(), ad->atlas.getCornerResvSz(), ad->fmt);
    if (ad->isRt)
      d3d::resource_barrier({ad->atlas.tex.first.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    ad->atlas.copy_left_top_margin_cb = copy_left_top_margin_cb;
    ad->atlas.copy_left_top_margin_cb_arg = ad;
#if DAGOR_DBGLEVEL > 0
    const DataBlock *settings = dgs_get_settings();
    if (
      settings && settings->getBlockByNameEx("debug")->getBool("clearDiscardedImagesInAtlas", !d3d::get_driver_code().is(d3d::vulkan)))
      ad->atlas.clear_discarded_cb = clear_discarded_cb;
#endif
    return true;
  }

  size.set(texBlk.getReal("wd", 0.0f), texBlk.getReal("ht", 0.0f));
  if (size.x <= 0.0f || size.y <= 0.0f)
  {
    logerr("PM: bad texture size=%d,%d for atlas '%s'", size.x, size.y, file_name);
    return false;
  }

  ad->atlas.init(IPoint2(size.x, size.y), texBlk.blockCount(), 0, NULL, TEXFMT_DEFAULT | TEXCF_UPDATE_DESTINATION);
  texId = get_managed_texture_id(validTexName); // already exist?
  if (texId == BAD_TEXTUREID)
    texId = add_managed_texture(validTexName);
  smpId = get_texture_separate_sampler(texId);
  refCount = 0;
  G_ASSERTF(texId != BAD_TEXTUREID, "'%s'", validTexName);

  // read pictures
  String picName;
  for (int j = 0, picNid = texBlk.getNameId("pic"), stripeNid = texBlk.getNameId("stripe"); j < texBlk.blockCount(); j++)
  {
    const DataBlock &picBlk = *texBlk.getBlock(j);

    if ((picBlk.getBlockNameId() != picNid) && (picBlk.getBlockNameId() != stripeNid))
      continue;

    // common parameters
    int left = (int)picBlk.getReal("left", 0.0f);
    int top = (int)picBlk.getReal("top", 0.0f);
    int w = (int)picBlk.getReal("wd", 0.0f);
    int h = (int)picBlk.getReal("ht", 0.0f);

    if (picBlk.getBlockNameId() == picNid)
    {
      // load single picture
      const char *name = picBlk.getStr("name", NULL);
      if (!name || !*name)
      {
        debug("invalid name for picture #%d in atlas '%s'", j + 1, file_name);
        continue;
      }
      picName.printf(0, "%s#%s", file_name, name);
      DynamicPicAtlas::ItemData *d =
        const_cast<DynamicPicAtlas::ItemData *>(ad->atlas.addDirectItemOnly(PictureManager::str_hash_fnv1(picName), left, top, w, h));

      if (int transf = picBlk.getInt("transf", 0))
      {
        float u0 = d->u0, v0 = d->v0, u1 = d->u1, v1 = d->v1;
        if (transf == 1)
          d->u0 = u1, d->u1 = u0;
        else if (transf == 2)
          d->v0 = v1, d->v1 = v0;
        else if (transf == 3)
          d->u0 = u1, d->u1 = u0, d->v0 = v1, d->v1 = v0;
      }
    }
    else if (picBlk.getBlockNameId() == stripeNid)
    {
      // load line of pictures
      const int count = picBlk.getInt("count", 0);
      const char *prefix = picBlk.getStr("prefix", NULL);
      const int baseNumber = picBlk.getInt("base", 0);

      if (count == 0)
      {
        logerr("PM: invalid count for picture #%d in atlas '%s'", j + 1, file_name);
        continue;
      }
      if (!prefix || !*prefix)
      {
        logerr("PM: invalid prefix for picture #%d in atlas '%s'", j + 1, file_name);
        continue;
      }

      for (int p = 0; p < count; p++)
      {
        picName.printf(0, "%s#%s%d", file_name, prefix, baseNumber + p);
        ad->atlas.addDirectItemOnly(PictureManager::str_hash_fnv1(picName), left, top, w, h);
        left += w;
      }
    }
  }

  addRef();
  PICMGR_DEBUG("PM: initAtlas(%s) static %dx%d (%d pics) refCount=%d{%d}", getName(), (int)size.x, (int)size.y,
    ad->atlas.getItemsCount(), refCount, get_managed_texture_refcount(texId));
  G_ASSERTF(ad->atlas.getItemsCount() < 0x3FFF, "%d pics for '%s'", ad->atlas.getItemsCount(), getName());
  return true;
}
bool PictureManager::TexRec::reinitDynAtlas()
{
  if (!ad)
    return false;
  PICMGR_DEBUG("PM: discard %d items of dynamic atlas '%s'", ad->atlas.getItemsUsed(), getName());
  ad->atlas.discardAllItems();

  const char *file_name = nameMap.getName(nameId);
  String validTexName;
  if (!readAtlasDesc(file_name, ad->blk, validTexName))
    return false;

  const DataBlock &texBlk = *ad->blk.getBlockByNameEx("tex");
  if (!texBlk.getBool("dynAtlas", false))
    return false;
  if (texBlk.getInt("refHRes", 0) == 0) // is not screen-resolution dependant
    return true;

  IPoint2 sz, max_pic_sz;
  get_effective_dyn_atlas_sizes(texBlk, sz, max_pic_sz, file_name);
  int tex_fmt = ad->texFmt | TEXCF_LINEAR_LAYOUT | TEXCF_CLEAR_ON_CREATE |
                (texBlk.getBool("rtTex", false) ? TEXCF_RTARGET : TEXCF_UPDATE_DESTINATION);
  ad->maxAllowedPicSz = max_pic_sz;
  size.set(sz.x, sz.y);
  ad->atlas.init(sz, 0, ad->atlas.getMargin(), nullptr, tex_fmt);

  BaseTexture *old_tex = ad->atlas.tex.first.getTex();
  BaseTexture *new_tex = d3d::create_tex(NULL, sz.x, sz.y, tex_fmt, 1, texBlk.getStr("name", NULL));
  change_managed_texture(ad->atlas.tex.first.getId(), new_tex);
  ad->atlas.tex.first.setRaw(new_tex, ad->atlas.tex.first.getId());
  del_d3dres(old_tex);
  if (ad->isRt)
    d3d::resource_barrier({ad->atlas.tex.first.getTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (ad->maxAllowedPicSz.x > sz.x - ad->atlas.getMargin())
    ad->maxAllowedPicSz.x = sz.x - ad->atlas.getMargin();
  if (ad->maxAllowedPicSz.y > sz.y - ad->atlas.getMargin())
    ad->maxAllowedPicSz.y = sz.y - ad->atlas.getMargin();
  PICMGR_DEBUG("PM: dynamic atlas '%s' reinited sz=%@ maxAllowedPicSz=%@", getName(), sz, ad->maxAllowedPicSz);
  return true;
}

void PictureManager::AsyncPicLoadJob::loadPicInAtlas()
{
  PICMGR_DEBUG("PM: %s load (pic in atlas) '%s', pic=%08X", done_cb ? "ASYNC" : "sync", name, picId);
  reportErr = 0;
  {
    WinAutoLock lock(critSec);
    int tex_idx;
    DynamicPicAtlas::ItemData *d = const_cast<DynamicPicAtlas::ItemData *>(decodePicId(picId, tex_idx));
    if (!d || tex_idx == -1 || !texRec[tex_idx]->ad)
    {
      logerr("PM: load failed, name='%s', pic=%08X, d=%p tex_rec_idx=%d ad=%p", name, picId, d, tex_idx,
        tex_idx >= 0 ? texRec[tex_idx]->ad : NULL);
      if (doFatalOnPictureLoadFailed && is_main_thread())
        DAG_FATAL("PM: internal error: pic=%08X (%s)", picId, name);
      else if (doFatalOnPictureLoadFailed)
        reportErr = 11;
      return;
    }
    if (d->valid())
    {
      // already loaded in earlier jobs
      TexRec &tr = *texRec[tex_idx];
      finalizePic(d, tr);
      PICMGR_DEBUG("PM: refCount(%s)=%d{%d} (already loaded),  %d pending load jobs", tr.getName(), tr.refCount,
        get_managed_texture_refcount(tr.texId), interlocked_acquire_load(numJobsInFlight));
      return;
    }
  }

  eastl::unique_ptr<BaseTexture, DestroyDeleter<BaseTexture>> pic_tex;
  DataBlock rendPicProps;
  TextureInfo ti;
  ti.w = ti.h = 0;
  PictureRenderFactory *prf = match_render_pic(name, rendPicProps, ti.w, ti.h);
  if (!prf)
  {
    TextureMetaData tmd;
    String stor;
    if (const char *dec_name = tmd.decode(name, &stor))
    {
      if (tmd.flags & tmd.FLG_OVERRIDE)
        tmd.flags &= ~tmd.FLG_OVERRIDE; // do not propagate this flag to create_texture_via_factories
      else if (premulAlpha)
        tmd.flags |= tmd.FLG_PREMUL_A;
      else
        tmd.flags &= ~tmd.FLG_PREMUL_A;
      pic_tex.reset(create_texture_via_factories(dec_name, TEXCF_SYSMEM | TEXCF_DYNAMIC | TEXCF_LINEAR_LAYOUT, 1,
        dd_get_fname_ext(dec_name), tmd, NULL));
    }
  }

  if (debugAsyncSleepMs > 0 && done_cb)
    sleep_msec(debugAsyncSleepMs);

  WinAutoLock lock(critSec);
  int tex_idx;
  DynamicPicAtlas::ItemData *d = const_cast<DynamicPicAtlas::ItemData *>(decodePicId(picId, tex_idx));
  if (!d || tex_idx == -1 || !texRec[tex_idx]->ad)
  {
    logerr("PM: load failed, name='%s', pic=%08X, d=%p tex_rec_idx=%d ad=%p", name, picId, d, tex_idx,
      tex_idx >= 0 ? texRec[tex_idx]->ad : NULL);
    if (doFatalOnPictureLoadFailed && is_main_thread())
      DAG_FATAL("PM: internal error: pic=%08X (%s)", picId, name);
    else if (doFatalOnPictureLoadFailed)
      reportErr = 12;
    return;
  }

  TexRec &tr = *texRec[tex_idx];
  if (!pic_tex && !prf)
  {
    bool new_missing = tr.ad->missingPicHash.addInt(d->hash);
    if (new_missing && !df_is_fname_url_like(name))
      logerr("PM: texture load failed, name='%s', pic=%08X", name, picId);
    finalizePic(d, tr);
    d->dx = 0; // prevent futher load attemps
    if (new_missing && doFatalOnPictureLoadFailed && is_main_thread())
      DAG_FATAL("PM: load failed, name='%s', pic=%08X (%d pics missing)", name, picId, tr.ad->missingPicHash.size());
    else if (new_missing && doFatalOnPictureLoadFailed)
      reportErr = 13;
    return;
  }

  if (!prf && pic_tex)
  {
    if (pic_tex->restype() != RES3D_TEX || !pic_tex.get()->getinfo(ti))
      ti.w = ti.h = ti.cflg = 0;
    if ((ti.cflg & TEXFMT_MASK) != tr.ad->texFmt)
    {
      logerr("PM: texture format %08X != atlas format %08X, cannot copy data, name='%s', pic=%08X", ti.cflg & TEXFMT_MASK,
        tr.ad->texFmt, name, picId);
      finalizePic(d, tr);
      d->dx = 0; // prevent futher load attemps
      return;
    }
  }

  unsigned pic_hash = d->hash;

  if (ti.w > tr.ad->maxAllowedPicSz.x || ti.h > tr.ad->maxAllowedPicSz.y)
  {
    logerr("PM: pic '%s' sz=%dx%d is too big for dynamic atlas '%s' %dx%d, maxPicSz=%dx%d", name, ti.w, ti.h, tr.getName(),
      tr.ad->atlas.getSz().x, tr.ad->atlas.getSz().y, tr.ad->maxAllowedPicSz.x, tr.ad->maxAllowedPicSz.y);
    finalizePic(d, tr);
    d->dx = 0; // prevent futher load attemps
    G_ASSERTF(0, "PM: pic '%s' sz=%dx%d is too big for dynamic atlas '%s' %dx%d, maxPicSz=%dx%d", name, ti.w, ti.h, tr.getName(),
      tr.ad->atlas.getSz().x, tr.ad->atlas.getSz().y, tr.ad->maxAllowedPicSz.x, tr.ad->maxAllowedPicSz.y);
    return;
  }

  d->hash = 0;
  if (!skipAtlasPic && tr.ad->atlas.addItem(0, 0, ti.w, ti.h, pic_hash, tr.ad->atlas.getItemIdx(d)) == d)
  {
    debug("PM: add(%s) tex=%p prf=%p %dx%d -> %s[%d] at %d,%d", name, pic_tex.get(), prf, ti.w, ti.h, tr.getName(),
      tr.ad->atlas.getItemIdx(d), d->x0, d->y0);
    G_ASSERTF(tr.ad->atlas.getItemIdx(d) < 0x3FFF, "pic=%d for '%s'", tr.ad->atlas.getItemIdx(d), tr.getName());

    if (prf)
    {
      // mark image as not valid, to avoid rendering it to screen from uninitialized rect
      d->dx = DynamicPicAtlas::ItemData::ST_restoring;

      uint8_t gen = texRecGen[tex_idx];
      PictureRenderContext ctx(prf, tr.ad->atlas.tex.first.getTex2D(), d->x0, d->y0, ti.w, ti.h,
        *rendPicProps.getBlockByNameEx("render"), picId, gen);
      render_pic_with_factory(ctx, &lock);
      //< atlas data may be reallocated during unlock/lock in previous render_pic_with_factory() call
      d = const_cast<DynamicPicAtlas::ItemData *>(decodePicId(picId, tex_idx, /*update_lru*/ false, &gen));
      if (!d) // very unlikely
      {
        logerr("PM: pic '%s' %dx%d was removed during render_pic_with_factory() call", name, ti.w, ti.h);
        return;
      }
      // check that d->dx is not changed to something else
      if (d->dx == DynamicPicAtlas::ItemData::ST_restoring)
        // restore image dx field, to make it vaild and available for rendering
        d->dx = 0;
    }
    else
    {
      bool upd_ok = tr.ad->atlas.tex.first.getTex2D()->updateSubRegion(pic_tex.get(), 0, 0, 0, 0, ti.w, ti.h, 1, 0, d->x0, d->y0, 0);
#if DAGOR_DBGLEVEL > 0
      if (!upd_ok)
        logerr("PM: failed to copy pic '%s' %dx%d to atlas '%s' at (%d,%d)", name, ti.w, ti.h, tr.getName(), d->x0, d->y0);
#endif
      G_UNUSED(upd_ok);
      if (tr.ad->isRt)
        d3d::resource_barrier({tr.ad->atlas.tex.first.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (int margin = tr.ad->atlas.getMargin())
    {
      copy_vert_line(tr.ad->atlas.tex.first.getTex2D(), d->x0 - margin, d->y0 - margin, d->h + 2 * margin, margin, tr.ad->fmt);
      copy_vert_line(tr.ad->atlas.tex.first.getTex2D(), d->x0 + d->w, d->y0 - margin, d->h + 2 * margin, margin, tr.ad->fmt);
      copy_hor_line(tr.ad->atlas.tex.first.getTex2D(), d->x0, d->y0 - margin, d->w, margin, tr.ad->fmt);
      copy_hor_line(tr.ad->atlas.tex.first.getTex2D(), d->x0, d->y0 + d->h, d->w, margin, tr.ad->fmt);
      if (tr.ad->isRt)
        d3d::resource_barrier({tr.ad->atlas.tex.first.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (char *p = strstr(name, "@@"))
      if (strchr("hvs", p[2]) && p[3] == 's')
      {
        float tc_xofs = tr.ad->atlas.getInvW(), tc_yofs = tr.ad->atlas.getInvH();
        if (p[2] == 'h' || p[2] == 's')
          d->u0 += tc_xofs, d->u1 -= tc_xofs;
        if (p[2] == 'v' || p[2] == 's')
          d->v0 += tc_yofs, d->v1 -= tc_yofs;
      }

    finalizePic(d, tr);
    PICMGR_DEBUG("PM: refCount(%s)=%d{%d},  %d pending load jobs", tr.getName(), tr.refCount, get_managed_texture_refcount(tr.texId),
      interlocked_acquire_load(numJobsInFlight));
  }
  else
  {
    d->hash = pic_hash;
    d->w = ti.w;
    d->h = ti.h;
    d->discard();
    finalizePic(d, tr);
    if (!skipAtlasPic)
      logwarn("PM: failed to alloc item #%d hash=%08X sz=%dx%d, name='%s', pic=%08X", tr.ad->atlas.getItemIdx(d), pic_hash, ti.w, ti.h,
        name, picId);
  }
}
void PictureManager::AsyncPicLoadJob::loadTexPic()
{
  PICMGR_DEBUG("PM: %s load (texture pic) '%s', pic=%08X", done_cb ? "ASYNC" : "sync", name, picId);
  reportErr = 0;
  outTexId = get_managed_texture_id(name);
  if (outTexId == BAD_TEXTUREID)
    outTexId = add_managed_texture(name);
  smpId = get_texture_separate_sampler(outTexId);
  if (smpId == d3d::INVALID_SAMPLER_HANDLE)
    smpId = d3d::request_sampler({});

  BaseTexture *tex = acquire_managed_tex(outTexId);
  TextureInfo ti;
  if (!tex || tex->restype() != RES3D_TEX || !tex->getinfo(ti))
    ti.w = ti.h = 0;

  if (debugAsyncSleepMs > 0 && done_cb)
    sleep_msec(debugAsyncSleepMs);
  if (const char *name_e = strchr(name, '*'))
  {
    size_t hash_len = name_e - name;
    bool is_cdn_hash_name = (hash_len >= 33 + 1 && hash_len <= 33 + 8 && name[32] == '-' && !memchr(name + 33, '.', hash_len - 33));
    if (is_cdn_hash_name || is_main_thread())
      prefetch_managed_texture(outTexId);
    else
    {
      // load actual data for textures from DxP
      if (!is_managed_textures_streaming_load_on_demand())
        ddsx::tex_pack2_perform_delayed_data_loading();

      const int64_t reft = ref_time_ticks();
      const unsigned f = dagor_frame_no();
      int max_frame_dur_usec = 100000; // 100 ms
#if DAGOR_THREAD_SANITIZER
      max_frame_dur_usec *= 10;
#endif
      while (!prefetch_and_check_managed_textures_loaded(make_span_const(&outTexId, 1), false))
      {
        sleep_msec(2);
        if (dagor_frame_no() < f + 2 && get_time_usec(reft) > max_frame_dur_usec * 2)
        {
          logwarn("%s: timeout=%d usec passed, waiting for 1 tex, frame=%d (started at %d)\n"
                  "tex(%s) id=0x%x refc=%d loaded=%d ldLev=%d (maxReqLev=%d lfu=%d, curQL=%d)",
            __FUNCTION__, get_time_usec(reft), dagor_frame_no(), f, get_managed_texture_name(outTexId), outTexId,
            get_managed_texture_refcount(outTexId), check_managed_texture_loaded(outTexId, false),
            get_managed_res_loaded_lev(outTexId), get_managed_res_maxreq_lev(outTexId), get_managed_res_lfu(outTexId),
            get_managed_res_cur_tql(outTexId));

          reportErr = 101;
          break; // cannot load texture now, dead-lock is detected, will try next time
        }
      }
    }
  }

  WinAutoLock lock(critSec);
  int tex_idx;
  DynamicPicAtlas::ItemData *d = const_cast<DynamicPicAtlas::ItemData *>(decodePicId(picId, tex_idx));
  if (d || tex_idx == -1)
  {
    logerr("PM: load failed, name='%s', pic=%08X, tex_rec_idx=%d d=%p", name, picId, tex_idx, d);
    release_managed_tex(outTexId);
    outTexId = BAD_TEXTUREID;
    if (doFatalOnPictureLoadFailed && is_main_thread())
      DAG_FATAL("PM: internal error: pic=%08X (%s)", picId, name);
    else if (doFatalOnPictureLoadFailed)
      reportErr = 21;
    return;
  }

  TexRec &tr = *texRec[tex_idx];
  if (!tex)
  {
    bool new_missing = missingTexNameId.addInt(tr.nameId);
    if (new_missing && !df_is_fname_url_like(name))
      logerr("PM: texture load failed, name='%s', pic=%08X", name, picId);

    tr.addPendingTexRef();
    tr.size.y = 1;
    outTc0->set(0, 0);
    outTc1->set(0, 0);
    outSz->set(1, 1);

    if (new_missing && doFatalOnPictureLoadFailed && is_main_thread())
      DAG_FATAL("PM: load failed, name='%s', pic=%08X texId=0x%x", name, picId, outTexId);
    else if (new_missing && doFatalOnPictureLoadFailed)
      reportErr = 22;
    outTexId = BAD_TEXTUREID;
    return;
  }

  if (tr.texId == outTexId)
  {
    // already loaded in earlier jobs
    outTc0->set(0, 0);
    outTc1->set(1, 1);
    outSz->set(ti.w, ti.h);
    tr.addRef();
    release_managed_tex(outTexId); // release texture since we are not the first who acquired it!
    jobDone = true;
    interlocked_decrement(numJobsInFlight);
    PICMGR_DEBUG("PM: refCount(%s)=%d{%d} (already loaded),  %d pending load jobs", name, tr.refCount,
      get_managed_texture_refcount(tr.texId), interlocked_acquire_load(numJobsInFlight));
    return;
  }

  PICMGR_DEBUG("PM: loaded(%s) texId=%d %dx%d", name, outTexId, ti.w, ti.h);
  if (int add_texref = tr.getPendingTexRef()) // get this before trashing tr.size.x where counter is stored
  {
    PICMGR_DEBUG("PM: loaded(%s) add %d pending references for texId=%d", name, add_texref, outTexId);
    interlocked_increment(tr.refCount); // convert one pending ref (added in PictureManager::get_picture_ex) to real ref
    for (int i = 1; i < add_texref; i++)
      acquire_managed_tex(outTexId);
  }

  tr.size.set(ti.w, ti.h);
  outTc0->set(0, 0);
  outTc1->set(1, 1);
  outSz->set(ti.w, ti.h);
  tr.texId = outTexId;
  tr.smpId = smpId;
  tr.addRef();
  jobDone = true;
  interlocked_decrement(numJobsInFlight);
  PICMGR_DEBUG("PM: refCount(%s)=%d{%d},  %d pending load jobs", name, tr.refCount, get_managed_texture_refcount(tr.texId),
    interlocked_acquire_load(numJobsInFlight));
}
void PictureManager::AsyncPicLoadJob::discardUndonePic()
{
  int tex_idx;
  if (DynamicPicAtlas::ItemData *d = const_cast<DynamicPicAtlas::ItemData *>(decodePicId(picId, tex_idx)))
    if (!d->valid())
      d->discard();
}
void PictureManager::AsyncPicLoadJob::finalizePic(DynamicPicAtlas::ItemData *d, TexRec &tr)
{
  outTexId = tr.texId;
  smpId = tr.smpId;
  if (d->valid())
  {
    outTc0->set(d->u0, d->v0);
    outTc1->set(d->u1, d->v1);
  }
  else
  {
    outTc0->set(0, 0);
    outTc1->set(0, 0);
  }
  *outSz = tr.picSz(d);

  jobDone = true;
  interlocked_decrement(numJobsInFlight);
}
