// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/conLogWriter.h>
#include <regExp/regExp.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/signSha256.h>
#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <3d/ddsxTex.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_globDef.h>
#include <util/dag_base64.h>
#include <util/dag_base32.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_logSys.h>
#include <startup/dag_globalSettings.h>
#include <hash/sha1.h>
#include <hash/crc32c.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#undef ERROR
#include <supp/dag_define_KRNLIMP.h>

static ConsoleLogWriter conlog;

static bool cmp_blk_differ(const DataBlock &blk1, const DataBlock &blk2);
static const char *get_file_hash(const char *fn, String &out_hash);
static bool checkFilesEqual(const char *fn, MemoryChainedData *mem);
static const char *find_char_rep_rev(const char *str, char c, int cnt);

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Custom content processor tool v1.4\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

namespace texconvcache
{
extern bool force_ddsx(DagorAsset &a, ddsx::Buffer &dest, unsigned t, const char *p, ILogWriter *log, const char *built_fn);
}

namespace texconv
{
static DagorAssetMgr *mgr = NULL;
static int tex_atype = -1;
bool init(const char *start_dir, const char *cache_dir)
{
  DataBlock appblk;
  appblk.addBlock("assets")->addBlock("types")->setStr("type", "tex");
  appblk.setStr("appDir", ".");
  if (cache_dir)
    appblk.addBlock("assets")->addBlock("export")->setStr("cache", cache_dir);

  if (ddsx::ConvertParams::allowOodlePacking)
  {
    appblk.addBlock("assets")->addBlock("build")->setBool("preferZSTD", true);
    appblk.addBlock("assets")->addBlock("build")->setBool("allowOODLE", true);
  }
  else if (ddsx::ConvertParams::preferZstdPacking)
  {
    appblk.addBlock("assets")->addBlock("build")->setBool("preferZSTD", true);
  }
  else if (ddsx::ConvertParams::forceZlibPacking)
  {
    appblk.addBlock("assets")->addBlock("build")->setBool("preferZLIB", true);
  }

  mgr = new DagorAssetMgr;
  mgr->setupAllowedTypes(*appblk.getBlockByNameEx("assets")->getBlockByNameEx("types"));
  tex_atype = mgr->getTexAssetTypeId();
  return texconvcache::init(*mgr, appblk, start_dir, !cache_dir);
}
void term()
{
  texconvcache::term();
  del_it(mgr);
}
} // namespace texconv

namespace dblk
{
extern KRNLIMP void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);

static bool pack_to_bbz(const DataBlock &blk, const char *filename)
{
  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
    return false;

  DAGOR_TRY
  {
    MemorySaveCB mcwr(512 << 10);
    dblk::save_to_bbf3(blk, mcwr);

    int sz = mcwr.getSize();
    cwr.writeInt(_MAKE4C('BBz'));
    cwr.writeInt(sz);

    int pos = cwr.tell();
    cwr.writeInt(0);

    MemoryLoadCB mcrd(mcwr.getMem(), false);
    sz = zlib_compress_data(cwr, 9, mcrd, sz);
    cwr.seekto(pos);
    cwr.writeInt(sz);
    cwr.seekto(pos + 4 + sz);
  }
  DAGOR_CATCH(const IGenSave::SaveException &) { return false; }
  return true;
}
} // namespace dblk

static int hash_path_dir1 = 0, hash_path_dir2 = 0;

#define D1 hash_path_dir1
#define D2 hash_path_dir2
static String make_path_by_hash(const char *hash_nm, const char *base_dir = NULL)
{
  String s;

  if (base_dir)
    s.printf(0, "%s/", base_dir);

  if (D1)
  {
    s.aprintf(0, "%.*s/", D1, hash_nm);
    hash_nm += D1;
  }
  if (D2)
  {
    s.aprintf(0, "%.*s/", D2, hash_nm);
    hash_nm += D2;
  }
  s.aprintf(0, "%s.ddsx", hash_nm);
  return s;
}
static String make_hash_by_path(const char *path)
{
  const char *name = find_char_rep_rev(path, '/', 1 + (D1 ? 1 : 0) + (D2 ? 1 : 0));
  if (!name)
    return String();

  int len = i_strlen(++name);
  if (const char *p = strchr(name, '.'))
    len = p - name;
  if (len < 32 + 2 + (D1 ? 1 : 0) + (D2 ? 1 : 0))
    return String();
  if (D1 && name[D1] != '/')
    return String();
  if (D2 && name[(D1 ? D1 + 1 : 0) + D2] != '/')
    return String();
  if (name[32 + (D1 ? 1 : 0) + (D2 ? 1 : 0)] != '-')
    return String();

  return String(0, "%.*s%.*s%.*s", D1, name, D2, name + (D1 ? D1 + 1 : 0), len - (D1 ? D1 + 1 : 0) - (D2 ? D2 + 1 : 0),
    name + (D1 ? D1 + 1 : 0) + (D2 ? D2 + 1 : 0));
}
#undef D1
#undef D2

