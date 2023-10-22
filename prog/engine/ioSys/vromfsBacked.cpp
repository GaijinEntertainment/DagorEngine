#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_base32.h>
#include <stdio.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#define FS_OFFS offsetof(VirtualRomFsData, files)

static VirtualRomFsPack *resolve_backed_entry(VirtualRomFsData *fs, int &inout_entry, bool sync_wait_ready, bool dont_load,
  const char *mnt)
{
  if (!fs || !fs->data[inout_entry].size())
    return static_cast<VirtualRomFsPack *>(fs);

  VirtualRomFsPack::BackedData *bd = static_cast<VirtualRomFsPack *>(fs)->getBackedData();
  if (!bd)
    return NULL;

  int idx = inout_entry;
  if (fs->data[idx].data())
  {
  return_entry:
    if (bd->cache)
    {
      ptrdiff_t cache_idx = ptrdiff_t(fs->data[idx].data());
      if (cache_idx >= 1 && cache_idx <= bd->cache->data.size())
      {
        inout_entry = cache_idx - 1;
        return bd->cache;
      }
    }

    inout_entry = 0;
    return reinterpret_cast<VirtualRomFsSingleFile *>((void *)fs->data[idx].data());
  }

  static constexpr int CSZ = VirtualRomFsPack::BackedData::C_SHA1_RECSZ;
  const char *fn = fs->files.map[idx];
  if (!fs->data[idx].data() && bd->cache)
  {
    int cache_idx = bd->cache->files.getNameId(fn);
    if (cache_idx >= 0 && memcmp(bd->contentSHA1BasePtr + idx * CSZ, (char *)bd->cache->ptr + cache_idx * CSZ, CSZ) == 0)
      fs->data[idx].init((void *)intptr_t(cache_idx + 1), fs->data[idx].size());
  }
  if (!fs->data[idx].data() && bd->getData)
  {
    String out_fn;
    int out_base_ofs = 0;
    char hash_b32[40], hash_nm[32 + 2 + 1 + 8];
    char sz_b32[13 + 1];

    data_to_str_b32_buf(hash_b32, sizeof(hash_b32), bd->contentSHA1BasePtr + idx * CSZ, CSZ);
    SNPRINTF(hash_nm, sizeof(hash_nm), "%.2s/%s-%s", hash_b32, hash_b32 + 2,
      uint64_to_str_b32(fs->data[idx].size(), sz_b32, sizeof(sz_b32)));
    if (bd->getData(hash_nm, mnt, out_fn, out_base_ofs, sync_wait_ready, dont_load, fn, bd->getDataArg))
    {
      VirtualRomFsSingleFile *vrom1 = VirtualRomFsSingleFile::make_file_link(inimem, out_fn, out_base_ofs, fs->data[idx].size());
      if (vrom1)
      {
        if (bd->resolvedFilesLinkedList)
          vrom1->nextPtr = bd->resolvedFilesLinkedList;
        bd->resolvedFilesLinkedList = vrom1;
        fs->data[idx].init(vrom1, fs->data[idx].size());
        // debug("resolved to %s, %d, vrom1=%p", out_fn, out_base_ofs, vrom1);
      }
    }
    else if (!dont_load && !out_fn.empty())
    {
      if (out_fn[0] == '*' && bd->cache)
      {
        int cache_idx = bd->cache->files.getNameId(fn);
        if (cache_idx >= 0)
        {
          fs->data[idx].init((void *)intptr_t(cache_idx + 1), bd->cache->data[cache_idx].size()); //-V1028
          logwarn("bvrom: fallback to cache-vrom for \"%s\" (content is not identical)", fn);
        }
      }
      if (!fs->data[idx].data() && (out_fn[0] != '*' || out_fn.length() > 1))
      {
        const char *subst_fn = out_fn;
        int subst_sz = out_base_ofs;
        if (subst_fn[0] == '*')
          subst_fn++;

        VromReadHandle p1 = vromfs_get_file_data(subst_fn);
        VirtualRomFsSingleFile *vrom1 = p1.data()
                                          ? VirtualRomFsSingleFile::make_mem_data(inimem, (void *)p1.data(), data_size(p1), "", false)
                                          : VirtualRomFsSingleFile::make_file_link(inimem, subst_fn, 0, subst_sz);
        if (vrom1)
        {
          if (bd->resolvedFilesLinkedList)
            vrom1->nextPtr = bd->resolvedFilesLinkedList;
          bd->resolvedFilesLinkedList = vrom1;
          fs->data[idx].init(vrom1, subst_sz);
          logwarn("bvrom: fallback to stubfile \"%s\" for %s", subst_fn, fn);
        }
      }
    }
  }
  if (fs->data[idx].data())
    goto return_entry;

  if (sync_wait_ready)
    logerr("bvrom: cannot open \"%s\"", fn);
  return NULL;
}


