#include <scene/dag_loadLevel.h>
#include <scene/dag_loadLevelVer.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_asyncIo.h>
#include <ioSys/dag_fastSeqRead.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_texMetaData.h>
#include <startup/dag_startupTex.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <scene/dag_occlusionMap.h>
#include <perfMon/dag_perfTimer2.h>
#include <math/dag_mesh.h>

static bool auto_load_pending_tex = true;

#define LOGLEVEL_DEBUG _MAKE4C('LOAD')

void disable_auto_load_tex_after_binary_dump_loaded(bool dis) { auto_load_pending_tex = !dis; }

class BinaryDump
{
public:
  BinaryDump() : texture(midmem_ptr()), texture_id(midmem_ptr()), envi(midmem_ptr()), scn(midmem_ptr())
  {
    occluderGroupId = 0xFFFFFFFF;
  }

  Tab<BaseTexture *> texture;
  Tab<TEXTUREID> texture_id;
  Tab<RenderScene *> envi;
  Tab<RenderScene *> scn;
  SimpleString ddsxTexPack2;
  unsigned int occluderGroupId;

  void finishLoading(dag::ConstSpan<TEXTUREID> /*texMap*/, bool load_pending_tex)
  {
    if (load_pending_tex)
      ddsx::tex_pack2_perform_delayed_data_loading();
  }
};

static RenderScene *load_rscene(IGenLoad &crd, dag::ConstSpan<TEXTUREID> texmap, bool use_vis)
{
  RenderScene *scn = new RenderScene;
  scn->loadBinary(crd, texmap, use_vis);
  return scn;
}
static TEXTUREID load_tex(BinaryDump &bin_dump, IGenLoad &crd)
{
  String name;
  crd.readString(name);
  debug_ctx("load bin tex: %s", (char *)name);

  BaseTexture *tex = NULL;

  crd.beginBlock();
  int ofs = crd.tell();
  tex = d3d::create_ddsx_tex(crd, TEXCF_RGB | TEXCF_ABEST | TEXCF_LOADONCE, ::dgs_tex_quality, 0, name);
  set_ddsx_reloadable_callback(tex, BAD_TEXTUREID, crd.getTargetName(), ofs, BAD_TEXTUREID);
  crd.endBlock();
  if (!tex)
  {
    if (!d3d::is_in_device_reset_now())
      logerr("Texture %s wasn't loaded", name.c_str());
    return BAD_TEXTUREID;
  }

  tex->setAnisotropy(::dgs_tex_anisotropy);

  TEXTUREID tex_id = register_managed_tex(name, tex);

  bin_dump.texture.push_back(tex);
  bin_dump.texture_id.push_back(tex_id);

  return tex_id;
}

