#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/basePath.h>
#include <stdio.h>

namespace vromfsinternal
{
void (*on_vromfs_unmounted)(VirtualRomFsData *fs) = nullptr;
}

static constexpr int MAX_VROMFS_NUM = 64;
static VirtualRomFsData *vromfs[MAX_VROMFS_NUM] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static const char *vromfsMp[MAX_VROMFS_NUM] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static int vromfsMpLen[MAX_VROMFS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

VirtualRomFsPack *(*VirtualRomFsPack::resolve_backed_entry)(VirtualRomFsData *fs, int &inout_entry, bool sync_wait_ready,
  bool dont_load, const char *mnt) = NULL;

bool vromfs_first_priority = true;
ReadWriteLock VromReadHandle::lock;

typedef ScopedLockReadTemplate<ReadWriteLock> LockForRead;
typedef ScopedLockWriteTemplate<ReadWriteLock> LockForWrite;

void add_vromfs(VirtualRomFsData *fs, bool insert_first, char *mount_path)
{
  LockForWrite lock(VromReadHandle::lock);
  bool uses_named_mount = mount_path && *mount_path == '%';
  G_UNUSED(uses_named_mount);
  G_ASSERTF(!uses_named_mount, "mount_path=\"%s\", named mounts must be resolved first!", mount_path);
  for (int i = 0; i < MAX_VROMFS_NUM; i++)
  {
    if (!vromfs[i])
    {
      if (insert_first)
      {
        memmove(&vromfs[1], &vromfs[0], sizeof(vromfs[0]) * (MAX_VROMFS_NUM - 1));
        memmove(&vromfsMp[1], &vromfsMp[0], sizeof(vromfsMp[0]) * (MAX_VROMFS_NUM - 1));
        memmove(&vromfsMpLen[1], &vromfsMpLen[0], sizeof(vromfsMpLen[0]) * (MAX_VROMFS_NUM - 1));
        i = 0;
      }
      vromfs[i] = fs;
      vromfsMp[i] = mount_path;
      if (mount_path)
      {
        dd_simplify_fname_c(mount_path);
        dd_strlwr(mount_path);
        vromfsMpLen[i] = i_strlen(mount_path);
      }
      else
        vromfsMpLen[i] = 0;
      rebuild_basepath_vrom_mounted();
      return;
    }
  }
  VromReadHandle::lock.unlockWrite();
  G_ASSERT(0 && "all vromfs are used, cannot add");
  VromReadHandle::lock.lockWrite();
}
char *remove_vromfs(VirtualRomFsData *fs)
{
  if (!fs)
    return NULL;
  LockForWrite lock(VromReadHandle::lock);
  for (int i = 0; i < MAX_VROMFS_NUM; i++)
  {
    if (vromfs[i] == fs)
    {
      char *mp = (char *)vromfsMp[i];
      vromfs[i] = NULL;
      vromfsMp[i] = NULL;
      if (i + 1 < MAX_VROMFS_NUM)
      {
        memmove(&vromfs[i], &vromfs[i + 1], sizeof(vromfs[0]) * (MAX_VROMFS_NUM - i - 1));
        memmove(&vromfsMp[i], &vromfsMp[i + 1], sizeof(vromfsMp[0]) * (MAX_VROMFS_NUM - i - 1));
        memmove(&vromfsMpLen[i], &vromfsMpLen[i + 1], sizeof(vromfsMpLen[0]) * (MAX_VROMFS_NUM - i - 1));
      }
      rebuild_basepath_vrom_mounted();
      if (vromfsinternal::on_vromfs_unmounted)
        vromfsinternal::on_vromfs_unmounted(fs);
      return mp;
    }
  }
  VromReadHandle::lock.unlockWrite();
  G_ASSERT(0 && "try to remove fs which not added");
  VromReadHandle::lock.lockWrite();
  return NULL;
}
VirtualRomFsData *replace_vromfs(int idx, VirtualRomFsData *fs)
{
  if (!fs || idx < 0 || idx >= MAX_VROMFS_NUM)
    return NULL;
  LockForWrite lock(VromReadHandle::lock);
  VirtualRomFsData *old_fs = vromfs[idx];
  if (!old_fs)
    return NULL;
  vromfs[idx] = fs;
  if (vromfsinternal::on_vromfs_unmounted)
    vromfsinternal::on_vromfs_unmounted(old_fs);
  return old_fs;
}

char *get_vromfs_mount_path(VirtualRomFsData *fs)
{
  LockForRead lock(VromReadHandle::lock);
  for (int i = 0; i < MAX_VROMFS_NUM; i++)
    if (vromfs[i] == fs)
      return (char *)vromfsMp[i];
  return NULL;
}
char *set_vromfs_mount_path(VirtualRomFsData *fs, char *mount_path)
{
  LockForRead lock(VromReadHandle::lock);
  for (int i = 0; i < MAX_VROMFS_NUM; i++)
    if (vromfs[i] == fs)
    {
      char *p = (char *)vromfsMp[i];
      vromfsMp[i] = mount_path;

      if (mount_path)
      {
        dd_simplify_fname_c(mount_path);
        dd_strlwr(mount_path);
        vromfsMpLen[i] = i_strlen(mount_path);
      }
      else
        vromfsMpLen[i] = 0;
      return p;
    }
  return NULL;
}

VromReadHandle vromfs_get_file_data_one(const char *fname, VirtualRomFsData **out_vrom)
{
#if _TARGET_PC_WIN
  if (fname[0] == '/')
    fname++;
#endif

  if (out_vrom)
    *out_vrom = NULL;
  if (vromfs[0])
  {
    LockForRead lock(VromReadHandle::lock);
    char namebuf[DAGOR_MAX_PATH];
    resolve_named_mount_s(namebuf, sizeof(namebuf), fname);
    dd_simplify_fname_c(namebuf);
    dd_strlwr(namebuf);

    for (int i = 0; i < MAX_VROMFS_NUM; i++)
      if (VirtualRomFsData *fs = vromfs[i])
      {
        const char *name = namebuf;
        if (vromfsMp[i])
        {
          if (strncmp(name, vromfsMp[i], vromfsMpLen[i]) == 0)
            name += vromfsMpLen[i];
          else
            continue;
        }

        int idx = fs->files.getNameId(name);
        if (idx >= 0)
        {
          if (static_cast<VirtualRomFsPack *>(fs)->isValid() && !out_vrom)
            return {};
          if (out_vrom)
            *out_vrom = fs;
          if (!fs->data[idx].size())
            return make_span_const(fs->data[idx]);

          if (VirtualRomFsPack::resolve_backed_entry && out_vrom)
            if (VirtualRomFsPack::BackedData *bd = static_cast<VirtualRomFsPack *>(fs)->getBackedData())
            {
              int entry = idx;
              *out_vrom = VirtualRomFsPack::resolve_backed_entry(fs, entry, true, false, vromfsMp[i]);
              // debug("%p,%d -> %p,%d (%p,%d)", fs, idx, *out_vrom, entry, (*out_vrom)->data[entry].data(),
              // (*out_vrom)->data[entry].size());
              if (*out_vrom)
                return make_span_const((*out_vrom)->data[entry]);
              return {};
            }
          return make_span_const(fs->data[idx]);
        }
      }
      else
        break;
  }
  return {};
}

VromReadHandle vromfs_get_file_data(const char *fname, VirtualRomFsData **out_vrom)
{
  if (!fname)
    return {};

  char fn[DAGOR_MAX_PATH];
  bool root_tested = false;
  const char *mnt_path = "";
  fname = resolve_named_mount_in_path(fname, mnt_path);
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i])
    {
      if (!df_base_path_vrom_mounted[i])
        continue;
      if (!df_base_path[i][0])
        root_tested = true;
      snprintf(fn, sizeof(fn), "%s%s%s", df_base_path[i], mnt_path, fname);
      VromReadHandle ret = vromfs_get_file_data_one(fn, out_vrom);
      if (ret.data())
        return ret;
    }
    else
      break;
  if (!root_tested)
  {
    if (*mnt_path)
    {
      snprintf(fn, sizeof(fn), "%s%s", mnt_path, fname);
      fname = fn;
    }
    return vromfs_get_file_data_one(fname, out_vrom);
  }
  return {};
}

dag::Span<VirtualRomFsData *> vromfs_get_entries_unsafe()
{
  int n = 0;
  for (; n < MAX_VROMFS_NUM && vromfs[n]; ++n)
    ;
  return dag::Span<VirtualRomFsData *>(vromfs, n);
}

void rebuild_basepath_vrom_mounted()
{
  for (int i = 0; i < DF_MAX_BASE_PATH_NUM; i++)
    if (df_base_path[i])
    {
      if (!*df_base_path[i])
      {
        df_base_path_vrom_mounted[i] = true;
        continue;
      }

      df_base_path_vrom_mounted[i] = false;
      for (int j = 0; j < MAX_VROMFS_NUM; j++)
        if (vromfs[j] && vromfsMp[j] && *vromfsMp[j])
          if (strncmp(vromfsMp[j], df_base_path[i], strlen(df_base_path[i])) == 0)
          {
            df_base_path_vrom_mounted[i] = true;
            break;
          }
    }
    else
      df_base_path_vrom_mounted[i] = false;
}
