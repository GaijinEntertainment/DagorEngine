#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <zlib.h>
#include <ioSys/dag_zstdIo.h>
#include <generic/dag_tab.h>
#include <hash/md5.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <supp/dag_starForce.h>
#include <memory/dag_mem.h>
#include <stddef.h> // offsetof
#include <supp/dag_zstdObfuscate.h>

#define SIGNATURE_MAX_SIZE (1024)

#if _TARGET_PC
static const unsigned targetCode = _MAKE4C('PC');
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_IOS | _TARGET_TVOS
static const unsigned targetCode = _MAKE4C('iOS');
#elif _TARGET_ANDROID
static const unsigned targetCode = _MAKE4C('and');
#elif _TARGET_C3

#elif _TARGET_XBOX
static const unsigned targetCode = _MAKE4C('Xb1');
#else
!error: unsupported target!
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
static const unsigned targetCode2[2] = {_MAKE4C('PC'), targetCode};
#elif _TARGET_PC
static const unsigned targetCode2[2] = {_MAKE4C('iOS'), _MAKE4C('and')};
#elif _TARGET_ANDROID
static const unsigned targetCode2[2] = {_MAKE4C('PC'), _MAKE4C('iOS')};
#elif _TARGET_IOS | _TARGET_TVOS
static const unsigned targetCode2[2] = {_MAKE4C('PC'), _MAKE4C('and')};
#elif _TARGET_C3

#else
static const unsigned targetCode2[2] = {targetCode, targetCode};
#endif
static inline bool checkTargetCode(unsigned code) { return targetCode == code || targetCode2[0] == code || targetCode2[1] == code; }