static bool load_binary_dump(BinaryDump &bin_dump, String originalName, IGenLoad &crd, Tab<TEXTUREID> &texMap,
  IBinaryDumpLoaderClient &client, unsigned bindump_id)
{
#ifndef NO_3D_GFX
  RenderScene *scn = NULL;
#endif
  String nm;
  int tag;
  FATAL_CONTEXT_AUTO_SCOPE(originalName);

  PerformanceTimer2 totalTimer(true);
  PerformanceTimer2 tagTimer;

  for (;;)
  {
    tag = crd.beginTaggedBlock();

    debug("[BIN] tag %c%c%c%c sz %dk", _DUMP4C(tag), crd.getBlockLength() >> 10);

    if (tag == _MAKE4C('END')) // valid end of binary dump
    {
      BBox3 bbox;
      client.bdlGetPhysicalWorldBbox(bbox);

      totalTimer.stop();
      debug("[BIN] load_binary_dump completed in %.2f ms", (float)(totalTimer.getTotalUsec()) / 1000.f);

      return true;
    }

    if (tag == _MAKE4C('TEX'))
    { // textures, handled specifically
#ifndef NO_3D_GFX
      tagTimer.go();
      if (!client.bdlConfirmLoading(bindump_id, tag))
        texMap.push_back(BAD_TEXTUREID);
      else
        texMap.push_back(load_tex(bin_dump, crd));
      tagTimer.stop();
      debug("[BIN] 'TEX' loaded in %.2f ms", (float)(tagTimer.getTotalUsec()) / 1000.f);
#endif
    }
    else if (client.bdlConfirmLoading(bindump_id, tag))
    {
      tagTimer.go();
      switch (tag)
      {
#ifndef NO_3D_GFX
        case _MAKE4C('TEX.'):
        {
          client.bdlTextureMap(bindump_id, texMap);
        }
        break;

        case _MAKE4C('DxP2'):
        {
          // load texture packs and resolve textures by name
          String str;
          String stor;
          String tmp_storage;

          crd.readString(str);
          bin_dump.ddsxTexPack2 = str;

          char *p = str;
          while (p)
          {
            char *pp = strchr(p, '~');
            if (pp)
            {
              *pp = '\0';
              pp++;
            }

            if (p[0])
              ddsx::load_tex_pack2(p);
            p = pp;
          }

          texMap.resize(crd.readInt());
          Tab<TEXTUREID> new_tex(tmpmem);
          new_tex.reserve(texMap.size());

          dagor_set_sm_tex_load_ctx_type('DxP2');
          dagor_set_sm_tex_load_ctx_name(originalName);
          for (int i = 0; i < texMap.size(); i++)
          {
            crd.readString(str);
            if (const char *name = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(str, tmp_storage))
              str = name;
            else
            {
              texMap[i] = D3DRESID(D3DRESID::INVALID_ID2);
              continue;
            }
            texMap[i] = get_managed_texture_id(str);

            if (texMap[i] == BAD_TEXTUREID)
            {
              logerr_ctx("tex <%s> not found", str.str());
              SimpleString original_str(str.str());

              str = TextureMetaData::decodeFileName(dd_get_fname(str), &stor);
              char *p_at = strchr(str, '@');
              if (p_at)
                *p_at = 0;
              p_at = (char *)dd_get_fname_ext(str);
              if (p_at)
                *p_at = 0;
              str.resize(strlen(str) + 1);
              str += "*";
              texMap[i] = get_managed_texture_id(str);
              if (texMap[i] == BAD_TEXTUREID)
              {
                if (strcmp(original_str, str) != 0)
                  logerr_ctx("even tex <%s> not found", str.str());
                continue;
              }
            }

            if (find_value_idx(new_tex, texMap[i]) == -1)
              new_tex.push_back(texMap[i]);
          }
          dagor_reset_sm_tex_load_ctx();

          char buf[2048];
          get_memory_stats(buf, 2048);
          debug("before delayed_binary_dumps_unload: %s", buf);
          delayed_binary_dumps_unload();
          get_memory_stats(buf, 2048);
          debug("after delayed_binary_dumps_unload: %s", buf);
        }
        break;

        case _MAKE4C('INC'):
        { // include
          crd.readString(nm);
          nm = originalName + nm;
          FullFileLoadCB crdInclude(nm);
          if (!crdInclude.fileHandle)
          {
            debug_ctx("invalid include : file %s", (const char *)nm);
            return false;
          }
          else
          {
            debug_ctx("loading include : file %s", (const char *)nm);
          }
          if (!load_binary_dump(bin_dump, originalName, crdInclude, texMap, client, bindump_id))
            return false;
        }
        break;

        case _MAKE4C('ENVI'):
          scn = load_rscene(crd, texMap, false);
          if (!scn)
          {
            debug_ctx("error loading envi scene");
            return false;
          }
          if (scn->obj.empty())
            delete scn;
          else
          {
            client.bdlEnviLoaded(bindump_id, scn);
            bin_dump.envi.push_back(scn);
          }
          break;

        case _MAKE4C('SCN'):
          scn = load_rscene(crd, texMap, true);
          if (!scn)
          {
            debug_ctx("error loading scene (%s)", (char *)nm);
            return false;
          }
          if (scn->obj.empty())
            delete scn;
          else
          {
            client.bdlSceneLoaded(bindump_id, scn);
            bin_dump.scn.push_back(scn);
          }
          break;

          /*?
          case _MAKE4C('OPLS'):
            if ( !::place_objects( crd )) {
              debug_ctx ( "OPLS not loaded" );
              return false;
            }
            break;
          */

        case _MAKE4C('SCNP'): break;

        case _MAKE4C('OCCL'):
        {
          unsigned int version = crd.readInt();
          if (version == 0x20080514)
          {
            if (!occlusion_map)
              occlusion_map = new OcclusionMap();
            occlusion_map->load(crd);
          }
        }
        break;

        case _MAKE4C('PORT'): break;
#endif // NO_3D_GFX

        case _MAKE4C('eVER'):
        {
          String ver;
          crd.readString(ver);
          debug("eVER: %s", (char *)ver);
          client.bdlBinDumpVerLoaded(bindump_id, ver, crd.getTargetName());
          if (crd.getBlockRest() >= 24)
            client.bdlCustomLoad(bindump_id, tag, crd, texMap);
          break;
        }

        default:
          if (!client.bdlCustomLoad(bindump_id, tag, crd, texMap))
          {
            debug_ctx("error loading custom field %c%c%c%c (%08X)", _DUMP4C(tag), tag);
            return false;
          }
          break;
      }
      tagTimer.stop();
      debug("[BIN] tag '%c%c%c%c'(%08X) loaded in %.2f ms", _DUMP4C(tag), tag, (float)(tagTimer.getTotalUsec()) / 1000.f);
    }
    crd.endBlock();
  }
  return false;
}

