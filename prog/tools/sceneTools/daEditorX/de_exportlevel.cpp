// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include "de_ExportToDagDlg.h"
#include "de_batch_log.h"

#include <oldEditor/de_util.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <de3_entityFilter.h>

#include <coolConsole/coolConsole.h>

#include <3d/dag_texPackMgr2.h>
#include <3d/dag_texIdSet.h>

#include <libTools/dtx/dtxSave.h>

#include <generic/dag_sort.h>
#include <libTools/util/hash.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/binDumpReader.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/dagFileRW/textureNameResolver.h>

#include <libTools/staticGeom/matFlags.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <util/dag_texMetaData.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>
#include <assets/assetMgr.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>

#include <osApiWrappers/dag_files.h>

#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>
#include <debug/dag_debug.h>
#include <scene/dag_loadLevelVer.h>
#include <shaders/dag_renderScene.h>
#include <perfMon/dag_cpuFreq.h>

#include <3d/ddsFormat.h>
#include <3d/ddsxTex.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_busy.h>

#include <sepGui/wndGlobal.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>

#undef ERROR

using hdpi::_pxScaled;

const char *BASE_EXPORT_PATH_PC = "/../game/levels/";
const char *BASE_EXPORT_PATH_360 = "/../game360/levels/";
const char *BASE_EXPORT_PATH_PS3 = "/../game.PS3/levels/";
const char *BASE_EXPORT_PATH_PS4 = "/../game.PS4/levels/";
const char *BASE_EXPORT_PATH_iOS = "/../game.iOS/levels/";
const char *BASE_EXPORT_PATH_AND = "/../game.and/levels/";

extern bool minimize_dabuild_usage;

struct ExportPluginRec
{
  IGenEditorPlugin *plug;
  bool *doExport;
  bool *isExternal;
};

struct ExportFiltersPlgRec
{
  IGenEditorPlugin *filter;
  bool *useFilter;
};

static int get_tex_asset_data_size(DagorAsset *a, unsigned target)
{
  if (!a)
    return 0;
  if (a->getType() != a->getMgr().getTexAssetTypeId())
    return 0;

  if (minimize_dabuild_usage)
  {
    ddsx::DDSxDataPublicHdr ddsx_hdr;
    int ddsx_msize = ddsx::read_ddsx_header(String(0, "%s*", a->getName()), ddsx_hdr, true);
    if (ddsx_msize >= 0)
      return ddsx_msize;
  }

  return texconvcache::get_tex_size(*a, target, NULL);
}

unsigned TextureRemapHelper::getTexturesSize(unsigned target) const
{
  unsigned size = 0;

  for (int i = 0; i < getDDSxTexturesCount(); i++)
    size += getDDSxTextureSize(i);

  if (::get_gameres_sys_ver() == 2)
  {
    int actype = assetrefs::get_tex_type();
    for (int i = 0; i < texname.nameCount(); ++i)
    {
      DagorAsset *a = DAEDITOR3.getAssetByName(texname.getName(i), actype);
      int sz = get_tex_asset_data_size(a, target);
      if (sz < 0)
        DAEDITOR3.conWarning("cannot get size of %s texture asset", texname.getName(i));
      size += sz;
    }
    return size;
  }

  for (int i = 0; i < texname.nameCount(); ++i)
  {
    file_ptr_t f;

    f = ::df_open(texname.getName(i), DF_READ);

    if (f)
    {
      size += ::df_length(f);
      ::df_close(f);
    }
  }

  return size;
}


static void write_textures(mkbindump::BinDumpSaveCB &cwr, const NameMap &ddsxTexName, dag::ConstSpan<ddsx::Buffer> ddsxTex,
  const char *exp_fname_without_ext)
{
  String tex_name;
  for (int i = 0; i < ddsxTexName.nameCount(); i++)
  {
    const char *nm = ddsxTexName.getName(i);
    if (*nm == '$')
    {
      tex_name.printf(0, "%s%s", exp_fname_without_ext, nm + 1);
      nm = tex_name;
    }

    cwr.beginTaggedBlock(_MAKE4C('TEX'));
    cwr.writeDwString(nm);
    cwr.writeInt32e(ddsxTex[i].len);
    cwr.writeRaw(ddsxTex[i].ptr, ddsxTex[i].len);
    cwr.align8();
    cwr.endBlock();
  }
}

TextureRemapHelper::TextureRemapHelper(int target) { targetCode = target; }
TextureRemapHelper::~TextureRemapHelper()
{
  for (int i = 0; i < ddsxTex.size(); i++)
    ddsxTex[i].free();
  clear_and_shrink(ddsxTex);
}

int TextureRemapHelper::getDDSxTextureSize(int i) const
{
  if (ddsxTex[i].len)
    return mkbindump::le2be32_cond(((const ddsx::Header *)ddsxTex[i].ptr)->memSz, dagor_target_code_be(targetCode));
  return 0;
}
bool TextureRemapHelper::saveTextures(mkbindump::BinDumpSaveCB &cwr, const char *exp_fname_without_ext, ILogWriter &log) const
{
  if (::get_gameres_sys_ver() == 2)
  {
    cwr.beginTaggedBlock(_MAKE4C('DxP2'));
    cwr.writeDwString("~");
    cwr.writeInt32e(texname.nameCount());
    for (int i = 0; i < texname.nameCount(); i++)
    {
      // debug("export texref[%d]: %s", i, texname.getName(i));
      cwr.writeDwString(String(260, "%s*", texname.getName(i)));
    }
    cwr.align8();
    cwr.endBlock();
    write_textures(cwr, ddsxTexName, ddsxTex, exp_fname_without_ext);
    return true;
  }

  SmallTab<char, TmpmemAlloc> buf;
  for (int i = 0; i < texname.nameCount(); i++)
  {
    // debug("export texref[%d]: %s", i, texname.getName(i));

    TextureMetaData tmd;
    const char *name = tmd.decode(texname.getName(i));

    cwr.beginTaggedBlock(_MAKE4C('TEX'));
    cwr.writeDwString(name);

    // debug("export tex: %s", name);

    file_ptr_t handle = df_open(name, DF_READ);
    if (handle)
    {
      const int len = df_length(handle);
      if (buf.size() < len)
        clear_and_resize(buf, len);
      df_read(handle, buf.data(), len);
      df_close(handle);

      ddsx::Buffer b;
      ddsx::ConvertParams cp;
      cp.addrU = tmd.d3dTexAddr(tmd.addrU);
      cp.addrV = tmd.d3dTexAddr(tmd.addrV);
      cp.hQMip = tmd.hqMip;
      cp.mQMip = tmd.mqMip;
      cp.lQMip = tmd.lqMip;
      cp.mipOrdRev = true;

      if (ddsx::convert_dds(cwr.getTarget(), b, buf.data(), len, cp))
      {
        cwr.writeInt32e(b.len);
        cwr.writeRaw(b.ptr, b.len);
        // DEBUG_CTX("DDS(%d)->DDSx(%d)= +%d b", len, b.len, b.len-len);
        b.free();
      }
      else
      {
        cwr.writeInt32e(0);
        log.addMessage(ILogWriter::ERROR, "Can't export image: %s", ddsx::get_last_error_text());
        return false;
      }
    }
    else
    {
      cwr.writeInt32e(0);
      log.addMessage(ILogWriter::ERROR, "can't open %s", (char *)name);
      return false;
    }
    cwr.align8();
    cwr.endBlock();
  }
  write_textures(cwr, ddsxTexName, ddsxTex, exp_fname_without_ext);
  return true;
}


