// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <zlib.h>
#include <ioSys/dag_zstdIo.h>
#include <../ioSys/dataBlock/blk_shared.h>
#include <generic/dag_tab.h>
#include <hash/md5.h>
#include <hash/BLAKE3/blake3.h>
#include <util/dag_globDef.h>
#include <util/dag_strUtil.h>
#include <debug/dag_debug.h>
#include <memory/dag_mem.h>
#include <stddef.h> // offsetof
#include <supp/dag_zstdObfuscate.h>
#include <util/dag_unpatchedRoNameMap.h>

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

static inline void dump_file_hash(const char *fname, const void *data, unsigned sz)
{
  const void *buffers[] = {data};
  unsigned buf_sizes[] = {sz};
  dump_buffers(fname, buffers, buf_sizes, countof(buffers));
}

#define FS_OFFS offsetof(VirtualRomFsData, files)

static void patch_fs(VirtualRomFsData *fs, void *data_offs = NULL)
{
  void *base = (char *)fs + FS_OFFS;
  fs->files.patchData(base);

  // detect index-only dumps
  if (fs->files.map.size() && !((uint32_t *)&fs->data)[1])
  {
    uint32_t data_tab[4];
    G_STATIC_ASSERT(sizeof(fs->data) == sizeof(data_tab));

    memcpy(data_tab, (void *)&fs->data, sizeof(fs->data)); //-V512
    data_tab[1] = fs->files.map.size();
    memcpy((void *)&fs->data, data_tab, sizeof(fs->data)); //-V512

    fs->data.patch(base);
    for (int i = 0, ie = fs->data.size(); i < ie; i++)
      fs->data[i].patch(NULL);
    fs->data.init(fs->data.data(), 0);
    return;
  }

  fs->data.patch(base);
  for (int i = 0, ie = fs->data.size(); i < ie; i++)
  {
    fs->data[i].patch(data_offs ? data_offs : base);
    if (!fs->data[i].size())
      fs->data[i].init((char *)NULL + 1, 0);
  }
}

bool init_vromfs_file_attr(VirtualRomFsFileAttributes &fsAttr, const VromfsDumpBodySections &bodySections)
{
  if (!bodySections.attributes.data() || !bodySections.attributes.size())
    return true;
  uint8_t *attrData = (uint8_t *)bodySections.attributes.data();
  uint8_t attrVersion = attrData[0];
  if (attrVersion == 1)
  {
    G_STATIC_ASSERT(sizeof(EVirtualRomFsFileAttributes) == 1);
    int totalSize = bodySections.filesCount + 1;
    if (totalSize > bodySections.attributes.size())
      return false;
    fsAttr.version = attrVersion;
    fsAttr.attrData = (EVirtualRomFsFileAttributes *)(attrData + 1);
  }
  return true;
}

