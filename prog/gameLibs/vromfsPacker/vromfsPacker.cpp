// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <regExp/regExp.h>
#include <libTools/util/hash.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_memIo.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_basePath.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_roNameMap.h>
#include <util/dag_fastIntList.h>
#include <util/dag_strUtil.h>
#include <util/dag_base32.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stddef.h> // offsetof
#include <supp/dag_zstdObfuscate.h>
#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID
#include <unistd.h>
#elif _TARGET_PC_WIN
#include <direct.h>
#endif

#define USE_OPENSSL

#ifndef USE_OPENSSL
#include <md5.h>
#define MD5_CONTEXT md5_state_t
#define MD5_INIT    md5_init
#define MD5_UPDATE  md5_append
#define MD5_FINAL   md5_finish
#else
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#define MD5_CONTEXT           MD5_CTX
#define MD5_INIT              MD5_Init
#define MD5_UPDATE            MD5_Update
#define MD5_FINAL(ctx_, out_) MD5_Final(out_, ctx_)
#endif
#include <hash/BLAKE3/blake3.h>

namespace dblk
{
void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);
}

#define FS_OFFS int(offsetof(VirtualRomFsData, files))
static const int C_SHA1_RECSZ = VirtualRomFsPack::BackedData::C_SHA1_RECSZ;
static const int ZSTD_BLK_CLEVEL = 11;

static const char *targetString = NULL;
using RegExpPtr = eastl::unique_ptr<RegExp>;
static Tab<RegExpPtr> reExcludeFile(tmpmem);
static Tab<RegExpPtr> reIncludeBlk(tmpmem), reExcludeBlk(tmpmem);
static Tab<RegExpPtr> reIncludeKeepLines(tmpmem);
static bool blkCheckOnly = false;
static size_t z1_blk_sz = 1 << 20;
static bool force_legacy_format = true;
static unsigned write_version = 0;
static const char *version_legacy_fn = "version";
OAHashNameMap<true> preproc_defines;
static const char *export_data_for_dict_dir = nullptr;

static unsigned get_target_code(const char *targetStr)
{
  unsigned targetCode = 0;
  while (*targetStr)
  {
    targetCode = (targetCode >> 8) | (*targetStr << 24);
    targetStr++;
  }
  return targetCode;
}

bool loadPatterns(const DataBlock &b, const char *name, Tab<RegExpPtr> &reList)
{
  int nid = b.getNameId(name);
  for (int j = 0; j < b.paramCount(); j++)
    if (b.getParamNameId(j) == nid && b.getParamType(j) == DataBlock::TYPE_STRING)
    {
      const char *pattern = b.getStr(j);
      RegExpPtr re(new RegExp);
      if (re->compile(pattern, "i"))
        reList.push_back(eastl::move(re));
      else
      {
        printf("ERR: bad regexp /%s/\n", pattern);
        return false;
      }
    }
  return true;
}

bool checkPattern(const char *str, dag::ConstSpan<RegExpPtr> re)
{
  for (int i = 0; i < re.size(); i++)
    if (re[i]->test(str))
      return true;
  return false;
}

inline char *simplify_name(char *s, bool lwr = false)
{
  dd_simplify_fname_c(s);
  if (lwr)
    dd_strlwr(s);

  return s;
}
static void calc_md5(unsigned char out_hash[16], const MemoryChainedData *mem)
{
  memset(out_hash, 0, 16);
  MD5_CONTEXT md5s;
  MD5_INIT(&md5s);
  while (mem)
  {
    if (!mem->used)
      break;
    MD5_UPDATE(&md5s, (const unsigned char *)mem->data, mem->used);
    mem = mem->next;
  }
  MD5_FINAL(&md5s, out_hash);
}

String make_fname(const char *sname, const char *src_folder, const char *dst_folder)
{
  char folder[260];
  strcpy(folder, src_folder);
  simplify_name(folder);

  G_ASSERTF(dd_strnicmp(sname, folder, (int)strlen(folder)) == 0, "sname=<%s> folder=<%s>", sname, folder);
  String s(dst_folder);
  s += String::mk_sub_str(sname + strlen(folder), sname + strlen(sname));

  simplify_name(s);
  return s;
}

void scan_files(const char *root_path, const char *src_folder, const char *dst_folder, Tab<SimpleString> &wclist,
  dag::ConstSpan<RegExpPtr> exclist, bool scan_files, bool scan_subfolders, FastNameMapEx &files, Tab<String> &dst_files)
{
  alefind_t ff;
  String tmpPath;
  String srcDir(0, "%s/%s", root_path, src_folder);

  // scan for files
  if (scan_files)
    for (int i = 0; i < wclist.size(); i++)
    {
      tmpPath.printf(260, "%s/%s", srcDir, wclist[i].str());
      if (::dd_find_first(tmpPath, 0, &ff))
      {
        do
        {
          tmpPath.printf(260, "%s/%s", srcDir, ff.name);
          char *sname = simplify_name(tmpPath);
          if (write_version && force_legacy_format && strcmp(dd_get_fname(sname), version_legacy_fn) == 0)
            continue;
          if (checkPattern(sname, reExcludeFile) || checkPattern(sname, exclist))
          {
            if (!::dgs_execute_quiet)
              printf("EXCLUDE: %s\n", sname);
            continue;
          }

          int id = files.addNameId(sname);
          if (dst_files.size() <= id)
            dst_files.resize(id + 1);
          dst_files[id] = make_fname(sname, srcDir, dst_folder);
        } while (dd_find_next(&ff));
        dd_find_close(&ff);
      }
    }

  // scan for sub-folders
  if (scan_subfolders)
  {
    tmpPath.printf(260, "%s/%s/*", root_path, src_folder);
    if (::dd_find_first(tmpPath, DA_SUBDIR, &ff))
    {
      do
        if (ff.attr & DA_SUBDIR)
        {
          if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0)
            continue;

          ::scan_files(root_path, String(260, "%s/%s", src_folder, ff.name).str(), String(260, "%s/%s", dst_folder, ff.name).str(),
            wclist, exclist, true, scan_subfolders, files, dst_files);
        }
      while (dd_find_next(&ff));
      dd_find_close(&ff);
    }
  }
}

static bool write_digital_signature(FullFileSaveCB &cwr, const char *infname, MemoryChainedData *mcd, const DataBlock &inp)
{
  bool retval = true;
#ifdef USE_OPENSSL
  unsigned char *signature = NULL;
  EVP_MD_CTX *ctx = NULL;
  EVP_PKEY *pkey = NULL;
  const EVP_MD *md = NULL;
  const char *private_key = inp.getStr("sign_private_key", "private.pem");
  if (!*private_key)
    return true;
  FILE *fp = NULL;

  unsigned real_sign_size = 0;
  FullFileLoadCB fcrd(infname);
  MemoryLoadCB mcrd;
  const char *digest = inp.getStr("sign_digest", "sha256");
  if (!fcrd.fileHandle)
    goto done;
  fp = fopen(private_key, "r");
  if (!fp)
  {
    printf("ERR: can't open private key '%s'\n", private_key);
    retval = false;
    goto done;
  }
  pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
  if (!pkey)
  {
    printf("ERR: error reading private key from '%s'\n", private_key);
    ERR_print_errors_fp(stdout);
    retval = false;
    goto done;
  }
  fclose(fp);
  fp = NULL;
  OpenSSL_add_all_digests();
  md = EVP_get_digestbyname(digest);
  if (!md)
  {
    printf("ERR: can't get digest '%s'\n", digest);
    retval = false;
    goto done;
  }
  signature = (unsigned char *)alloca((int)EVP_PKEY_size(pkey));

  ctx = EVP_MD_CTX_create();
  if (EVP_SignInit(ctx, md))
  {
    IGenLoad &crd = mcd ? (IGenLoad &)mcrd : (IGenLoad &)fcrd;
    if (mcd)
      mcrd.setMem(mcd, false);
    unsigned char buf[4096];
    while (1)
    {
      int sz = crd.tryRead(buf, sizeof(buf));
      if (!sz)
        break;
      if (!EVP_SignUpdate(ctx, buf, sz))
        goto sign_fail;
    }
    const char *fileName = dd_get_fname(infname);
    if (!fileName || !*fileName)
    {
      printf("ERR: could not determine vrom file name, infname: '%s'\n", infname);
      retval = false;
      goto done;
    }
    int fileNameLength = i_strlen(fileName);
    if (!EVP_SignUpdate(ctx, fileName, fileNameLength))
      goto sign_fail;
    if (!EVP_SignFinal(ctx, signature, &real_sign_size, pkey))
      goto sign_fail;
    cwr.open(infname, DF_WRITE | DF_APPEND);
    cwr.write(signature, real_sign_size);
    cwr.close();
    printf("written digital signature '%s' of size %d; file name: '%s'\n", digest, real_sign_size, fileName);
  }
  else
  {
  sign_fail:
    printf("ERR: signing failed\n");
    retval = false;
    ERR_print_errors_fp(stdout);
  }
done:
  if (ctx)
    EVP_MD_CTX_destroy(ctx);
  if (pkey)
    EVP_PKEY_free(pkey);
  if (fp)
    fclose(fp);
  EVP_cleanup();
  fcrd.close();
#endif
  fflush(stdout);
  return retval;
}

void gatherFilesList(const DataBlock &blk, FastNameMapEx &files, Tab<String> &dst_files, unsigned targetCode, const char *output)
{
  String out_pname(32, "output_%s", mkbindump::get_target_str(targetCode));
  const char *def_outdir = blk.getStr(out_pname, blk.getStr("output", ""));

  Tab<SimpleString> wclist(tmpmem);
  Tab<RegExpPtr> exclist(tmpmem);
  int folder_nid = blk.getNameId("folder");
  int wildcard_nid = blk.getNameId("wildcard");
  int file_nid = blk.getNameId("file");
  const char *root_path = blk.getStr(String(0, "rootFolder_%s", mkbindump::get_target_str(targetCode)), blk.getStr("rootFolder", "."));
  int platform_nid = blk.getNameId("platform");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == folder_nid)
    {
      const DataBlock &b = *blk.getBlock(i);
      if (stricmp(b.getStr(out_pname, def_outdir), output) != 0)
        continue;
      if (b.getStr("platform", NULL))
      {
        bool included = false;
        for (int i = 0; i < b.paramCount(); i++)
          if (b.getParamNameId(i) == platform_nid && b.getParamType(i) == DataBlock::TYPE_STRING)
            if (targetCode == get_target_code(b.getStr(i)))
            {
              included = true;
              break;
            }
        if (!included)
          continue;
      }

      const char *src_folder = b.getStr("path", ".");
      const char *dst_folder = b.getStr("dest_path", src_folder);

      wclist.clear();
      for (int j = 0; j < b.paramCount(); j++)
        if (b.getParamNameId(j) == wildcard_nid && b.getParamType(j) == DataBlock::TYPE_STRING)
          wclist.push_back() = b.getStr(j);
      if (!wclist.size())
      {
        printf("WARN: empty wildcard list in block #%d", i + 1);
        continue;
      }

      clear_and_shrink(exclist);
      if (!loadPatterns(b, "exclude", exclist))
      {
        printf("WARN: failed to load exclude patterns in block #%d", i + 1);
        continue;
      }

      scan_files(root_path, src_folder, dst_folder, wclist, exclist, b.getBool("scan_files", true), b.getBool("scan_subfolders", true),
        files, dst_files);
    }

  clear_and_shrink(exclist);

  if (stricmp(def_outdir, output) != 0)
    return;
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == file_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      String fpath(260, "%s/%s", root_path, blk.getStr(i));
      char *sname = simplify_name(fpath);
      int id = files.addNameId(sname);
      if (dst_files.size() <= id)
        dst_files.resize(id + 1);
      dst_files[id] = sname;
    }
}