int TextureRemapHelper::addTextureName(const char *src_fname)
{
  String tmp_stor;
  const char *dec_fn = TextureMetaData::decodeFileName(src_fname, &tmp_stor);
  String fname(DagorAsset::fpath2asset(dec_fn));
  String name;
  if (::get_gameres_sys_ver() == 2)
  {
    DagorAsset *a = DAEDITOR3.getAssetByName(fname, assetrefs::get_tex_type());
    if (!a && trail_strcmp(fname, "*"))
    {
      name = fname;
      remove_trailing_string(name, "*");
      a = DAEDITOR3.getAssetByName(name, assetrefs::get_tex_type());
    }
    else if (!a && dd_get_fname_ext(dec_fn))
      a = DAEDITOR3.getAssetByName(dec_fn, assetrefs::get_tex_type());
    if (!a)
    {
      DAEDITOR3.conError("cannot resolve texture ref: %s", fname);
      return -1;
    }
    name = a->getName();
    dd_strlwr(name);
    return texname.addNameId(name);
  }

  if (!get_global_tex_name_resolver()->resolveTextureName(fname, name))
  {
    DAEDITOR3.conError("cannot resolve texture ref: %s", fname);
    return -1;
  }
  dd_strlwr(name);

  return (validateTexture_(name)) ? texname.addNameId(name) : -1;
}
int TextureRemapHelper::addDDSxTexture(const char *fname, ddsx::Buffer &b)
{
  int id = ddsxTexName.addNameId(fname);
  if (id == ddsxTex.size())
    ddsxTex.push_back(b);
  else
    b.free();
  return id;
}


int TextureRemapHelper::getTextureOrdinal(const char *src_fname) const
{
  String tmp_stor;
  const char *dec_fn = TextureMetaData::decodeFileName(src_fname, &tmp_stor);
  String fname(DagorAsset::fpath2asset(dec_fn));
  String name;
  int id = ddsxTexName.getNameId(src_fname);
  if (id >= 0)
    return texname.nameCount() + id;
  if (::get_gameres_sys_ver() == 2)
  {
    DagorAsset *a = DAEDITOR3.getAssetByName(fname, assetrefs::get_tex_type());
    if (!a && trail_strcmp(fname, "*"))
    {
      name = fname;
      remove_trailing_string(name, "*");
      a = DAEDITOR3.getAssetByName(name, assetrefs::get_tex_type());
    }
    else if (!a && dd_get_fname_ext(dec_fn))
      a = DAEDITOR3.getAssetByName(dec_fn, assetrefs::get_tex_type());
    if (!a)
    {
      DAEDITOR3.conError("cannot resolve texture ref: %s", fname);
      return -1;
    }
    name = a->getName();
    dd_strlwr(name);
    return texname.getNameId(name);
  }

  if (!get_global_tex_name_resolver()->resolveTextureName(fname, name))
  {
    DAEDITOR3.conError("cannot resolve texture ref: %s", fname);
    return -1;
  }
  dd_strlwr(name);

  return texname.getNameId(name);
}


static inline bool is_pow_2(int i) { return (i & (i - 1)) == 0; }

bool TextureRemapHelper::validateTexture_(const char *file_name_param)
{
  TextureMetaData tmd;
  const char *file_name = tmd.decode(file_name_param);

  bool success = false;
  const char *ext = dd_get_fname_ext(file_name);

  if (::dd_file_exist(file_name))
  {
    if (stricmp(ext, ".dds") == 0)
    {
      file_ptr_t file = df_open(file_name, DF_READ);

      if (file)
      {
        uint32_t magic = 0;

        df_read(file, &magic, 4);

        if (magic == 0x20534444)
        {
          DDSURFACEDESC2 header;
          df_read(file, &header, sizeof(header));

          success = is_pow_2(header.dwHeight) && is_pow_2(header.dwWidth);

          if (!success)
            DAEDITOR3.conError("wrong image size [%s] %d x %d", file_name, header.dwHeight, header.dwWidth);
        }
        else
          DAEDITOR3.conError("'%s' is not a valid DDS file", file_name);

        df_close(file);
      }
      else
        DAEDITOR3.conError("failed to open '%s' -- possibly corrupt file", file_name);
    }
    else
      DAEDITOR3.conError("Texture '%s' is not .DDS", file_name);
  }
  else
    DAEDITOR3.conError("File \"%s\" not found", file_name);

  return success;
};

static void make_scene_fname(String &dest_fn, const char *scene_dir, int target_code)
{
  dest_fn = ::make_full_path(scene_dir, String(64, "scene-%s.scn", mkbindump::get_target_str(target_code)));
}

void add_built_scene_textures(const char *scene_dir, ITextureNumerator &tn)
{
  int i, n;
  String fn;

  make_scene_fname(fn, scene_dir, tn.getTargetCode());

  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    DEBUG_CTX("can't open scene file '%s'", (char *)fn);
    return;
  }

  if (crd.readInt() != _MAKE4C('scnf'))
  {
    DEBUG_CTX("no scnf label: %s", (char *)fn);
    return;
  }

  crd.beginBlock();
  n = crd.readInt();
  String name;

  for (i = 0; i < n; ++i)
  {
    int l = crd.readIntP<1>();
    if (l)
    {
      name.resize(l + 1);
      crd.read(&name[0], l);
      name[l] = 0;
      tn.addTextureName(name);
    }
  }

  crd.endBlock();
}

struct SceneHdr
{
  int version, rev;
  int texNum, matNum, vdataNum, mvhdrSz;
  int ltmapNum, ltmapFormat;
  float lightmapScale, directionalLightmapScale;
  int objNum, fadeObjNum, ltobjNum;
  int objDataSize;
};

bool check_built_scene_data_exist(const char *scene_dir, int target_code)
{
  String fn;
  make_scene_fname(fn, scene_dir, target_code);
  return dd_file_exist(fn);
}

//==============================================================================
bool check_built_scene_data(const char *scene_dir, int target_code)
{
  String fn;
  bool read_be = dagor_target_code_be(target_code);

  make_scene_fname(fn, scene_dir, target_code);

  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    DAEDITOR3.conError("can't open scene file '%s'", fn.str());
    return false;
  }

  if (crd.readInt() != _MAKE4C('scnf'))
  {
    DAEDITOR3.conError("no scnf label: %s", fn.str());
    return false;
  }
  crd.beginBlock();
  crd.endBlock();

  SceneHdr shdr;
  crd.read(&shdr, sizeof(shdr));
  if (read_be)
  {
    // mkbindump::le2be32_s(&shdr, &shdr, sizeof(shdr)/sizeof(uint32_t));
    shdr.texNum = mkbindump::le2be32(shdr.texNum);
    shdr.ltmapNum = mkbindump::le2be32(shdr.ltmapNum);
    shdr.rev = mkbindump::le2be32(shdr.rev);
  }

  if (shdr.version != _MAKE4C('scn2'))
  {
    DAEDITOR3.conError("invalid version=%c%c%c%c: %s", _DUMP4C(shdr.version), fn.str());
    return false;
  }
  if (shdr.rev < 0x20130524)
  {
    DAEDITOR3.conError("obsolete revision=%p: %s", shdr.rev, fn.str());
    return false;
  }
  if (shdr.texNum < 0)
  {
    DAEDITOR3.conError("invalid texNum=%d: %s", _DUMP4C(shdr.version), fn.str());
    return false;
  }

  if (shdr.ltmapFormat == _MAKE4C('SB2'))
    crd.seekrel(2 * 3 * 4);

  crd.seekrel(4); // restDataSize
  crd.seekrel(shdr.texNum * 4);

  if (!shdr.ltmapNum && shdr.ltmapFormat && shdr.ltmapFormat != _MAKE4C('SB2'))
    DAEDITOR3.conWarning("obsolete %s (ltmap fmt=%c%c%c%c with count=0), please re-receive light", fn.str(),
      _DUMP4C(shdr.ltmapFormat));

  DEBUG_CTX("ltmap: num=%d format=%p", shdr.ltmapNum, shdr.ltmapFormat);
  if (shdr.ltmapNum && shdr.ltmapFormat != _MAKE4C('DUAL') && shdr.ltmapFormat != _MAKE4C('HDR') &&
      shdr.ltmapFormat != _MAKE4C('DDS') && shdr.ltmapFormat != _MAKE4C('TGA') && shdr.ltmapFormat != _MAKE4C('BUMP') &&
      shdr.ltmapFormat != _MAKE4C('SB2'))
  {
    DAEDITOR3.conError("invalid ltmap fmt=%c%c%c%c: %s", _DUMP4C(shdr.ltmapFormat), fn.str());
    return false; // bad lightmap format
  }

  if (shdr.ltmapFormat == _MAKE4C('DUAL'))
    shdr.ltmapNum *= 2;

  for (int i = 0; i < shdr.ltmapNum; i++)
    if (crd.readInt() == -1)
    {
      DAEDITOR3.conError("wrong catalog entry %d: %s", i, fn.str());
      return false; // incorrect (incomplete) catalog
    }

  return true;
}

