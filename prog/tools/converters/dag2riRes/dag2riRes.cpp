#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <libTools/dtx/dtx.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <libTools/util/fileUtils.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/dagFileRW/textureNameResolver.h>


struct ObjectData
{
  ObjectData() : isOrigin(false) {}

public:
  TMatrix tm;
  SimpleString name;
  bool isOrigin;
};

extern bool skip_tex_convert;
extern bool ignore_origin;
extern bool clear_nodeprops;
extern bool export_all, export_node_001;

static String res_base;
static DataBlock *rdb_folders_blk = NULL, *rdb_objects_blk = NULL, *rdb_tex_blk = NULL;
static DataBlock *texref_blk = NULL, *texslot_blk = NULL;
static DataBlock *landClassBlk = NULL;
static bool missing_added = false;
static FastNameMapEx texres_names;
static OAHashNameMap<true> fileNm;
static Point2 boxX(999999, 0), boxY(999999, 0);
static Tab<ObjectData> compObj(midmem);


static const unsigned __missing_tex_dds[38] = {
  0x20534444,
  0x0000007C,
  0x000A1007,
  0x00000004,
  0x00000004,
  0x00000008,
  0x00000000,
  0x00000003,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000020,
  0x00000004,
  0x31545844,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00401008,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0xAEDDAEBC,
  0xAAAAAAAA,
  0xAEDDAEBC,
  0xAEAEAAAA,
  0xAEDDAEBC,
  0xABABABAA,
};


static inline void addComposit(const char *name, const TMatrix &tm, const char *orig_name)
{
  append_items(compObj, 1);
  compObj.back().tm = tm;
  compObj.back().name = name;
  compObj.back().isOrigin = trail_stricmp(orig_name, "_origin");
}

static inline void generateComposit(DataBlock &compositBlk)
{
  Point3 c0((boxX.x + boxX.y) / 2.0, 0, (boxY.x + boxY.y) / 2.0);

  const int cnt = compObj.size();
  for (int i = 0; i < cnt; i++)
    if (!ignore_origin && compObj[i].isOrigin)
    {
      c0 = compObj[i].tm.getcol(3);
      break;
    }
  for (int i = 0; i < cnt; i++)
  {
    DataBlock *nodeBlk = compositBlk.addNewBlock("node");
    nodeBlk->setStr("name", String(0, "%s:%s", compObj[i].name, "rendInst"));

    TMatrix &tm = compObj[i].tm;
    Point3 pos(tm.getcol(3));
    pos -= c0;
    tm.setcol(3, pos);
    nodeBlk->setTm("tm", tm);
  }
}

static inline void addLandObject(const char *name, const TMatrix &tm)
{
  if (!landClassBlk)
    return;

  DataBlock *objectBlk = landClassBlk->addNewBlock("object");

  objectBlk->setStr("name", name);
  objectBlk->setTm("matrix", tm);
}