static void dump_buffers(const char *fname, const void **buffers, const unsigned *buf_sizes, int count)
{
  md5_state_t s;
  md5_byte_t d[16];

  md5_init(&s);
  for (int i = 0; i < count; ++i)
    if (buffers[i])
      md5_append(&s, (const unsigned char *)buffers[i], buf_sizes[i]);
  md5_finish(&s, d);

  debug("loaded vromfs dump <%s> "
        "MD5=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
    fname, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
  (void)fname;
}

static inline void dump_file_hash(const char *fname, const VirtualRomFsDataHdr *hdr, const void *data, unsigned sz)
{
  const void *buffers[] = {hdr, data};
  unsigned buf_sizes[] = {(unsigned)sizeof(*hdr), sz};
  dump_buffers(fname, buffers, buf_sizes, countof(buffers));
}

#define FS_OFFS offsetof(VirtualRomFsData, files)

static VirtualRomFsData *patch_fs(VirtualRomFsData *fs, void *data_offs = NULL)
{
  void *base = (char *)fs + FS_OFFS;
  fs->files.patchData(base);

  // detect index-only dumps
  if (fs->files.map.size() && !((uint32_t *)&fs->data)[1])
  {
    uint32_t data_tab[4];
    G_STATIC_ASSERT(sizeof(fs->data) == sizeof(data_tab));

    memcpy(data_tab, (void *)&fs->data, sizeof(fs->data));
    data_tab[1] = fs->files.map.size();
    memcpy((void *)&fs->data, data_tab, sizeof(fs->data));

    fs->data.patch(base);
    for (int i = 0, ie = fs->data.size(); i < ie; i++)
      fs->data[i].patch(NULL);
    fs->data.init(fs->data.data(), 0);
    return fs;
  }

  fs->data.patch(base);
  for (int i = 0, ie = fs->data.size(); i < ie; i++)
  {
    fs->data[i].patch(data_offs ? data_offs : base);
    if (!fs->data[i].size())
      fs->data[i].init((char *)NULL + 1, 0);
  }
  return fs;
}

VirtualRomFsData *load_vromfs_dump_from_mem(dag::ConstSpan<char> data, IMemAlloc *mem)
{
  if (data_size(data) < sizeof(VirtualRomFsDataHdr))
    return NULL;

  const VirtualRomFsDataHdr *hdr = (const VirtualRomFsDataHdr *)data.data();
  int err = 0;

  if (hdr->label != _MAKE4C('VRFs') && hdr->label != _MAKE4C('VRFx'))
    return NULL;
  if (!checkTargetCode(hdr->target))
    return NULL;

  VirtualRomFsData *fs = (VirtualRomFsData *)mem->tryAlloc(FS_OFFS + hdr->fullSz);
  if (!fs)
    return NULL;
  new (fs, _NEW_INPLACE) VirtualRomFsData();

  int vrom_hdr_sz = sizeof(VirtualRomFsDataHdr);
  if (hdr->label == _MAKE4C('VRFx'))
  {
    const VirtualRomFsExtHdr *hdr_ext = (const VirtualRomFsExtHdr *)(data.data() + vrom_hdr_sz);
    if (hdr_ext->size >= sizeof(VirtualRomFsExtHdr))
    {
      fs->flags = hdr_ext->flags;
      fs->version = hdr_ext->version;
    }
    vrom_hdr_sz += hdr_ext->size;
  }
  unsigned char *buf = (((unsigned char *)data.data()) + vrom_hdr_sz);

  bool hash_ok = true;
  if (hdr->packedSz())
  {
    dump_file_hash("mem://", hdr, buf, hdr->packedSz());
    if (hash_ok)
    {
      unsigned long sz = hdr->fullSz;
      if (hdr->zstdPacked())
      {
        Tab<unsigned char> buf1;
        buf1.resize(data_size(data) - vrom_hdr_sz);
        mem_copy_from(buf1, buf);
        buf = buf1.data();

        DEOBFUSCATE_ZSTD_DATA(buf, hdr->packedSz());
        sz = (int)zstd_decompress((unsigned char *)fs + FS_OFFS, sz, buf, hdr->packedSz());
        if (sz != hdr->fullSz)
          goto load_fail;
      }
      else
      {
        err = uncompress((unsigned char *)fs + FS_OFFS, &sz, buf, hdr->packedSz());
        G_ASSERT(err == Z_OK);
        if (err != Z_OK)
          goto load_fail;
      }
      G_ASSERT(hdr->fullSz == sz);
    }
  }
  else
  {
    dump_file_hash("mem://", hdr, buf, hdr->fullSz);
    memcpy((char *)fs + FS_OFFS, buf, hdr->fullSz);
  }

  if (!hash_ok)
  {
  load_fail:
    mem->free(fs);
    return NULL;
  }

  return patch_fs(fs);
}

VirtualRomFsData *make_non_intrusive_vromfs(dag::ConstSpan<char> dump, IMemAlloc *mem, unsigned *out_hdr_sz, int *out_signature_ofs)
{
  const VirtualRomFsDataHdr *hdr = reinterpret_cast<const VirtualRomFsDataHdr *>(dump.data());
  const VirtualRomFsExtHdr *hdr_ext = nullptr;
  if (hdr->label != _MAKE4C('VRFs') && hdr->label != _MAKE4C('VRFx'))
    return nullptr;
  if (hdr->packedSz())
    return nullptr;

  size_t vrom_hdr_sz = sizeof(VirtualRomFsDataHdr);
  if (hdr->label == _MAKE4C('VRFx'))
  {
    hdr_ext = reinterpret_cast<const VirtualRomFsExtHdr *>(hdr + 1);
    vrom_hdr_sz += hdr_ext->size;
    if (hdr_ext->size < sizeof(VirtualRomFsExtHdr))
      hdr_ext = nullptr;
  }
  G_ASSERTF_RETURN(vrom_hdr_sz <= data_size(dump), nullptr, "vrom_hdr_sz=%d dump=%p,%d", vrom_hdr_sz, dump.data(), data_size(dump));
  const char *fs_data_start = vrom_hdr_sz + (const char *)dump.data();
  VirtualRomFsData fs_copy;
  memcpy(&fs_copy.files, fs_data_start, sizeof(fs_copy) - FS_OFFS); //-V780
  fs_copy.data.patch(nullptr);

  vrom_hdr_sz += data_size(fs_copy.data) + (intptr_t)fs_copy.data.data();
  if (out_hdr_sz)
    *out_hdr_sz = (unsigned)vrom_hdr_sz;
  if (out_signature_ofs)
    *out_signature_ofs = dump.size() > hdr->fullSz ? hdr->fullSz : -1;

  VirtualRomFsData *fs = new (mem->tryAlloc(FS_OFFS + vrom_hdr_sz), _NEW_INPLACE) VirtualRomFsData;
  if (!fs)
    return nullptr;
  memcpy(&fs->files, fs_data_start, vrom_hdr_sz); //-V780
  if (hdr_ext)
  {
    fs->flags = hdr_ext->flags;
    fs->version = hdr_ext->version;
  }
  return patch_fs(fs, const_cast<char *>(fs_data_start));
}

bool get_vromfs_dump_digest(const char *fname, unsigned char *out_md5_digest)
{
  VirtualRomFsDataHdr hdr;
  file_ptr_t fp = df_open(fname, DF_READ);

  if (!fp || df_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr))
  {
  load_fail:
    if (fp)
      df_close(fp);
    return false;
  }

  if (hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx'))
    goto load_fail;
  if (!checkTargetCode(hdr.target))
    goto load_fail;
  if (df_seek_rel(fp, hdr.packedSz() ? hdr.packedSz() : hdr.fullSz) != 0)
    goto load_fail;
  if (df_read(fp, out_md5_digest, 16) != 16)
    goto load_fail;
  df_close(fp);
  return true;
}

uint32_t get_vromfs_dump_version(const char *fname)
{
  VirtualRomFsDataHdr hdr;
  file_ptr_t fp = df_open(fname, DF_READ);

  if (!fp || df_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr))
  {
  load_fail:
    if (fp)
      df_close(fp);
    return 0;
  }

  if (hdr.label != _MAKE4C('VRFx'))
    goto load_fail;
  if (!checkTargetCode(hdr.target))
    goto load_fail;
  VirtualRomFsExtHdr hdr_ext;
  if (df_read(fp, &hdr_ext, sizeof(hdr_ext)) != sizeof(hdr_ext))
    goto load_fail;
  if (hdr_ext.size < sizeof(hdr_ext))
    goto load_fail;
  df_close(fp);
  return hdr_ext.version;
}