static const char *dumps_base_path = NULL;

void set_binary_dump_base_path(const char *path) { dumps_base_path = path; }

static void free_binary_dump(BinaryDump *bin_dump);
static void clear_binary_dump(BinaryDump *bin_dump, dag::ConstSpan<TEXTUREID> texMap)
{
  // FIXME: there is no clear understanding who owns this scenes - BinaryDump or RenderSceneManager (assume later here)
  clear_and_shrink(bin_dump->envi);
  clear_and_shrink(bin_dump->scn);

  bin_dump->finishLoading(texMap, false);

  free_binary_dump(bin_dump);
}

BinaryDump *load_binary_dump(const char *fname, IBinaryDumpLoaderClient &client, unsigned bindump_id)
{
  int t0 = get_time_msec();
  FastSeqReadCB crd;
  Tab<TEXTUREID> texMap(tmpmem);
  BinaryDump *bin_dump = NULL;

  if (!crd.open(fname, 32 << 10, dumps_base_path))
  {
    logerr("Failed to open level file %s", fname);
    return NULL;
  }

  int dbg = 0;
  DAGOR_TRY
  {
    if (crd.readInt() != _MAKE4C('DBLD'))
    {
      logerr("Bad file mark of level %s", fname);
      return NULL;
    }
    if (crd.readInt() != DBLD_Version)
    {
      logerr("Invalid version of level '%s'", fname);
      return NULL;
    }

    int tex_count = crd.readInt();
    texMap.resize(tex_count);
    for (int i = 0; i < tex_count; i++)
      texMap[i] = BAD_TEXTUREID;
    texMap.resize(0);

    bin_dump = new (inimem) BinaryDump;
    debug("Starting actual loading... bin_dump = %p", bin_dump);
    if (!load_binary_dump(*bin_dump, String(fname), crd, texMap, client, bindump_id))
    {
      logerr("load_binary_dump 1: can't load binary dump '%s'", fname);
      clear_binary_dump(bin_dump, texMap);
      bin_dump = NULL;
    }
    debug("End of actual loading... bin_dump=%p", bin_dump);
    dbg = 1;
  }
  DAGOR_CATCH(IGenLoad::LoadException exc)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    debug("exception: can't load binary dump '%s'", fname);
    debug("exception at filepos %d\n%s", exc.fileOffset, DAGOR_EXC_STACK_STR(exc));
#endif
#if DAGOR_DBGLEVEL <= 0
    DAGOR_RETHROW();
#endif
  }
  if (!dbg) //-V547
    logerr("WTF? Uncatched exception");
  debug("return from load_binary_dump, bin_dump = %p", bin_dump);
  t0 = get_time_msec() - t0;
  debug("binary loaded for %d ms, %dK/s", t0, t0 > 0 ? crd.getSize() / t0 : crd.getSize());

  if (bin_dump)
    bin_dump->finishLoading(texMap, auto_load_pending_tex);

  crd.close();
  client.bdlBinDumpLoaded(bindump_id);

  return bin_dump;
}

// generic load interface implemented as async reader
class AsyncProxyLoadCB : public IBaseLoad
{
  AsyncLoadCB *asyncCB;
  struct PackedFile
  {
    char *fileName;
    unsigned fileStart;
    int fileSize;
  };
  PackedFile *files;
  int fileCount;
  PackedFile *currentFile;
  String proxyFileName;

public:
  AsyncProxyLoadCB() : asyncCB(NULL)
  {
    currentFile = 0;
    fileCount = 0;
    files = NULL;
  }