static bool init_patched_fs_file_attr(VirtualRomFsData *fs, const char *fs_data, unsigned full_sz, int fs_attr_ofs)
{
  // the FS needs to be already patched, we need to know fs->data.size()
  if (fs_attr_ofs < 0)
    return false;
  if (!fs_attr_ofs)
    return true;
  int potentialMaxSize = full_sz - fs_attr_ofs;
  if (potentialMaxSize < 1)
    return false;
  uint8_t *attrData = (uint8_t *)fs_data + fs_attr_ofs;
  uint8_t attrVersion = attrData[0];
  if (attrVersion == 1)
  {
    G_STATIC_ASSERT(sizeof(EVirtualRomFsFileAttributes) == 1);
    int totalSize = fs->data.size() + 1;
    if (totalSize > potentialMaxSize)
      return false;
    fs->attr.version = attrVersion;
    fs->attr.attrData = (EVirtualRomFsFileAttributes *)(attrData + 1);
  }
  return true;
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

  int fs_attr_ofs = 0;
  int vrom_hdr_sz = sizeof(VirtualRomFsDataHdr);
  if (hdr->label == _MAKE4C('VRFx'))
  {
    const VirtualRomFsExtHdr *hdr_ext = (const VirtualRomFsExtHdr *)(data.data() + vrom_hdr_sz);
    if (hdr_ext->size >= sizeof(VirtualRomFsExtHdr))
    {
      fs->flags = hdr_ext->flags;
      fs->version = hdr_ext->version;
      if (fs->flags & EVFSEF_HAVE_FILE_ATTRIBUTES)
        fs_attr_ofs = *(const int *)(const char *)(hdr_ext + 1);
    }
    vrom_hdr_sz += hdr_ext->size;
  }
  unsigned char *buf = (((unsigned char *)data.data()) + vrom_hdr_sz);

  dump_file_hash("mem://", data.data(), data.size());
  if (hdr->packedSz())
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
  else
  {
    memcpy((char *)fs + FS_OFFS, buf, hdr->fullSz);
  }

  patch_fs(fs);

  if (!init_patched_fs_file_attr(fs, (char *)fs + FS_OFFS, hdr->fullSz, fs_attr_ofs))
    goto load_fail;

  return fs;

load_fail:
  mem->free(fs);
  return NULL;
}

bool dissect_vromfs_dump(dag::ConstSpan<char> dump, VromfsDumpSections &result)
{
  if (dump.size() < sizeof(VirtualRomFsDataHdr))
    return false;

  const VirtualRomFsDataHdr *hdr = reinterpret_cast<const VirtualRomFsDataHdr *>(dump.data());
  if (hdr->label != _MAKE4C('VRFs') && hdr->label != _MAKE4C('VRFx'))
    return false;
  result.header = make_span_const((uint8_t *)dump.data(), sizeof(*hdr));
  result.isPacked = hdr->packedSz() > 0;

  unsigned consumed = sizeof(*hdr);

  const VirtualRomFsExtHdr *hdr_ext = nullptr;
  if (hdr->label == _MAKE4C('VRFx'))
  {
    if (dump.size() < consumed + sizeof(VirtualRomFsExtHdr))
      return false;
    hdr_ext = reinterpret_cast<const VirtualRomFsExtHdr *>(dump.data() + consumed);
    if (hdr_ext->size < sizeof(VirtualRomFsExtHdr))
      return false;
    consumed += hdr_ext->size;
    if (dump.size() < consumed)
      return false;
    result.headerExt = make_span_const((uint8_t *)hdr_ext, hdr_ext->size);
  }
  else
  {
    result.headerExt.reset();
  }

  unsigned bodySize = hdr->packedSz() ? hdr->packedSz() : hdr->fullSz;
  if (dump.size() < consumed + bodySize)
    return false;
  result.body = make_span_const((uint8_t *)dump.data() + consumed, bodySize);
  consumed += bodySize;

  if (dump.size() < consumed + 16)
    return false;
  result.md5 = make_span_const((uint8_t *)dump.data() + consumed, 16);
  consumed += 16;

  if (hdr->signedContents())
  {
    if (dump.size() <= consumed)
      return false;
    result.signature = make_span_const((uint8_t *)dump.data() + consumed, dump.size() - consumed);
  }
  else
  {
    result.signature.reset();
  }

  return true;
}