void store_built_scene_data(const char *scene_dir, mkbindump::BinDumpSaveCB &cwr, const ITextureNumerator &tn)
{
  String fn;
  bool write_be = dagor_target_code_be(cwr.getTarget());

  make_scene_fname(fn, scene_dir, cwr.getTarget());

  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    DEBUG_CTX("can't open scene file '%s'\n", (char *)fn);
    return;
  }


  SmallTab<int, TmpmemAlloc> tex_idx, tex_map;
  String name;

  if (crd.readInt() != _MAKE4C('scnf'))
    return;

  crd.beginBlock();
  clear_and_resize(tex_idx, crd.readInt());
  for (int i = 0; i < tex_idx.size(); i++)
  {
    int l = crd.readIntP<1>();

    name.resize(l + 1);
    crd.read(&name[0], l);
    name[l] = 0;

    tex_idx[i] = tn.getTextureOrdinal(name);
  }
  crd.endBlock();

  SceneHdr shdr;
  crd.read(&shdr, sizeof(shdr));

  if (shdr.version != _MAKE4C('scn2'))
    return;

  Color3 sunColor;
  Point3 sunDirection;

  if (shdr.ltmapFormat == _MAKE4C('SB2'))
  {
    crd.read(&sunColor, sizeof(sunColor));
    crd.read(&sunDirection, sizeof(sunDirection));
  }

  int tex_num = shdr.texNum, rest_size;
  crd.read(&rest_size, sizeof(rest_size));

  if (write_be)
  {
    tex_num = mkbindump::le2be32(tex_num);
    rest_size = mkbindump::le2be32(rest_size);
  }

  clear_and_resize(tex_map, tex_num);
  crd.readTabData(tex_map);
  mkbindump::le2be32_s_cond(tex_map.data(), tex_map.data(), tex_map.size(), write_be);

  for (int i = 0; i < tex_num; i++)
    tex_map[i] = tex_idx[tex_map[i]];

  cwr.writeRaw(&shdr, sizeof(shdr));

  if (shdr.ltmapFormat == _MAKE4C('SB2'))
  {
    cwr.writeRaw(&sunColor, sizeof(sunColor));
    cwr.writeRaw(&sunDirection, sizeof(sunDirection));
  }

  cwr.writeInt32e(rest_size);
  cwr.writeTabData32ex(tex_map);
  cwr.copyRaw(crd, rest_size - data_size(tex_map));
  return;
}


bool DagorEdAppWindow::showPluginDlg(Tab<IGenEditorPlugin *> &build_plugin, Tab<bool *> &do_export, Tab<bool *> &is_external)
{
  class ExportPanelClient : public PropPanel::DialogWindow // IPropertyPanelDlgClient
  {
  public:
    ExportPanelClient(const Tab<IGenEditorPlugin *> &build_plugin, const Tab<bool *> &do_export, const Tab<bool *> &is_external,
      const Tab<IGenEditorPlugin *> &filter_plugin, const Tab<bool *> &use_filter) :
      DialogWindow(NULL, _pxScaled(300), _pxScaled(600), "Plugins to export"), exportData(tmpmem), filterPlugs(tmpmem)
    {
      G_ASSERT(build_plugin.size() == do_export.size());
      G_ASSERT(build_plugin.size() == is_external.size());

      exportData.resize(build_plugin.size());

      for (int i = 0; i < build_plugin.size(); ++i)
      {
        ExportPluginRec &rec = exportData[i];
        rec.plug = build_plugin[i];
        rec.doExport = do_export[i];
        rec.isExternal = is_external[i];
      }

      sort(exportData, &sortPluginsToExport);

      const int fpc = filter_plugin.size();

      filterPlugs.resize(fpc);
      for (int i = fpc - 1; i >= 0; i--)
      {
        ExportFiltersPlgRec &rec = filterPlugs[i];
        rec.filter = filter_plugin[i];
        rec.useFilter = use_filter[i];
      }

      mPanel = getPanel();
      G_ASSERT(mPanel && "No panel in CamerasConfigDlg");
      fillPanel();
    }

    void fillPanel()
    {
      int i;
      for (i = 0; i < exportData.size(); ++i)
      {
        const ExportPluginRec &rec = exportData[i];
        mPanel->createCheckBox(i, rec.plug->getMenuCommandName(), *rec.doExport);
      }

      if (filterPlugs.size())
      {
        PropPanel::ContainerPropertyControl *maxGrp = mPanel->createGroup(PID_FILTER_LIST_GRP, "Filters");

        for (int i = 0; i < filterPlugs.size(); ++i)
          maxGrp->createCheckBox(PID_FILTER_LIST_START + i, filterPlugs[i].filter->getMenuCommandName(),
            *filterPlugs[i].useFilter ? 1 : 0);
        mPanel->setBool(PID_FILTER_LIST_GRP, true);
      }


      PropPanel::ContainerPropertyControl *maxGrp = mPanel->createGroup(PID_EXTERNAL_LIST_GRP, "External plugins");

      if (maxGrp)
        for (i = 0; i < exportData.size(); ++i)
        {
          const ExportPluginRec &rec = exportData[i];
          maxGrp->createCheckBox(PID_EXTERNAL_LIST_START + i, rec.plug->getMenuCommandName(), *rec.isExternal ? 1 : 0);
          mPanel->setBool(PID_EXTERNAL_LIST_GRP, true);
        }
    }

    virtual bool onOk()
    {
      int i;
      for (i = 0; i < exportData.size(); ++i)
      {
        ExportPluginRec &rec = exportData[i];
        *rec.doExport = mPanel->getBool(i);
      }

      const int fltrCnt = filterPlugs.size();
      for (i = 0; i < fltrCnt; i++)
        *filterPlugs[i].useFilter = mPanel->getBool(PID_FILTER_LIST_START + i);

      if (mPanel->getById(PID_EXTERNAL_LIST_GRP))
      {
        for (i = 0; i < exportData.size(); ++i)
        {
          ExportPluginRec &rec = exportData[i];
          *rec.isExternal = mPanel->getBool(PID_EXTERNAL_LIST_START + i);
        }
      }

      IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, 0);
      if (mPanel->getById(PID_FILTER_LIST_GRP))
        for (i = 0; i < filterPlugs.size(); ++i)
          if (mPanel->getBool(PID_FILTER_LIST_START + i))
            filterPlugs[i].filter->queryInterface<IObjEntityFilter>()->applyFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT, true);

      return true;
    }

  private:
    enum
    {
      PID_EXTERNAL_LIST_GRP = 10000,
      PID_EXTERNAL_LIST_START,
      PID_FILTER_LIST_GRP = 20000,
      PID_FILTER_LIST_START,
    };

    Tab<ExportPluginRec> exportData;
    Tab<ExportFiltersPlgRec> filterPlugs;

    PropPanel::ContainerPropertyControl *mPanel;

    static int sortPluginsToExport(const ExportPluginRec *rec1, const ExportPluginRec *rec2)
    {
      return stricmp(rec1->plug->getMenuCommandName(), rec2->plug->getMenuCommandName());
    }
  };


  getSortedListForBuild(build_plugin, do_export, is_external);

  Tab<IGenEditorPlugin *> filter_plugin(tmpmem);
  Tab<bool *> use_filter(tmpmem);

  getFiltersList(filter_plugin, use_filter);

  ExportPanelClient exportPanelDlg(build_plugin, do_export, is_external, filter_plugin, use_filter);
  if (mNeedSuppress)
  {
    exportPanelDlg.onOk();
    return true;
  }
  return exportPanelDlg.showDialog() == PropPanel::DIALOG_ID_OK;

  // int height = build_plugin.size() * 30 + 70;
  // PropertyPanelDlg dlg(client, this, 240, height, "Plugins to export");

  // return dlg.execute() == CTL_IDOK;


  // PluginExportDialog plugDlg(mManager->getMainWindow());

  // int i;
  // for (i = 0; i < build_plugin.size(); ++i)
  //{
  //   if (plugin[i].p)
  //     plugDlg.addPlugin(build_plugin[i]->getMenuCommandName(), *do_export[i],
  //       *is_external[i], i);
  // }

  // if (plugDlg.showDialog() == p2dlg::MBS_OK)
  //{
  //   for ( i = 0; i < build_plugin.size(); i ++ )
  //   {
  //     *do_export[i] = plugDlg.getCheckState(i);
  //     *is_external[i] = plugDlg.getCheckStateExternal(i);
  //   }

  //  return true;
  //}
  // else
  //  return false;
  return true; // huck
}