void process_and_copy_file_to_stream(const char *fname, IGenSave &cwr, const char *targetString, bool keepLines);

namespace dblk
{
extern bool add_name_to_name_map(DBNameMap &nm, const char *s);
}

class MultiOutputSave : public IGenSave
{
public:
  MultiOutputSave(IGenSave &out) { addOut(out); }

  void addOut(IGenSave &out) { outs.push_back(&out); }

  void write(const void *data, int sz) override
  {
    for (IGenSave *out : outs)
      out->write(data, sz);
  }

  int tell() override
  {
    G_ASSERTF(0, "Not supported");
    return 0;
  }
  const char *getTargetName() override
  {
    G_ASSERTF(0, "Not supported");
    return "";
  }
  void flush() override { G_ASSERTF(0, "Not supported"); }
  void seekto(int) override { G_ASSERTF(0, "Not supported"); }
  void seektoend(int) override { G_ASSERTF(0, "Not supported"); }
  void beginBlock() override { G_ASSERTF(0, "Not supported"); }
  void endBlock(unsigned) override { G_ASSERTF(0, "Not supported"); }
  int getBlockLevel() override
  {
    G_ASSERTF(0, "Not supported");
    return 0;
  }

private:
  Tab<IGenSave *> outs;
};

bool buildVromfsDump(const char *fname, unsigned targetCode, FastNameMapEx &files, Tab<String> &dst_files,
  OAHashNameMap<true> &parse_ext, bool zpack, const DataBlock &inp, bool content_sha1, const char *alt_outdir)
{
  int reft = get_time_msec();
  if (!fname)
  {
    printf("ERR: output file not defined!");
    return false;
  }
  const DataBlock &vfs_props = *inp.getBlockByNameEx("per_output_setup")->getBlockByNameEx(dd_get_fname(fname));
#define READ_PROP(TYPE, NAME, DEF) vfs_props.get##TYPE(NAME, inp.get##TYPE(NAME, DEF))

  String alt_fname_stor;
  if (alt_outdir)
  {
    alt_fname_stor.printf(0, "%s/%s", alt_outdir, dd_get_fname(fname));
    fname = alt_fname_stor;
  }

  bool force_legacy_blk_bbf3 = READ_PROP(Bool, "forceBlkBBF3", false);
  DBNameMap *shared_nm = nullptr;
  String shared_namemap_fn;
  int shared_nm_sz = 0, shared_nm_nm = 0, shared_nm_sz_inc = 0, shared_nm_nm_inc = 0, shared_nm_str_inc = 0;
  int blk2_packed_count = 0;
  if (bool use_shared_namemap = force_legacy_blk_bbf3 ? false : READ_PROP(Bool, "blkUseSharedNamemap", false))
  {
    shared_namemap_fn.printf(0, "%s/%s-shared_nm.bin", inp.getStr("blkSharedNamemapLocation", "."), dd_get_fname(fname));
    shared_nm = dblk::create_db_names();
    dd_mkpath(shared_namemap_fn);
    FullFileLoadCB crd(shared_namemap_fn);
    if (crd.fileHandle)
      dblk::read_names(crd, *shared_nm, nullptr);
    else
      printf("WARN: shared namemap file <%s> not found, will create anew\n", shared_namemap_fn.str());
    shared_nm_sz = crd.getTargetDataSize();
    shared_nm_nm = dblk::db_names_count(shared_nm);

    files.addNameId(dblk::SHARED_NAMEMAP_FNAME);
    dst_files.push_back() = dblk::SHARED_NAMEMAP_FNAME;
  }
  else if (export_data_for_dict_dir)
    return true;

  ZSTD_CDict_s *zstd_cdict = nullptr;
  uint8_t zstd_dict_blake3_digest[BLAKE3_OUT_LEN];
  memset(zstd_dict_blake3_digest, 0, sizeof(zstd_dict_blake3_digest));
  if (!export_data_for_dict_dir)
    if (const char *dict_fn = force_legacy_blk_bbf3 ? nullptr : READ_PROP(Str, "blkUseComprDict", nullptr))
    {
      FullFileLoadCB crd(dict_fn);
      if (!crd.fileHandle)
      {
        printf("ERR: ZTD compression dictionary %s not found\n", dict_fn);
        remove(fname);
        exit(13);
      }

      Tab<char> dict_buf;
      dict_buf.resize(df_length(crd.fileHandle));
      crd.readTabData(dict_buf);

      blake3_hasher hasher;
      blake3_hasher_init(&hasher);
      blake3_hasher_update(&hasher, dict_buf.data(), data_size(dict_buf));
      blake3_hasher_finalize(&hasher, zstd_dict_blake3_digest, BLAKE3_OUT_LEN);
      String tmpstr;

      zstd_cdict = zstd_create_cdict(dict_buf, ZSTD_BLK_CLEVEL);
      printf("INFO: using compr. dict. %s: zstd_cdict=%p sz=%dK (blake3: %s)\n", dict_fn, zstd_cdict, int(dict_buf.size() >> 10),
        data_to_str_hex(tmpstr, zstd_dict_blake3_digest, BLAKE3_OUT_LEN));
      if (READ_PROP(Bool, "embedComprDictToVrom", false))
      {
        files.addNameId(dict_fn);
        dst_files.push_back() = tmpstr + ".dict";
      }
    }

  Tab<int> name_ofs(tmpmem);
  Tab<IPoint2> data_ofs(tmpmem);
  /*
  printf("%d files:\n", files.nameCount());
  iterate_names(files, [](int, const char *name) { printf("%s\n", name); });
  */

  bool blk_err = false;
  bool write_be = dagor_target_code_be(targetCode);
  if (write_be)
  {
    printf("ERR: big endian format not supported!\n");
    remove(fname);
    exit(13);
  }
  bool sign_contents = READ_PROP(Bool, "signedContents", true);
  bool sign_packed = READ_PROP(Bool, "signPacked", true);
  bool sign_plain_data = !force_legacy_format && shared_nm && sign_contents && !sign_packed;

  mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
  mkbindump::PatchTabRef pt_names, pt_data;
  pt_names.reserveTab(cwr);
  pt_data.reserveTab(cwr);
  int hdr_sz_pos = cwr.tell();
  if (!zpack || content_sha1)
    cwr.writeZeroes(16);

  FastNameMapEx sorted_fnlist;
  String tmpbuf;
  for (int id = 0; id < files.nameCount(); id++)
  {
    tmpbuf = dst_files[id];
    dd_strlwr(tmpbuf);
    int idx = sorted_fnlist.addNameId(tmpbuf.data());
    if (idx + 1 != sorted_fnlist.nameCount())
    {
      printf("ERR: duplicate DEST <%s>\n", tmpbuf.data());
      remove(fname);
      exit(13);
    }
  }

  SmallTab<int> sorted_fnlist_ids;
  gather_ids_in_lexical_order(sorted_fnlist_ids, sorted_fnlist);

  pt_names.reserveData(cwr, files.nameCount(), cwr.PTR_SZ);
  cwr.align16();
  int names_data_ofs = cwr.tell();
  name_ofs.resize(files.nameCount());
  int i = 0;
  iterate_names_in_order(sorted_fnlist, sorted_fnlist_ids, [&](int, const char *name) {
    name_ofs[i] = cwr.tell();
    cwr.writeRaw(name, (int)strlen(name) + 1);
    if (!::dgs_execute_quiet)
      printf("DST: %s\n", name);
    i++;
  });
  int names_data_size = cwr.tell() - names_data_ofs;

  cwr.align16();
  pt_data.reserveData(cwr, files.nameCount(), cwr.TAB_SZ);

  Tab<uint8_t> file_sha1;
  int file_sha1_pos = -1;
  if (content_sha1)
  {
    file_sha1.resize(C_SHA1_RECSZ * sorted_fnlist.nameCount());
    mem_set_0(file_sha1);
    file_sha1_pos = cwr.tell();
    cwr.writeInt32eAt(file_sha1_pos, hdr_sz_pos + 8);
    cwr.writeRaw(file_sha1.data(), data_size(file_sha1));
  }

  struct BlkFileUsedNotify : public DataBlock::IFileNotify
  {
    VirtualRomFsSingleFile *vrom = NULL;
    DynamicMemGeneralSaveCB memCwr = DynamicMemGeneralSaveCB(tmpmem);
    const char *targetString = NULL;
    BlkFileUsedNotify(const char *tgt) { targetString = tgt; }
    ~BlkFileUsedNotify() { releaseCurVrom(); }

    virtual void onFileLoaded(const char *fname)
    {
      if (!dd_file_exists(fname))
      {
        printf("ERR: failed to load include file %s\n", fname);
        return;
      }
      releaseCurVrom();
      memCwr.setsize(0);
      if (!::dgs_execute_quiet)
        debug("preprocessing included file: %s", fname);
      process_and_copy_file_to_stream(fname, memCwr, targetString, false);
      vrom = VirtualRomFsSingleFile::make_mem_data(tmpmem, memCwr.data(), memCwr.size(), fname, true);
      add_vromfs(vrom, true);
    }
    void releaseCurVrom()
    {
      if (!vrom)
        return;
      remove_vromfs(vrom);
      vrom->release();
      vrom = NULL;
    }
  } fnotify(targetString);


  Tab<DataBlock *> preloaded_blk;
  preloaded_blk.resize(files.nameCount());
  mem_set_0(preloaded_blk);

  Tab<uint8_t> file_attributes;
  bool enable_file_attributes = sign_plain_data;
  int file_attributes_pos = -1;
  if (enable_file_attributes)
  {
    file_attributes.resize(files.nameCount());
    mem_set_0(file_attributes);
    file_attributes_pos = cwr.tell();
    cwr.getRawWriter().writeIntP<1>(1); // attributes version
    cwr.writeRaw(file_attributes.data(), data_size(file_attributes));
  }

  if (shared_nm)
  {
    // first pass to gather actual shared namemap
    for (int id : sorted_fnlist_ids)
    {
      const char *orig_fn = files.getName(id);
      bool convert_blk = !checkPattern(orig_fn, reExcludeBlk);
      if (convert_blk)
        convert_blk = checkPattern(orig_fn, reIncludeBlk);

      if (!convert_blk)
        continue;

      bool preprocess = false;
      bool keep_lines = false;
      if (const char *ext = dd_get_fname_ext(orig_fn))
      {
        ext++;
        preprocess = (parse_ext.getNameId(ext) != -1);
        keep_lines = checkPattern(orig_fn, reIncludeKeepLines);
      }

      DynamicMemGeneralSaveCB memCwr(tmpmem);
      if (preprocess)
        process_and_copy_file_to_stream(orig_fn, memCwr, targetString, keep_lines);
      else
        copy_file_to_stream(orig_fn, memCwr);

      InPlaceMemLoadCB memCrd(memCwr.data(), memCwr.size());
      DataBlock &blk = *(preloaded_blk[id] = new DataBlock);
      if (!dblk::load_from_stream(blk, memCrd, dblk::ReadFlag::ALLOW_SS, orig_fn, &fnotify))
      {
        printf("ERR: read BLK %s\n", orig_fn);
        blk_err = true;
        continue;
      }
      blk.appendNamemapToSharedNamemap(*shared_nm, nullptr);
      blk2_packed_count++;
    }

    // another pass to add all strings to namemap
    int all_nm_count = dblk::db_names_count(shared_nm);
    shared_nm_nm_inc = all_nm_count - shared_nm_nm;
    if (READ_PROP(Bool, "blkShareStrParams", true))
      for (int id : sorted_fnlist_ids)
      {
        if (const DataBlock *blk = preloaded_blk[id])
          dblk::iterate_blocks(*blk, [&](const DataBlock &d) {
            for (uint32_t i = 0, e = d.paramCount(); i < e; ++i)
              if (d.getParamType(i) == DataBlock::TYPE_STRING)
                dblk::add_name_to_name_map(*shared_nm, d.getStr(i));
          });
      }

    shared_nm_nm = dblk::db_names_count(shared_nm);
    shared_nm_str_inc = shared_nm_nm - all_nm_count;
    // if no name or string added we shall not resave identical namemap
    if (shared_nm_nm_inc + shared_nm_str_inc) // -V793
    {
      FullFileSaveCB cwr(shared_namemap_fn);
      dblk::write_names(cwr, *shared_nm, nullptr);
      shared_nm_sz_inc = cwr.tell() - shared_nm_sz;
      shared_nm_sz = cwr.tell();
    }
  }

  FullFileSaveCB cwr_data_for_dict;
  if (export_data_for_dict_dir)
  {
    if (!shared_nm)
    {
      printf("ERR: no sense in building data for dictionary when blkUseSharedNamemap:t= not set\n");
      exit(13);
    }
    String dest_fn(0, "%s/%s.dict-data.bin", export_data_for_dict_dir, dd_get_fname(fname));
    dd_mkpath(dest_fn);
    cwr_data_for_dict.open(dest_fn, DF_WRITE);
    if (!cwr_data_for_dict.fileHandle)
    {
      printf("ERR: cannot create %s to write data for dictionary\n", cwr_data_for_dict.getTargetName());
      exit(13);
    }
  }
#undef READ_PROP

  cwr.align16();
  data_ofs.resize(files.nameCount());
  MemorySaveCB plain_cwr(1 << 20);
  plain_cwr.setMcdMinMax(1 << 20, 8 << 20);
  MemorySaveCB tmp_cwr(128 << 10);

  int mi = 0;
  for (int id : sorted_fnlist_ids)
  {
    const char *orig_fn = files.getName(id);
    if (trail_stricmp(orig_fn, ".shdump.bin"))
      cwr.align16();

    data_ofs[mi][0] = cwr.tell();

    bool preprocess = false;
    bool keep_lines = false;

    const char *ext = dd_get_fname_ext(orig_fn);
    if (ext)
    {
      ext++;
      preprocess = (parse_ext.getNameId(ext) != -1);
      keep_lines = checkPattern(orig_fn, reIncludeKeepLines);
    }

    bool convert_blk = !checkPattern(orig_fn, reExcludeBlk);
    if (convert_blk)
      convert_blk = checkPattern(orig_fn, reIncludeBlk);

    IGenSave *file_data_cwr = &cwr.getRawWriter();
    if (content_sha1)
      file_data_cwr = create_hash_computer_cb(HASH_SAVECB_SHA1, file_data_cwr);

    MultiOutputSave mos_cwr(*file_data_cwr);
    if (sign_plain_data)
      mos_cwr.addOut(plain_cwr);

    if (preloaded_blk[id])
    {
      G_ASSERT(shared_nm);

      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_BLK;

      if (!export_data_for_dict_dir)
      {
        if (sign_plain_data)
        {
          tmp_cwr.seekto(0);
          preloaded_blk[id]->saveDumpToBinStream(tmp_cwr, shared_nm);
          int sz = tmp_cwr.tell();
          MemoryLoadCB mcrd(tmp_cwr.getMem(), false);
          dblk::pack_shared_nm_dump_to_stream(*file_data_cwr, mcrd, sz, zstd_cdict);
          mcrd.seekto(0);
          copy_stream_to_stream(mcrd, plain_cwr, sz);
        }
        else
        {
          preloaded_blk[id]->saveBinDumpWithSharedNamemap(*file_data_cwr, shared_nm, true, zstd_cdict);
        }
      }
      else
      {
        cwr_data_for_dict.beginBlock();
        preloaded_blk[id]->saveBinDumpWithSharedNamemap(cwr_data_for_dict, shared_nm);
        cwr_data_for_dict.endBlock();
        if (file_data_cwr != &cwr.getRawWriter())
          destory_hash_computer_cb(file_data_cwr);
        continue; // we are not going to build target vrom
      }
      goto done_write;
    }

    if (shared_nm && strcmp(orig_fn, dblk::SHARED_NAMEMAP_FNAME) == 0)
    {
      MemorySaveCB mcwr(1 << 20);
      uint64_t names_hash;

      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_SHARED_NM;

      dblk::write_names(mcwr, *shared_nm, &names_hash);
      // write names hash
      file_data_cwr->write(&names_hash, sizeof(names_hash));
      // write ZSTD dictionary hash
      file_data_cwr->write(zstd_dict_blake3_digest, sizeof(zstd_dict_blake3_digest));
      // write shared name map
      MemoryLoadCB mcrd(mcwr.takeMem(), true);
      zstd_stream_compress_data(*file_data_cwr, mcrd, mcrd.getTargetDataSize(), ZSTD_BLK_CLEVEL);

      if (sign_plain_data)
      {
        plain_cwr.write(&names_hash, sizeof(names_hash));
        plain_cwr.write(zstd_dict_blake3_digest, sizeof(zstd_dict_blake3_digest));
        mcrd.seekto(0);
        copy_stream_to_stream(mcrd, plain_cwr, mcrd.getTargetDataSize());
      }
      goto done_write;
    }

    if (write_version && force_legacy_format && strcmp(orig_fn, version_legacy_fn) == 0)
    {
      G_ASSERT(!sign_plain_data);
      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_PLAIN;
      String ver(0, "%d.%d.%d.%d", (write_version >> 24) & 0xFF, (write_version >> 16) & 0xFF, (write_version >> 8) & 0xFF,
        write_version & 0xFF);
      file_data_cwr->write(ver.data(), ver.length());
    }
    else if (convert_blk)
    {
      G_ASSERT(!sign_plain_data);
      DataBlock blk;

      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_BLK;

      {
        DynamicMemGeneralSaveCB memCwr(tmpmem);
        if (preprocess)
          process_and_copy_file_to_stream(orig_fn, memCwr, targetString, keep_lines);
        else
          copy_file_to_stream(orig_fn, memCwr);


        InPlaceMemLoadCB memCrd(memCwr.data(), memCwr.size());
        if (!dblk::load_from_stream(blk, memCrd, dblk::ReadFlag::ALLOW_SS, orig_fn, &fnotify))
        {
          printf("ERR: read BLK %s\n", orig_fn);
          blk_err = true;
          continue;
        }

        if (blkCheckOnly)
        {
          file_data_cwr->write(memCwr.data(), memCwr.size());
          goto done_write;
        }
      }

      DynamicMemGeneralSaveCB memCwrText(tmpmem);
      DynamicMemGeneralSaveCB memCwrBin(tmpmem);
      blk.saveToTextStreamCompact(memCwrText);
      if (!force_legacy_blk_bbf3)
        blk.saveToStream(memCwrBin);
      else
        dblk::save_to_bbf3(blk, memCwrBin);
      if (memCwrText.size() < 32 || memCwrText.size() + 4096 < memCwrBin.size())
        file_data_cwr->write(memCwrText.data(), memCwrText.size());
      else
        file_data_cwr->write(memCwrBin.data(), memCwrBin.size());
    }
    else if (preprocess)
    {
      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_PLAIN;
      process_and_copy_file_to_stream(orig_fn, mos_cwr, targetString, keep_lines);
    }
    else
    {
      if (enable_file_attributes)
        file_attributes[mi] |= EVFSFA_TYPE_PLAIN;
      copy_file_to_stream(orig_fn, mos_cwr);
    }
  done_write:
    if (file_data_cwr != &cwr.getRawWriter())
    {
      get_computed_hash(file_data_cwr, &file_sha1[mi * C_SHA1_RECSZ], C_SHA1_RECSZ);
      destory_hash_computer_cb(file_data_cwr);

      bool the_same_sha1 = false;
      for (int pmi = 0; pmi < mi; pmi++)
        if (memcmp(&file_sha1[pmi * C_SHA1_RECSZ], &file_sha1[mi * C_SHA1_RECSZ], C_SHA1_RECSZ) == 0)
        {
          // seek back to the beginning of file (duplicates will be shared)
          cwr.seekto(data_ofs[mi][0]);

          // point to previously added file with the same content
          data_ofs[mi][0] = data_ofs[pmi][0];
          data_ofs[mi][1] = data_ofs[pmi][1];
          the_same_sha1 = true;
          if (!::dgs_execute_quiet)
            printf("SRC: %s (copy of %s)\n", orig_fn, files.getName(pmi));
          break;
        }
      if (the_same_sha1)
      {
        mi++;
        continue;
      }
    }
    if (!::dgs_execute_quiet)
      printf("SRC: %s\n", orig_fn);

    data_ofs[mi][1] = cwr.tell() - data_ofs[mi][0];
    cwr.align16();
    mi++;
  }
  clear_all_ptr_items(preloaded_blk);
  if (shared_nm)
    dblk::destroy_db_names(shared_nm);
  zstd_destroy_cdict(zstd_cdict);
  zstd_cdict = nullptr;

  if (blk_err)
  {
    printf("ERR: some BLK were unreadable\n");
    remove(fname);
    return false;
  }
  if (export_data_for_dict_dir)
  {
    if (shared_nm_sz_inc)
      printf("INFO: sharedNameMap updated (%+dK, +%d names, +%d str)\n", shared_nm_sz_inc >> 10, shared_nm_nm_inc, shared_nm_str_inc);
    printf("INFO: written %s with data for BLK dictionary (%d samples, total %dK)\n", cwr_data_for_dict.getTargetName(),
      blk2_packed_count, cwr_data_for_dict.tell() >> 10);
    fflush(stdout);
    return true;
  }

  if (cwr.tell() < cwr.getSize())
  {
    // truncate memory file to current pos
    MemoryChainedData *mcd = cwr.getRawWriter().getMem();
    ptrdiff_t rest_size = cwr.tell();
    while (mcd && rest_size)
    {
      if (mcd->used > rest_size)
        mcd->used = rest_size;
      rest_size -= mcd->used;
      mcd = mcd->next;
    }
    while (mcd)
    {
      mcd->used = 0;
      mcd = mcd->next;
    }
  }

  cwr.seekto(pt_names.resvDataPos);
  for (int i = 0; i < name_ofs.size(); i++)
    cwr.writePtr64e(name_ofs[i]);

  cwr.seekto(pt_data.resvDataPos);
  for (int i = 0; i < data_ofs.size(); i++)
    cwr.writeRef(data_ofs[i][0], data_ofs[i][1]);

  if (content_sha1)
  {
    cwr.seekto(file_sha1_pos);
    cwr.writeRaw(file_sha1.data(), data_size(file_sha1));
  }

  if (enable_file_attributes)
  {
    cwr.seekto(file_attributes_pos + 1);
    cwr.writeRaw(file_attributes.data(), data_size(file_attributes));
  }

  pt_names.finishTab(cwr);
  pt_data.finishTab(cwr);
  if (!zpack || content_sha1)
    cwr.writeInt32eAt(cwr.tell(), hdr_sz_pos);

  // finally, write file on disk
  dd_mkpath(fname);
  FullFileSaveCB fcwr(fname);
  if (!fcwr.fileHandle)
  {
    printf("ERR: can't write to %s", fname);
    return false;
  }
  fcwr.writeInt(force_legacy_format ? _MAKE4C('VRFs') : _MAKE4C('VRFx'));
  fcwr.writeInt(targetCode);
  int sz = cwr.getSize();
  fcwr.writeInt(mkbindump::le2be32_cond(sz, write_be));
  fcwr.writeInt(0);         // placeholder for packed size
  if (!force_legacy_format) // new format with extended header and version
  {
    uint16_t flags = 0;
    uint16_t ext_hdr_size = 8;
    if (sign_plain_data)
      flags |= EVFSEF_SIGN_PLAIN_DATA;
    if (enable_file_attributes)
    {
      ext_hdr_size += 4;
      flags |= EVFSEF_HAVE_FILE_ATTRIBUTES;
    }
    fcwr.writeIntP<2>(mkbindump::le2be16_cond(ext_hdr_size, write_be)); // size of of extended header
    fcwr.writeIntP<2>(mkbindump::le2be16_cond(flags, write_be));        // flags
    fcwr.writeInt(mkbindump::le2be32_cond(write_version, write_be));    // version
    if (enable_file_attributes)
      fcwr.writeInt(mkbindump::le2be32_cond(file_attributes_pos, write_be));
  }

  int packed_sz = fcwr.tell();
  bool pack_zstd = true;
  if (zpack && pack_zstd) // -V560
  {
    MemoryLoadCB crd(cwr.getMem(), false);
    DynamicMemGeneralSaveCB mcwr(tmpmem, sz + 4096);

    packed_sz = zstd_compress_data(mcwr, crd, sz, z1_blk_sz, 11);
    OBFUSCATE_ZSTD_DATA(mcwr.data(), packed_sz);
    fcwr.write(mcwr.data(), packed_sz);
  }
  else if (zpack && !pack_zstd) // -V560
  {
    ZlibSaveCB z_cwr(fcwr, ZlibSaveCB::CL_BestSpeed + 5);
    cwr.copyDataTo(z_cwr);
    z_cwr.finish();
    packed_sz = fcwr.tell() - packed_sz;
  }
  else
  {
    cwr.copyDataTo(fcwr);
    packed_sz = 0;
  }
  unsigned char vromfs_md5[16];
  calc_md5(vromfs_md5, cwr.getMem());
  fcwr.write(vromfs_md5, sizeof(vromfs_md5));
  fcwr.seekto(12);
  unsigned hw32 = packed_sz | (sign_contents ? 0x80000000U : 0) | ((pack_zstd && packed_sz > 0) ? 0x40000000U : 0);
  fcwr.writeInt(mkbindump::le2be32_cond(hw32, write_be));

  if (shared_nm_sz_inc)
    printf("INFO: sharedNameMap updated (%+dK, +%d names, +%d str)\n", shared_nm_sz_inc >> 10, shared_nm_nm_inc, shared_nm_str_inc);
  printf("packed %d files (total size=%d) into vromfs binary dump %s", files.nameCount(), cwr.getSize(), fcwr.getTargetName());
  if (zpack)
    printf(", packed size=%d", packed_sz);

  if (!shared_namemap_fn.empty())
    printf(", used sharedNameMap (%dK, %d names) for %d BLKs, clev=%d", shared_nm_sz >> 10, shared_nm_nm, blk2_packed_count,
      ZSTD_BLK_CLEVEL);

  printf(" [for %.3f sec]\n", (get_time_msec() - reft) / 1e3f);
  fcwr.close();

  fflush(stdout);

  MemoryChainedData *data_to_sign = nullptr;
  if (sign_contents)
  {
    if (sign_plain_data)
    {
      // things that must be signed:
      // - file data (already in plain_cwr)
      // - file name pointer table
      // - file name data
      // - file attributes
      int name_ptrs_size = files.nameCount() * cwr.PTR_SZ;
      int file_attributes_size = enable_file_attributes ? (1 + data_size(file_attributes)) : 0;
      MemorySaveCB &raw_cwr = cwr.getRawWriter();
      MemoryLoadCB raw_crd(raw_cwr.getMem(), false);
      raw_crd.seekto(pt_names.resvDataPos);
      copy_stream_to_stream(raw_crd, plain_cwr, name_ptrs_size);
      raw_crd.seekto(names_data_ofs);
      copy_stream_to_stream(raw_crd, plain_cwr, names_data_size);
      if (file_attributes_size)
      {
        raw_crd.seekto(file_attributes_pos);
        copy_stream_to_stream(raw_crd, plain_cwr, file_attributes_size);
      }
      data_to_sign = plain_cwr.getMem();
    }
    else
      data_to_sign = cwr.getMem();
  }
  return write_digital_signature(fcwr, fname, data_to_sign, inp);
}