VirtualRomFsPack *open_backed_vromfs_pack(const char *fname, IMemAlloc *mem, VirtualRomFsPack *cache_vrom,
  vromfs_user_get_file_data_t get_file_data, void *gfd_arg, int file_flags)
{
  debug("%s <%s>", __FUNCTION__, fname);
  VirtualRomFsPack *fs = static_cast<VirtualRomFsPack *>(load_vromfs_dump(fname, mem, NULL, NULL, file_flags));
  VirtualRomFsPack::BackedData *bd = NULL;
  if (!fs)
    goto load_fail;

  if (!(fs->files.map.size() && !fs->data.size()))
  {
    logerr("%s <%s> is not index-only vromfs", __FUNCTION__, fname);
    goto load_fail;
  }
  if (fs->files.map.data() < (void *)(fs + 1) || ptrdiff_t(fs->ptr) < sizeof(*fs))
  {
    logerr("%s <%s> broken fmt", __FUNCTION__, fname);
    goto load_fail;
  }

  if (cache_vrom && (void *)cache_vrom->ptr < (void *)cache_vrom)
  {
    logerr("bvrom: cache_vrom=%p (%s) doesn't contain content-SHA1 (ptr=%p), cannot be used", cache_vrom, cache_vrom->getFilePath(),
      cache_vrom->ptr);
    G_ASSERTF((void *)cache_vrom < (void *)cache_vrom->ptr,
      "cache_vrom=%p cache_vrom->ptr=%p\n"
      "cache will be released and not used",
      cache_vrom, cache_vrom->ptr);
    close_vromfs_pack(cache_vrom, mem);
    cache_vrom = NULL;
  }
  bd = new (mem) VirtualRomFsPack::BackedData;
  bd->contentSHA1BasePtr = (char *)fs + FS_OFFS + ptrdiff_t(fs->ptr);
  bd->cache = cache_vrom;
  bd->getData = get_file_data;
  bd->getDataArg = gfd_arg;

  fs->data.init(fs->data.data(), fs->files.map.size());
  fs->_resv = 'BCKD';
  fs->ptr = bd;
  G_ASSERT(fs->isValid());
  VirtualRomFsPack::resolve_backed_entry = &resolve_backed_entry;
  return fs;

load_fail:
  logerr("%s <%s>  failed", __FUNCTION__, fname);
  if (fs)
    memfree(fs, mem);
  return NULL;
}

VirtualRomFsPack *backed_vromfs_replace_cache(VirtualRomFsPack *fs, VirtualRomFsPack *cache_vrom)
{
  VirtualRomFsPack::BackedData *bd = fs->getBackedData();
  if (!bd || bd->cache == cache_vrom)
    return NULL;
  VirtualRomFsPack *prev = bd->cache;

  // reset already resolved entries
  for (int i = 0; i < fs->files.map.size(); i++)
  {
    if (fs->data[i].data() || !fs->data[i].size())
      continue;
    ptrdiff_t cache_idx = ptrdiff_t(fs->data[i].data());
    if (cache_idx >= 1 && cache_idx <= bd->cache->data.size())
      fs->data[i].init(NULL, fs->data[i].size());
  }
  // replace cache
  bd->cache = cache_vrom;
  return prev;
}
void backed_vromfs_prefetch_all_files(VirtualRomFsPack *fs)
{
  VirtualRomFsPack::BackedData *bd = fs->getBackedData();
  if (!bd)
    return;

  String mnt(get_vromfs_mount_path(fs)), out_fn;
  for (int i = 0; i < fs->files.map.size(); i++)
  {
    if (fs->data[i].data() || !fs->data[i].size())
      continue;
    int entry = i;
    resolve_backed_entry(fs, entry, false, false, mnt);
    if (!fs->data[i].data() && !bd->cache && !bd->getData)
      logerr("bvrom: \"%s\" not found in cache-vrom and no get-method provided", fs->files.map[i]);
  }
}