static void endExport(Tab<IOnExportNotify *> &expNotify, CoolConsole &console, unsigned target_code)
{
  if (expNotify.size())
  {
    console.startProgress();
    console.setTotal(expNotify.size());
    console.setActionDesc("Notify export finished...");
    for (int i = 0; i < expNotify.size(); i++)
    {
      expNotify[i]->onAfterExport(target_code);
      console.incDone();
    }
    console.endProgress();
  }
  clear_and_shrink(expNotify);
}

static int asset_ri_type = -1;
static int asset_dm_type = -1;
static DagorAsset *resolve_texture_id(TEXTUREID tid, DagorAssetMgr &assetMgr)
{
  String tmp_stor, out_str(DagorAsset::fpath2asset(TextureMetaData::decodeFileName(get_managed_texture_name(tid), &tmp_stor)));
  if (out_str.length() > 0 && out_str[out_str.length() - 1] == '*')
    erase_items(out_str, out_str.length() - 1, 1);
  return assetMgr.findAsset(out_str, assetMgr.getTexAssetTypeId());
}
template <class RES, unsigned CLSID>
static inline void addUsedTextures(DagorAsset &a, OAHashNameMap<true> &resTexList, OAHashNameMap<true> *bakedImpTexList = nullptr)
{
  FastNameMap resNameMap;
  resNameMap.addNameId(a.getName());
  ::set_required_res_list_restriction(resNameMap);
  if (auto res = (RES *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(a.getName()), CLSID))
  {
    TextureIdSet tex_id_list;
    res->gatherUsedTex(tex_id_list);
    for (TEXTUREID tid : tex_id_list)
      if (DagorAsset *ta = resolve_texture_id(tid, a.getMgr()))
      {
        if (bakedImpTexList &&
            (trail_strcmp(ta->getName(), "_aa") || trail_strcmp(ta->getName(), "_as") || trail_strcmp(ta->getName(), "_nt")))
          bakedImpTexList->addNameId(ta->getName());
        else
          resTexList.addNameId(ta->getName());
      }
      else
        logwarn("failed to resolve tex asset: tid=0x%x, name=<%s> for asset <%s>", tid, get_managed_texture_name(tid), a.getName());
    release_game_resource((GameResource *)res);
  }
  else
    logerr("failed to get/build gameres <%s>", a.getName());
  ::reset_required_res_list_restriction();
}

static void add_tex_refs_recursive(DagorAsset &a, OAHashNameMap<true> &resList, OAHashNameMap<true> &resTexList,
  OAHashNameMap<true> &bakedImpTexList)
{
  int tex_asset_id = assetrefs::get_tex_type();
  if (a.getType() == tex_asset_id)
  {
    DAEDITOR3.conWarning("not expected referenced asset %s to be texture", a.getName());
    return;
  }

  resList.addNameId(a.getName());
  if (asset_ri_type >= 0 && asset_ri_type == a.getType())
    return addUsedTextures<RenderableInstanceLodsResource, RendInstGameResClassId>(a, resTexList, &bakedImpTexList);
  if (asset_ri_type >= 0 && asset_dm_type == a.getType())
    return addUsedTextures<DynamicRenderableSceneLodsResource, DynModelGameResClassId>(a, resTexList);

  IDagorAssetRefProvider *r = a.getMgr().getAssetRefProvider(a.getType());
  if (!r)
    return;
  SmallTab<IDagorAssetRefProvider::Ref, TmpmemAlloc> refs(r->getAssetRefs(a));
  if (!refs.size())
    return;

  for (int i = 0; i < refs.size(); i++)
  {
    if (!refs[i].getAsset())
      continue;
    if (refs[i].refAsset->getType() == tex_asset_id)
      resTexList.addNameId(refs[i].refAsset->getName());
    else if (refs[i].flags & r->RFLG_EXTERNAL)
      add_tex_refs_recursive(*refs[i].refAsset, resList, resTexList, bakedImpTexList);
  }
}

//==============================================================================

// batch export logging

const char *BATCH_LOG_PATH = "/.log/batch_log.txt";
BatchLogCB *bl_callback = NULL;

BatchLogCB::BatchLogCB(BatchLog *_bl) : bl(_bl) { G_ASSERT(bl); }

void BatchLogCB::write(const char *text) { bl->write(text); }

void BatchLog::open(const char *path)
{
  fp = fopen(path, "wtc");
  bl_callback = new BatchLogCB(this);
}

BatchLog::~BatchLog()
{
  del_it(bl_callback);
  if (fp)
    fclose(fp);
}

void BatchLog::write(const char *text)
{
  if (fp)
  {
    fputs(text, fp);
    fputs("\n", fp);
  }
}

namespace sgg
{
const char *get_start_path();
};

//-------------------------------------------------------------


static bool cmp_data_eq(mkbindump::BinDumpSaveCB &cwr, const char *pack_fname)
{
  static const int BUF_SZ = 16 << 10;
  char buf[BUF_SZ], buf2[BUF_SZ];
  file_ptr_t fp = df_open(pack_fname, DF_READ);

  if (!fp)
    return false;
  int sz = cwr.getSize();
  if (df_length(fp) != sz)
  {
    df_close(fp);
    return false;
  }

  MemoryLoadCB crd(cwr.getMem(), false);
  while (sz > 0)
  {
    int rdsz = __min(sz, BUF_SZ);
    df_read(fp, buf, rdsz);
    crd.read(buf2, rdsz);
    if (memcmp(buf, buf2, rdsz) != 0)
    {
      df_close(fp);
      return false;
    }
    sz -= rdsz;
  }
  df_close(fp);
  return true;
}