static file_ptr_t open_vrom_fp(const char *fname, int file_flags, DagorStat &st)
{
  file_ptr_t fp = df_open(fname, DF_READ | file_flags);
  if (df_fstat(fp, &st) != 0)
    st.mtime = -1;
  return fp;
}

VirtualRomFsData *load_vromfs_dump(const char *fname, IMemAlloc *mem, verify_signature_cb sigcb,
  const dag::ConstSpan<uint8_t> *to_verify, int file_flags)
{
  debug("%s <%s>", __FUNCTION__, fname);

  VirtualRomFsDataHdr hdr;
  VirtualRomFsData *fs = NULL;
  DagorStat st;
  int err = 0;
  char embedded_md5[16];
  unsigned char signature[SIGNATURE_MAX_SIZE];
  int signature_size = 0;
  enum
  {
    HDR,
    CONTENT,
    MD5,
    ADDITIONAL_CONTENT,
    SIGNATURE
  };
  const void *buffers[] = {&hdr, NULL, embedded_md5, to_verify ? to_verify->data() : NULL, signature};
  unsigned buf_sizes[] = {sizeof(hdr), 0, sizeof(embedded_md5), to_verify ? (unsigned)to_verify->size() : 0, 0};
  void *buf = NULL;
  file_ptr_t fp = open_vrom_fp(fname, file_flags, st);
  if (!fp || df_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr))
    goto load_fail;
  if (hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx'))
    goto load_fail;
  if (!checkTargetCode(hdr.target))
    goto load_fail;

  fs = (VirtualRomFsData *)mem->tryAlloc(FS_OFFS + hdr.fullSz);
  if (!fs)
    goto load_fail;
  new (fs, _NEW_INPLACE) VirtualRomFsData();
  fs->mtime = st.mtime;

  if (hdr.label == _MAKE4C('VRFx'))
  {
    VirtualRomFsExtHdr hdr_ext;
    df_read(fp, &hdr_ext, sizeof(hdr_ext));
    if (hdr_ext.size >= sizeof(VirtualRomFsExtHdr))
    {
      fs->flags = hdr_ext.flags;
      fs->version = hdr_ext.version;
    }
    df_seek_rel(fp, hdr_ext.size - sizeof(hdr_ext));
  }

  if (hdr.packedSz())
  {
    buf = mem->tryAlloc(hdr.packedSz());
    if (!buf)
      goto load_fail;
    if (df_read(fp, buf, hdr.packedSz()) != hdr.packedSz())
      goto load_fail;

    unsigned long sz = hdr.fullSz;
    if (hdr.zstdPacked())
    {
      DEOBFUSCATE_ZSTD_DATA(buf, hdr.packedSz());
      sz = (int)zstd_decompress((unsigned char *)fs + FS_OFFS, sz, buf, hdr.packedSz());
      if (sz != hdr.fullSz)
        goto load_fail;
      OBFUSCATE_ZSTD_DATA(buf, hdr.packedSz());
    }
    else
    {
      err = uncompress((unsigned char *)fs + FS_OFFS, &sz, (unsigned char *)buf, hdr.packedSz());
      G_ASSERT(err == Z_OK);
      if (err == Z_OK)
      {
        hdr.fullSz = sz;
        buffers[CONTENT] = buf;
        buf_sizes[CONTENT] = hdr.packedSz();
      }
      else
        goto load_fail;
    }
  }
  else
  {
    if (df_read(fp, (char *)fs + FS_OFFS, hdr.fullSz) != hdr.fullSz)
      goto load_fail;
    buffers[CONTENT] = (char *)fs + FS_OFFS;
    buf_sizes[CONTENT] = hdr.fullSz;
  }

  if (df_read(fp, embedded_md5, sizeof(embedded_md5)) != sizeof(embedded_md5))
    buffers[MD5] = NULL;

  signature_size = buffers[MD5] ? df_read(fp, signature, sizeof(signature)) : 0;
  if (signature_size == 0)
    buffers[SIGNATURE] = NULL;
  else
    buf_sizes[SIGNATURE] = signature_size;

  if (buffers[SIGNATURE] && sigcb)
  {
    if (hdr.signedContents())
    {
      const void *tmpBuf[] = {(char *)fs + FS_OFFS, buffers[ADDITIONAL_CONTENT], NULL};
      unsigned bsizes[] = {hdr.fullSz, buf_sizes[ADDITIONAL_CONTENT], 0};
      if (!sigcb(tmpBuf, bsizes, signature, signature_size))
        goto load_fail;
    }
    else
    {
      buffers[SIGNATURE] = NULL; // don't verify signature itself
      if (!sigcb(buffers, buf_sizes, signature, signature_size))
        goto load_fail;
      buffers[SIGNATURE] = signature;
    }
  }
  else
  {
    if (sigcb && !sigcb(NULL, NULL, NULL, 0)) // empty signature allowed?
      goto load_fail;
    if (buffers[MD5]) // if md5 exist
    {
      md5_state_t s;
      md5_byte_t d[16];

      md5_init(&s);
      md5_append(&s, (const unsigned char *)fs + FS_OFFS, hdr.fullSz);
      md5_finish(&s, d);
      if (memcmp(d, embedded_md5, sizeof(embedded_md5)) != 0)
        goto load_fail;
    }
  }

  // dumps hash of WHOLE file as it is on disk (md5sum of file)
  buffers[ADDITIONAL_CONTENT] = NULL; // additonal content isn't part of file
  dump_buffers(fname, buffers, buf_sizes, countof(buffers));

  if (buf)
    mem->free(buf);
  df_close(fp);

  return patch_fs(fs);

load_fail:
  if (fs)
    memfree(fs, mem);
  if (fp)
    df_close(fp);
  if (buf)
    mem->free(buf);
  return NULL;
}