bool dissect_vromfs_dump_body(const VromfsDumpSections &sections, VromfsDumpBodySections &result)
{
  if (sections.isPacked)
    return false;

  G_ASSERT(sections.header.size() >= sizeof(VirtualRomFsDataHdr));
  const VirtualRomFsDataHdr *hdr = reinterpret_cast<const VirtualRomFsDataHdr *>(sections.header.data());

  const VirtualRomFsExtHdr *hdr_ext = nullptr;
  int fsAttrOffset = 0;
  if (hdr->label == _MAKE4C('VRFx'))
  {
    G_ASSERT(sections.headerExt.size() >= sizeof(VirtualRomFsExtHdr));
    hdr_ext = reinterpret_cast<const VirtualRomFsExtHdr *>(sections.headerExt.data());
    if (hdr_ext->size < sizeof(VirtualRomFsExtHdr))
    {
      hdr_ext = nullptr;
    }
    else if ((hdr_ext->flags & EVFSEF_HAVE_FILE_ATTRIBUTES))
    {
      if (hdr_ext->size < sizeof(VirtualRomFsExtHdr) + sizeof(fsAttrOffset))
        return false;
      memcpy(&fsAttrOffset, sections.headerExt.data() + sizeof(VirtualRomFsExtHdr), sizeof(fsAttrOffset));
      if (fsAttrOffset <= 0)
        return false;
    }
  }

  const char *fs_data_start = (const char *)sections.body.data();
  VirtualRomFsData fs_copy;
  if (sections.body.size() < sizeof(fs_copy) - FS_OFFS)
    return false;
  memcpy(&fs_copy.files, fs_data_start, sizeof(fs_copy) - FS_OFFS); //-V780
  fs_copy.data.patch(nullptr);
  result.filesCount = fs_copy.data.size();

  // the vromfs dump body has the following layout:
  // - files.map (PatchableTab< PatchablePtr< const char > >), 16 bytes
  // - data (PatchableTab< PatchableTab< const char > >), 16 bytes
  // - (optional, packs only) offset of file data (4 bytes), reserved(?) 4 bytes,
  //                          offset of sha1 section (4 bytes), reserved(?) 4 bytes, 16 bytes total
  // - files.map.dptr (array of N PatchablePtr<const char>), N * 8 bytes
  // - alignment on 16 bytes, 0 or 8 bytes (do we really need it here?)
  // - file names, N asciiz strings
  // - alignment on 16 bytes, 0-15 bytes
  // - data.dptr (array of N PatchableTab<const char>), N * 16 bytes
  // - (optional) sha1 hash of each file (array of N sha1 hashes), N * 20 bytes
  // - (optional) file attributes section, N + 1 bytes for attributes version 1
  // - alignment on 16 bytes, 0-15 bytes
  // - file data, N or less 16-byte aligned chunks of data;
  // files with same sha1 hash will point at the same data if sha1 section is present

  size_t tables_data_sz = (intptr_t)fs_copy.data.data() + data_size(fs_copy.data);
  if (sections.body.size() < tables_data_sz)
    return false;
  result.tables = make_span_const((uint8_t *)fs_data_start, tables_data_sz);

  fs_copy.files.map.patch(nullptr);
  if (fs_copy.files.map.size() != result.filesCount)
    return false;

  int fileDataOffsetToCheck = 0;
  int filesNum = fs_copy.data.size();
  result.sha1Hashes.reset();
  if ((size_t)fs_copy.files.map.data() - (sizeof(fs_copy) - FS_OFFS) == 16)
  {
    // there are 16 bytes between fs.data and files.map.dptr, we have pack header
    int sha1SectionOffset = 0;
    const char *pack_extra_data = fs_data_start + (sizeof(fs_copy) - FS_OFFS);
    memcpy(&fileDataOffsetToCheck, pack_extra_data, 4);
    memcpy(&sha1SectionOffset, pack_extra_data + 8, 4);
    if (sha1SectionOffset > 0)
    {
      int sha1SectionSize = filesNum * VirtualRomFsPack::BackedData::C_SHA1_RECSZ;
      if (sha1SectionOffset + sha1SectionSize > sections.body.size())
        return false;
      result.sha1Hashes = make_span_const((uint8_t *)fs_data_start + sha1SectionOffset, sha1SectionSize);
    }
  }

  result.data.reset();
  ptrdiff_t firstFileDataOffset = 0;
  if (filesNum > 0)
  {
    const char *firstFileTabPtr = fs_data_start + (ptrdiff_t)fs_copy.data.data();
    PatchableTab<const char> firstFileTab, lastFileTab;
    memcpy(&firstFileTab, firstFileTabPtr, sizeof(firstFileTab));
    firstFileTab.patch(nullptr);
    firstFileDataOffset = (ptrdiff_t)firstFileTab.data();
    if (firstFileDataOffset < tables_data_sz || firstFileDataOffset >= sections.body.size())
      return false;

    int fileDataOffsetToCheckAligned = (fileDataOffsetToCheck + 0x0F) & 0xFFFFFFF0;
    if (fileDataOffsetToCheck > 0 && firstFileDataOffset != fileDataOffsetToCheckAligned)
      return false;

    const char *lastFileTabPtr = fs_data_start + (ptrdiff_t)fs_copy.data.data() + sizeof(firstFileTab) * (filesNum - 1);
    memcpy(&lastFileTab, lastFileTabPtr, sizeof(lastFileTab));
    lastFileTab.patch(nullptr);
    ptrdiff_t fileDataEndOffset = (ptrdiff_t)lastFileTab.data() + lastFileTab.size();
    if (fileDataEndOffset > sections.body.size())
      return false;

    result.data = make_span_const((uint8_t *)fs_data_start + firstFileDataOffset, fileDataEndOffset - firstFileDataOffset);
  }

  result.attributes.reset();
  if (fsAttrOffset > 0)
  {
    int fsAttrSize = filesNum + 1;
    if (fsAttrOffset + fsAttrSize > sections.body.size())
      return false;
    result.attributes = make_span_const((uint8_t *)fs_data_start + fsAttrOffset, fsAttrSize);
  }

  return true;
}