static const int CRC32_USED_BITS = 4 * 4;
static DagorAsset *make_texture_asset(const char *fn, const DataBlock &asset_props, int lod)
{
  DataBlock props;
  String assetName(0, "%d$%s-%0*x", lod, DagorAsset::fpath2asset(fn), CRC32_USED_BITS / 4,
    crc32c_append(0, (const unsigned char *)fn, strlen(fn)) & ((1u << CRC32_USED_BITS) - 1u));

  props = asset_props;
  props.setStr("name", fn);
  props.setBool("convert", true);
  if (const char *suf = props.getStr("harmony_suffix", NULL))
    if (const char *n = strstr(fn, suf))
    {
      props.setStr("baseImage", String(0, "%.*s%s", n - fn, fn, n + strlen(suf)));
      props.removeParam("harmony_suffix");
    }
  return texconv::mgr->makeAssetDirect(assetName, props, texconv::tex_atype);
}

static const char *make_sha1sz_b32_hash(String &out_hash, const unsigned char *sha1_digest, int64_t sz)
{
  char sz_b32[13 + 1];
  data_to_str_b32(out_hash, sha1_digest, 20);
  out_hash.aprintf(0, "-%s", uint64_to_str_b32(sz, sz_b32, sizeof(sz_b32)));
  return out_hash;
}

static const char *get_file_hash(const char *fn, String &out_hash)
{
  FullFileLoadCB crd(fn);
  out_hash = "";
  if (!crd.fileHandle)
    return NULL;

  static const int BUF_SZ = 32 << 10;
  int len, sz = 0;
  char buf[BUF_SZ];

  sha1_context sha1c;
  sha1_starts(&sha1c);
  while ((len = crd.tryRead(buf, BUF_SZ)) > 0)
  {
    sha1_update(&sha1c, (unsigned char *)buf, len);
    sz += len;
  }
  unsigned char digest[20];
  sha1_finish(&sha1c, digest);

  return make_sha1sz_b32_hash(out_hash, digest, sz);
}

bool write_index_hash(const char *out_dir, const char *idx_fn, const DataBlock &cfgBlk, bool quiet)
{
  const char *indexHashDest = cfgBlk.getStr("indexHashDest", NULL);
  if (!indexHashDest)
    return true;

  String hash, hash_txt(0, "%s/%s", out_dir, indexHashDest);
  if (get_file_hash(idx_fn, hash))
  {
    if (const char *ext = dd_get_fname_ext(idx_fn))
    {
      String hash_dest_fn(0, "%.*s.%s%s", ext - idx_fn, idx_fn, hash, ext);
      if (dag_copy_file(idx_fn, hash_dest_fn, true))
      {
        int64_t f_time = time(NULL);
        dag_get_file_time(idx_fn, f_time);
        dag_set_file_time(hash_dest_fn, f_time);
        FullFileSaveCB cwr(hash_txt);
        if (cwr.fileHandle)
        {
          cwr.write(hash.data(), i_strlen(hash));
          if (!quiet)
            conlog.addMessage(conlog.NOTE, "%s SHA1 %s", idx_fn, hash);
          if (const char *fn = cfgBlk.getStr("indexHashBlk", NULL))
          {
            String hash_blk(0, "%s/%s", out_dir, fn);
            DataBlock blk;
            if (!blk.load(hash_blk))
              blk.reset();
            blk.setStr(cfgBlk.getStr("indexHashType", cfgBlk.getStr("type", NULL)), hash);
            if (cmp_blk_differ(blk, DataBlock(hash_blk)))
              blk.saveToTextFile(hash_blk);
          }
          return true;
        }
        else
          conlog.addMessage(conlog.ERROR, "failed to write %s to %s", hash, hash_txt);
      }
      else
        conlog.addMessage(conlog.ERROR, "failed to copy %s to %s", idx_fn, hash_dest_fn);
    }
    else
      conlog.addMessage(conlog.ERROR, "failed to get extension of fname %s", idx_fn);
  }
  else
    conlog.addMessage(conlog.ERROR, "failed to get hash from %s", idx_fn);
  return false;
}

static const char *convert_texture(DagorAsset *a, const char *out_dir, int64_t f_time, const char *ref_result_to_recreate_cache)
{
  if (!a)
    return NULL;
  unsigned four_cc = _MAKE4C('PC');
  ddsx::Buffer buf;
  if (ref_result_to_recreate_cache)
  {
    if (!texconvcache::force_ddsx(*a, buf, four_cc, "ddsx", &conlog, ref_result_to_recreate_cache))
    {
      conlog.addMessage(conlog.ERROR, "Can't export image: %s, err=%s", a->getTargetFilePath(), ddsx::get_last_error_text());
      return NULL;
    }
  }
  else if (!texconvcache::get_tex_asset_built_ddsx(*a, buf, four_cc, "ddsx", &conlog))
  {
    conlog.addMessage(conlog.ERROR, "Can't export image: %s, err=%s", a->getTargetFilePath(), ddsx::get_last_error_text());
    return NULL;
  }

  sha1_context sha1c;
  unsigned char digest[20];
  ::sha1_starts(&sha1c);
  ::sha1_update(&sha1c, (const unsigned char *)buf.ptr, buf.len);
  ::sha1_finish(&sha1c, digest);

  static String hash_name; // to be returned!
  make_sha1sz_b32_hash(hash_name, digest, buf.len);

  String out_fn = make_path_by_hash(hash_name, out_dir);
  if (file_ptr_t fp = df_open(out_fn, DF_READ))
  {
    static const int FBUF_LEN = 64 << 10;
    static char fbuf[FBUF_LEN];
    const char *mbuf = (const char *)buf.ptr;
    int sz = df_length(fp);
    bool files_eq = true;

    if (sz != buf.len)
      files_eq = false;
    else
      while (sz)
      {
        int rsz = df_read(fp, fbuf, sz > FBUF_LEN ? FBUF_LEN : sz);
        if (memcmp(fbuf, mbuf, rsz) != 0)
        {
          files_eq = false;
          break;
        }
        sz -= rsz;
        mbuf += rsz;
      }
    df_close(fp);
    if (files_eq)
      return hash_name;
    printf("ERR: new texture %s built to %s differs from existing %s\n", a->getName(), hash_name.str(), out_fn.str());
  }

  dd_mkpath(out_fn);
  file_ptr_t fp = df_open(out_fn, DF_CREATE | DF_WRITE);
  if (!fp)
  {
    buf.free();
    conlog.addMessage(conlog.ERROR, "cannot open output <%s>", out_fn);
    return NULL;
  }
  df_write(fp, buf.ptr, buf.len);
  df_close(fp);
  dag_set_file_time(out_fn, f_time);
  buf.free();

  return hash_name;
}