bool DagorEdAppWindow::gatherUsedResStats(dag::ConstSpan<IBinaryDataBuilder *> exporters, unsigned target_code,
  const OAHashNameMap<true> &levelResList, TextureRemapHelper &out_trh, int64_t &out_texSize, int &out_texCount,
  bool verbose_to_console)
{
  for (int i = 0; i < exporters.size(); i++)
  {
    if (!exporters[i]->addUsedTextures(out_trh))
      return false;
    console->incDone();
  }

  if (console->hasErrors())
    return false;

  out_texSize = out_trh.getTexturesSize(target_code);
  out_texCount = out_trh.getTexturesCount() + out_trh.getDDSxTexturesCount();

  DAEDITOR3.conNote("Level textures count is %d", out_texCount);
  DAEDITOR3.conNote("Approx. level textures size is %s", ::bytes_to_mb(out_texSize));

  // Gather and measure textures used by resources
  if (::get_gameres_sys_ver() == 2)
  {
    OAHashNameMap<true> resList, resTexList, bakedImpTexList;
    int64_t levelTexSize = out_texSize;
    int levelTexCount = out_texCount;

    asset_ri_type = DAEDITOR3.getAssetTypeId("rendInst");
    asset_dm_type = DAEDITOR3.getAssetTypeId("dynModel");
    iterate_names(levelResList, [&](int, const char *name) {
      DagorAsset *a = DAEDITOR3.getAssetByName(name, assetrefs::get_exported_types());
      if (!a)
        DAEDITOR3.conWarning("missing referenced asset %s", name);
      else
        add_tex_refs_recursive(*a, resList, resTexList, bakedImpTexList);
    });

    for (int i = 0; i < out_trh.getDDSxTexturesCount(); i++)
      if (verbose_to_console)
        DAEDITOR3.conNote("level DDSx %5d: +%5dK [%s]", i, out_trh.getDDSxTextureSize(i) >> 10, out_trh.getDDSxTextureName(i));
      else
        debug("level DDSx %d: %s -> sz=%d", i, out_trh.getDDSxTextureName(i), out_trh.getDDSxTextureSize(i));
    for (int i = 0; i < out_trh.getTexturesCount(); i++)
    {
      DagorAsset *a = DAEDITOR3.getAssetByName(out_trh.getTextureName(i), assetrefs::get_tex_type());
      int sz = get_tex_asset_data_size(a, target_code);
      if (verbose_to_console)
        DAEDITOR3.conNote("level tex %6d: +%5dK [%s]", i, sz >> 10, out_trh.getTextureName(i));
      else
        debug("level tex %d: %s -> sz=%d", i, out_trh.getTextureName(i), sz);
    }
    iterate_names(resTexList, [&](int id, const char *name) {
      if (out_trh.getTextureOrdinal(name) != -1)
        return;

      DagorAsset *a = DAEDITOR3.getAssetByName(name, assetrefs::get_tex_type());
      int sz = get_tex_asset_data_size(a, target_code);
      if (verbose_to_console)
        DAEDITOR3.conNote("resource tex %3d: +%5dK [%s]", id, sz >> 10, name);
      else
        debug("resource tex %d: %s -> sz=%d", id, name, sz);
      if (sz < 0)
        DAEDITOR3.conWarning("cannot get size of %s texture asset", name);
      else
      {
        out_texCount++;
        out_texSize += sz;
      }
    });

    int64_t bakedImpTexSize = 0;
    int bakedImpTexCount = 0;
    iterate_names(bakedImpTexList, [&](int id, const char *name) {
      if (out_trh.getTextureOrdinal(name) != -1)
        return;

      DagorAsset *a = DAEDITOR3.getAssetByName(name, assetrefs::get_tex_type());
      int sz = get_tex_asset_data_size(a, target_code);
      if (verbose_to_console)
        DAEDITOR3.conNote("bakedImp tex %3d: +%5dK [%s]", id, sz >> 10, name);
      else
        debug("bakedImp tex %d: %s -> sz=%d", id, name, sz);
      if (sz < 0)
        DAEDITOR3.conWarning("cannot get size of %s texture asset", name);
      else
      {
        bakedImpTexCount++;
        bakedImpTexSize += sz;
      }
    });

    if (verbose_to_console)
    {
      DAEDITOR3.conNote("Level textures count is %d", levelTexCount);
      DAEDITOR3.conNote("Approx. level textures size is %s", ::bytes_to_mb(levelTexSize));
    }
    DAEDITOR3.conNote("Full textures count is %d", out_texCount);
    DAEDITOR3.conNote("Approx. full textures size is %s", ::bytes_to_mb(out_texSize));

    if (bakedImpTexList.nameCount())
      DAEDITOR3.conNote("Baked impostors (%s in %d tex) are excluded from metrics", ::bytes_to_mb(bakedImpTexSize), bakedImpTexCount);
  }
  return true;
}
void DagorEdAppWindow::exportLevelToGame(int target_code)
{
  FastIntList sha1Tags;
  int sha1TagsPos = -1;
  static const int HASH_SZ = 20;

  DataBlock appBlk;
  appBlk.load(DAGORED2->getWorkspace().getAppPath());
  if (const DataBlock *b = appBlk.getBlockByNameEx("SDK")->getBlockByName("levelExpSHA1Tags"))
    for (int i = 0, nid = b->getNameId("tag"); i < b->paramCount(); i++)
      if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
      {
        const char *tagStr = b->getStr(i);
        unsigned tag = 0;
        while (*tagStr)
        {
          tag = (tag >> 8) | (*tagStr << 24);
          tagStr++;
        }
        sha1Tags.addInt(tag);
        debug("SHA11-tag: %c%c%c%c (%s)", _DUMP4C(tag), b->getStr(i));
      }

  struct AutoFullThrottle
  {
    bool old;
    bool prev_ddsx_fast_conv = false;
    AutoFullThrottle()
    {
      old = ::dgs_dont_use_cpu_in_background;
      ::dgs_dont_use_cpu_in_background = false;
      dagor_enable_idle_priority(false);
      prev_ddsx_fast_conv = texconvcache::set_fast_conv(false);
      debug("max CPU usage on");
      debug_flush(false);
    }
    ~AutoFullThrottle()
    {
      ::dgs_dont_use_cpu_in_background = old;
      dagor_enable_idle_priority(old);
      texconvcache::set_fast_conv(prev_ddsx_fast_conv);
      debug("max CPU usage off (%d)", old);
      debug_flush(false);
      load_exp_shaders_for_target(_MAKE4C('PC'));
    }
  } auto_full_throttle;

  load_exp_shaders_for_target(target_code);

  String *exportPath = NULL;
  for (int i = 0; i < exportPaths.size(); i++)
    if (exportPaths[i].platfId == target_code)
    {
      exportPath = &exportPaths[i].path;

      if (exportPath->empty())
      {
        String binFn(DAGORED2->getProjectFileName());
        remove_trailing_string(binFn, ".level.blk");
        binFn += ".bin";

        String defExpPath(wsp->getSdkDir());

        if (_MAKE4C('PC') == target_code)
          defExpPath += BASE_EXPORT_PATH_PC;
        if (_MAKE4C('PS4') == target_code)
          defExpPath += BASE_EXPORT_PATH_PS4;
        if (_MAKE4C('iOS') == target_code)
          defExpPath += BASE_EXPORT_PATH_iOS;
        if (_MAKE4C('and') == target_code)
          defExpPath += BASE_EXPORT_PATH_AND;

        dd_mkpath(defExpPath);
        exportPaths[i].path = defExpPath + binFn;
      }

      break;
    }

  if (!exportPath)
    return;

  class ExportPPClient : public PropPanel::DialogWindow // IPropertyPanelDlgClient
  {
  public:
    DynMap<IBinaryDataBuilder *, PropPanel::ContainerPropertyControl *> pluginParams;

    ExportPPClient(const Tab<IBinaryDataBuilder *> &plugins, const Tab<const char *> &names) :
      DialogWindow(NULL, _pxScaled(500), _pxScaled(550), "Export plugins parameters"),
      exporters(plugins),
      exportNames(names),
      pluginParams(tmpmem)
    {
      mPanel = getPanel();
      G_ASSERT(mPanel && "No panel in ExportPPClient");
      fillPanel();
    }

    void fillPanel()
    {
      String uid;

      for (int i = 0; i < exporters.size(); ++i)
      {
        if (exporters[i]->useExportParameters())
        {
          const char *caption = exportNames[i];
          uid.printf(64, "%sExportGrp", caption);

          PropPanel::ContainerPropertyControl *grp = mPanel->createGroup(i, caption);

          if (grp)
            exporters[i]->fillExportPanel(*grp);
        }
      }
    }

    bool onOk()
    {
      for (int i = 0; i < exporters.size(); ++i)
      {
        if (exporters[i]->useExportParameters())
        {
          PropPanel::ContainerPropertyControl *plugPanel = mPanel->getById(i)->getContainer();

          if (plugPanel)
            pluginParams.add(exporters[i], plugPanel);
        }
      }

      return true;
    }

    virtual bool doScrollPanel() const { return true; }

  private:
    const Tab<IBinaryDataBuilder *> &exporters;
    const Tab<const char *> &exportNames;

    PropPanel::ContainerPropertyControl *mPanel;
  };


  console->addMessage(ILogWriter::NOTE, "Export level to game...");
  console->startLog();

  Tab<IGenEditorPlugin *> buildPlugin(tmpmem);
  Tab<bool *> doExport(tmpmem);
  Tab<bool *> doExternal(tmpmem);

  if (!showPluginDlg(buildPlugin, doExport, doExternal))
    return;

  // TODO: use this array everywhere instead of buildPlugin
  Tab<IBinaryDataBuilder *> exporters(tmpmem);
  Tab<const char *> exportNames(tmpmem);

  int i;
  for (i = 0; i < buildPlugin.size(); ++i)
  {
    if (*doExport[i])
    {
      IBinaryDataBuilder *iface = buildPlugin[i]->queryInterface<IBinaryDataBuilder>();

      if (iface)
      {
        exporters.push_back(iface);
        exportNames.push_back(buildPlugin[i]->getMenuCommandName());
      }
    }
  }

  bool showExportDlg = false;

  for (int i = 0; i < exporters.size(); ++i)
    if (exporters[i]->useExportParameters())
    {
      showExportDlg = true;
      break;
    }

  ExportPPClient dialogPanelClient(exporters, exportNames);
  // PropertyPanelDlg dlg(dialogPanelClient, this, 300, 500, "Export plugins parameters");

  if (showExportDlg)
  {
    if (mNeedSuppress)
      dialogPanelClient.onOk();
    else if (dialogPanelClient.showDialog() != PropPanel::DIALOG_ID_OK)
      return;
  }

  String saveFname, savePath;
  ::split_path(exportPath->str(), savePath, saveFname);

  String fn;
  if (mNeedSuppress)
    fn = mSuppressDlgResult;
  else
    fn = wingw::file_save_dlg(mManager->getMainWindow(), "Select output binary dump filename", "BIN files|*.bin|All files|*.*", "bin",
      savePath, saveFname);

  if (!fn.empty())
  {
    dd_simplify_fname_c(fn.str());
    *exportPath = fn;
  }
  else
  {
    console->endProgress();
    console->endLog();
    return;
  }

  bool console_was_visible = console->isVisible();
  console->showConsole(true);
  BatchLog blogwr;
  String log_message;
  if (mNeedSuppress)
  {
    log_message = ::make_full_path(sgg::get_start_path(), BATCH_LOG_PATH);
    dd_mkpath(log_message);
    debug("Start batch log \"%s\"", log_message.str());
    blogwr.open(log_message.str());
    console->addMessage(ILogWriter::NOTE, "Start batch log \"%s\"", log_message.str());
    log_message = "";
  }

  wingw::set_busy(true);
  int exportStart = ::get_time_msec();
  // notify listeners about export begins
  Tab<IOnExportNotify *> expNotify(tmpmem);
  getInterfacesEx(expNotify);
  if (expNotify.size())
  {
    console->startProgress();
    console->setTotal(expNotify.size());
    console->setActionDesc("Prepare for export...");
    for (i = 0; i < expNotify.size(); i++)
    {
      expNotify[i]->onBeforeExport(target_code);
      console->incDone();
    }
    console->endProgress();
    console->addMessage(ILogWriter::NOTE, "prepared in %g seconds", (::get_time_msec() - exportStart) / 1000.0);
  }

  console->startProgress();
  console->setTotal(buildPlugin.size() + 1);
  console->setActionDesc("Validating data for build...");

  bool validate_ok = true;
  int validateStart = ::get_time_msec();
  for (i = 0; i < exporters.size(); ++i)
  {
    PropPanel::ContainerPropertyControl *params;
    if (!dialogPanelClient.pluginParams.get(exporters[i], params))
      params = NULL;

    if (!exporters[i]->validateBuild(target_code, *console, params))
    {
      log_message = String(512, "%s: pre-export validation failed", exportNames[i]);
      DAEDITOR3.conError(log_message.str());
      if (mNeedSuppress)
        blogwr.write(log_message.str());
      validate_ok = false;
    }
  }
  console->addMessage(ILogWriter::NOTE, "validated in %g seconds", (::get_time_msec() - validateStart) / 1000.0);

  console->incDone();

  if (!validate_ok || console->hasErrors())
  {
    console->endProgress();
    endExport(expNotify, *console, target_code);
    console->endLog();
    wingw::set_busy(false);

    if (!mNeedSuppress)
      wingw::message_box(wingw::MBS_EXCL, "Export to game error", "Errors during level validation.\nSee console for details.");

    console->showConsole(true);
    return;
  }

  // gather textures used in all components of level
  validateStart = ::get_time_msec();
  console->setActionDesc("Gathering textures...");

  // Gather resource list used in level
  OAHashNameMap<true> levelResList;
  for (i = 0; i < buildPlugin.size(); i++)
    if (*doExport[i])
      if (ILevelResListBuilder *iface = buildPlugin[i]->queryInterface<ILevelResListBuilder>())
        iface->addUsedResNames(levelResList);

  TextureRemapHelper trh(target_code);
  int64_t texSize = 0;
  int texCount = 0;
  if (!gatherUsedResStats(exporters, target_code, levelResList, trh, texSize, texCount))
  {
    console->endProgress();
    endExport(expNotify, *console, target_code);
    console->endLog();
    wingw::set_busy(false);

    if (!mNeedSuppress)
      wingw::message_box(wingw::MBS_EXCL, "Export to game error", "Errors during level validation.\nSee console for details.");

    console->showConsole(true);
    return;
  }

  DAEDITOR3.conNote("Gathered tex/res lists for %g seconds", (::get_time_msec() - validateStart) / 1000.0);

  DataBlock metrixBlk;
  bool metrixFound = wsp->getMetricsBlk(metrixBlk);
  const DataBlock *levelMetricsBlk = NULL;

  static SimpleString lastExportPathWithFailedMetrics;
  bool second_export = stricmp(*exportPath, lastExportPathWithFailedMetrics) == 0;

  if (metrixFound)
  {
    bool metrixFailed = false;
    log_message = "";

    console->setActionDesc("Level metrics validation...");
    console->setTotal(buildPlugin.size());
    console->setDone(0);

    levelMetricsBlk = metrixBlk.getBlockByName("whole_level");

    if (levelMetricsBlk)
    {
      const int maxTexCount = levelMetricsBlk->getInt("textures_count", 0);

      if (texCount > maxTexCount)
      {
        metrixFailed = true;

        log_message = String(512,
          "Metrics validation failed: textures count %i more than "
          "maximum allowed %i",
          texCount, maxTexCount);
        console->addMessage(ILogWriter::FATAL, log_message);
        if (second_export)
          metrixFailed = false;
      }
      else
      {
        const int64_t maxTexSize = levelMetricsBlk->getReal("textures_size", 0) * int64_t(1 << 20);

        if (texSize > maxTexSize)
        {
          metrixFailed = true;

          log_message = String(512,
            "Metrics validation failed: textures size %s (%i bytes) more "
            "than maximum %s (%i bytes)",
            (const char *)::bytes_to_mb(texSize), texSize, (const char *)::bytes_to_mb(maxTexSize), maxTexSize);
          console->addMessage(ILogWriter::FATAL, log_message);
          if (second_export)
            metrixFailed = false;
        }
      }
    }
    else
      console->addMessage(ILogWriter::WARNING, "Whole level metrics not found in \"%s\" file.", (const char *)wsp->getAppPath());

    if (!metrixFailed)
    {
      for (i = 0; i < buildPlugin.size(); i++)
      {
        console->incDone();

        if (*doExport[i])
        {
          const char *plugName = buildPlugin[i]->getMenuCommandName();
          console->addMessage(ILogWriter::NOTE, "%s plugin...", plugName);
          log_message = String(512, "Metrix for plugin %s", plugName);

          IBinaryDataBuilder *iface = buildPlugin[i]->queryInterface<IBinaryDataBuilder>();

          if (iface)
          {
            DataBlock *metrBlk = NULL;
            const int nid = metrixBlk.getNameId("plugin");

            for (int pi = 0; pi < metrixBlk.blockCount(); ++pi)
            {
              metrBlk = metrixBlk.getBlock(pi);
              if (metrBlk && metrBlk->getBlockNameId() == nid)
              {
                const char *name = metrBlk->getStr("name", NULL);

                if (name && !stricmp(name, plugName))
                  break;
              }

              metrBlk = NULL;
            }

            if (metrBlk)
              metrixFailed = !iface->checkMetrics(*metrBlk);
            else
              console->addMessage(ILogWriter::REMARK, "No %s plugin metrics", plugName);

            if (second_export)
              metrixFailed = false;
            if (metrixFailed)
              break;
          }
        }
      }
    }

    if (metrixFailed)
    {
      lastExportPathWithFailedMetrics = *exportPath;

      console->endProgress();
      endExport(expNotify, *console, target_code);
      console->endLog();
      wingw::set_busy(false);

      if (!mNeedSuppress)
        wingw::message_box(wingw::MBS_EXCL, "Export to game error", "Errors during level validation.\nSee console for details.");
      else
        blogwr.write(log_message.str());

      console->showConsole(true);
      return;
    }
  }
  else
  {
    console->addMessage(ILogWriter::WARNING, "No metrics found in \"%s\" file. Level metrics validation skiped.", wsp->getAppPath());
  }
  lastExportPathWithFailedMetrics = NULL;

  // write file label
  bool write_be = dagor_target_code_be(target_code);

  mkbindump::BinDumpSaveCB cwr(384 << 20, target_code, write_be);

  String binFName(*exportPath);
  String binFNameWithoutExt;
  if (const char *ext = dd_get_fname_ext(binFName))
    binFNameWithoutExt.printf(0, "%.*s", ext - dd_get_fname(binFName), dd_get_fname(binFName));
  else
    binFNameWithoutExt = dd_get_fname(binFName);

  console->setDone(0);
  console->setTotal(buildPlugin.size() + 1);

  DAGOR_TRY
  {
    cwr.writeFourCC(_MAKE4C('DBLD')); // Dagor Binary Level Data
    cwr.writeFourCC(DBLD_Version);    // version

    cwr.writeInt32e(trh.getTexturesCount()); // number of used textures

    // Dagor Editor's version information
    cwr.beginTaggedBlock(_MAKE4C('eVER'));
    cwr.writeDwString(IDagorEd2Engine::getBuildVersion());
    sha1TagsPos = cwr.tell();
    cwr.writeZeroes((sha1Tags.size() + 1) * (HASH_SZ + 4));
    cwr.align8();
    cwr.endBlock();

    // write required resource list to binary
    {
      cwr.beginTaggedBlock(_MAKE4C('RqRL'));
      mkbindump::StrCollector all_names;
      mkbindump::RoNameMapBuilder b;

      iterate_names(levelResList, [&](int, const char *name) { all_names.addString(name); });

      b.prepare(all_names);

      cwr.setOrigin();
      b.writeHdr(cwr);
      all_names.writeStrings(cwr);
      b.writeMap(cwr);
      cwr.popOrigin();

      cwr.align8();
      cwr.endBlock();
    }

    // write textures (each in own block)
    console->setActionDesc("Writing textures...");

    if (!trh.saveTextures(cwr, binFNameWithoutExt, *console))
    {
      dd_erase(binFName);
      console->endProgress();
      endExport(expNotify, *console, target_code);
      wingw::set_busy(false);
      log_message = "Can't convert one or more textures";
      if (!mNeedSuppress)
        wingw::message_box(wingw::MBS_EXCL, "Export to Game", log_message.str());
      else
        blogwr.write(log_message.str());
      console->endLog();
      console->showConsole(true);
      return;
    }

    cwr.beginTaggedBlock(_MAKE4C('TEX.'));
    cwr.endBlock();
    console->incDone();

    // write data (scenes, envi, and other plugin's data)
    for (i = 0; i < buildPlugin.size(); i++)
    {
      if (!*doExport[i])
        continue;

      console->setActionDesc(String(1024, "Building data: %s", buildPlugin[i]->getMenuCommandName()));

      const int exportPluginStart = get_time_msec();
      const int startLen = cwr.tell();

      IBinaryDataBuilder *iface = buildPlugin[i]->queryInterface<IBinaryDataBuilder>();

      if (iface)
      {
        if (*doExternal[i])
        {
          String includeSuffic(512, "_%s.bin", buildPlugin[i]->getInternalName());

          String includeName = binFName + includeSuffic;
          mkbindump::BinDumpSaveCB cwrInclude(128 << 20, target_code, write_be);

          PropPanel::ContainerPropertyControl *params;
          if (!dialogPanelClient.pluginParams.get(iface, params))
            params = NULL;

          dd_erase(includeName);
          if (!iface->buildAndWrite(cwrInclude, trh, params))
          {
            dd_erase(binFName);
            log_message = String(512, "export error in plugin: %s", buildPlugin[i]->getMenuCommandName());
            console->addMessage(console->ERROR, log_message.str());
            console->endProgress();
            endExport(expNotify, *console, target_code);
            wingw::set_busy(false);
            if (!mNeedSuppress)
              wingw::message_box(wingw::MBS_EXCL, "Export to Game", "Some plugins could not perform operation");
            else
              blogwr.write(log_message.str());

            clear_and_shrink(doExport);
            clear_and_shrink(doExternal);

            console->endLog();
            console->showConsole(true);
            return;
          }
          else
          {
            cwrInclude.beginTaggedBlock(_MAKE4C('END'));
            cwrInclude.endBlock();
            cwr.beginTaggedBlock(_MAKE4C('INC'));
            cwr.writeDwString(includeSuffic);
            cwr.align8();
            cwr.endBlock();
          }

          dd_mkpath(includeName);
          FullFileSaveCB fcwrInclude(includeName);
          cwrInclude.copyDataTo(fcwrInclude);
        }
        else
        {

          PropPanel::ContainerPropertyControl *params;
          if (!dialogPanelClient.pluginParams.get(iface, params))
            params = NULL;

          if (!iface->buildAndWrite(cwr, trh, params))
          {
            dd_erase(binFName);
            log_message = String(512, "export error in plugin: %s", buildPlugin[i]->getMenuCommandName());
            console->addMessage(console->ERROR, log_message.str());
            console->endProgress();
            endExport(expNotify, *console, target_code);
            wingw::set_busy(false);
            if (!mNeedSuppress)
              wingw::message_box(wingw::MBS_EXCL, "Export to Game", "Some plugins could not perform operation");
            else
              blogwr.write(log_message.str());
            clear_and_shrink(doExport);

            console->endLog();
            console->showConsole(true);
            return;
          }
        }
      }

      const int endLen = cwr.tell();
      const int delta = endLen - startLen;

      float secs = (get_time_msec() - exportPluginStart) / 1000.0f;
      console->addMessage(ILogWriter::NOTE, "Written %s (%i bytes) in %g seconds", (const char *)::bytes_to_mb(delta), delta, secs);
      console->incDone();
    }

    // finish level dump
    cwr.beginTaggedBlock(_MAKE4C('END'));
    cwr.endBlock();
  }
  DAGOR_CATCH(const IGenSave::SaveException &)
  {
    dd_erase(binFName);
    console->endProgress();
    endExport(expNotify, *console, target_code);
    console->endLog();

    log_message = String(512, "Couldn't write file \"%s\"", (const char *)binFName);
    console->addMessage(ILogWriter::FATAL, log_message);
    wingw::set_busy(false);
    if (!mNeedSuppress)
      wingw::message_box(wingw::MBS_EXCL, "Export error", "Couldn't write file \"%s\"", (const char *)binFName);
    else
      blogwr.write(log_message.str());

    return;
  }

  console->endProgress();
  console->endLog();

  char hash[32];
  IGenSave *hcb = create_hash_computer_cb(HASH_SAVECB_SHA1);
  cwr.copyDataTo(*hcb);
  if (get_computed_hash(hcb, hash, sizeof(hash)) == HASH_SZ)
  {
    cwr.seekto(sha1TagsPos + 4);
    cwr.writeRaw(hash, HASH_SZ);
  }
  if (sha1Tags.size())
  {
    MemoryLoadCB mcrd(cwr.getMem(), false);
    BinDumpReader crd(&mcrd, target_code, write_be);
    int sha1_written = 1;

    crd.readFourCC(); // _MAKE4C('DBLD')
    crd.readFourCC(); // DBLD_Version
    crd.readInt32e(); // total tex number
    for (;;)
    {
      int tag = crd.beginTaggedBlock();

      if (tag == _MAKE4C('END'))
      {
        // valid end of binary dump
        crd.endBlock();
        break;
      }
      else if (sha1Tags.hasInt(tag) && sha1_written <= sha1Tags.size())
      {
        copy_stream_to_stream(crd.getRawReader(), *hcb, crd.getBlockRest());
        if (get_computed_hash(hcb, hash, sizeof(hash)) == HASH_SZ)
        {
          cwr.seekto(sha1TagsPos + sha1_written * (4 + HASH_SZ));
          cwr.writeFourCC(tag);
          cwr.writeRaw(hash, HASH_SZ);
          sha1_written++;
        }
      }
      crd.endBlock();
    }
    cwr.seekto(sha1TagsPos);
    cwr.writeInt32e(sha1_written - 1);
  }
  destory_hash_computer_cb(hcb);

  if (cmp_data_eq(cwr, binFName))
    console->addMessage(ILogWriter::NOTE, "Export done; binary remains the same");
  else
  {
    dd_mkpath(binFName);
    FullFileSaveCB fcwr(binFName);
    cwr.copyDataTo(fcwr);
    fcwr.close();
  }

  if (metrixFound && levelMetricsBlk)
  {
    const int maxFileSize = levelMetricsBlk->getReal("level_size", 0) * 1024 * 1024;
    const int levelSize = cwr.tell();

    if (maxFileSize < levelSize)
    {
      endExport(expNotify, *console, target_code);
      log_message = String(512,
        "Metrics validation failed: level size %s (%i bytes) more than "
        "maximum %s (%i bytes)",
        (const char *)::bytes_to_mb(levelSize), levelSize, (const char *)::bytes_to_mb(maxFileSize), maxFileSize);
      console->addMessage(ILogWriter::FATAL, log_message);

      wingw::set_busy(false);
      if (!mNeedSuppress)
        wingw::message_box(wingw::MBS_EXCL, "Export to game error", "Errors during level metrics check.\nSee console for details.");
      else
        blogwr.write(log_message.str());

      console->showConsole(true);
      return;
    }
  }

  endExport(expNotify, *console, target_code);
  const int sec = (get_time_msec() - exportStart) / 1000;
  console->addMessage(ILogWriter::NOTE, "Export time is %i minutes %i seconds; dest: %s", sec / 60, sec % 60, binFName);

  wingw::set_busy(false);
  log_message = String(512, "Dagor Binary Level Dump was written to\n\n%s", binFName.str());
  if (!console_was_visible)
    console->hideConsole();
  if (!mNeedSuppress)
    wingw::message_box(wingw::MBS_INFO, "Export to game", log_message.str());
  else
    blogwr.write(log_message.str());
}