VirtualRomFsData *load_crypted_vromfs_dump(const char *fname, IMemAlloc *mem)
{
  (void)(fname);
  (void)(mem);
  return NULL; // not supported
}

struct VirtualRomFsPackHdr
{
  RoNameMap files;
  PatchableTab<PatchableTab<const char>> data;

  int hdrSz;
  int _resv;
  PATCHABLE_DATA64(char *, fpath);
};

VirtualRomFsPack *open_vromfs_pack(const char *fname, IMemAlloc *mem, int file_flags)
{
  debug("%s <%s>", __FUNCTION__, fname);

  struct Hdr
  {
    VirtualRomFsDataHdr file;
    VirtualRomFsExtHdr ext;
    VirtualRomFsPackHdr pack;
  } hdr;
  DagorStat st;
  const char *fpath = NULL;
  int fpathLen = 0;
  VirtualRomFsPack *fs = NULL;

  file_ptr_t fp = open_vrom_fp(fname, file_flags, st);
  if (!fp || df_read(fp, &hdr.file, sizeof(hdr.file)) != sizeof(hdr.file))
    goto load_fail;
  if (hdr.file.label != _MAKE4C('VRFs') && hdr.file.label != _MAKE4C('VRFx'))
    goto load_fail;
  if (!checkTargetCode(hdr.file.target))
    goto load_fail;

  if (hdr.file.label == _MAKE4C('VRFx'))
  {
    if (df_read(fp, &hdr.ext, sizeof(hdr.ext)) != sizeof(hdr.ext))
      goto load_fail;
    if (hdr.ext.size < sizeof(hdr.ext))
      goto load_fail;
    df_seek_rel(fp, hdr.ext.size - sizeof(hdr.ext));
  }
  if (df_read(fp, &hdr.pack, sizeof(hdr.pack)) != sizeof(hdr.pack))
    goto load_fail;
  if (hdr.file.packedSz() || ptrdiff_t(hdr.pack.files.map.data()) < sizeof(hdr.pack))
    goto load_fail;

  fpath = df_get_real_name(fname);
  fpathLen = (int)strlen(fpath);

  fs = (VirtualRomFsPack *)mem->tryAlloc(FS_OFFS + hdr.pack.hdrSz + fpathLen + 1);
  if (!fs)
    goto load_fail;
  new (fs, _NEW_INPLACE) VirtualRomFsPack();
  fs->flags = hdr.ext.flags;
  fs->version = hdr.ext.version;
  df_seek_to(fp, sizeof(hdr.file) + hdr.ext.size);
  if (df_read(fp, (char *)fs + FS_OFFS, hdr.pack.hdrSz) != hdr.pack.hdrSz)
    goto load_fail;
  fs->mtime = st.mtime;

  patch_fs(fs, (char *)NULL + sizeof(hdr.file) + hdr.ext.size);

  fs->_resv = 0;
  fs->ptr = fs->ptr ? (char *)fs + FS_OFFS + ptrdiff_t(fs->ptr) : (char *)NULL + 1;

  G_ASSERTF((char *)fs + FS_OFFS + hdr.pack.hdrSz == fs->getFilePath(), "fs=%p getFilePath()=%p inlinePtr=%p", fs, fs->getFilePath(),
    (char *)fs + FS_OFFS + hdr.pack.hdrSz);
  if (char *fn_dest = (char *)fs->getFilePath())
    memcpy(fn_dest, fpath, fpathLen + 1);

  df_close(fp);
  G_ASSERT(fs->isValid());
  return fs;

load_fail:
  if (!(file_flags & DF_IGNORE_MISSING))
    logerr("%s <%s>  failed", __FUNCTION__, fname);
  if (fs)
    memfree(fs, mem);
  if (fp)
    df_close(fp);
  return NULL;
}
void close_vromfs_pack(VirtualRomFsPack *fs, IMemAlloc *mem)
{
  if (!fs || !static_cast<VirtualRomFsPack *>(fs)->isValid())
    return;
  if (fs->_resv == 'BCKD')
  {
    if (VirtualRomFsPack::BackedData *bd = fs->getBackedData())
    {
      VirtualRomFsSingleFile *p = bd->resolvedFilesLinkedList;
      while (p)
      {
        VirtualRomFsSingleFile *n = p->nextPtr;
        p->release();
        p = n;
      }
      bd->resolvedFilesLinkedList = NULL;

      close_vromfs_pack(bd->cache, mem);
      bd->~BackedData();
    }
    memfree(fs->ptr, mem);
  }
  memfree(fs, mem);
}