static bool checkFilesEqual(const char *fn, MemoryChainedData *mem)
{
  MemoryLoadCB mcrd(mem, false);
  FullFileLoadCB fcrd(fn);
  if (!fcrd.fileHandle)
    return false;
  int sz = MemoryChainedData::calcTotalUsedSize(mem);
  if (df_length(fcrd.fileHandle) != sz)
    return false;
  static const int BUF_SZ = 8 << 10;
  char buf1[BUF_SZ], buf2[BUF_SZ];
  while (sz > BUF_SZ)
  {
    fcrd.read(buf1, BUF_SZ);
    mcrd.read(buf2, BUF_SZ);
    if (memcmp(buf1, buf2, BUF_SZ) != 0)
      return false;
    sz -= BUF_SZ;
  }
  fcrd.read(buf1, sz);
  mcrd.read(buf2, sz);
  return memcmp(buf1, buf2, sz) == 0;
}

static bool check_bin_blk_signing_changed(const char *fn, const char *private_key)
{
  bool need_sign = private_key && *private_key;
  bool has_sign = false;
  if (file_ptr_t fp = df_open(fn, DF_READ | DF_IGNORE_MISSING))
  {
    df_seek_to(fp, 8);
    int32_t packed_sz = 0;
    df_read(fp, &packed_sz, sizeof(packed_sz));
    has_sign = df_length(fp) > (packed_sz + 12) + 64;
    df_close(fp);
  }
  return need_sign != has_sign;
}

static const char *resolve_suffix(dag::ConstSpan<RegExp *> reSuffix, const DataBlock &reSuffixBlk, const char *tex_name)
{
  G_ASSERT(reSuffix.size() == reSuffixBlk.paramCount());
  for (int i = 0; i < reSuffix.size(); i++)
    if (reSuffix[i]->test(tex_name))
      return reSuffixBlk.getParamName(i);
  return "";
}

static bool cmp_blk_differ(const DataBlock &blk1, const DataBlock &blk2)
{
  DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 128 << 10), cwr_old(tmpmem, 0, 128 << 10);
  blk1.saveToTextStream(cwr_old);
  blk2.saveToTextStream(cwr_new);
  return cwr_new.size() != cwr_old.size() || memcmp(cwr_new.data(), cwr_old.data(), cwr_new.size()) != 0;
}

static void gather_ddsx_ref(const DataBlock &blk, FastNameMap &ddsx_ref)
{
  if (const char *hash = blk.getStr("lod0", NULL))
    ddsx_ref.addNameId(hash);
  if (const char *hash = blk.getStr("lod1", NULL))
    ddsx_ref.addNameId(hash);
  if (const char *hash = blk.getStr("lod2", NULL))
    ddsx_ref.addNameId(hash);
  if (const char *hash = blk.getStr("lod0_b", NULL))
    ddsx_ref.addNameId(hash);
  if (const char *hash = blk.getStr("lod1_b", NULL))
    ddsx_ref.addNameId(hash);

  for (int i = 0; i < blk.blockCount(); i++)
    gather_ddsx_ref(*blk.getBlock(i), ddsx_ref);
}

static const char *find_char_rep_rev(const char *str, char c, int cnt)
{
  for (const char *p = str + strlen(str) - 1; p >= str; p--)
    if (*p == c)
    {
      if (cnt > 1)
        cnt--;
      else
        return p;
    }
  return NULL;
}