//==============================================================================
void DagorEdAppWindow::exportLevelToDag(bool visual)
{
  ExportToDagDlg dlg(toDagPlugNames, visual, visual ? "Plugins for visual geom" : "Plugins for collision geom");

  if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    Tab<int> selPlugs(tmpmem);
    dlg.getSelPlugins(selPlugs);

    toDagPlugNames.clear();

    if (selPlugs.empty())
      return;

    String location(de_get_sdk_dir());
    if (location == "")
      location = sgg::get_exe_path_full();
    const String fname = wingw::file_save_dlg(NULL, "Save DAG file", "DAG-files (*.dag)|*.dag|All files (*.*)|*.*", "dag", location);

    if (!fname.length())
      return;

    StaticGeometryContainer geoCont;

    int i;
    int mask = visual ? IObjEntityFilter::STMASK_TYPE_RENDER : IObjEntityFilter::STMASK_TYPE_COLLISION;
    int prevSubtype = IObjEntityFilter::getSubTypeMask(mask);

    IObjEntityFilter::setSubTypeMask(mask, 0);
    for (i = 0; i < selPlugs.size(); ++i)
    {
      IGenEditorPlugin *plug = DAGORED2->getPlugin(selPlugs[i]);
      if (plug->queryInterface<IGatherStaticGeometry>())
        continue;

      IObjEntityFilter *filter = plug->queryInterface<IObjEntityFilter>();
      if (filter && filter->allowFiltering(mask))
        filter->applyFiltering(mask, true);
    }

    for (i = 0; i < selPlugs.size(); ++i)
    {
      IGenEditorPlugin *plug = DAGORED2->getPlugin(selPlugs[i]);

      toDagPlugNames.push_back() = plug->getMenuCommandName();

      IGatherStaticGeometry *statGeom = plug->queryInterface<IGatherStaticGeometry>();
      if (!statGeom)
        continue;
      if (visual)
      {
        if (IExportToDag *exportToDag = plug->queryInterface<IExportToDag>())
          exportToDag->gatherExportToDagGeometry(geoCont);
        statGeom->gatherStaticVisualGeometry(geoCont);
      }
      else
        statGeom->gatherStaticCollisionGeomGame(geoCont);
    }
    IObjEntityFilter::setSubTypeMask(mask, prevSubtype);

    const unsigned projHash = ::get_hash(sceneFname, 0);

    for (i = 0; i < geoCont.nodes.size(); ++i)
    {
      StaticGeometryNode *node = geoCont.nodes[i];

      if (!node)
        continue;

      node->name = String(256, "%s_%08X", (const char *)node->name, projHash);
    }

    if (visual)
      geoCont.applyNodesLighting();
    geoCont.exportDag(fname);
  }
}