VirtualRomFsSingleFile *VirtualRomFsSingleFile::make_file_link(IMemAlloc *mem, const char *dest_fn, int base_ofs, int data_len,
  const char *rel_fn)
{
  int dest_fn_lenz = i_strlen(dest_fn) + 1, rel_fn_lenz = rel_fn ? i_strlen(rel_fn) + 1 : 0;

  VirtualRomFsSingleFile *fs = (VirtualRomFsSingleFile *)mem->alloc(
    sizeof(VirtualRomFsSingleFile) + sizeof(PatchableTab<const char>) + sizeof(PatchablePtr<const char>) + dest_fn_lenz + rel_fn_lenz);

  PatchableTab<const char> *inl_data = (PatchableTab<const char> *)(fs + 1);
  PatchablePtr<const char> *inl_name = (PatchablePtr<const char> *)(inl_data + 1);
  char *inl_packfn = (char *)(inl_name + 1);
  char *inl_relfn = inl_packfn + dest_fn_lenz;

  fs->firstPriority = true;
  fs->mtime = -1;
  fs->files.map.init(inl_name, 1);
  fs->data.init(inl_data, 1);
  fs->_resv = 0;
  fs->ptr = (char *)NULL + 1;
  fs->hdrSz = inl_packfn - (char *)&fs->files;
  fs->nextPtr = NULL;
  fs->mem = mem;

  inl_data->init((void *)intptr_t(base_ofs ? base_ofs : -1), data_len);
  memcpy(inl_packfn, dest_fn, dest_fn_lenz);
  if (rel_fn)
  {
    memcpy(inl_relfn, rel_fn, rel_fn_lenz);
    dd_strlwr(inl_relfn);
    dd_simplify_fname_c(inl_relfn);
    inl_name->setPtr(inl_relfn);
  }
  else
    inl_name->setPtr(NULL);

  return fs;
}