VirtualRomFsData *make_non_intrusive_vromfs(const VromfsDumpSections &sections, const VromfsDumpBodySections &bodySections,
  IMemAlloc *mem, unsigned *out_hdr_sz)
{
  size_t vrom_hdr_sz = sections.header.size() + sections.headerExt.size();

  const char *fs_data_start = (const char *)bodySections.tables.data();
  if (!fs_data_start)
    return nullptr;

  size_t fs_data_to_copy_sz = bodySections.tables.size();
  vrom_hdr_sz += fs_data_to_copy_sz;
  if (out_hdr_sz)
    *out_hdr_sz = (unsigned)vrom_hdr_sz;

  VirtualRomFsData *fs = new (mem->tryAlloc(FS_OFFS + fs_data_to_copy_sz), _NEW_INPLACE) VirtualRomFsData;
  if (!fs)
    return nullptr;
  memcpy(&fs->files, fs_data_start, fs_data_to_copy_sz); //-V780

  if (sections.headerExt.size() >= sizeof(VirtualRomFsExtHdr))
  {
    const VirtualRomFsExtHdr *hdr_ext = reinterpret_cast<const VirtualRomFsExtHdr *>(sections.headerExt.data());
    fs->flags = hdr_ext->flags;
    fs->version = hdr_ext->version;
  }
  patch_fs(fs, const_cast<char *>(fs_data_start));
  if (!init_vromfs_file_attr(fs->attr, bodySections))
  {
    mem->free(fs);
    return nullptr;
  }
  return fs;
}