bool optimizeDag(const char *base_path, const char *file)
{
  class NoChangeResolver : public ITextureNameResolver
  {
  public:
    virtual bool resolveTextureName(const char *src_name, String &out_str) { return false; }
  } resolveCb;

  static char fileNameBuf[260];
  _snprintf(fileNameBuf, 260, "%s/p_RendInst/%s.lod00.dag", base_path, file);
  StaticGeometryContainer sgc;

  set_global_tex_name_resolver(&resolveCb);
  bool ret = sgc.loadFromDag(fileNameBuf, NULL);
  set_global_tex_name_resolver(NULL);
  if (!ret)
    return false;

  sgc.optimize();

  static OAHashNameMap<true> localNm;
  localNm.reset();

  for (int i = sgc.nodes.size() - 1; i >= 0; i--)
  {
    StaticGeometryNode *node = sgc.nodes[i];
    if (!node->mesh)
      continue;
    PtrTab<StaticGeometryMaterial> &mats = node->mesh->mats;

    for (int j = mats.size() - 1; j >= 0; j--)
    {
      if (!mats[j])
        continue;

      for (int l = 0; l < StaticGeometryMaterial::NUM_TEXTURES; l++)
      {
        if (!mats[j]->textures[l])
          continue;
        const char *texFile = mats[j]->textures[l]->fileName.str();
        if (texFile)
          localNm.addNameId(texFile);
      }
    }
  }

  unlink(fileNameBuf);
  sgc.exportDag(fileNameBuf, false);

  _snprintf(fileNameBuf, 260, "%s/p_RendInst/%s.rendinst.blk", base_path, file);

  DataBlock resBlk;
  resBlk.load(fileNameBuf);
  resBlk.removeBlock("textures");
  DataBlock *texBlk = resBlk.addBlock("textures");
  for (int i = localNm.nameCount() - 1; i >= 0; i--)
    texBlk->addStr("tex", localNm.getName(i));

  resBlk.saveToTextFile(fileNameBuf);

  resBlk.reset();
  _snprintf(fileNameBuf, 260, "%s/resdb/%s.rdbres.blk", base_path, file);
  resBlk.load(fileNameBuf);
  if (stricmp(resBlk.getStr("resName", ""), file) != 0)
    DAG_FATAL("not compare resource name: '%s' '%s'", resBlk.getStr("resName", ""), file);

  const int refId = resBlk.getNameId("ref");
  for (int i = resBlk.blockCount() - 1; i >= 0; i--)
  {
    DataBlock *refBlk = resBlk.getBlock(i);
    if (refBlk->getBlockNameId() != refId)
      continue;

    if (localNm.getNameId(refBlk->getStr("slot", "")) == -1)
      resBlk.removeBlock(i);
  }

  resBlk.saveToTextFile(fileNameBuf);

  return true;
}

static bool convertTexture(const char *src_fn, const char *dest_fn, bool fast_tex_cvt)
{
  if (skip_tex_convert)
    return true;
  if (fast_tex_cvt && dd_file_exist(src_fn) && dd_file_exist(dest_fn))
    return true;

  bool dds_ext = stricmp(dd_get_fname_ext(src_fn), ".dds") == 0;
  if (dds_ext && dd_file_exist(src_fn))
    return dag_copy_file(src_fn, dest_fn, true);

  bool alpha_used = false;
  TexImage32 *img;
  if (!dds_ext)
    img = ::load_image(src_fn, tmpmem, &alpha_used);
  else
    img = ::load_image(String::mk_sub_str(src_fn, dd_get_fname_ext(src_fn)), tmpmem, &alpha_used);
  if (!img)
    return false;

  ddstexture::Converter cnv;
  cnv.format = ddstexture::Converter::fmtARGB;
  cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
  cnv.mipmapCount = ddstexture::Converter::AllMipMaps;

  FullFileSaveCB cwr(dest_fn);
  if (!cwr.fileHandle)
    return false;
  return cnv.convertImage(cwr, (TexPixel32 *)(img + 1), img->w, img->h, img->w * 4, true, 0);
}

static void addObject(Node &n, const char *name, const DataBlock &obj_blk)
{
  class ParamEnum : public Node::NodeEnumCB
  {
  public:
    virtual int node(Node &c)
    {
      c.script = NULL;
      return 0;
    }
  } cb;

  if (clear_nodeprops)
    n.enum_nodes(cb);

  Node *parent = n.parent;
  n.parent = NULL;
  n.tm.identity();
  n.tm.setcol(1, Point3(0, 0, 1));
  n.tm.setcol(2, Point3(0, 1, 0));
  n.invalidate_wtm();
  n.calc_wtm();

  DagExporter::exportGeometry(String(260, "%s/p_RendInst/%s.lod00.dag", res_base.str(), name), &n, true, false);
}

static int processName(const char *name)
{
  const int len = strlen(name);
  if (export_all)
    return len;
  for (int i = len - 1; i > 0; i--)
    if (name[i] == '_')
      return i;

  return len;
}