static int do_sweep_pass(const DataBlock &cfgBlk, bool quiet, bool sweep_erase_invalid)
{
  const char *out_dir = cfgBlk.getStr("outDir", ".");
  const char *index_filename = cfgBlk.getStr("indexFile", "?");
  const char *type = cfgBlk.getStr("type", "");
  String idx_fn(0, "%s/%s", out_dir, index_filename);
  int64_t f_time = time(NULL);
  int cnt_rm = 0, cnt_add = 0, cnt_upd = 0, cnt_ddsx_del_inval = 0, cnt_ddsx_del_unref = 0;

  if (strcmp(type, "remove") == 0)
  {
    conlog.addMessage(conlog.ERROR, "bad type %s for cleanup in %s", type, cfgBlk.resolveFilename());
    return 13;
  }
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Performing cleanup for <%s> in: %s", type, out_dir);

  DataBlock idxBlk;
  if (dd_file_exists(idx_fn))
    if (!idxBlk.load(idx_fn))
    {
      conlog.addMessage(conlog.WARNING, "failed to read indexFile \"%s\", resetting", idx_fn.str());
      idxBlk.clearData();
    }
  idxBlk.removeParam("ver");

  Tab<SimpleString> ddsx_list, blk_list, all_blk_list;
  find_files_in_folder(ddsx_list, out_dir, ".ddsx", false, true, true);
  find_files_in_folder(blk_list, out_dir, String(0, ".%s.blk", type), false, true, false);
  find_files_in_folder(all_blk_list, out_dir, ".blk", false, true, false);
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Found %d content BLK(s) and %d DDSx file(s)", blk_list.size(), ddsx_list.size());

  // sweep away invalid and unreferenced DDSx
  FastNameMapEx ddsx_ref;
  for (int i = 0; i < all_blk_list.size(); i++)
  {
    if (trail_stricmp(all_blk_list[i], index_filename))
      continue; // skip index BLK that is being updated

    DataBlock blk;
    if (!blk.load(all_blk_list[i]))
    {
      conlog.addMessage(conlog.WARNING, "failed to load blk[%d]=%s", i, all_blk_list[i]);
      continue;
    }
    gather_ddsx_ref(blk, ddsx_ref);
  }
  clear_and_shrink(all_blk_list);

  for (int i = 0; i < ddsx_list.size(); i++)
  {
    bool rm_ddsx = false;
    String hash_nm = make_hash_by_path(ddsx_list[i]);
    if (hash_nm.empty())
    {
      if (!sweep_erase_invalid)
      {
        conlog.addMessage(conlog.WARNING, "skipping invalid DDSx: %s", ddsx_list[i]);
        continue;
      }
      conlog.addMessage(conlog.WARNING, "removing invalid DDSx: %s", ddsx_list[i]);
      rm_ddsx = true;
      cnt_ddsx_del_inval++;
    }
    if (!rm_ddsx)
    {
      if (ddsx_ref.getNameId(hash_nm) < 0)
      {
        if (!quiet)
          conlog.addMessage(conlog.NOTE, "removing unused DDSx: %s (%s)", ddsx_list[i], hash_nm);
        rm_ddsx = true;
        cnt_ddsx_del_unref++;
      }
    }
    if (rm_ddsx)
      dd_erase(ddsx_list[i]);
  }
  clear_and_shrink(ddsx_list);
  if (cnt_ddsx_del_unref + cnt_ddsx_del_inval)
    conlog.addMessage(conlog.NOTE, "DDSx stat: removed %d (unreferenced) and %d (invalid)", cnt_ddsx_del_unref, cnt_ddsx_del_inval);

  // rebuild new index BLK
  FastNameMapEx guids;
  for (int i = 0, suff_len = i_strlen(type) + 5; i < blk_list.size(); i++)
    if (guids.addNameId(dd_get_fname(String(0, "%.*s", strlen(blk_list[i]) - suff_len, blk_list[i]))) != i)
    {
      conlog.addMessage(conlog.ERROR, "internal error while adding blk[%d]=%s", i, blk_list[i]);
      return 13;
    }

  for (int i = idxBlk.blockCount() - 1; i >= 0; i--)
    if (guids.getNameId(idxBlk.getBlock(i)->getBlockName()) < 0)
    {
      idxBlk.removeBlock(i);
      cnt_rm++;
    }

  for (int i = 0; i < blk_list.size(); i++)
  {
    DataBlock contentBlk;
    if (!contentBlk.load(blk_list[i]))
    {
      conlog.addMessage(conlog.ERROR, "failed to load blk[%d]=%s", i, blk_list[i]);
      return 13;
    }

    DataBlock *res = idxBlk.getBlockByName(guids.getName(i));
    if (!res)
    {
      res = idxBlk.addBlock(guids.getName(i));
      cnt_add++;
    }
    else if (cmp_blk_differ(*res, contentBlk))
      cnt_upd++;

    res->clearData();
    *res = contentBlk;
  }

  if (!check_bin_blk_signing_changed(idx_fn, cfgBlk.getStr("sign_private_key", NULL)) && !cmp_blk_differ(idxBlk, DataBlock(idx_fn)))
  {
    if (!quiet)
      conlog.addMessage(conlog.NOTE, "No changes to index: %s", idx_fn);
    write_index_hash(out_dir, idx_fn, cfgBlk, quiet);
    return 0;
  }

  conlog.addMessage(conlog.NOTE, "GUIDs stat: %d removed, %d added, %d updated", cnt_rm, cnt_add, cnt_upd);
  conlog.addMessage(conlog.NOTE, "using mod-time %d", f_time);

  dd_mkpath(idx_fn);
  bool force_legacy_blk_bbz = cfgBlk.getBool("forceBlkBBz", false);
  if (force_legacy_blk_bbz && !dblk::pack_to_bbz(idxBlk, idx_fn))
  {
    conlog.addMessage(conlog.ERROR, "failed to store %s (BBz)", idx_fn.str());
    return -3;
  }
  if (!force_legacy_blk_bbz && !dblk::pack_to_binary_file(idxBlk, idx_fn))
  {
    conlog.addMessage(conlog.ERROR, "failed to store %s", idx_fn.str());
    return -3;
  }
  if (!add_digital_signature(idx_fn, cfgBlk.getStr("sign_private_key", NULL), conlog))
  {
    dd_erase(idx_fn);
    return -4;
  }
  dag_set_file_time(idx_fn, f_time);
  write_index_hash(out_dir, idx_fn, cfgBlk, quiet);
  return 0;
}
static int do_split_pass(const DataBlock &cfgBlk, bool quiet)
{
  const char *out_dir = cfgBlk.getStr("outDir", ".");
  const char *index_filename = cfgBlk.getStr("indexFile", "?");
  const char *type = cfgBlk.getStr("type", "");
  String idx_fn(0, "%s/%s", out_dir, index_filename);
  int64_t f_time = time(NULL);
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Performing split for <%s> in: %s", type, out_dir);

  DataBlock idxBlk;
  if (dd_file_exists(idx_fn))
    if (!idxBlk.load(idx_fn))
    {
      conlog.addMessage(conlog.WARNING, "failed to read indexFile \"%s\", resetting", idx_fn.str());
      idxBlk.clearData();
    }

  int split_cnt = 0;
  dblk::iterate_child_blocks(idxBlk, [&](const DataBlock &b) {
    String dest_fn(0, "%s/%s.%s.blk", out_dir, b.getBlockName(), type);
    if (dblk::save_to_text_file(b, dest_fn))
      split_cnt++;
    else
      conlog.addMessage(conlog.ERROR, "failed to write %s", dest_fn);
  });
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Split %d BLKs from index \"%s\"", split_cnt, idx_fn.str());
  return 0;
}