  bool initProxy(const char *proxyFname)
  {
    if (dd_stricmp(proxyFileName, proxyFname) == 0)
    {
      return files != NULL;
    }
    proxyFileName = proxyFname;
    if (asyncCB)
      delete asyncCB;
    asyncCB = new (tmpmem) AsyncLoadCB(proxyFname);
    if (!asyncCB->isOpen())
    {
      delete asyncCB;
      asyncCB = NULL;
      return false;
    }
#if _TARGET_64BIT
    return false;
#else
    int magic;
    asyncCB->read(&magic, 4);
    if (magic != 0x10101010)
    {
      delete asyncCB;
      asyncCB = NULL;
      return false;
    }
    int fileTableSize;
    asyncCB->read(&fileTableSize, 4);
    asyncCB->read(&fileCount, 4);
    files = (PackedFile *)memalloc(fileTableSize, midmem);
    asyncCB->read(files, fileTableSize);
    for (int fi = 0; fi < fileCount; ++fi)
    {
      files[fi].fileName += int(files);
    }
    currentFile = NULL;
#endif
    return true;
  }
  ~AsyncProxyLoadCB()
  {
    if (files)
      memfree(files, midmem);
    if (asyncCB)
      delete asyncCB;
    files = NULL;
  }
  bool open(const char *fname)
  {
    String proxy(fname);
    G_ASSERTF(proxy.length() > 0, "AsyncProxyLoadCB::open() - empty file name");
    int i;
    blocks.clear();
    for (i = proxy.length(); i >= 0; --i)
      if (proxy[i] == '/' || proxy[i] == '\\')
      {
        proxy[i] = 0;
        proxy.resize(i + 1);
        break;
      }
    if (i && !initProxy(proxy + ".pak.bin"))
    {
      debug_ctx("no proxy %s for %s", (char *)proxy, fname);
    }
    if (files)
    {
      int fi;

      for (fi = 0; fi < fileCount; ++fi)
      {
        if (dd_stricmp(files[fi].fileName, fname) == 0)
          break;
      }

      if (fi >= fileCount)
        return false;
      currentFile = &files[fi];
      seekto(0);
      return true;
    }
    else
    {
      if (asyncCB)
        close();
      asyncCB = new (tmpmem) AsyncLoadCB(fname);
      if (!asyncCB->isOpen())
      {
        delete asyncCB;
        asyncCB = NULL;
        return false;
      }
      return true;
    }
  }
  void close()
  {
    if (files)
    {
      currentFile = NULL;
    }
    else
    {
      if (asyncCB)
        delete asyncCB;
      asyncCB = NULL;
    }
  }

  inline bool isOpen() { return files ? currentFile != NULL : (asyncCB != NULL ? asyncCB->isOpen() : false); }

  virtual void read(void *ptr, int size)
  {
    if (!asyncCB)
      return;
    asyncCB->read(ptr, size);
  }
  virtual int tryRead(void *ptr, int size)
  {
    if (!asyncCB)
      return 0;
    if (currentFile)
    {
      int pos = tell();
      if (pos + size > currentFile->fileSize)
        size = currentFile->fileSize - pos;
      if (size <= 0)
        return 0;
    }
    return asyncCB->tryRead(ptr, size);
  }
  virtual int tell()
  {
    if (!asyncCB)
      return 0;
    if (currentFile)
    {
      return asyncCB->tell() - int(currentFile->fileStart);
    }
    else
    {
      return asyncCB->tell();
    }
  }
  virtual void seekto(int to)
  {
    if (!asyncCB)
      return;
    if (currentFile)
    {
      if (to < 0)
        to = 0;
      else if (to > currentFile->fileSize)
        to = currentFile->fileSize;
      asyncCB->seekto(currentFile->fileStart + to);
    }
    else
    {
      asyncCB->seekto(to);
    }
  }
  virtual void seekrel(int to)
  {
    if (!asyncCB)
      return;
    if (currentFile)
    {
      seekto(tell() + to);
    }
    else
    {
      asyncCB->seekrel(to);
    }
  }
  virtual const char *getTargetName() { return proxyFileName; }
};

static AsyncProxyLoadCB asyncLoadBinaryDumps;