static void processNode(Node &n)
{
  if (!(n.flags & NODEFLG_RENDERABLE))
    return;
  {
    DataBlock blk;

    if (dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST))
      if (!blk.getBool("renderable", true))
        return;
  }
  if (export_node_001 && !String(n.name).suffix("_001"))
    return;
  DataBlock nb;
  char name[260];
  _snprintf(name, 260, "%.*s", processName(n.name), n.name.str());

  TMatrix tm = n.wtm;
  TMatrix itm_swapyz;
  itm_swapyz.identity();
  itm_swapyz.setcol(1, Point3(0, 0, 1));
  itm_swapyz.setcol(2, Point3(0, 1, 0));
  tm = n.wtm * itm_swapyz;

  Point3 sp = tm.getcol(3);
  if (sp.x < boxX.x)
    boxX.x = sp.x;
  if (sp.x > boxX.y)
    boxX.y = sp.x;

  if (sp.z < boxY.x)
    boxY.x = sp.z;
  if (sp.z > boxY.y)
    boxY.y = sp.z;

  addComposit(name, tm, n.name);
  addLandObject(name, tm);
  if (fileNm.getNameId(name) != -1)
    return;

  fileNm.addNameId(name);

  if (clear_nodeprops)
    n.script = NULL;
  addObject(n, name, nb);

  DataBlock resblk;
  resblk.addNewBlock("lod")->addReal("range", 1000);
  resblk.addNewBlock(texslot_blk, "textures");
  resblk.saveToTextFile(String(260, "%s/p_RendInst/%s.rendinst.blk", res_base.str(), name));

  resblk.setFrom(texref_blk);
  resblk.setInt("version", 1);
  resblk.setStr("className", "RendInst");
  resblk.setStr("resName", name);
  resblk.saveToTextFile(String(260, "%s/resdb/%s.rdbres.blk", res_base.str(), name));

  rdb_objects_blk->addStr("res", name);
  return;
}

static int getFilePath(const char *file)
{
  for (int i = strlen(file) - 1; i > 0; i--)
    if ((file[i] == '\\') || (file[i] == '/'))
      return i;
  return 0;
}

static bool processDag(const char *fn, const char *base_path, bool fast_tex_cvt)
{
  AScene scene;
  const char *texPath = base_path;

  load_ascene(fn, scene, LASF_MATNAMES);
  if (!scene.root)
    return false;
  scene.root->calc_wtm();

  texref_blk = new DataBlock;
  texslot_blk = new DataBlock;

  // process textures
  class TexReplace : public Node::NodeEnumCB
  {
  public:
    Tab<MaterialData *> mat;

    TexReplace() : mat(tmpmem) {}

    virtual int node(Node &c)
    {
      if (c.mat)
        for (int i = 0; i < c.mat->list.size(); i++)
          addMat(c.mat->list[i].get());
      return 0;
    }

    void addMat(MaterialData *m)
    {
      for (int i = 0; i < mat.size(); i++)
        if (mat[i] == m)
          return;
      mat.push_back(m);
    }
  } cb;
  scene.root->enum_nodes(cb);

  scene.root->invalidate_wtm();
  scene.root->calc_wtm();

  FastNameMap origTex;
  Tab<String> slotTex(tmpmem);
  String tmp;
  String resStr;

  for (int i = 0; i < cb.mat.size(); i++)
  {
    // cb.mat[i]->className = "dynamic_simple";

    /*
        if (cb.mat[i]->mtex[0] == BAD_TEXTUREID)
        {
          printf("missing texture in material %d: %s\n", i, cb.mat[i]->matName.str());
          cb.mat[i]->mtex[0] = add_managed_texture("missing_tex");
        }
    */

    for (int j = 0; j < MAXMATTEXNUM; j++)
      if (cb.mat[i]->mtex[j] != BAD_TEXTUREID)
      {
        const char *texNm = dd_get_fname(get_managed_texture_name(cb.mat[i]->mtex[j]));
        char texname[260];
        if (texNm && dd_file_exist(texNm))
          _snprintf(texname, 260, "%s", texNm);
        else
          _snprintf(texname, 260, "%s%s", texPath, texNm);

        int id = origTex.addNameId(texname);
        if (id >= slotTex.size())
        {
          tmp = dd_get_fname(texname);
          const char *ext = dd_get_fname_ext(tmp);
          if (ext)
            erase_items(tmp, ext - tmp.data(), strlen(ext));
          slotTex.push_back(tmp);
          printf("'%s' -> '%s'\n", texname, slotTex[id].str());
          G_ASSERT(slotTex.size() == origTex.nameCount());

          DataBlock *rb = texref_blk->addNewBlock("ref");
          texslot_blk->addStr("tex", tmp);
          rb->setStr("slot", tmp);

          if (convertTexture(texname, String(260, "%s/p_Texture/%s_tex.dds", res_base.str(), tmp.str()), fast_tex_cvt))
            resStr.printf(260, "%s_tex", tmp.str());
          else
          {
            printf("can not convert: '%s'\n", texname);

            FullFileSaveCB cwr(String(260, "%s/p_Texture/missing_tex.dds", res_base.str()));
            if (!cwr.fileHandle)
              DAG_FATAL("%s/p_Texture/missing_tex.dds", res_base.str());
            cwr.write(__missing_tex_dds, sizeof(__missing_tex_dds));

            resStr = "missing_tex";
            missing_added = true;
          }

          rb->setStr("ref", resStr);
          if (texres_names.getNameId(resStr) == -1)
          {
            rdb_tex_blk->addStr("res", resStr);

            DataBlock rblk;
            rblk.setInt("version", 1);
            rblk.setStr("className", "Texture");
            rblk.setStr("resName", resStr);
            rblk.saveToTextFile(String(260, "%s/resdb/%s.rdbres.blk", res_base.str(), resStr.str()));
            texres_names.addNameId(resStr);
          }
        }

        cb.mat[i]->mtex[j] = add_managed_texture(slotTex[id]);
      }
  }

  // process nodes
  for (int i = 0; i < scene.root->child.size(); i++)
    processNode(*scene.root->child[i]);

  del_it(texref_blk);
  del_it(texslot_blk);
  return true;
}