int DagorWinMain(bool debugmode)
{
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;
  DataBlock::parseIncludesAsParams = true;

  signal(SIGINT, ctrl_break_handler);
  Tab<char *> argv(tmpmem);
  bool quiet = false;
  bool sweep_pass = false;
  bool split_pass = false;
  bool recreate_cache = false;
  bool sweep_erase_invalid = true;
  bool skip_update_idx = false;
  const char *out_list_fn = NULL;
  String cmd_blk;

  for (int i = 1; i < dgs_argc; i++)
  {
    if (dgs_argv[i][0] != '-')
      argv.push_back(dgs_argv[i]);
    else if (stricmp(&dgs_argv[i][1], "q") == 0)
      quiet = true;
    else if (stricmp(&dgs_argv[i][1], "sweep") == 0)
      sweep_pass = true;
    else if (stricmp(&dgs_argv[i][1], "split_idx") == 0)
      split_pass = true;
    else if (stricmp(&dgs_argv[i][1], "recreate_cache") == 0)
      recreate_cache = true;
    else if (stricmp(&dgs_argv[i][1], "sweepSoftly") == 0)
      sweep_pass = true, sweep_erase_invalid = false;
    else if (strnicmp(&dgs_argv[i][1], "outlist:", 8) == 0)
      out_list_fn = &dgs_argv[i][9];
    else if (strnicmp(&dgs_argv[i][1], "blk:", 4) == 0)
      cmd_blk.aprintf(0, "%s\n", &dgs_argv[i][5]);
    else if (stricmp(&dgs_argv[i][1], "noidx") == 0)
      skip_update_idx = true;
    else
    {
      print_title();
      printf("ERR: unknown option <%s>\n", dgs_argv[i]);
      return 1;
    }
  }

  if (argv.size() == 2 && !cmd_blk.empty())
    argv.push_back(NULL);
  if (argv.size() != ((sweep_pass || split_pass) ? 1 : 3))
  {
    print_title();
    printf("usage: customContentTool-dev.exe [options] <config_blk> <GUID> <src_skin_blk>|{-blk:<expr>}|nul\n"
           "options:\n"
           "  -q       run in quiet mode\n"
           "  -sweep   rebuilds content index BLK from existing *.<type>.blk and removes unreferenced DDSx\n"
           "  -noidx   skips index update (builds only DDSx and *.<type>.blk)\n"
           "  -outlist:<file.txt>  outputs changes list to <file.txt>\n");
    return -1;
  }
  if (sweep_pass && skip_update_idx)
  {
    print_title();
    printf("ERR: incompatible -sweep and -noidx switches!\n");
    return -1;
  }
  if (split_pass && sweep_pass)
  {
    printf("ERR: incompatible -sweep and -split_idx switches!\n");
    return -1;
  }

  if (!quiet)
    print_title();

  DataBlock cfgBlk;
  DataBlock::parseIncludesAsParams = false;
  if (!cfgBlk.load(argv[0]))
  {
    conlog.addMessage(conlog.ERROR, "Can't read config from: %s", argv[0]);
    return 13;
  }
  DataBlock::parseIncludesAsParams = true;
  hash_path_dir1 = cfgBlk.getInt("hashSplitDir1Cnt", 2);
  hash_path_dir2 = cfgBlk.getInt("hashSplitDir2Cnt", 0);

  ddsx::ConvertParams::forceZlibPacking = false;
  ddsx::ConvertParams::allowOodlePacking = false;
  ddsx::ConvertParams::preferZstdPacking = false;
  if (cfgBlk.getBool("allowOODLE", false))
    ddsx::ConvertParams::allowOodlePacking = ddsx::ConvertParams::preferZstdPacking = true;
  else if (cfgBlk.getBool("preferZSTD", false))
    ddsx::ConvertParams::preferZstdPacking = true;
  else if (cfgBlk.getBool("preferZLIB", false))
    ddsx::ConvertParams::forceZlibPacking = true;

  if (sweep_pass)
    return do_sweep_pass(cfgBlk, quiet, sweep_erase_invalid);
  if (split_pass)
    return do_split_pass(cfgBlk, quiet);

  char start_dir[260];
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));
  int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", start_dir));
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Loaded %d plugin(s)", pc);


  char data_dir[DAGOR_MAX_PATH];
  if (!argv[2])
    strcpy(data_dir, ".");
  else if (!dd_get_fname_location(data_dir, argv[2]))
    return -1;
  if (!data_dir[0])
    strcpy(data_dir, ".");
  DataBlock contentBlk;
  String contentFolder, contentBasefile;
  if (argv[2])
    split_path(argv[2], contentFolder, contentBasefile);
  else
    contentFolder = ".";
  contentFolder = String(dd_get_fname(contentFolder));
  if (char *p = (char *)dd_get_fname_ext(contentBasefile))
    *p = '\0';

  if (argv[2] && !contentBlk.load(argv[2]))
  {
    conlog.addMessage(conlog.ERROR, "Can't read source content data description from: %s", argv[2]);
    return 13;
  }
  if (!argv[2] && !dblk::load_text(contentBlk, cmd_blk))
  {
    conlog.addMessage(conlog.ERROR, "Can't parse source content data description from:\n%s", cmd_blk.str());
    return 13;
  }
  if (!quiet)
    conlog.addMessage(conlog.NOTE, "Processing folder: %s", data_dir);

  bool force_legacy_blk_bbz = cfgBlk.getBool("forceBlkBBz", false);
  const char *out_dir = cfgBlk.getStr("outDir", ".");
  String idx_fn(0, "%s/%s", out_dir, cfgBlk.getStr("indexFile", "?"));
  SimpleString hash_nm;

  DataBlock idxBlk;
  if (dd_file_exists(idx_fn))
    if (!idxBlk.load(idx_fn))
    {
      conlog.addMessage(conlog.ERROR, "failed to read indexFile \"%s\"", idx_fn.str());
      return -2;
    }
  idxBlk.removeParam("ver");

  const DataBlock &reSuffixBlk = *cfgBlk.getBlockByNameEx("cvtSuffix");
  Tab<RegExp *> reSuffix;
  reSuffix.resize(reSuffixBlk.paramCount());
  mem_set_0(reSuffix);
  for (int i = 0; i < reSuffixBlk.paramCount(); i++)
    if (reSuffixBlk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      reSuffix[i] = new RegExp;
      if (!reSuffix[i]->compile(reSuffixBlk.getStr(i), "i"))
      {
        conlog.addMessage(conlog.ERROR, "failed to compile regexp \"%s\"", reSuffixBlk.getStr(i));
        return -2;
      }
    }
    else
    {
      conlog.addMessage(conlog.ERROR, "bad parameter (non-text type) in cvtSuffix block");
      return -2;
    }

  int64_t f_time = time(NULL);
  conlog.addMessage(conlog.NOTE, "using mod-time %d", f_time);

  FastNameMap outNameList;
  if (!texconv::init(start_dir, cfgBlk.getStr("cacheDir", NULL)))
  {
    conlog.addMessage(conlog.ERROR, "failed to init texExp plugin for texture conversion (start_dir=%s)", start_dir);
    return 13;
  }
  const char *guid = contentBlk.getStr("guid", argv[1]);
  if (strchr(guid, '/') || strchr(guid, '\\'))
  {
    conlog.addMessage(conlog.ERROR, "invalid guid=%s (slashes ARE NOT ALLOWED!!!)", guid);
    return 13;
  }
  const char *tex_dir = contentBlk.getStr("texRoot", cfgBlk.getStr("texRoot", data_dir));
  if (strcmp(cfgBlk.getStr("type", ""), "skin") == 0)
  {
    DataBlock refBlk;
    String result_blk_fn(0, "%s/%s.skin.blk", out_dir, guid);
    String tmp_ref_ddsx_fn;
    if (recreate_cache)
      if (!refBlk.load(result_blk_fn))
      {
        conlog.addMessage(conlog.WARNING, "cannot load <%s> for recreate-cache mode", result_blk_fn);
        recreate_cache = false;
      }

    DataBlock *res = idxBlk.addBlock(guid);
    const char *diff_suffix = cfgBlk.getStr("diffSuffix", NULL);
    res->clearData();
    res->setStr("name", contentBlk.getStr("name", contentBlk.getStr("mangledID", NULL) ? "" : contentFolder));
    if (!res->getStr("name")[0])
      res->removeParam("name");
    res->setStr("modelName", contentBlk.getStr("modelName", contentBasefile));
    if (const char *nm = contentBlk.getStr("mangledID", NULL))
      res->setStr("mangledID", nm);
    int nid_replace_tex = contentBlk.getNameId("replace_tex");
    int nid_set_tex = contentBlk.getNameId("set_tex");
    Tab<DagorAsset *> assets;
    assets.reserve(contentBlk.blockCount() * 3);
    for (int i = 0; i < contentBlk.blockCount(); i++)
      if (contentBlk.getBlock(i)->getBlockNameId() == nid_replace_tex || contentBlk.getBlock(i)->getBlockNameId() == nid_set_tex)
      {
        const char *from_nm = contentBlk.getBlock(i)->getStr("from", NULL);
        if (!from_nm || !*from_nm)
        {
          conlog.addMessage(conlog.ERROR, "bad 'from' property in block #%d", i + 1);
          return -2;
        }
        const char *tex_fn = contentBlk.getBlock(i)->getStr("to", "");
        const char *ls = resolve_suffix(reSuffix, reSuffixBlk, contentBlk.getBlock(i)->getStr("from", ""));
        const char *ls2 = resolve_suffix(reSuffix, reSuffixBlk, contentBlk.getBlock(i)->getStr("to", ""));
        if (ls2 && *ls2 && cfgBlk.getBlockByNameEx(String(0, "lod0%s", ls2))->getStr("harmony_suffix", NULL))
          ls = ls2;
        for (int l = 0; l < 3; l++)
        {
          assets.push_back(
            make_texture_asset(String(0, "%s/%s", tex_dir, tex_fn), *cfgBlk.getBlockByNameEx(String(0, "lod%d%s", l, ls)), l));
          if (!assets.back())
          {
            conlog.addMessage(conlog.ERROR, "failed to convert \"%s\"", tex_fn);
            return -2;
          }
        }
      }

    for (int i = 0; i < assets.size(); i++)
      if (diff_suffix && *diff_suffix && !contentBlk.getBool("disableDiff", false))
        for (int i = 0; i < assets.size(); i++)
        {
          String diff_nm(0, "%.*s%s", strlen(assets[i]->getName()) - CRC32_USED_BITS / 4 - 1, assets[i]->getName(), diff_suffix);
          for (int j = 0; j < assets.size(); j++)
            if (i != j && !assets[i]->props.getBool("disableDiff", false) && !assets[j]->props.getBool("disableDiff", false) &&
                strcmp(diff_nm, String(0, "%.*s", strlen(assets[j]->getName()) - CRC32_USED_BITS / 4 - 1, assets[j]->getName())) == 0)
            {
              assets[i]->props.setStr("pairedToTex", assets[j]->getName());
              assets[j]->props.setStr("baseTex", assets[i]->getName());
            }
        }

    for (int i = 0, ai = 0; i < contentBlk.blockCount(); i++)
      if (contentBlk.getBlock(i)->getBlockNameId() == nid_replace_tex || contentBlk.getBlock(i)->getBlockNameId() == nid_set_tex)
      {
        const char *from_nm = contentBlk.getBlock(i)->getStr("from", NULL);
        DataBlock *b = res->addBlock(from_nm);
        const DataBlock &refBlkTo = *refBlk.getBlockByNameEx(b->getBlockName());
        const char *tex_fn = contentBlk.getBlock(i)->getStr("to", "");
        const char *lod_suffix = resolve_suffix(reSuffix, reSuffixBlk, contentBlk.getBlock(i)->getStr("from", ""));
        for (int lod = 0; lod < 3; lod++, ai++)
        {
          String lod_nm(0, "lod%d", lod);
          if (recreate_cache)
          {
            if (const char *lod_fn = refBlkTo.getStr(lod_nm, nullptr))
              tmp_ref_ddsx_fn = make_path_by_hash(lod_fn, out_dir);
            else
              tmp_ref_ddsx_fn.clear();
          }
          if (!quiet)
            conlog.addMessage(conlog.NOTE, "Converting: %s for %s%s", tex_fn, lod_nm, lod_suffix);
          hash_nm = convert_texture(assets[ai], out_dir, f_time, !tmp_ref_ddsx_fn.empty() ? tmp_ref_ddsx_fn.str() : nullptr);
          if (hash_nm.empty())
          {
            conlog.addMessage(conlog.ERROR, "failed to convert \"%s\" for in block #%d", tex_fn, i + 1);
            return -2;
          }
          outNameList.addNameId(make_path_by_hash(hash_nm));
          b->setStr(lod_nm, hash_nm);
          if (const char *base_nm = assets[ai]->props.getStr("baseTex", NULL))
            for (int j = 0; j < assets.size(); j++)
              if (strcmp(base_nm, assets[j]->getName()) == 0)
              {
                if (recreate_cache)
                {
                  if (const char *lod_fn = refBlkTo.getStr(lod_nm + "_b", nullptr))
                    tmp_ref_ddsx_fn = make_path_by_hash(lod_fn, out_dir);
                  else
                    tmp_ref_ddsx_fn.clear();
                }
                b->setStr(lod_nm + "_b",
                  convert_texture(assets[j], out_dir, f_time, !tmp_ref_ddsx_fn.empty() ? tmp_ref_ddsx_fn.str() : nullptr));
                break;
              }
        }
        TextureMetaData tmd;
        tmd.read(*cfgBlk.getBlockByNameEx(String(0, "lod0%s", lod_suffix)), "PC");
        const char *tp = tmd.encode("");
        if (*tp)
          b->setStr("texProps", tp);
        if (contentBlk.getBlock(i)->getBlockNameId() == nid_set_tex)
          b->setStr("param", contentBlk.getBlock(i)->getStr("param", ""));
      }
    res->saveToTextFile(result_blk_fn);
    outNameList.addNameId(String(0, "%s.skin.blk", guid));
  }
  else if (strcmp(cfgBlk.getStr("type", ""), "decal") == 0)
  {
    DataBlock refBlk;
    String result_blk_fn(0, "%s/%s.decal.blk", out_dir, guid);
    String tmp_ref_ddsx_fn;
    if (recreate_cache)
      if (!refBlk.load(result_blk_fn))
      {
        conlog.addMessage(conlog.WARNING, "cannot load <%s> for recreate-cache mode", result_blk_fn);
        recreate_cache = false;
      }

    DataBlock *res = idxBlk.addBlock(guid);
    res->clearData();
    const DataBlock &cblk = contentBlk.blockCount() == 1 && contentBlk.paramCount() == 0 ? *contentBlk.getBlock(0) : contentBlk;
    const char *tex_fn = (&cblk == &contentBlk) ? cblk.getStr("tex", "") : cblk.getBlockName();
    float forced_ar = cblk.getReal("aspect_ratio", -1);

    if (const char *name = cblk.getStr("name", NULL))
      if (name[0])
        res->setStr("name", name);
    if (const char *desc = cblk.getStr("desc", NULL))
      res->setStr("desc", desc);
    res->setStr("category", cblk.getStr("category", ""));
    res->setReal("aspect_ratio", forced_ar > 0 ? forced_ar : 1.0f);
    if (const DataBlock *b = cblk.getBlockByName("countries"))
      res->addNewBlock(b, "countries");
    for (int lod = 0; lod < 2; lod++)
    {
      String lod_nm(0, "lod%d", lod);
      if (recreate_cache)
        tmp_ref_ddsx_fn = make_path_by_hash(refBlk.getStr(lod_nm), out_dir);
      if (!quiet)
        conlog.addMessage(conlog.NOTE, "Converting: %s for %s", tex_fn, lod_nm);
      hash_nm = convert_texture(make_texture_asset(String(0, "%s/%s", tex_dir, tex_fn), *cfgBlk.getBlockByNameEx(lod_nm), lod),
        out_dir, f_time, recreate_cache ? tmp_ref_ddsx_fn.str() : nullptr);
      if (hash_nm.empty())
      {
        conlog.addMessage(conlog.ERROR, "failed to convert \"%s\"", tex_fn);
        return -2;
      }
      outNameList.addNameId(make_path_by_hash(hash_nm));

      {
        FullFileLoadCB tex_crd(make_path_by_hash(hash_nm, out_dir));
        if (tex_crd.fileHandle)
        {
          ddsx::Header hdr;
          tex_crd.read(&hdr, sizeof(hdr));
          G_ASSERT(hdr.label == _MAKE4C('DDSx'));
          if (lod == 0 && forced_ar <= 0)
          {
            forced_ar = float(hdr.w) / float(hdr.h);
            conlog.addMessage(conlog.NOTE, "  decal sz %dx%d, aspec ratio: %.3f", hdr.w, hdr.h, forced_ar);
            res->setReal("aspect_ratio", forced_ar);
          }

          // remove packing info
          hdr.packedSz = 0;
          hdr.flags &= ~hdr.FLG_COMPR_MASK;

          // write DDSx header as encoded base64 string
          SimpleString stor;
          res->setStr(String(0, "hdr%d", lod), data_to_str_b64(stor, &hdr, sizeof(hdr)));
        }
      }
      res->setStr(lod_nm, hash_nm);
    }
    res->saveToTextFile(result_blk_fn);
    outNameList.addNameId(String(0, "%s.decal.blk", guid));
  }
  else if (strcmp(cfgBlk.getStr("type", ""), "remove") == 0)
  {
    if (!idxBlk.removeBlock(guid))
    {
      conlog.addMessage(conlog.ERROR, "cannot find <%s> in <%s> to delete", guid, idx_fn);
      return -2;
    }
    conlog.addMessage(conlog.NOTE, "Removed %s", guid);
  }
  else
  {
    conlog.addMessage(conlog.ERROR, "unknown content type: <%s>", cfgBlk.getStr("type", ""));
    return -2;
  }

  if (out_list_fn)
  {
    if (!quiet)
      conlog.addMessage(conlog.NOTE, "Writing file list to : %s", out_list_fn);
    file_ptr_t fp = df_open(out_list_fn, DF_CREATE | DF_WRITE);
    iterate_names_in_lexical_order(outNameList, [&](int, const char *name) { df_printf(fp, "%s\n", name); });
    df_close(fp);
  }

  if (skip_update_idx)
    goto cleanup_end;

  if (!check_bin_blk_signing_changed(idx_fn, cfgBlk.getStr("sign_private_key", NULL)) && !cmp_blk_differ(idxBlk, DataBlock(idx_fn)))
  {
    if (!quiet)
      conlog.addMessage(conlog.NOTE, "No changes to index: %s", idx_fn);
    goto finalize_changes;
  }

  dd_mkpath(idx_fn);
  if (force_legacy_blk_bbz && !dblk::pack_to_bbz(idxBlk, idx_fn))
  {
    conlog.addMessage(conlog.ERROR, "failed to store %s (BBz)", idx_fn.str());
    return -3;
  }
  if (!force_legacy_blk_bbz && !dblk::pack_to_binary_file(idxBlk, idx_fn))
  {
    conlog.addMessage(conlog.ERROR, "failed to store %s", idx_fn.str());
    return -3;
  }
  if (!add_digital_signature(idx_fn, cfgBlk.getStr("sign_private_key", NULL), conlog))
  {
    dd_erase(idx_fn);
    return -4;
  }
  dag_set_file_time(idx_fn, f_time);

finalize_changes:
  if (cfgBlk.getBool("writeIndexHashAlways", false))
    if (!write_index_hash(out_dir, idx_fn, cfgBlk, quiet))
      return -4;

  // clean up
cleanup_end:
  texconv::term();
  clear_all_ptr_items(reSuffix);

  if (!quiet)
    iterate_names(outNameList, [&](int, const char *name) { conlog.addMessage(conlog.NOTE, "  %s", name); });

  return 0;
}