static int getPartialNameId(PatchableTab<PatchablePtr<const char>> &map, const char *name, int len)
{
  int lo = 0, hi = map.size() - 1, m;
  int cmp;

  // check bounds first
  if (hi < 0)
    return -1;

  cmp = strncmp(name, map[0], len);
  if (cmp < 0)
    return -1;
  if (cmp == 0)
    return 0;

  if (hi != 0)
    cmp = strncmp(name, map[hi], len);
  if (cmp > 0)
    return -1;
  if (cmp == 0)
    return hi;

  // binary search
  while (lo < hi)
  {
    m = (lo + hi) >> 1;
    cmp = strncmp(name, map[m], len);

    if (cmp == 0)
      return m;
    if (m == lo)
      return -1;

    if (cmp < 0)
      hi = m;
    else
      lo = m;
  }
  return -1;
}

static VirtualRomFsPack *find_backed_vromfs_file_entry(const char *fname, bool fn_is_prefix, int &out_idx)
{
#if _TARGET_PC_WIN
  if (fname[0] == '/')
    fname++;
#endif

  char namebuf[DAGOR_MAX_PATH];
  strncpy(namebuf, fname, DAGOR_MAX_PATH - 1);
  namebuf[DAGOR_MAX_PATH - 1] = 0;
  dd_simplify_fname_c(namebuf);
  dd_strlwr(namebuf);

  VirtualRomFsPack *result = nullptr;

  iterate_vroms([&](VirtualRomFsData *entry, size_t) {
    if (VirtualRomFsPack *fs = static_cast<VirtualRomFsPack *>(entry))
    {
      VirtualRomFsPack::BackedData *bd = fs->getBackedData();
      if (!bd)
        return true; // continue

      const char *name = namebuf;
      const char *vromfsMp = get_vromfs_mount_path(fs);
      if (vromfsMp)
      {
        int vromfsMpLen = i_strlen(vromfsMp);
        if (strncmp(name, vromfsMp, vromfsMpLen) == 0)
          name += vromfsMpLen;
        else
          return true; // continue
      }

      int idx = fn_is_prefix ? getPartialNameId(fs->files.map, name, i_strlen(name)) : fs->files.getNameId(name);
      if (idx >= 0)
      {
        out_idx = idx;
        result = fs;
        return false; // early exit
      }
    }
    return true; // continue
  });

  return result;
}

void backed_vromfs_prefetch_file(const char *fn, bool fn_is_prefix)
{
  int i = -1;
  if (VirtualRomFsPack *fs = find_backed_vromfs_file_entry(fn, fn_is_prefix, i))
  {
    if (fs->data[i].data() || !fs->data[i].size())
      return;
    String mnt(get_vromfs_mount_path(fs)), out_fn;
    resolve_backed_entry(fs, i, false, false, mnt);
  }
}
bool backed_vromfs_is_file_prefetched(const char *fn, bool fn_is_prefix)
{
  int i = -1;
  if (VirtualRomFsPack *fs = find_backed_vromfs_file_entry(fn, fn_is_prefix, i))
  {
    if (fs->data[i].data() || !fs->data[i].size())
      return true;
    String out_fn;
    return resolve_backed_entry(fs, i, false, true, get_vromfs_mount_path(fs)) != NULL;
  }
  return false;
}