bool generate_res_db(const char *src_dag_file, const char *dst_dir, const char *composit_file, const char *land_class_file,
  bool fast_tex_cvt)
{
  char base_path[260];
  dd_get_fname_location(base_path, src_dag_file);
  dd_simplify_fname_c(base_path);
  dd_append_slash_c(base_path);

  rdb_folders_blk = new DataBlock;

  rdb_objects_blk = rdb_folders_blk->addNewBlock("folder");
  rdb_objects_blk->setStr("name", "objects");
  rdb_objects_blk->setBool("isExported", true);
  rdb_objects_blk->setBool("isPacked", true);

  rdb_tex_blk = rdb_folders_blk->addNewBlock("folder");
  rdb_tex_blk->setStr("name", "tex");
  rdb_tex_blk->setBool("isExported", true);
  rdb_tex_blk->setBool("isPacked", true);

  res_base = dst_dir;

  dd_mkdir(res_base);
  dd_mkdir(res_base + "/p_Texture");
  dd_mkdir(res_base + "/p_RendInst");
  dd_mkdir(res_base + "/resdb");

  texres_names.reset();

  DataBlock landClBlk;
  landClBlk.setStr("className", "landClass");

  landClBlk.addBlock("colliders")->addStr("plugin", "Heightmap");
  landClBlk.addBool("usePosY", true);

  DataBlock *objGenBlk = landClBlk.addNewBlock("resources");
  landClassBlk = objGenBlk;

  processDag(src_dag_file, base_path, fast_tex_cvt);

  landClBlk.setPoint2("landBBoxLim0", Point2(boxX.x, boxY.x));
  landClBlk.setPoint2("landBBoxLim1", Point2(boxX.y, boxY.y));

  landClBlk.saveToTextFile(land_class_file);

  // composit
  DataBlock compBlk;
  compBlk.setStr("className", "composit");
  generateComposit(compBlk);
  compBlk.saveToTextFile(composit_file);

  for (int i = fileNm.nameCount() - 1; i >= 0; i--)
  {
    const char *fn = fileNm.getName(i);
    G_ASSERT(optimizeDag(res_base.str(), fn));
  }

  del_it(rdb_folders_blk);
  rdb_objects_blk = rdb_tex_blk = NULL;
  landClassBlk = NULL;
  clear_and_shrink(res_base);
  clear_and_shrink(compObj);
  texres_names.reset();

  return true;
}