VirtualRomFsData *make_non_intrusive_vromfs(dag::ConstSpan<char> dump, IMemAlloc *mem, unsigned *out_hdr_sz, int *out_signature_ofs)
{
  VromfsDumpSections sections;
  if (!dissect_vromfs_dump(dump, sections))
    return nullptr;

  VromfsDumpBodySections bodySections;
  if (!dissect_vromfs_dump_body(sections, bodySections))
    return nullptr;

  VirtualRomFsData *result = make_non_intrusive_vromfs(sections, bodySections, mem, out_hdr_sz);
  if (!result)
    return nullptr;

  if (out_signature_ofs)
  {
    if (sections.signature.size())
    {
      ptrdiff_t signatureOffset = sections.signature.data() - sections.header.data();
      G_ASSERT(signatureOffset > 0 && signatureOffset < dump.size());
      *out_signature_ofs = (unsigned)signatureOffset;
    }
    else
    {
      *out_signature_ofs = -1;
    }
  }

  return result;
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

static bool check_signature_in_one_pass(VromfsSignatureChecker &checker, const void **buffers, const unsigned *buf_sizes,
  const unsigned char *sigbuf, int siglen)
{
  if (!buffers || !buf_sizes || !sigbuf)
    return false;
  for (int i = 0; buffers[i]; ++i)
    if (!checker.append(buffers[i], buf_sizes[i]))
      return false;
  return checker.check(sigbuf, siglen);
}

EVirtualRomFsFileAttributes guess_vromfs_file_type(const char *fileName, const dag::ConstSpan<const char> &fileData)
{
  if (fileName)
  {
    if (strcmp(fileName, dblk::SHARED_NAMEMAP_FNAME) == 0)
      return EVFSFA_TYPE_SHARED_NM;
    const char *fileExt = dd_get_fname_ext(fileName);
    if (fileExt && strcmp(fileExt, ".blk") == 0)
    {
      if (fileData.size() > 1)
      {
        unsigned label = fileData[0];
        if (label == dblk::BBF_binary_with_shared_nm || label == dblk::BBF_binary_with_shared_nm_z ||
            label == dblk::BBF_binary_with_shared_nm_zd)
        {
          return EVFSFA_TYPE_BLK;
        }
      }
    }
  }
  return EVFSFA_TYPE_PLAIN;
}

static void append_stream_to_checker(VromfsSignatureChecker &checker, IGenLoad &crd, Tab<char> &plainBuf)
{
  const int plainBufSize = plainBuf.size();
  int bytesRead = plainBufSize;
  while (bytesRead == plainBufSize)
  {
    bytesRead = crd.tryRead(&plainBuf[0], plainBufSize);
    if (bytesRead)
      checker.append(&plainBuf[0], bytesRead);
  }
}

static bool check_plain_data_signature(VromfsSignatureChecker &checker, const VromfsDumpSections &vfsdump,
  const dag::ConstSpan<uint8_t> *to_verify)
{
  ZSTD_DDict_s *ddict = nullptr;
  constexpr bool zstdtmp = true; // Use framemem if possible
  constexpr int PLAINBUF_SIZE = 16 << 10;
  Tab<char> plainBuf(PLAINBUF_SIZE);

  VromfsDumpBodySections bodySections;
  if (!dissect_vromfs_dump_body(vfsdump, bodySections))
    return false;

  const char *baseAddress = (const char *)bodySections.tables.data();
  const auto *fileNamePointers = reinterpret_cast<const UnpatchedTabReader<UnpatchedPtrReader<const char>> *>(baseAddress);
  dag::ConstSpan<UnpatchedPtrReader<const char>> fileNamePtrsAccessor = fileNamePointers->getAccessor(baseAddress);
  const UnpatchedRoNameMap *fileNames = reinterpret_cast<const UnpatchedRoNameMap *>(baseAddress);

  constexpr ptrdiff_t dataTabOffset = offsetof(VirtualRomFsData, data) - FS_OFFS;
  const auto *fileDataTabs = reinterpret_cast<const UnpatchedTabReader<UnpatchedTabReader<const char>> *>(baseAddress + dataTabOffset);
  dag::ConstSpan<UnpatchedTabReader<const char>> fileDataTabsAccessor = fileDataTabs->getAccessor(baseAddress);

  VirtualRomFsFileAttributes fsAttr;
  init_vromfs_file_attr(fsAttr, bodySections);
  bool haveAttrV1 = fsAttr.version == 1 && fsAttr.attrData;

  // 0. Find shared namemap and zstd dict
  int sharedNmIdx = -1;
  if (fileDataTabsAccessor.size())
  {
    int lastIdx = fileDataTabsAccessor.size() - 1;
    const char *lastFileName = fileNamePtrsAccessor[lastIdx].get(baseAddress);
    const dag::ConstSpan<const char> lastFileData = fileDataTabsAccessor[lastIdx].getAccessor(baseAddress);
    EVirtualRomFsFileAttributes attr = haveAttrV1 ? fsAttr.attrData[lastIdx] : guess_vromfs_file_type(lastFileName, lastFileData);
    sharedNmIdx = attr == EVFSFA_TYPE_SHARED_NM ? lastIdx : fileNames->getNameId(dblk::SHARED_NAMEMAP_FNAME, baseAddress);
  }
  if (sharedNmIdx >= 0)
  {
    dag::ConstSpan<char> dictHash = dblk::get_vromfs_dict_hash(fileDataTabsAccessor[sharedNmIdx].getAccessor(baseAddress));
    if (dictHash.size() == BLAKE3_OUT_LEN)
    {
      char dictNameBuf[BLAKE3_OUT_LEN * 2 + 8];
      data_to_str_hex_buf(dictNameBuf, sizeof(dictNameBuf), dictHash.data(), dictHash.size());
      strcat(dictNameBuf, ".dict");
      int dictIdx = fileNames->getNameId(dictNameBuf, baseAddress);
      if (dictIdx >= 0)
      {
        const dag::ConstSpan<const char> dictData = fileDataTabsAccessor[dictIdx].getAccessor(baseAddress);
        constexpr bool dontCopyDictData = true;
        ddict = zstd_create_ddict(dictData, dontCopyDictData);
      }
    }
  }

  // 1. Plain file data
  // Since vroms are built using a lexicographically sorted id list we can expect the order
  // of file data and file name data to be the same
  for (int i = 0; i < fileDataTabsAccessor.size(); i++)
  {
    const char *fileName = fileNamePtrsAccessor[i].get(baseAddress);
    const dag::ConstSpan<const char> fileData = fileDataTabsAccessor[i].getAccessor(baseAddress);
    EVirtualRomFsFileAttributes attr = haveAttrV1 ? fsAttr.attrData[i] : guess_vromfs_file_type(fileName, fileData);
    switch (attr & EVFSFA_TYPE_MASK)
    {
      case EVFSFA_TYPE_PLAIN: checker.append(fileData.data(), fileData.size()); break;
      case EVFSFA_TYPE_BLK:
        if (!ddict)
          goto failed;
        if (fileData.size() > 1)
        {
          unsigned label = fileData.data()[0];
          if (label == dblk::BBF_binary_with_shared_nm_zd)
          {
            ZstdLoadFromMemCB zcrd(make_span_const(fileData.data() + 1, fileData.size() - 1), ddict, zstdtmp);
            append_stream_to_checker(checker, zcrd, plainBuf);
          }
          else if (label == dblk::BBF_binary_with_shared_nm)
          {
            checker.append(fileData.data() + 1, fileData.size() - 1);
          }
          else if (label == dblk::BBF_binary_with_shared_nm_z)
          {
            ZstdLoadFromMemCB zcrd(make_span_const(fileData.data() + 1, fileData.size() - 1), nullptr, zstdtmp);
            append_stream_to_checker(checker, zcrd, plainBuf);
          }
        }
        break;
      case EVFSFA_TYPE_SHARED_NM:
      {
        constexpr int plainPartSize = sizeof(uint64_t) + BLAKE3_OUT_LEN;
        if (fileData.size() < plainPartSize)
          goto failed;
        checker.append(fileData.data(), plainPartSize);
        ZstdLoadFromMemCB zcrd(make_span_const(fileData.data() + plainPartSize, fileData.size() - plainPartSize), nullptr, zstdtmp);
        append_stream_to_checker(checker, zcrd, plainBuf);
      }
      break;
      default: goto failed;
    }
  }

  if (ddict)
    zstd_destroy_ddict(ddict);

  // 2. Unpatched file name pointers
  checker.append(fileNamePtrsAccessor.data(), data_size(fileNamePtrsAccessor));

  // 3. File name data
  if (fileNamePtrsAccessor.size() > 0)
  {
    const char *begin = fileNamePtrsAccessor[0].get(baseAddress);
    const char *end = fileNamePtrsAccessor[fileNamePtrsAccessor.size() - 1].get(baseAddress);
    end += strlen(end) + 1;
    int nameDataSize = end - begin;
    checker.append(begin, nameDataSize);
  }

  // 4. File attributes data, if present
  if (bodySections.attributes.data() && bodySections.attributes.size() > 0)
    checker.append(bodySections.attributes.data(), bodySections.attributes.size());

  // 5. Externally supplied aux data, typically vrom file name
  if (to_verify && to_verify->data() && to_verify->size() > 0)
    checker.append(to_verify->data(), to_verify->size());

  return checker.check(vfsdump.signature.data(), vfsdump.signature.size());

failed:
  if (ddict)
    zstd_destroy_ddict(ddict);
  return false;
}


bool check_vromfs_dump_signature(VromfsSignatureChecker &checker, const VromfsDumpSections &vfsdump,
  const dag::ConstSpan<uint8_t> *to_verify)
{
  if (!vfsdump.signature.data() || vfsdump.signature.size() <= 0)
    return false;
  if (vfsdump.header.size() < sizeof(VirtualRomFsDataHdr))
    return false;
  const VirtualRomFsDataHdr *hdr = (const VirtualRomFsDataHdr *)vfsdump.header.data();

  const VirtualRomFsExtHdr *hdr_ext = nullptr;
  if (hdr->label == _MAKE4C('VRFx'))
  {
    if (vfsdump.headerExt.size() < sizeof(*hdr_ext))
      return false;
    hdr_ext = (const VirtualRomFsExtHdr *)vfsdump.headerExt.data();
  }
  bool plainSign = hdr_ext && (hdr_ext->flags & EVFSEF_SIGN_PLAIN_DATA);

  if (hdr->signedContents())
  {
    if (plainSign)
    {
      if (!check_plain_data_signature(checker, vfsdump, to_verify))
        return false;
    }
    else
    {
      const void *tmpBuf[] = {vfsdump.body.data(), to_verify ? to_verify->data() : NULL, NULL};
      unsigned bsizes[] = {vfsdump.body.size(), to_verify ? to_verify->size() : 0, 0};
      if (!check_signature_in_one_pass(checker, tmpBuf, bsizes, vfsdump.signature.data(), vfsdump.signature.size()))
        return false;
    }
  }
  else
  {
    const void *tmpBuf[] = {vfsdump.header.data(), NULL, NULL, NULL, NULL, NULL};
    unsigned bsizes[] = {vfsdump.header.size(), 0, 0, 0, 0, 0};
    int nextIdx = 1;
    auto appendBuf = [&](const auto &span) {
      G_ASSERT(nextIdx < countof(tmpBuf) - 1);
      if (span.data() && span.size() > 0)
      {
        tmpBuf[nextIdx] = span.data();
        bsizes[nextIdx++] = span.size();
      }
    };
    appendBuf(vfsdump.headerExt);
    appendBuf(vfsdump.body);
    appendBuf(vfsdump.md5);
    if (to_verify)
      appendBuf(*to_verify);
    if (!check_signature_in_one_pass(checker, tmpBuf, bsizes, vfsdump.signature.data(), vfsdump.signature.size()))
      return false;
  }

  return true;
}

VirtualRomFsData *load_vromfs_dump(const char *fname, IMemAlloc *mem, signature_checker_factory_cb checker_cb,
  const dag::ConstSpan<uint8_t> *to_verify, int file_flags)
{
  debug("%s <%s>", __FUNCTION__, fname);

  VromfsSignatureCheckerPtr checker = checker_cb ? checker_cb() : nullptr;
  VirtualRomFsDataHdr hdr;
  Tab<uint8_t> hdr_ext_data;
  VirtualRomFsData *fs = NULL;
  DagorStat st;
  int err = 0;
  char embedded_md5[16];
  int fs_attr_ofs = 0;
  unsigned char signature[SIGNATURE_MAX_SIZE];
  int signature_size = 0;
  enum
  {
    HDR,
    HDR_EXT,
    CONTENT,
    MD5,
    ADDITIONAL_CONTENT,
    SIGNATURE
  };
  const void *buffers[] = {&hdr, NULL, NULL, embedded_md5, to_verify ? to_verify->data() : NULL, signature};
  unsigned buf_sizes[] = {sizeof(hdr), 0, 0, sizeof(embedded_md5), to_verify ? (unsigned)to_verify->size() : 0, 0};
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
      hdr_ext_data.resize(hdr_ext.size);
      memcpy(&hdr_ext_data[0], &hdr_ext, sizeof(hdr_ext));
      if (hdr_ext.size > sizeof(hdr_ext))
        df_read(fp, &hdr_ext_data[sizeof(hdr_ext)], hdr_ext.size - sizeof(hdr_ext));
      fs->flags = hdr_ext.flags;
      fs->version = hdr_ext.version;
      if (fs->flags & EVFSEF_HAVE_FILE_ATTRIBUTES)
      {
        if (hdr_ext.size < sizeof(hdr_ext) + sizeof(fs_attr_ofs))
          goto load_fail;
        memcpy(&fs_attr_ofs, &hdr_ext_data[sizeof(hdr_ext)], sizeof(fs_attr_ofs));
      }
    }
    buffers[HDR_EXT] = hdr_ext_data.data();
    buf_sizes[HDR_EXT] = hdr_ext_data.size();
  }

  if (hdr.packedSz())
  {
    buf = mem->tryAlloc(hdr.packedSz());
    if (!buf)
      goto load_fail;
    if (df_read(fp, buf, hdr.packedSz()) != hdr.packedSz())
      goto load_fail;

    buffers[CONTENT] = buf;
    buf_sizes[CONTENT] = hdr.packedSz();

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

  if (buffers[SIGNATURE] && checker)
  {
    VromfsDumpSections sections;
    sections.header = make_span_const((uint8_t *)&hdr, sizeof(hdr));
    if (hdr_ext_data.size())
      sections.headerExt = make_span_const(hdr_ext_data);
    if (hdr.signedContents())
      sections.body = make_span_const((uint8_t *)fs + FS_OFFS, hdr.fullSz);
    else
      sections.body = make_span_const((uint8_t *)buffers[CONTENT], buf_sizes[CONTENT]);
    if (buffers[MD5])
      sections.md5 = make_span_const((uint8_t *)embedded_md5, sizeof(embedded_md5));
    sections.signature = make_span_const((uint8_t *)signature, signature_size);
    if (!check_vromfs_dump_signature(*checker, sections, to_verify))
      goto load_fail;
  }
  else
  {
    if (checker && !checker->allowAbsentSignature()) // empty signature allowed?
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

  patch_fs(fs);
  if (!init_patched_fs_file_attr(fs, (char *)fs + FS_OFFS, hdr.fullSz, fs_attr_ofs))
    goto load_fail;
  return fs;

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
  int fs_attr_ofs = 0;
  G_STATIC_ASSERT(sizeof(fs_attr_ofs) == 4);
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
    int consumed = sizeof(hdr.ext);
    if (hdr.ext.flags & EVFSEF_HAVE_FILE_ATTRIBUTES)
    {
      if (df_read(fp, &fs_attr_ofs, sizeof(fs_attr_ofs)) != sizeof(fs_attr_ofs))
        goto load_fail;
      consumed += sizeof(fs_attr_ofs);
    }
    df_seek_rel(fp, hdr.ext.size - consumed);
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
  if (!init_patched_fs_file_attr(fs, (char *)fs + FS_OFFS, hdr.pack.hdrSz, fs_attr_ofs))
    goto load_fail;

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