bool unpackVromfs(const char *fname, const char *dest_folder, bool txt_blk, int content_hash_fn_base, int content_hash_fn_prefix_cnt,
  bool new_sha1sz_fmt, bool dump_ver_only = false)
{
  VirtualRomFsDataHdr hdr;
  VirtualRomFsPack *fs = NULL;
  void *base = NULL;
  bool read_be = false;
  String version;

  {
    FullFileLoadCB crd(fname);
    if (!crd.fileHandle)
    {
      printf("ERR: can't open %s", fname);
      return false;
    }
    crd.read(&hdr, sizeof(hdr));
    if (hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx'))
    {
      printf("ERR: VRFS label not found in %s", fname);
      return false;
    }
    if (!dagor_target_code_valid(hdr.target))
    {
      printf("ERR: unknowm format %c%c%c%c, %s", _DUMP4C(hdr.target), fname);
      return false;
    }

    read_be = dagor_target_code_be(hdr.target);
    G_ASSERT(!read_be);
    if (read_be)
    {
      hdr.fullSz = mkbindump::le2be32(hdr.fullSz);
      hdr.hw32 = mkbindump::le2be32(hdr.hw32);
    }

    uint32_t hdr_ex_data[64];
    if (hdr.label == _MAKE4C('VRFx'))
    {
      uint16_t hdr_ex[2];
      crd.read(hdr_ex, sizeof(hdr_ex));
      if (read_be)
      {
        hdr_ex[0] = mkbindump::le2be16(hdr_ex[0]);
        hdr_ex[1] = mkbindump::le2be16(hdr_ex[1]);
      }
      if (hdr_ex[0] > sizeof(hdr_ex) + sizeof(hdr_ex_data))
      {
        printf("ERR: too big extended header for VRFx (sz=%d flags=0x%X) in %s", hdr_ex[0], hdr_ex[1], fname);
        return false;
      }
      crd.read(hdr_ex_data, hdr_ex[0] - sizeof(hdr_ex));
      uint32_t ver = hdr_ex_data[0];
      if (read_be)
        ver = mkbindump::le2be32(ver);
      version.printf(0, "%d.%d.%d.%d", (ver >> 24) & 0xFF, (ver >> 16) & 0xFF, (ver >> 8) & 0xFF, ver & 0xFF);
      if (dump_ver_only)
      {
        printf("  VROMFS content version %s\n", version.str());
        return true;
      }
    }

    fs = (VirtualRomFsPack *)memalloc(hdr.fullSz + FS_OFFS, tmpmem);
    if (fs == nullptr)
    {
      printf("ERR: memalloc(%d, tmpmem); returned null", hdr.fullSz + FS_OFFS);
      return false;
    }
    base = (char *)fs + FS_OFFS;
    if (hdr.packedSz() && hdr.zstdPacked())
    {
      Tab<char> src;
      src.resize(hdr.packedSz());
      crd.read(src.data(), data_size(src));
      DEOBFUSCATE_ZSTD_DATA(src.data(), data_size(src));
      size_t dsz = zstd_decompress(base, hdr.fullSz, src.data(), data_size(src));
      if (dsz != hdr.fullSz)
      {
        printf("ERR: failed to decode data, ret=%d (decSz=%d), %s", (int)dsz, hdr.fullSz, fname);
        return false;
      }
    }
    else if (hdr.packedSz())
    {
      ZlibLoadCB zcrd(crd, hdr.packedSz());
      zcrd.read(base, hdr.fullSz);
      zcrd.ceaseReading();
    }
    else
      crd.read(base, hdr.fullSz);
  }

  bool index_only = false;

  mkbindump::le2be32_s_cond(base, base, (sizeof(VirtualRomFsData) - FS_OFFS) / 4, read_be);
  fs->files.map.patch(base);

  uint32_t &fs_data_cnt = ((uint32_t *)&fs->data)[1];
  if (fs->files.map.size() && !fs_data_cnt) // detect index-only dumps
  {
    fs_data_cnt = fs->files.map.size();
    index_only = true;
  }
  fs->data.patch(base);

  mkbindump::le2be32_s_cond(fs->files.map.data(), fs->files.map.data(), data_size(fs->files.map) / 4, read_be);
  mkbindump::le2be32_s_cond(fs->data.data(), fs->data.data(), data_size(fs->data) / 4, read_be);
  for (int i = fs->files.map.size() - 1; i >= 0; i--)
    fs->files.map[i].patch(base);
  for (int i = fs->data.size() - 1; i >= 0; i--)
    fs->data[i].patch(base);
  if ((void *)fs->files.map.data() >= (void *)(fs + 1))
    fs->ptr = ptrdiff_t(fs->ptr) > 0 ? (char *)base + ptrdiff_t(fs->ptr) : NULL;

  if (index_only)
  {
    printf("%s is index-only, 'unpacked' files will be zero-length\n", fname);
    txt_blk = false;
  }
  if (content_hash_fn_base && !fs->ptr)
  {
    printf("ERR: %s doesn't contain content-SHA1\n", fname);
    memfree(fs, tmpmem);
    return false;
  }

  add_vromfs(fs);

  // write files from vromfs pack
  String tmpPath;
  String fileHash;
  FastNameMap sha1_fn;
  int unp_cnt = 0, zerofile_cnt = 0;
  if (!dest_folder && !dump_ver_only) // dump only file list with filesizes
    printf("listing %d files from %c%c%c%c vromfs binary dump %s:\n", fs->files.nameCount(), _DUMP4C(hdr.target), fname);
  for (int i = 0; i < fs->files.nameCount(); i++)
  {
    tmpPath.printf(260, "%s/%s", dest_folder, fs->files.map[i]);
    if (strcmp(fs->files.map[i], version_legacy_fn) == 0)
    {
      version.printf(0, "%.*s", data_size(fs->data[i]), fs->data[i].data());
      if (dump_ver_only)
        goto print_version;
    }
    if (dump_ver_only)
      continue;
    if (!dest_folder) // dump only file list with filesizes
    {
      printf("  %s, sz=%d\n", fs->files.map[i].get(), data_size(fs->data[i]));
      continue;
    }

    char label[4] = {0, 0, 0, 0};
    memcpy(label, fs->data[i].data(), min<int>(data_size(fs->data[i]), 4));
    if (txt_blk && trail_strcmp(tmpPath, ".blk") && data_size(fs->data[i]) > 0 && (memcmp(label, "\0BBF", 4) == 0 || label[0] < 0x10))
    {
      DataBlock blk;
      // InPlaceMemLoadCB fcrd(fs->data[i].data(), data_size(fs->data[i]));
      // blk.loadFromStream(fcrd, tmpPath);
      blk.load(fs->files.map[i]);
      dd_mkpath(tmpPath);
      blk.saveToTextFile(tmpPath);
    }
    else
    {
      if (content_hash_fn_base && fs->ptr)
      {
        if (!data_size(fs->data[i]))
        {
          zerofile_cnt++;
          continue;
        }
        if (content_hash_fn_base == 16)
          data_to_str_hex(fileHash, (char *)fs->ptr + i * C_SHA1_RECSZ, C_SHA1_RECSZ);
        else if (content_hash_fn_base == 32)
          data_to_str_b32(fileHash, (char *)fs->ptr + i * C_SHA1_RECSZ, C_SHA1_RECSZ);

        if (new_sha1sz_fmt)
        {
          char sz_b32[13 + 1];
          fileHash.aprintf(0, "-%s", uint64_to_str_b32(data_size(fs->data[i]), sz_b32, sizeof(sz_b32)));
        }

        if (content_hash_fn_prefix_cnt)
          tmpPath.printf(260, "%s/%.*s/%s", dest_folder, content_hash_fn_prefix_cnt, &fileHash[0],
            &fileHash[content_hash_fn_prefix_cnt]);
        else
          tmpPath.printf(260, "%s/%s", dest_folder, fileHash);
        if (sha1_fn.getNameId(tmpPath) >= 0)
          continue;
        sha1_fn.addNameId(tmpPath);
      }
      dd_mkpath(tmpPath);
      FullFileSaveCB cwr(tmpPath);
      if (!index_only)
      {
        if (cwr.fileHandle)
          cwr.writeTabData(fs->data[i]);
        else
        {
          if (strcmp(fs->files.map[i], dblk::SHARED_NAMEMAP_FNAME) == 0)
          {
            uint64_t names_hash;
            uint8_t zstd_dict_blake3_digest[BLAKE3_OUT_LEN];
            String tmpstr;
            memcpy(&names_hash, fs->data[i].data(), sizeof(names_hash));
            memcpy(zstd_dict_blake3_digest, fs->data[i].data() + sizeof(names_hash), sizeof(zstd_dict_blake3_digest));

            tmpPath.printf(0, "%s/__SHARED_NAMEMAP-%016llx-%s.bin", dest_folder, names_hash,
              data_to_str_hex(tmpstr, zstd_dict_blake3_digest, BLAKE3_OUT_LEN));
            cwr.open(tmpPath, DF_WRITE);
          }
          if (cwr.fileHandle)
            cwr.writeTabData(fs->data[i]);
          else
            printf("skip %s of size %d\n", tmpPath.str(), (int)data_size(fs->data[i]));
        }
      }
    }
    unp_cnt++;
  }
  remove_vromfs(fs);
  if (dump_ver_only || !dest_folder)
  {
    version = "0.0.0.0";
    goto print_version;
  }
  if (content_hash_fn_base && fs->ptr)
    tmpPath.printf(0, " (content-SHA1 base%d %d/x fnames; %d duplicates, %d zero-length)", content_hash_fn_base,
      content_hash_fn_prefix_cnt, fs->files.nameCount() - unp_cnt - zerofile_cnt, zerofile_cnt);
  else
    tmpPath = "";
  printf("unpacked %d files from %c%c%c%c vromfs binary dump%s\n", unp_cnt, _DUMP4C(hdr.target), tmpPath.str());

print_version:
  if (!version.empty())
  {
    if (!dest_folder && !dump_ver_only && fs->files.nameCount())
      printf("\n");
    printf("  VROMFS content version %s\n", version.str());
  }

  memfree(fs, tmpmem);
  return true;
}

bool repackVromfs(const char *fname, const char *dest_fname, bool store_packed, bool index_only = false)
{
  VirtualRomFsDataHdr hdr;
  Tab<char> fs(tmpmem);
  bool read_be = false;
  unsigned char md5_digest[16];
  Tab<char> digitalSignature(tmpmem);
  Tab<uint8_t> hdr_ex_data;

  {
    FullFileLoadCB crd(fname);
    if (!crd.fileHandle)
    {
      printf("ERR: can't open %s", fname);
      return false;
    }
    crd.read(&hdr, sizeof(hdr));
    if (hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx'))
    {
      printf("ERR: VRFS label not found in %s", fname);
      return false;
    }
    if (!dagor_target_code_valid(hdr.target))
    {
      printf("ERR: unknowm format %c%c%c%c, %s", _DUMP4C(hdr.target), fname);
      return false;
    }

    if (hdr.label == _MAKE4C('VRFx'))
    {
      uint16_t hdr_ex;
      crd.read(&hdr_ex, 2);
      crd.seekrel(-2);
      if (read_be) // -V547
        hdr_ex = mkbindump::le2be16(hdr_ex);
      hdr_ex_data.resize(hdr_ex);
      crd.read(hdr_ex_data.data(), data_size(hdr_ex_data));
    }

    read_be = dagor_target_code_be(hdr.target);
    G_ASSERT(!read_be);
    if (read_be)
    {
      hdr.fullSz = mkbindump::le2be32(hdr.fullSz);
      hdr.hw32 = mkbindump::le2be32(hdr.hw32);
    }

    fs.resize(hdr.fullSz);
    if (hdr.packedSz() && hdr.zstdPacked())
    {
      Tab<char> src;
      src.resize(hdr.packedSz());
      crd.read(src.data(), data_size(src));
      DEOBFUSCATE_ZSTD_DATA(src.data(), data_size(src));
      size_t dsz = zstd_decompress(fs.data(), hdr.fullSz, src.data(), data_size(src));
      if (dsz != hdr.fullSz)
      {
        printf("ERR: failed to decode data, ret=%d (decSz=%d), %s", (int)dsz, hdr.fullSz, fname);
        return false;
      }
    }
    else if (hdr.packedSz())
    {
      ZlibLoadCB zcrd(crd, hdr.packedSz());
      zcrd.read(fs.data(), hdr.fullSz);
      zcrd.ceaseReading();
    }
    else
      crd.read(fs.data(), hdr.fullSz);

    if (crd.tryRead(md5_digest, 16) != 16)
      memset(md5_digest, 0, sizeof(md5_digest));
    else
    {
      char buffer[4096];
      const int maxSignatureSize = 65536;
      int bytesRead = 0;
      while ((bytesRead = crd.tryRead(buffer, sizeof(buffer))) > 0)
      {
        int currentSize = digitalSignature.size();
        int newSize = currentSize + bytesRead;
        if (newSize > maxSignatureSize)
        {
          printf("The digital signature is oversized and will be deleted!\n");
          clear_and_shrink(digitalSignature);
          break;
        }
        digitalSignature.resize(newSize);
        memcpy(&digitalSignature[currentSize], buffer, bytesRead);
      }
    }
  }

  int sz = hdr.fullSz;
  int index_only_files_count = 0;
  if (index_only)
  {
    VirtualRomFsPack &fs_view = *(VirtualRomFsPack *)(fs.data() - FS_OFFS);
    // file layout of PatchableTab may differ from memory layout, do patch!
    fs_view.files.map.patch(nullptr);
    fs_view.data.patch(nullptr);
    index_only_files_count = fs_view.files.map.size();
    int fs_names_ofs = mkbindump::le2be32_cond(ptrdiff_t(fs_view.files.map.data()), read_be);
    int fs_data_ofs = mkbindump::le2be32_cond(ptrdiff_t(fs_view.data.data()), read_be);
    int fs_hash_ofs = mkbindump::le2be32_cond(ptrdiff_t(fs_view.ptr), read_be);
    if (fs_names_ofs >= sizeof(fs_view) - FS_OFFS && fs_hash_ofs > 0 &&
        fs_hash_ofs + C_SHA1_RECSZ * mkbindump::le2be32_cond(fs_view.data.size(), read_be) <=
          mkbindump::le2be32_cond(fs_view.hdrSz, read_be))
    {
      sz = mkbindump::le2be32_cond(fs_view.hdrSz, read_be);
      for (int ofs = fs_data_ofs, c = mkbindump::le2be32_cond(fs_view.data.size(), read_be); c > 0;
           ofs += elem_size(fs_view.data), c--)
        *(unsigned *)&fs[ofs] = 0;
      fs_view.data.init(fs_view.data.data(), 0);
      clear_and_shrink(digitalSignature);
#if _TARGET_64BIT // file layout of PatchableTab differs from memory layout in 64 bit!
      fs_view.files.map.init((uint64_t(fs_view.files.map.size()) << 32) + (char *)fs_view.files.map.data(), 0);
      fs_view.data.init((uint64_t(fs_view.data.size()) << 32) + (char *)fs_view.data.data(), 0);
#endif

      MD5_CONTEXT md5s;
      MD5_INIT(&md5s);
      MD5_UPDATE(&md5s, (const unsigned char *)fs.data(), sz);
      MD5_FINAL(&md5s, md5_digest);
    }
    else
    {
      printf("ERR: obsolete vrom format (or missing content-SHA1) in %s, cannot build index-only", fname);
      return false;
    }
  }

  FullFileSaveCB cwr(dest_fname);
  cwr.writeInt(data_size(hdr_ex_data) ? _MAKE4C('VRFx') : _MAKE4C('VRFs'));
  cwr.writeInt(hdr.target);
  cwr.writeInt(mkbindump::le2be32_cond(sz, read_be));
  cwr.writeInt(0);
  if (data_size(hdr_ex_data))
    cwr.write(hdr_ex_data.data(), data_size(hdr_ex_data));

  int packed_sz = cwr.tell();
  bool pack_zstd = true;
  if (store_packed && pack_zstd) // -V560
  {
    InPlaceMemLoadCB crd(fs.data(), sz);
    DynamicMemGeneralSaveCB mcwr(tmpmem, sz + 4096);

    packed_sz = zstd_compress_data(mcwr, crd, sz, z1_blk_sz, 11);
    OBFUSCATE_ZSTD_DATA(mcwr.data(), packed_sz);
    cwr.write(mcwr.data(), packed_sz);
  }
  else if (store_packed && !pack_zstd) // -V560
  {
    ZlibSaveCB z_cwr(cwr, ZlibSaveCB::CL_BestSpeed + 5);
    z_cwr.write(fs.data(), sz);
    z_cwr.finish();
    packed_sz = cwr.tell() - packed_sz;
  }
  else
  {
    cwr.write(fs.data(), sz);
    packed_sz = 0;
  }
  cwr.write(md5_digest, 16);
  if (digitalSignature.size())
    cwr.write(digitalSignature.data(), digitalSignature.size());

  cwr.seekto(12);
  unsigned hw32 = packed_sz | (digitalSignature.size() ? 0x80000000U : 0) | ((pack_zstd && packed_sz > 0) ? 0x40000000U : 0);

  cwr.writeInt(mkbindump::le2be32_cond(hw32, read_be));
  if (index_only)
    printf("built vromfs-index for %d files (packed size=%d, memory size=%d)\n",
      mkbindump::le2be32_cond(index_only_files_count, read_be), packed_sz, sz);
  return true;
}

static void print_usage()
{
  printf(
    "usage(pack):   vromfsPacker-dev.exe <build.blk> [options ...]\n"
    "usage(pack):   vromfsPacker-dev.exe -B:<src_folder> -out:<fname> [options ...]\n"
    "usage(unpack): vromfsPacker-dev.exe [-u|-U|-uh|-uh<16|32[:N]>] <data.vromfs.bin> [dest_folder]"
    " [-dict:<dict.vromfs.bin|dict_data.bin>]\n"
    "usage(unpack): vromfsPacker-dev.exe -uz <data.vromfs.bin> [nonpacked-data.vromfs.bin]\n"
    "usage(repack): vromfsPacker-dev.exe -zp[:blockSizeKB] <data.vromfs.bin> [repacked-data.vromfs.bin]\n"
    "usage(index):  vromfsPacker-dev.exe -mkidx <data.vromfs.bin> <index-only.vromfs.bin>\n"
    "usage(index):  vromfsPacker-dev.exe -dumpver <data.vromfs.bin>\n"
    "usage(index):  vromfsPacker-dev.exe -dump <data.vromfs.bin>\n"
    "usage(index):  vromfsPacker-dev.exe -mkdict <dest_dict.bin> <dict_sz_KB> <data_for_dict.bin>...\n"
    "\noptions are:\n"
    "  -D:<def>          define macro for preprocessing\n"
    "  -out:<fname>      set output file\n"
    "  -platform:<4cc>   select platform(s) to build\n"
    "  -build:target_fn  select one of targets (configured in build.blk) to build\n"
    "  -altOutDir:dir    replace output dir\n"
    "  -rootDir:dir      replace root dir\n"
    "  -mount:mnt=dir    add named mount\n"
    "  -writeVersion:<DDD.DDD.DDD.DDD>   store content version (32 bit as 8b.8b.8b.8b)\n"
    "  -forceLegacyFormat                use legacy VROMFS format even when -writeVersion is specified\n"
    "  -exportDataForDict:<dest_dir>     export data for compr. dict. instead of building vromfs\n"
    "  -blkSharedNamemapLocation:<dir>   override blkSharedNamemapLocation:t option of build.blk\n"
    "  -makeDict:<dest_file>:<data_dir>  build compr. dict. from data files instead of building vromfs\n"
    "additional options for -B case:\n"
    "  -dest_path:<path>       set dest root path for files in vrom (-dest_path:* uses name of src_folder)\n"
    "  -include:<wildcard>     use wildcard to include files (if none specified, *.* is used)\n"
    "  -exclude:<regexp>       use regexp to exclude files\n"
    "  -subdirs<+|->           scan/don't scan subdirs recursively, def: -subdirs+\n"
    "  -rootfiles<+|->         scan/don't scan root files,          def: -rootfiles+\n"
    "  -packBlk<+|->           pack/don't BLKs to binary format,    def: -packBlk-\n"
    "  -pack<+|->              pack/don't pack vrom contents,       def: -pack+\n"
    "  -signPacked<+|->        sign zstd/plain content if packing,  def: -signPacked+\n"
    "  -sign:<private_key_fn>  sign content with private key and SHA256 digest\n"
    "  -verbose                use verbose mode (default is -quiet for -B)\n"
    "\nbuild.blk format:\n"
    "  platform:t=\"PC\"             // \"PC\"=default, \"iOS\", \"and\" [multi]\n"
    "  //rootFolder:t=\".\"          // [optional] root path\n"
    "  //pack:b=true               // [optional] use ZSTD pack or not, def=true\n"
    "  //signPacked:b=true         // [optional] sign ZSTD-packed content instead of plain, def=true\n"
    "  //storeContentSHA1:b=false  // [optional] store content-SHA1 for each file, def=false\n"
    "  //packBlockSizeKB:i=1024    // [optional] use specific block size while packing data, def=1024 (KB)\n"
    "  //blkUseSharedNamemap:b=    // [optional] use shared namemap for all BLKs in vrom (BLK will be saved compressed), def=false\n"
    "  //blkSharedNamemapLocation:t=  // [optional] use location prefix for files with blkUseSharedNamemap option\n"
    "  //blkShareStrParams:b=      // [optional] adds value of str params to shared namemap (when it is used), def=false\n"
    "  //blkUseComprDict:t=        // [optional] use compression dictionary to pack all BLKs in vrom (BLK will be saved compressed)\n"
    "  //embedComprDictToVrom:b=   // [optional] add compression dictionary to vrom as <BLAKE3-hash>.dict named file, def=false\n"
    "  output:t=\"game.vromfs.bin\"  // filename to build vromfs to\n"
    "  output_[CODE]:t=\"pc/game.vromfs.bin\" // per-CODE filename to build vromfs to\n"
    "  //sign_private_key:t=\"private.pem\"   // [optional] sign contents of vromfs with PEM private key\n"
    "  //sign_digest:t=\"sha256\"             // [optional] type of sign digest\n"
    "\n"
    "  // list of folders to scan files in\n"
    "  folder {\n"
    "    //platform:t=\"PC\"         // [optional] folder restricted to specified platforms only [multi]\n"
    "    //output_[CODE]:t=\"pc/lang.vromfs.bin\" // [optional] redirect this folder to-non-default vromfs\n"
    "    //path:t=\".\"              // [optional] source path\n"
    "    //dest_path:t=\".\"         // [optional] destination path prefix; by default \"path\" used\n"
    "    //scan_files:b=true       // [optional] scan files in this folder, def=true\n"
    "    //scan_subfolders:b=true  // [optional] scan sub-folders in this folder, def=true\n"
    "\n"
    "    wildcard:t=\"*.blk\"        // wildcard to scan files\n"
    "    //wildcard:t=\"*.blk\"      // [optional] other wildcards\n"
    "  }\n"
    "  //file:t=\"folder/fname.ext\" // [optional] list of files to include directly\n\n"
    "  // package options\n"
    "  preprocess {\n"
    "    ext:t=blk               // fname extension to apply preprocessing\n"
    "    //ext:t=nut             // [optional] other extensions\n"
    "    keepLines {\n"
    "      include:t=\".*\\.nut$\"  // regexp for files where preprocess should keep \\n \n"
    "    }\n"
    "  }\n"
    "  packBlk {\n"
    "    //dontPackCheckOnly:b=false // [optional] check syntax, but stored as text BLK, def=false\n"
    "    include:t=\".*\\.blk$\"        // regexp for files to be packed as BinBLK\n"
    "    //include:t=\".*\\.seq$\"      // [optional] other regexps for include files\n"
    "    //exclude:t=\"[/\\]_\\w*.blk$\" // [optional] other regexps for exclude files\n"
    "  }\n"
    "  exclude {\n"
    "    //exclude:t=\"[/\\\\]_[\\w\\.]*\\.blk$\" // [optional] regexps to exclude files from vromfs\n"
    "  }\n"
    "  defines {\n"
    "    //def:t=\"DEMO\" // [optional] defines for preprocessor, equiv. to -D:<def> switches\n"
    "  }\n\n");
}


int buildVromfs(DataBlock *explicit_build_rules, dag::ConstSpan<const char *> argv)
{
  int argc = argv.size();
  if (argc < 2)
  {
    print_usage();
    return -1;
  }

  if (dd_stricmp(argv[1], "-u") == 0 || strncmp(argv[1], "-uh", 3) == 0 || strncmp(argv[1], "-uH", 3) == 0)
  {
    // export files from VROMFS pack
    if (argc < 3)
    {
      print_usage();
      return -1;
    }
    int content_hash_fn_base = 0, content_hash_fn_prefix_cnt = 2;
    bool new_sha1sz_fmt = true;
    if (dd_stricmp(argv[1], "-uh") == 0)
      content_hash_fn_base = 32, new_sha1sz_fmt = (argv[1][2] == 'h');
    else if (dd_strnicmp(argv[1], "-uh", 3) == 0)
    {
      content_hash_fn_base = atoi(argv[1] + 3);
      new_sha1sz_fmt = (argv[1][2] == 'h');
      if (const char *p = strchr(argv[1] + 3, ':'))
        content_hash_fn_prefix_cnt = atoi(p + 1);
    }
    if (content_hash_fn_base)
    {
      if (content_hash_fn_base != 16 && content_hash_fn_base != 32)
      {
        printf("ERR: bad switch \"%s\", base=%d prefix=%d", argv[1], content_hash_fn_base, content_hash_fn_prefix_cnt);
        return 13;
      }
      if (content_hash_fn_prefix_cnt < 0 || content_hash_fn_prefix_cnt > 4)
      {
        printf("ERR: bad switch \"%s\", base=%d prefix=%d", argv[1], content_hash_fn_base, content_hash_fn_prefix_cnt);
        return 13;
      }
    }

    for (int i = 3; i < argc; i++)
      if (strnicmp(argv[i], "-dict:", 6) == 0)
      {
        const char *dict_fn = argv[i] + 6;
        if (trail_strcmp(dict_fn, ".vromfs.bin"))
        {
          if (auto *fs = load_vromfs_dump(dict_fn, tmpmem))
            add_vromfs(fs);
          else
          {
            printf("ERR: failed to load %s\n", dict_fn);
            exit(13);
          }
        }
        else
        {
          FullFileLoadCB crd(dict_fn);
          if (!crd.fileHandle)
          {
            printf("ERR: failed to load %s\n", dict_fn);
            exit(13);
          }

          Tab<char> dict_buf;
          dict_buf.resize(df_length(crd.fileHandle));
          crd.readTabData(dict_buf);

          uint8_t blake3_digest[BLAKE3_OUT_LEN];
          String fname;
          blake3_hasher hasher;
          blake3_hasher_init(&hasher);
          blake3_hasher_update(&hasher, dict_buf.data(), data_size(dict_buf));
          blake3_hasher_finalize(&hasher, blake3_digest, BLAKE3_OUT_LEN);
          data_to_str_hex(fname, blake3_digest, BLAKE3_OUT_LEN);

          add_vromfs(VirtualRomFsSingleFile::make_mem_data(tmpmem, dict_buf.data(), data_size(dict_buf), fname + ".dict", true), true);
        }
      }
    if (!unpackVromfs(argv[2], argc > 3 ? argv[3] : ".", strcmp(argv[1], "-U") == 0, content_hash_fn_base, content_hash_fn_prefix_cnt,
          new_sha1sz_fmt))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-uz") == 0)
  {
    // unpack VROMFS and store as unpacked binary
    if (argc < 3)
    {
      print_usage();
      return -1;
    }
    if (!repackVromfs(argv[2], argc > 3 ? argv[3] : argv[2], false))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-zp") == 0 || dd_strnicmp(argv[1], "-zp:", 4) == 0)
  {
    // repack VROMFS and store as packed binary
    if (argc < 3)
    {
      print_usage();
      return -1;
    }
    if (argv[1][3] == ':')
      z1_blk_sz = atoi(argv[1] + 4) << 10;
    if (!repackVromfs(argv[2], argc > 3 ? argv[3] : argv[2], true))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-mkidx") == 0)
  {
    // repack VROMFS and store as index-only packed binary
    if (argc < 4)
    {
      print_usage();
      return -1;
    }
    if (!repackVromfs(argv[2], argv[3], true, true))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-dumpver") == 0)
  {
    if (argc < 3 || !unpackVromfs(argv[2], nullptr, false, 0, 0, false, true))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-dump") == 0)
  {
    if (argc < 3 || !unpackVromfs(argv[2], nullptr, false, 0, 0, false))
      return 13;
  }
  else if (dd_stricmp(argv[1], "-mkdict") == 0)
  {
    if (argc < 5)
    {
      printf("usage(index):  vromfsPacker-dev.exe -mkdict <dest_dict.bin> <dict_sz_KB> <data_for_dict.bin>...\n");
      return 13;
    }
    size_t dictSize = atoi(argv[3]) << 10;
    Tab<char> dictBuffer;
    dictBuffer.resize(dictSize);

    Tab<char> samplesBuffer;
    Tab<size_t> samplesSizes;
    for (int i = 4; i < argc; i++)
    {
      FullFileLoadCB crd(argv[i]);
      if (!crd.fileHandle)
      {
        printf("ERR: cannot read data for dictionary from %s\n", argv[i]);
        return 13;
      }
      unsigned fsize = df_length(crd.fileHandle);
      samplesBuffer.reserve(fsize);
      while (crd.tell() < fsize)
      {
        crd.beginBlock();
        int sample_sz = crd.getBlockRest();
        samplesSizes.push_back(sample_sz);
        int ofs = append_items(samplesBuffer, sample_sz);
        crd.read(&samplesBuffer[ofs], sample_sz);
        crd.endBlock();
      }
    }

    int t0 = get_time_msec();
    dictSize = zstd_train_dict_buffer(make_span(dictBuffer), ZSTD_BLK_CLEVEL, samplesBuffer, samplesSizes);
    t0 = get_time_msec() - t0;

    dd_mkpath(argv[2]);
    FullFileSaveCB cwr(argv[2]);
    cwr.write(dictBuffer.data(), (unsigned)dictSize);
    printf("INFO: trained ZSTD dictionary using %d samples (%dK size total), maxDict=%dK -> %dK (clev=%d) for %.1f sec\n",
      (int)samplesSizes.size(), (int)samplesBuffer.size() >> 10, (int)data_size(dictBuffer) >> 10, (int)dictSize >> 10,
      ZSTD_BLK_CLEVEL, t0 / 1000.f);
    printf("  %s saved\n", argv[2]);
  }
  else if (dd_strnicmp(argv[1], "-B:", 3) == 0 || argv[1][0] != '-' /*mean path to blk like build.blk*/)
  {
    // build VROMFS pack
    DataBlock inp_stor;
    DataBlock &inp = explicit_build_rules ? *explicit_build_rules : inp_stor;

    bool simple_build_mode = false;

    const char *cmd_target_str = NULL;
    for (int i = 2; i < argc; i++)
      if (strnicmp("-D:", argv[i], 3) == 0)
        preproc_defines.addNameId(argv[i] + 3);
      else if (strnicmp("-platform:", argv[i], 10) == 0 && !cmd_target_str)
        cmd_target_str = argv[i] + 10;
    if (!cmd_target_str)
      cmd_target_str = "PC";

    if (strncmp(argv[1], "-B:", 3) == 0)
    {
      simple_build_mode = true;
      dgs_execute_quiet = true;
      inp.setStr("platform", cmd_target_str);
      inp.setStr("rootFolder", argv[1] + 3);
      inp.setStr("sign_private_key", "");

      DataBlock *folder_blk = inp.addBlock("folder");
      folder_blk->setStr("path", "./");
      for (int i = 2; i < argc; i++)
        if (strnicmp("-dest_path:", argv[i], 11) == 0)
        {
          if (strcmp(argv[i] + 11, "*") == 0)
          {
            char buf[DAGOR_MAX_PATH];
#if _TARGET_PC
            if (getcwd(buf, DAGOR_MAX_PATH))
            {
              dd_append_slash_c(buf);
              dd_simplify_fname_c(buf);
            }
            else
              buf[0] = '\0';
#else
            buf[0] = '\0';
#endif
            dd_add_base_path(buf, true);
            String real_folder_name(df_get_real_folder_name(inp.getStr("rootFolder")));
            if (real_folder_name.suffix("/"))
              real_folder_name.pop_back();
            folder_blk->setStr("dest_path", String(0, "%s/", dd_get_fname(real_folder_name)));
            dd_remove_base_path(buf);
          }
          else
            folder_blk->setStr("dest_path", argv[i] + 11);
        }
        else if (strnicmp("-out:", argv[i], 5) == 0)
          inp.setStr("output", argv[i] + 5);
        else if (strnicmp("-packBlk", argv[i], 8) == 0)
        {
          if (argv[i][8] == '+' || argv[i][8] == '\0')
            inp.addBlock("packBlk")->setStr("include", ".*\\.blk$");
          else
            inp.removeBlock("packBlk");
        }
        else if (strnicmp("-pack", argv[i], 5) == 0)
          inp.setBool("pack", argv[i][5] == '+' || argv[i][5] == '\0');
        else if (strnicmp("-signPacked", argv[i], 11) == 0)
          inp.setBool("signPacked", argv[i][11] == '+' || argv[i][11] == '\0');
        else if (strnicmp("-sign:", argv[i], 6) == 0)
        {
          inp.setStr("sign_private_key", argv[i] + 6);
          inp.setStr("sign_digest", "sha256");
        }
        else if (strnicmp("-exclude:", argv[i], 9) == 0)
          inp.addBlock("exclude")->addStr("exclude", argv[i] + 9);
        else if (strnicmp("-subdirs", argv[i], 8) == 0)
          inp.addBlock("folder")->setBool("scan_subfolders", argv[i][8] == '+' || argv[i][8] == '\0');
        else if (strnicmp("-rootfiles", argv[i], 10) == 0)
          inp.addBlock("folder")->setBool("scan_files", argv[i][10] == '+' || argv[i][10] == '\0');
        else if (strnicmp("-include:", argv[i], 9) == 0)
          folder_blk->addStr("wildcard", argv[i] + 9);
        else if (stricmp("-verbose", argv[i]) == 0)
          dgs_execute_quiet = false;
      if (!folder_blk->paramExists("wildcard"))
        folder_blk->addStr("wildcard", "*.*");

      if (!inp.paramExists("output"))
      {
        printf("ERR: -out: option missing for %s (simple build mode)\n", argv[1]);
        return -1;
      }
      // inp.saveToTextFile("_result.blk");
    }
    else
    {
      DynamicMemGeneralSaveCB cwr(tmpmem, 8 << 10, 4 << 10);
      process_and_copy_file_to_stream(argv[1], cwr, cmd_target_str, true);

      InPlaceMemLoadCB crd(cwr.data(), cwr.size());
      if (!inp.loadFromStream(crd, argv[1]))
      {
        printf("cannot read: %s\n", argv[1]);
        printf("preprocessed file content:\n--------------------------\n%.*s\n", (int)cwr.size(), cwr.data());
        return -1;
      }
      dgs_apply_command_line_to_config(&inp);
    }

    if (inp.getBool("crc32", false))
      printf("WARN: crc32 options is obsolete, skipped\n");

    if (!loadPatterns(*inp.getBlockByNameEx("exclude"), "exclude", reExcludeFile))
      return 13;
    if (!loadPatterns(*inp.getBlockByNameEx("packBlk"), "include", reIncludeBlk))
      return 13;
    if (!loadPatterns(*inp.getBlockByNameEx("packBlk"), "exclude", reExcludeBlk))
      return 13;
    blkCheckOnly = inp.getBlockByNameEx("packBlk")->getBool("dontPackCheckOnly", false);

    OAHashNameMap<true> parse_ext;
    DataBlock *blk = inp.getBlockByName("preprocess");
    if (blk)
    {
      int ext_nid = blk->getNameId("ext");
      for (int j = 0; j < blk->paramCount(); j++)
        if (blk->getParamNameId(j) == ext_nid && blk->getParamType(j) == DataBlock::TYPE_STRING)
          parse_ext.addNameId(blk->getStr(j));
      if (!loadPatterns(*blk->getBlockByNameEx("keepLines"), "include", reIncludeKeepLines))
        return 13;
    }
    blk = inp.getBlockByName("defines");
    if (blk)
    {
      int def_nid = blk->getNameId("def");
      for (int j = 0; j < blk->paramCount(); j++)
        if (blk->getParamNameId(j) == def_nid && blk->getParamType(j) == DataBlock::TYPE_STRING)
          preproc_defines.addNameId(blk->getStr(j));
    }

    FastIntList targetCodes;
    FastNameMap buildFnList;
    Tab<FastNameMapEx> outputFnames(midmem);
    String targetStrStor;
    const char *force_root_folder = NULL;
    const char *last_code = NULL;
    for (int i = 2; i < argc; i++)
      if (strnicmp("-D:", argv[i], 3) == 0)
        preproc_defines.addNameId(argv[i] + 3);
      else if (strnicmp("-out:", argv[i], 5) == 0)
        inp.setStr(last_code ? String(32, "output_%s", last_code).str() : "output", argv[i] + 5);
      else if (strnicmp("-platform:", argv[i], 10) == 0)
      {
        inp.addStr("platform", last_code = argv[i] + 10);
        targetCodes.addInt(get_target_code(last_code));
      }
      else if (strnicmp("-build:", argv[i], 7) == 0)
        buildFnList.addNameId(argv[i] + 7);
      else if (strnicmp("-altOutDir:", argv[i], 11) == 0)
        inp.setStr("altOutDir", argv[i] + 11);
      else if (strnicmp("-rootDir:", argv[i], 9) == 0)
      {
        if (force_root_folder)
          printf("WARN: duplicate switch %s\n\n", argv[i]);
        force_root_folder = argv[i] + 9;
      }
      else if (strnicmp("-mount:", argv[i], 7) == 0)
      {
        if (const char *p = strchr(argv[i] + 7, '='))
          dd_set_named_mount_path(String::mk_sub_str(argv[i] + 7, p), p + 1);
        else
        {
          printf("ERR: bad mount option: %s (expected name=dir format)\n\n", argv[i]);
          return 13;
        }
      }
      else if (strnicmp("-writeVersion:", argv[i], 14) == 0)
      {
        unsigned ver[4] = {0, 0, 0, 0};
        const char *s = argv[i] + 14;
        for (int j = 0; j < 4; j++)
        {
          ver[j] = atoi(s);
          s = strchr(s, '.');
          if (!s)
            break;
          if (ver[j] > 255)
          {
            printf("ERR: bad version <%s> at [%d]=%d\n\n", argv[i] + 14, j, int(ver[j]));
            ver[j] = 0;
          }
          s++;
        }
        write_version = (ver[0] << 24) | (ver[1] << 16) | (ver[2] << 8) | (ver[3]);
        force_legacy_format = false;
      }
      else if (stricmp("-forceLegacyFormat", argv[i]) == 0)
        force_legacy_format = true;
      else if (strnicmp("-exportDataForDict:", argv[i], 19) == 0)
        export_data_for_dict_dir = argv[i] + 19;
      else if (stricmp("-quiet", argv[i]) == 0)
        ; // skip
      else if (strnicmp("-blkSharedNamemapLocation:", argv[i], 26) == 0)
        inp.setStr("blkSharedNamemapLocation", argv[i] + 26);
      else if (strnicmp("-addpath:", argv[i], 9) == 0)
        ; // skip
      else if (strnicmp("-config:", argv[i], 8) == 0)
        ; // skip
      else if (simple_build_mode && (strnicmp("-dest_path:", argv[i], 11) == 0 || strnicmp("-out:", argv[i], 5) == 0 ||
                                      strnicmp("-packBlk", argv[i], 8) == 0 || strnicmp("-pack", argv[i], 5) == 0 ||
                                      strnicmp("-signPacked", argv[i], 11) == 0 || strnicmp("-sign:", argv[i], 6) == 0 ||
                                      strnicmp("-exclude:", argv[i], 9) == 0 || strnicmp("-subdirs", argv[i], 8) == 0 ||
                                      strnicmp("-rootfiles", argv[i], 10) == 0 || strnicmp("-include:", argv[i], 9) == 0) ||
               stricmp("-verbose", argv[i]) == 0)
        ; // skip
      else
      {
        printf("ERR: invalid switch %s\n\n", argv[i]);
        return -1;
      }


    int nid = inp.getNameId("platform");
    if (!targetCodes.size())
    {
      for (int i = 0; i < inp.paramCount(); i++)
        if (inp.getParamNameId(i) == nid && inp.getParamType(i) == DataBlock::TYPE_STRING)
          targetCodes.addInt(get_target_code(inp.getStr(i)));

      if (!targetCodes.size())
      {
        printf("WARN: no target codes specied - using PC\n\n");
        targetCodes.addInt(_MAKE4C('PC'));
      }
    }

    if (force_root_folder && targetCodes.size() > 1)
    {
      printf("ERR: cannot apply -rootDir:%s while specifiyin %d platforms > 1\n\n", force_root_folder, targetCodes.size());
      return -1;
    }
    if (force_root_folder)
    {
      inp.setStr("rootFolder", force_root_folder);
      if (targetCodes.size())
        inp.setStr(String(0, "rootFolder_%s", mkbindump::get_target_str(targetCodes[0])), force_root_folder);
    }

    outputFnames.resize(targetCodes.size());
    nid = inp.getNameId("folder");
    for (int i = 0; i < targetCodes.size(); i++)
    {
      String out_root_pname(0, "outputRoot_%s", mkbindump::get_target_str(targetCodes.getList()[i]));
      String out_pname(0, "output_%s", mkbindump::get_target_str(targetCodes.getList()[i]));
      const char *outroot = inp.getStr(out_root_pname, inp.getStr("outputRoot", ""));
      const char *outdir = inp.getStr(out_pname, inp.getStr("output", ""));
      inp.removeParam(out_root_pname);

      if (*outdir && *outroot)
      {
        inp.setStr(out_pname, String(0, "%s%s", outroot, outdir));
        outdir = inp.getStr(out_pname);
      }
      if (*outdir)
        outputFnames[i].addNameId(outdir);

      for (int j = 0; j < inp.blockCount(); j++)
        if (inp.getBlock(j)->getBlockNameId() == nid)
        {
          outdir = inp.getBlock(j)->getStr(out_pname, inp.getBlock(j)->getStr("output", ""));
          if (*outdir && *outroot)
          {
            inp.getBlock(j)->setStr(out_pname, String(0, "%s%s", outroot, outdir));
            outdir = inp.getBlock(j)->getStr(out_pname);
          }

          if (*outdir)
            outputFnames[i].addNameId(outdir);
        }
    }
    if (const char *outroot = inp.getStr("outputRoot", NULL))
    {
      if (const char *outdir = inp.getStr("output", NULL))
        if (*outroot && *outdir)
          inp.setStr("output", String(0, "%s%s", outroot, outdir));
      inp.removeParam("outputRoot");
    }

    int num_files = 0;
    for (int t = 0; t < targetCodes.size(); t++)
      for (int o = 0; o < outputFnames[t].nameCount(); o++)
      {
        unsigned targetCode = targetCodes.getList()[t];
        const char *output = outputFnames[t].getName(o);
        targetStrStor = mkbindump::get_target_str(targetCode);
        targetString = targetStrStor;

        if (buildFnList.nameCount() && buildFnList.getNameId(output) == -1)
        {
          printf("=== %4s: %s   -  skipping...\n", targetString, output);
          continue;
        }

        char dir_buf[260];
        dd_get_fname_location(dir_buf, output);
        if (!dir_buf[0])
          strcpy(dir_buf, ".");
        if (!inp.getBool("allowMkDir", false) && !dd_dir_exist(dir_buf))
        {
          printf("ERR: === %4s: %s   -  folder %s is missing and allowMkDir is not specified...\n", targetString, output, dir_buf);
          return 13;
        }

        printf("=== %4s: %s\n", targetString, output);

        OAHashNameMap<false> files;
        Tab<String> dst_files(tmpmem);
        if (write_version && force_legacy_format)
        {
          int id = files.addNameId(version_legacy_fn);
          if (dst_files.size() <= id)
            dst_files.resize(id + 1);
          dst_files[id] = version_legacy_fn;
        }
        gatherFilesList(inp, files, dst_files, targetCode, output);
        if (!files.nameCount())
          continue;

        String blk_root(260, "%s/%s/",
          inp.getStr(String(0, "rootFolder_%s", mkbindump::get_target_str(targetCode)), inp.getStr("rootFolder", ".")),
          inp.getStr("blkRelRoot", "."));
        dd_simplify_fname_c(blk_root);
        blk_root.resize(strlen(blk_root) + 1);
        DataBlock::setRootIncludeResolver(blk_root);

        z1_blk_sz = inp.getInt("packBlockSizeKB", 1024) << 10;
        if (!buildVromfsDump(output, targetCode, files, dst_files, parse_ext, inp.getBool("pack", true), inp,
              inp.getBool("storeContentSHA1", false), inp.getStr("altOutDir", NULL)))
          return 13;

        DataBlock::setIncludeResolver(NULL);
        // printHash(output, inp.getInt("hashSeed", 0));
        num_files += files.nameCount();
        if (write_version || !force_legacy_format)
          printf("writing VROMFS content version %d.%d.%d.%d (0x%08X), %s format\n", (write_version >> 24) & 0xFF,
            (write_version >> 16) & 0xFF, (write_version >> 8) & 0xFF, write_version & 0xFF, write_version,
            force_legacy_format ? "legacy format" : "2019-05-12 format");
      }

    if (!num_files)
      printf("WARN: no files found!\n");
  }
  else // Invalid command line
  {
    print_usage();
    return -1;
  }

  return 0;
}