bool get_scene_vertexes_faces_count(const char *path, int &faces_cnt)
{
  class TestScene : public RenderScene
  {
  public:
    void getVertexCount(int &faces_cnt) const
    {
      faces_cnt = 0;

      for (int oi = 0; oi < obj.size(); ++oi)
      {
        const RenderObject &object = obj[oi];

        for (int li = 0; li < object.getLods().size(); ++li)
        {
          const ShaderMesh *shMesh = object.getLods().at(li).mesh;
          if (shMesh)
            faces_cnt += shMesh->calcTotalFaces();
        }
      }
    }
  };

  TestScene scene;

  DAEDITOR3.setFatalHandler(true);
  scene.load(path, false);
  bool fs = DAEDITOR3.getFatalStatus();
  DAEDITOR3.popFatalHandler();

  if (fs)
    return false;

  scene.getVertexCount(faces_cnt);
  return true;
}


bool check_scene_vertex_count(const char *path, const char *lm_dir, int max_faces)
{
  CoolConsole &con = DAGORED2->getConsole();

  int totalFaces = 0;

  if (::dd_file_exist(path))
  {
    if (::get_scene_vertexes_faces_count(lm_dir, totalFaces))
    {
      if (totalFaces > max_faces)
      {
        con.addMessage(ILogWriter::FATAL, "Metrics validation failed: scene faces count (%i) more than maximum allowed (%i)",
          totalFaces, max_faces);

        return false;
      }
    }
    else
    {
      con.addMessage(ILogWriter::FATAL, "Metrics validation failed: errors while loading scene file \"%s\"", path);

      return false;
    }
  }
  else
    con.addMessage(ILogWriter::WARNING, "Scene file \"%s\" non exists, metrics check skiped.", path);

  return true;
}
