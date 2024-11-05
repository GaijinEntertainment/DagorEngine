// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/util/iLogWriter.h>

#include <ioSys/dag_dataBlock.h>

#include <3d/dag_materialData.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_dbgStr.h>

#include <obsolete/dag_cfg.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdlib.h>

#include <libTools/dagFileRW/textureNameResolver.h>

class DETextureNameResolver : public ITextureNameResolver
{
public:
  DETextureNameResolver(const char *fname) : basePath(fname) { ::location_from_path(basePath); }

  virtual bool resolveTextureName(const char *src_name, String &out_str)
  {
    DETextureName texName = src_name;
    if (texName.findExisting(basePath))
      out_str = texName;
    else
      out_str = "";

    return true;
  }

public:
  String basePath;
};

static const char *curLoadedDag = NULL;

StaticGeometryMaterial *StaticGeometryContainer::DagLoadData::getMaterial(MaterialData *m)
{
  if (!m)
    return NULL;

  for (int i = 0; i < mats.size(); ++i)
    if (mats[i].dagMat == m)
      return mats[i].sgMat;

  // convert MaterialData to StaticGeometryMaterial
  StaticGeometryMaterial *sgm = new StaticGeometryMaterial;
  sgm->name = m->matName;
  sgm->scriptText = m->matScript;
  sgm->className = m->className;
  sgm->amb = m->mat.amb;
  sgm->diff = m->mat.diff;
  sgm->spec = m->mat.spec;
  Color4 emission = m->mat.emis;
  sgm->power = m->mat.power;

  int i;
  for (i = 0; i < StaticGeometryMaterial::NUM_TEXTURES && i < MAXMATTEXNUM; ++i) //-V560
  {
    const char *texNameSrc = ::get_managed_texture_name(m->mtex[i]);

    if (texNameSrc)
      sgm->textures[i] = new StaticGeometryTexture(texNameSrc);
    else
    {
      if (logWriter && m->mtex[i] != BAD_TEXTUREID)
        logWriter->addMessage(ILogWriter::ERROR, "Texture \"%s\" not found", texNameSrc);

      sgm->textures[i] = useNotFoundTex ? new StaticGeometryTexture(texNameSrc) : NULL;
    }
  }

  CfgReader c;
  c.getdiv_text(String(512, "[q]\r\n%s\r\n", m->matScript.str()), "q");

  emission *= c.getreal("emission", 0);
  sgm->emis = emission * m->mat.diff;

  sgm->flags = 0;
  if (m->flags & MATF_2SIDED)
    sgm->flags |= MatFlags::FLG_2SIDED;

  if (c.getbool("real_two_sided", false))
    sgm->flags |= MatFlags::FLG_REAL_2SIDED;

#if HAVE_LIGHTMAPS // no lightmaps anymore
  const char *s = c.getstr("lighting", "");
  if (stricmp(s, "lightmap") == 0)
    sgm->flags |= MatFlags::FLG_USE_LM;
  else if (stricmp(s, "vltmap") == 0)
    sgm->flags |= MatFlags::FLG_USE_VLM;
#endif

  sgm->trans = Color4(0, 0, 0, 0);
  sgm->atest = c.getint("atest", 0);
  sgm->cosPower = 1;
  if (c.getreal("emission", 0) > 0.000001 || c.getreal("amb_emission", 0) > 0.000001)
  {
    sgm->amb *= c.getreal("amb_emission", 0);
    sgm->amb *= m->mat.diff;
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_GI_MUL_EMIS_TEX;
    sgm->cosPower = c.getreal("power", 1);
  }

  if (strstr(m->className, "alpha_blend")) //! no emission
  {
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_GI_MUL_EMIS_TEX;
    sgm->trans = Color4(1, 1, 1, 1);
    sgm->flags |= MatFlags::FLG_GI_MUL_TRANS_TEX | MatFlags::FLG_GI_MUL_TRANS_INV_A;
  }
  else if (strstr(m->className, "billboard"))
  {
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_BILLBOARD_VERTICAL;
  }
  else if (strstr(m->className, "facing_leaves"))
  {
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_BILLBOARD_FACING;
  }
  else if (strcmp(m->className, "hdr_envi") == 0 || strcmp(m->className, "hdr_envi2") == 0) // envi light code brabch
  {
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_GI_MUL_EMIS_TEX;
    sgm->emis = emission;
    sgm->amb *= c.getreal("amb_emission", 0);
    sgm->amb *= m->mat.diff;
    sgm->flags |= MatFlags::FLG_USE_IN_GI | MatFlags::FLG_GI_MUL_DIFF_TEX | MatFlags::FLG_GI_MUL_EMIS_TEX;
    sgm->cosPower = c.getreal("power", 1);
  }
  else
  {
    sgm->trans = c.getcolor4("gi_trans_color", Color4(0, 0, 0, 0));

    // gi flags
    if (c.getbool("use_in_gi", true))
      sgm->flags |= MatFlags::FLG_USE_IN_GI;
    if (c.getbool("gi_mul_emis_by_texture", true))
      sgm->flags |= MatFlags::FLG_GI_MUL_EMIS_TEX;
    if (c.getbool("gi_mul_diff_by_texture", true))
      sgm->flags |= MatFlags::FLG_GI_MUL_DIFF_TEX;

    if (c.getbool("gi_mul_diff_by_color", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_DIFF_COLOR;

    if (c.getbool("gi_mul_emis_by_alpha", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_EMIS_ALPHA;
    if (c.getbool("gi_mul_emis_by_angle", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_EMIS_ANGLE;

    if (c.getbool("gi_mul_trans_by_texture", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_TRANS_TEX;
    if (c.getbool("gi_mul_trans_by_alpha", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_TRANS_ALPHA;
    if (c.getbool("gi_mul_trans_by_inv_alpha", false))
      sgm->flags |= MatFlags::FLG_GI_MUL_TRANS_INV_A;
  }
  //== more flags?

  i = append_items(mats, 1);
  mats[i].dagMat = m;
  mats[i].sgMat = sgm;

  return sgm;
}


void StaticGeometryContainer::loadDagNode(class Node &node, DagLoadData &dag)
{

  if (node.obj && node.obj->isSubOf(OCID_MESHHOLDER))
  {
    Mesh *m = ((MeshHolderObj *)node.obj)->mesh;

    if (m)
    {
      StaticGeometryNode *gn = new StaticGeometryNode;
      gn->name = node.name;
      dblk::load_text(gn->script, make_span_const(node.script), dblk::ReadFlag::ROBUST, curLoadedDag);
      gn->wtm = node.wtm;
      gn->mesh = new StaticGeometryMesh;
      gn->mesh->mesh = *m;

      gn->flags = 0;

      if (node.flags & NODEFLG_RENDERABLE)
        gn->flags |= StaticGeometryNode::FLG_RENDERABLE;
      if (node.flags & NODEFLG_CASTSHADOW)
        gn->flags |= StaticGeometryNode::FLG_CASTSHADOWS;
      if (node.flags & NODEFLG_RCVSHADOW)
        gn->flags |= StaticGeometryNode::FLG_COLLIDABLE;

      // materials
      int num = 0;

      if (node.mat)
      {
        num = node.mat->subMatCount();
        if (!num)
          num = 1;
      }

      if (num)
      {
        gn->mesh->mats.resize(num);

        for (int i = 0; i < num; ++i)
        {
          MaterialData *sm = node.mat->getSubMat(i);
          if (!sm)
            continue;

          gn->mesh->mats[i] = dag.getMaterial((MaterialData *)sm);

          // check combination of force normals and FLG_REAL_2SIDED
          //(see commentaries in nsb_lightmappedScene.cpp)
          if (gn->flags & (StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS))
          {
            if ((gn->mesh->mats[i]->flags & (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)) ==
                (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED))
              gn->mesh->mats[i]->flags &= ~MatFlags::FLG_REAL_2SIDED;
          }
        }

        // erase unused mats

        gn->mesh->mesh.clampMaterials(num - 1);
      }

      String blkScript;
      String nonBlkScript;

      getBlkScript(node.script, blkScript, nonBlkScript);

      dblk::load_text(gn->script, blkScript, dblk::ReadFlag::ROBUST, curLoadedDag);

      if (gn->script.paramCount())
      {
        if (gn->script.getBool("renderable", gn->flags & StaticGeometryNode::FLG_RENDERABLE))
          gn->flags |= StaticGeometryNode::FLG_RENDERABLE;
        else
          gn->flags &= ~StaticGeometryNode::FLG_RENDERABLE;

        if (gn->script.getBool("cast_shadows", gn->flags & StaticGeometryNode::FLG_CASTSHADOWS))
          gn->flags |= StaticGeometryNode::FLG_CASTSHADOWS;
        else
          gn->flags &= ~StaticGeometryNode::FLG_CASTSHADOWS;

        if (gn->script.getBool("cast_on_self", false))
          gn->flags |= StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;

        if (gn->script.getBool("fade", false))
          gn->flags |= StaticGeometryNode::FLG_FADE;

        if (gn->script.getBool("fade_null", false))
          gn->flags |= StaticGeometryNode::FLG_FADENULL;

        if (strcmp(gn->script.getStr("vss_use", ""), "disable vss") == NULL)
          gn->vss = 0;
        else if (strcmp(gn->script.getStr("vss_use", ""), "force def") == NULL)
          gn->vss = 1;
        else if (strcmp(gn->script.getStr("vss_use", ""), "use as is") == NULL)
          gn->vss = -1;

        if (strcmp(gn->script.getStr("normals", ""), "world") == NULL)
        {
          gn->flags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
          gn->normalsDir = gn->script.getPoint3("dir", Point3(0, 1, 0));

          gn->normalsDir.normalize();

          if (gn->normalsDir.lengthSq() < 0.0001)
            gn->normalsDir = Point3(0, 1, 0);
        }
        else if (strcmp(gn->script.getStr("normals", ""), "local") == NULL)
        {
          gn->normalsDir = gn->script.getPoint3("dir", Point3(0, 0, 1));
          gn->flags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;

          gn->normalsDir.normalize();

          if (gn->normalsDir.lengthSq() < 0.0001)
            gn->normalsDir = Point3(0, 1, 0);
        }
        else if (strcmp(gn->script.getStr("normals", ""), "spherical") == NULL)
        {
          gn->flags |= StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        }

        gn->transSortPriority = gn->script.getInt("priority", 0);
        gn->visRange = gn->script.getReal("visibility_range", -1.f);

        gn->lodRange = gn->script.getReal("lod_range", 100.0);
        gn->topLodName = gn->script.getStr("top_lod", NULL);

        gn->unwrapScheme = gn->script.getInt("unwrapScheme", -1);
        gn->lodNearVisRange = gn->script.getReal("lodNearVisRange", -1);
        gn->lodFarVisRange = gn->script.getReal("lodFarVisRange", -1);

        gn->linkedResource = gn->script.getStr("linked_resource", NULL);

        if (gn->script.getBool("billboard", false))
          gn->flags |= StaticGeometryNode::FLG_BILLBOARD_MESH;

        if (gn->script.getBool("occluder", false))
        {
          // gn->flags &= ~StaticGeometryNode::FLG_RENDERABLE;
          gn->flags |= StaticGeometryNode::FLG_OCCLUDER;
        }

        if (gn->script.getBool("collidable", true))
          gn->flags |= StaticGeometryNode::FLG_COLLIDABLE;
        else
          gn->flags &= ~StaticGeometryNode::FLG_COLLIDABLE;

        if (gn->script.getBool("automatic_visrange", true) || gn->visRange == -1)
          gn->flags |= StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;

        if (gn->script.getBool("not_mix_lods", false))
          gn->flags |= StaticGeometryNode::FLG_DO_NOT_MIX_LODS;

        const char *lighting = gn->script.getStr("lighting", "default");
        gn->lighting = StaticGeometryNode::strToLighting(lighting);

        gn->ltMul = gn->script.getReal("lt_mul", 1.0);
        if (gn->ltMul <= 0.01)
          gn->ltMul = 1;

        gn->vltMul = gn->script.getInt("vlt_mul", 1);
        if (gn->vltMul <= 0)
          gn->vltMul = 1;
      }

      // old script format support (CFG)
      else
      {
        CfgReader c;
        c.readtext(String("[general]\r\n") + nonBlkScript);
        c.getdiv("shadows");
        if (c.getbool("cast_on_self", node.flags & NODEFLG_CASTSHADOW))
          gn->flags |= StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;

        // fade
        if (c.getdiv("fade"))
          gn->flags |= StaticGeometryNode::FLG_FADE;

        // fade null
        if (c.getdiv("fadenull"))
          gn->flags |= StaticGeometryNode::FLG_FADENULL;

        c.getdiv("normals");
        if (c.getstr("world"))
        {
          gn->normalsDir = c.getpoint3("world", Point3(0, 1, 0));
          gn->flags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        }
        else if (c.getstr("local"))
        {
          gn->normalsDir = c.getpoint3("local", Point3(0, 0, 1));
          gn->flags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
        }
        else if (c.getstr("spherical"))
        {
          gn->flags |= StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        }

        // vss
        c.getdiv("light");
        if (c.getstr("vss_use"))
          gn->vss = c.getbool("vss_use", false);
        else
          gn->vss = -1;

        // trans sort priority
        c.getdiv("general");
        gn->transSortPriority = c.getint("priority", 0);

        // visible range
        c.getdiv("vis");
        gn->visRange = c.getreal("visrange", c.getreal("range", -1.f));

        gn->flags |= StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;

        // lods
        c.getdiv("general");
        gn->lodRange = c.getreal("lod_range", 100);
        gn->topLodName = c.getstr("top_lod", NULL);

        // billboarding
        c.getdiv("general");
        if (c.getbool("billboard", false))
          gn->flags |= StaticGeometryNode::FLG_BILLBOARD_MESH;

        // occluder
        c.getdiv("general");
        if (c.getbool("occluder", false))
        {
          // gn->flags &= ~StaticGeometryNode::FLG_RENDERABLE;
          gn->flags |= StaticGeometryNode::FLG_OCCLUDER;
        }

        // collidable
        if (c.getdiv("interactive"))
        {
          if (c.getbool("collidable", true))
            gn->flags |= StaticGeometryNode::FLG_COLLIDABLE;
          else
            gn->flags &= ~StaticGeometryNode::FLG_COLLIDABLE;
        }
      }

      gn->nonBlkScript = (const char *)nonBlkScript;

      // billboards test
      if (!(gn->flags & StaticGeometryNode::FLG_BILLBOARD_MESH))
      {
        bool billboardMat = false;
        const int matCount = gn->mesh->mats.size();

        for (int i = 0; i < matCount; ++i)
        {
          if (!gn->mesh->mats[i])
            continue;
          if (gn->mesh->mats[i]->flags & MatFlags::FLG_BILLBOARD_MASK)
          {
            if (matCount == 1)
            {
              billboardMat = true;
              break;
            }
            else
            {
              if (!billboardMat)
                billboardMat = m->hasMatId(i);
            }

            if (billboardMat)
              gn->flags |= StaticGeometryNode::FLG_BILLBOARD_MESH;
          }
        }
      }

      gn->calcBoundBox();
      gn->calcBoundSphere();

      addNode(gn);
    }
  }

  for (int i = 0; i < node.child.size(); ++i)
    if (node.child[i])
      loadDagNode(*node.child[i], dag);
}


bool StaticGeometryContainer::loadFromDag(const char *filename, ILogWriter *log, bool use_not_found_tex, int load_flags)
{
  loaded = false;

  if (!::dd_file_exist(filename))
  {
    debug("\"%s\" file not found", (const char *)filename);
    return false;
  }

  DETextureNameResolver resolvCb(filename);

  ITextureNameResolver *oldResolver = get_global_tex_name_resolver();
  if (!oldResolver)
    set_global_tex_name_resolver(&resolvCb);

  AScene sc;
  if (!::load_ascene((char *)filename, sc, load_flags, false))
  {
    set_global_tex_name_resolver(oldResolver);
    debug("Couldn't load ascene from \"%s\" file with 0x%08x flags", filename, load_flags);
    return false;
  }

  set_global_tex_name_resolver(oldResolver);

  sc.root->calc_wtm();

  DagLoadData dag(filename);
  dag.setUseNotFoundTex(use_not_found_tex);
  dag.setLogWriter(log);

  curLoadedDag = filename;
  if (sc.root)
    loadDagNode(*sc.root, dag);
  curLoadedDag = NULL;

  loaded = true;

  markNonTopLodNodes(log, filename);
  return true;
}

void StaticGeometryContainer::getBlkScript(const char *script, String &blk_script, String &non_blk_script)
{
  const char *start = script;
  const char *end = NULL;

  const char *rStr = strchr(start, '\r');
  const char *nStr = strchr(start, '\n');

  String line;

  while (rStr || nStr)
  {
    if (rStr && nStr)
      end = __min(rStr, nStr);
    else if (rStr)
      end = rStr;
    else
      end = nStr;

    const int strLen = end - start;

    line.resize(strLen + 3);
    memcpy(&line[0], (const void *)start, strLen);
    line[strLen] = '\r';
    line[strLen + 1] = '\n';
    line[strLen + 2] = 0;

    if (strchr(line, ':'))
      blk_script += line;
    else
      non_blk_script += line;

    while (*end == '\r' || *end == '\n')
      ++end;

    start = end;

    rStr = strchr(start, '\r');
    nStr = strchr(start, '\n');
  }

  if (*start)
  {
    const char *end = script + strlen(script);
    const int strLen = end - start;

    line.resize(strLen + 1);
    memcpy(&line[0], (const void *)start, strLen);
    line[strLen] = 0;

    if (strchr(line, ':'))
      blk_script += line;
    else
      non_blk_script += line;
  }
}