BinaryDump *load_binary_dump_async(const char *fname, IBinaryDumpLoaderClient &client, unsigned bindump_id)
{
  FastSeqReadCB *sync_crd = NULL;
  int t0 = get_time_msec();

  // perform synchronous loading
  sync_crd = new (tmpmem) FastSeqReadCB;
  if (!sync_crd->open(fname, 32 << 10))
  {
    debug_ctx("invalid file %s", fname);
    return NULL;
  }

  IGenLoad &crd = *sync_crd;
  Tab<TEXTUREID> texMap(tmpmem);
  BinaryDump *bin_dump = NULL;

  int dbg = 0;
  DAGOR_TRY
  {
    if (crd.readInt() != _MAKE4C('DBLD'))
    {
      debug_ctx("invalid file mark");
      delete sync_crd;
      return NULL;
    }
    if (crd.readInt() != DBLD_Version)
    {
      debug_ctx("invalid version");
      delete sync_crd;
      return NULL;
    }

    int tex_count = crd.readInt();
    texMap.resize(tex_count);
    for (int i = 0; i < tex_count; i++)
      texMap[i] = BAD_TEXTUREID;

    bin_dump = new (inimem) BinaryDump;
    debug("Starting actual loading...,, bin_dump = %p", bin_dump);
    if (!load_binary_dump(*bin_dump, String(fname), crd, texMap, client, bindump_id))
    {
      debug("load_binary_dump 1: can't load binary dump '%s'", fname);
      clear_binary_dump(bin_dump, texMap);
      bin_dump = NULL;
    }
    debug("End of actual loading... bin_dump=%p", bin_dump);

    // we don't want to remember (and free on unload) these ids because texId is external
    if (bin_dump)
      clear_and_shrink(bin_dump->texture_id);

    dbg = 1;
  }
  DAGOR_CATCH(IGenLoad::LoadException exc)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    debug("exception: can't load binary dump '%s'", fname);
    debug("exception at filepos %d\n%s", exc.fileOffset, DAGOR_EXC_STACK_STR(exc));
#endif
  }
  if (!dbg) //-V547
  {
    debug("WTF? Uncatched exception");
    clear_binary_dump(bin_dump, texMap);
    bin_dump = NULL;
  }
  debug("return from load_binary_dump, bin_dump = %p", bin_dump);

  if (bin_dump)
    bin_dump->finishLoading(texMap, auto_load_pending_tex);
  t0 = get_time_msec() - t0;
  debug("binary \"%s\" loaded for %d ms, %dK/s", fname, t0, t0 > 0 && sync_crd ? sync_crd->getSize() / t0 : 0);
  delete sync_crd;
  client.bdlBinDumpLoaded(bindump_id);

  return bin_dump;
}

static void free_binary_dump(BinaryDump *bin_dump)
{
#ifndef NO_3D_GFX

  for (int i = 0; i < bin_dump->envi.size(); ++i)
    delete bin_dump->envi[i];
  clear_and_shrink(bin_dump->envi);

  for (int i = 0; i < bin_dump->scn.size(); ++i)
    delete bin_dump->scn[i];
  clear_and_shrink(bin_dump->scn);

  for (int i = 0; i < bin_dump->texture_id.size(); ++i)
  {
    debug("releasing %s", get_managed_texture_name(bin_dump->texture_id[i]));
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(bin_dump->texture_id[i], bin_dump->texture[i]);
  }

  if (!bin_dump->ddsxTexPack2.empty())
  {
    char *p = bin_dump->ddsxTexPack2;
    while (p)
    {
      char *pp = strchr(p, '~');
      if (pp)
      {
        *pp = '\0';
        pp++;
      }

      ddsx::release_tex_pack2(p);
      p = pp;
    }
  }
#endif // NO_3D_GFX

  delete bin_dump;
}

static Tab<BinaryDump *> dumps_to_unload(tmpmem);

bool unload_binary_dump(BinaryDump *bin_dump, bool delete_render_scenes)
{
  if (bin_dump == NULL)
    return false;

  if (!delete_render_scenes)
  {
    clear_and_shrink(bin_dump->envi);
    clear_and_shrink(bin_dump->scn);
  }
  if (!bin_dump->ddsxTexPack2.empty())
    dumps_to_unload.push_back(bin_dump);
  else
    free_binary_dump(bin_dump);
  return true;
}

void delayed_binary_dumps_unload()
{
  for (int i = 0; i < dumps_to_unload.size(); i++)
    free_binary_dump(dumps_to_unload[i]);
  dumps_to_unload.clear();
}