VirtualRomFsSingleFile *VirtualRomFsSingleFile::make_mem_data(IMemAlloc *mem, void *data, int data_len, const char *rel_fn,
  bool copy_data)
{
  int rel_fn_lenz = i_strlen(rel_fn) + 1;

  VirtualRomFsSingleFile *fs =
    (VirtualRomFsSingleFile *)mem->alloc(sizeof(VirtualRomFsSingleFile) + sizeof(PatchableTab<const char>) +
                                         sizeof(PatchablePtr<const char>) + rel_fn_lenz + (copy_data ? data_len : 0));

  PatchableTab<const char> *inl_data = (PatchableTab<const char> *)(fs + 1);
  PatchablePtr<const char> *inl_name = (PatchablePtr<const char> *)(inl_data + 1);
  char *inl_relfn = (char *)(inl_name + 1);
  char *inl_data_p = inl_relfn + rel_fn_lenz;

  fs->firstPriority = true;
  fs->mtime = -1;
  fs->files.map.init(inl_name, 1);
  fs->data.init(inl_data, 1);
  fs->_resv = 0;
  fs->ptr = 0;
  fs->hdrSz = inl_data_p - (char *)&fs->files;
  fs->nextPtr = NULL;
  fs->mem = mem;

  inl_data->init(copy_data ? inl_data_p : data, data_len);
  memcpy(inl_relfn, rel_fn, rel_fn_lenz);
  dd_strlwr(inl_relfn);
  dd_simplify_fname_c(inl_relfn);
  inl_name->setPtr(inl_relfn);
  if (copy_data)
    memcpy(inl_data_p, data, data_len);

  return fs;
}

void VirtualRomFsSingleFile::release()
{
  if (mem)
    mem->free(this);
}

#if _TARGET_ANDROID
#include <supp/dag_android_native_app_glue.h>
#include <android/asset_manager.h>
#include <osApiWrappers/dag_progGlobals.h>

bool android_get_asset_data(const char *asset_nm, Tab<char> &out_storage)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return false;

  if (AAsset *a = AAssetManager_open(state->activity->assetManager, asset_nm, AASSET_MODE_BUFFER))
  {
    out_storage.resize(AAsset_getLength(a));
    mem_copy_from(out_storage, AAsset_getBuffer(a));
    AAsset_close(a);
    return true;
  }
  return false;
}
VirtualRomFsData *load_vromfs_from_asset(const char *asset_nm, IMemAlloc *mem = tmpmem)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return NULL;

  if (AAsset *a = AAssetManager_open(state->activity->assetManager, asset_nm, AASSET_MODE_BUFFER))
  {
    VirtualRomFsData *vrom = load_vromfs_dump_from_mem(make_span((const char *)AAsset_getBuffer(a), AAsset_getLength(a)), mem);
    AAsset_close(a);
    return vrom;
  }
  return NULL;
}
#endif

#define EXPORT_PULL dll_pull_iosys_vromfsLoad
#include <supp/exportPull.h>
