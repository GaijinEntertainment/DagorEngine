// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/geomMeshHelper.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Resource converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

static OAHashNameMap<true> usedTex;
static OAHashNameMap<true> usedDmInPhobj;
static Tab<char> defPhobjModelContents(midmem);
static int convertPass = 0;

extern bool copy_dag_file_tex_subst(const char *src_name, const char *dst_name, dag::ConstSpan<const char *> srcTexName,
  dag::ConstSpan<const char *> destTexName);

static Tab<const char *> srcTexName(tmpmem), destTexName(tmpmem);

static int strSearch(const char *str, const char *str2)
{
  int len = strlen(str2);
  for (int i = strlen(str) - len - 1; i > -1; i--)
    if (strncmp(&str[i], str2, len) == 0)
      return i;

  return -1;
}

static const char *renamePhysSysRes(const char *name, const char *new_suf)
{
  static String buf;
  buf = name;
  ::remove_trailing_string(buf, "_phs");
  ::remove_trailing_string(buf, "_physsys");
  ::remove_trailing_string(buf, "_physobj");
  ::remove_trailing_string(buf, "_dynmodel");
  buf.append(new_suf);
  return buf;
}

static bool convertPhysSysSimple(const DataBlock &blk, const char *dag)
{
  GeomMeshHelper hlp;
  StaticGeometryContainer geom;
  StaticGeometryNode *n;

  int objNameId = blk.getNameId("obj");
  int bodies = 0;
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &sb = *blk.getBlock(i);
    if (sb.getBlockNameId() != objNameId)
      continue;

    const char *cls = sb.getStr("class", NULL);
    if (!cls)
      return false;

    if (stricmp(cls, "body") == 0)
    {
      bodies++;
      if (bodies > 1)
        return false;
      n = new StaticGeometryNode;
      n->name = "::mainBody";
      n->flags = 0;
      n->lighting = n->LIGHT_NONE;
      n->vss = 0;

      const char *mat_name = sb.getStr("materialName", NULL);
      if (mat_name)
        n->script.setStr("materialName", mat_name);
      n->script.setStr("name", sb.getStr("name", ""));

      geom.addNode(n);
    }
    else if (stricmp(cls, "multiobj") == 0)
    {
      n = new StaticGeometryNode;
      n->flags = n->FLG_COLLIDABLE;
      n->lighting = n->LIGHT_NONE;
      n->vss = 0;

      const char *mat_name = sb.getStr("materialName", NULL);
      if (mat_name)
        n->script.setStr("materialName", mat_name);
      n->script.setStr("collType", sb.getStr("collType", "none"));
      n->script.setStr("massType", sb.getStr("massType", "none"));
      if (sb.getReal("useMass", false))
        n->script.setReal("mass", sb.getReal("massDens", 0));
      else
        n->script.setReal("density", sb.getReal("massDens", 0));

      n->name = sb.getStr("name", "");
      n->wtm.setcol(0, sb.getPoint3("wtm0", Point3(1, 0, 0)));
      n->wtm.setcol(1, sb.getPoint3("wtm1", Point3(0, 0, 1)));
      n->wtm.setcol(2, sb.getPoint3("wtm2", Point3(0, 1, 0)));
      n->wtm.setcol(3, sb.getPoint3("wtm3", Point3(0, 0, 0)));
      hlp.load(*sb.getBlockByNameEx("mesh"));
      if (hlp.faces.size())
      {
        n->mesh = new StaticGeometryMesh;
        n->mesh->mesh.vert = hlp.verts;
        n->mesh->mesh.face.resize(hlp.faces.size());
        mem_set_0(n->mesh->mesh.face);
        for (int i = 0; i < hlp.faces.size(); i++)
          memcpy(n->mesh->mesh.face[i].v, &hlp.faces[i], 12);
      }
      geom.addNode(n);
    }
    else
      return false;
  }
  if (!bodies)
    return false;

  geom.exportDag(dag, true);
  return true;
}

static bool copyFileAndTime(const char *srcF, const char *dstF)
{
  if (srcTexName.size() && trail_stricmp(srcF, ".dag"))
  {
    if (!copy_dag_file_tex_subst(srcF, dstF, srcTexName, destTexName))
      return false;
  }
  else
  {
    if (!dag_copy_file(srcF, dstF, true))
      return false;
  }

  FILETIME ftCreate, ftAccess, ftWrite;
  HANDLE hFile = CreateFile(srcF, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
  {
    CloseHandle(hFile);
    hFile = CreateFile(dstF, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    SetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
  }
  CloseHandle(hFile);
  return true;
}

static int copyFiles(const char *src, const char *dst, const char *mask, const char *dest_name, bool fullReport)
{
  int cnt = 0;
  for (const alefind_t &ff : dd_find_iterator(String(260, "%s/%s", src, mask), DA_FILE))
  {
    String srcF(256, "%s%s", src, ff.name);
    const char *tmp = ff.name;
    tmp = &tmp[strSearch(ff.name, ".lod")];
    String dstF(256, "%s%s%s", (char *)dst, dest_name, tmp);

    String tmpStrDst(256, "%s%s", dest_name, tmp);
    if (fullReport)
      printf(" [copy] '%s' -> '%s'\t", (char *)ff.name, (char *)tmpStrDst);

    if (copyFileAndTime(srcF, dstF))
    {
      if (fullReport)
        printf("[complete]\n");
    }
    else if (!fullReport)
      printf(" [copy] '%s' -> '%s'\t[ERROR]\n", (char *)ff.name, (char *)tmpStrDst);
    else
      printf("[ERROR]\n");

    cnt++;
  }

  return cnt;
}

static void writeModel(const char *fld_prefix, const char *model, const char *root_fld, const char *resName, const char *destFld,
  const char *res, bool fullReport, dag::ConstSpan<char> blk_c)
{
  String pDynModelF(256, "%s/%s/%s.%s.blk", root_fld, fld_prefix, resName, model);

  DataBlock nameBlk(pDynModelF);

  String src(256, "%s%s/", root_fld, fld_prefix);
  String dst(256, "%s%s.dds", destFld, res);
  String msk(64, "%s.lod*.dag", resName);
  // DAG_FATAL("5");
  int cnt = copyFiles(src, destFld, msk, res, fullReport);

  DataBlock destDynModel;

  int lodId = nameBlk.getNameId("lod");
  int l = 0;
  String name;
  for (int k = 0; k < nameBlk.blockCount(); k++)
  {
    if (nameBlk.getBlock(k)->getBlockNameId() != lodId)
      continue;

    DataBlock *sub = destDynModel.addNewBlock("lod");

    if (l > 10)
      name.printf(128, "%s.lod%d.dag", res, l);
    else
      name.printf(128, "%s.lod0%d.dag", res, l);

    // sub->addStr("model", name);
    sub->addReal("range", nameBlk.getBlock(k)->getReal("range", 0));
    l++;
  }

  if (blk_c.size())
  {
    DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 4 << 10);
    destDynModel.saveToTextStream(cwr_new);
    if (data_size(blk_c) == cwr_new.size() && mem_eq(blk_c, cwr_new.data()))
      return;
  }

  String destDynModelF(256, "%s/%s.%s.blk", destFld, res, model);
  destDynModel.saveToTextFile(destDynModelF);
}

static Tab<String> tmpStr(tmpmem);
static void setupTexNameSubst(const DataBlock &blk)
{
  srcTexName.clear();
  destTexName.clear();
  tmpStr.clear();

  int nid = blk.getNameId("ref");
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      srcTexName.push_back(blk.getBlock(i)->getStr("slot", NULL));

      String &s = tmpStr.push_back();
      s = blk.getBlock(i)->getStr("ref", NULL);
      usedTex.addNameId(s);
      if (trail_stricmp(s, "_tex"))
        erase_items(s, s.size() - 5, 4);

      destTexName.push_back(s);
    }
}
static void resetTexNameSubst()
{
  srcTexName.clear();
  destTexName.clear();
  tmpStr.clear();
}

static void convert(const DataBlock *blk, const char *now_in, const char *root_fld, const char *dst_fld, bool fullReport,
  const OAHashNameMap<true> &restrict_types)
{
  int resId = blk->getNameId("res");

  for (int i = 0; i < blk->blockCount(); i++)
  {
    const DataBlock *subBlock = blk->getBlock(i);
    const char *tmp = subBlock->getStr("name", NULL);
    if (!tmp)
      DAG_FATAL("not found 'name' in block '%s'", subBlock->getBlockName());

    String destFld(256, "%s%s/", dst_fld, tmp);
    String newFld(256, "%s%s/", now_in, tmp);

    if (fullReport)
      printf("\n[mkdir]: '%s'\t", (char *)destFld);

    if (!dd_mkpath(destFld))
    {
      if (!fullReport)
        printf("\n[mkdir]: '%s'\t[ERROR]\n\n", (char *)destFld);
      else
        printf("[ERROR]\n\n");
    }
    else if (fullReport)
      printf("\n");

    DataBlock destBlk;
    DataBlock *texVRB = NULL, *a2dVRB = NULL, *skelVRB = NULL, *phobjVRB = NULL;
    bool destBlkNeedWrite = false;
    if (convertPass > 0)
      destBlk.load(String(256, "%s.folder.blk", (char *)destFld));

    if (!subBlock->getBool("isExported", false))
    {
      destBlk.addBool("exported", false);
      destBlkNeedWrite = true;
    }

    for (int j = 0; j < subBlock->paramCount(); j++)
    {
      if (subBlock->getParamNameId(j) != resId)
        continue;

      const char *res = subBlock->getStr(j);

      String resdbF(256, "%s/resdb/%s.rdbres.blk", root_fld, res);

      DataBlock resdb;
      if (!resdb.load(resdbF))
      {
        printf("could not open resdb: '%s'\n", resdbF.str());
        continue;
      }

      const char *className = resdb.getStr("className", NULL);
      const char *resName = resdb.getStr("resName", NULL);

      if (!className)
        DAG_FATAL("not found 'className' in '%s'", (char *)resdbF);

      if (restrict_types.getNameId(className) == -1)
        continue;

      if (!resName)
        DAG_FATAL("not found 'resName' in '%s'", (char *)resdbF);

      if (stricmp(className, "Texture") == 0)
      {
        String texres(res);
        if (convertPass > 0 && usedTex.getNameId(texres) == -1)
        {
          // printf("skip non-ref tex: %s\n", res);
          continue;
        }

        if (trail_stricmp(texres, "_tex"))
          erase_items(texres, texres.size() - 5, 4);
        res = texres;

        String pTextureF(256, "%s/p_Texture/%s.blk", root_fld, resName);
        String src(256, "%s/p_Texture/%s.dds", root_fld, resName);
        String dst(256, "%s%s.dds", (char *)destFld, res);

        if (fullReport)
          printf(" [copy] '%s' -> '%s'\t", resName, res);

        if (dag_copy_file(src, dst, true))
        {
          if (fullReport)
            printf("[complete]\n");
        }
        else if (!fullReport)
          printf(" [copy] '%s' -> '%s'\t[ERROR]\n", resName, res);
        else
          printf("[ERROR]\n");

        DataBlock nameBlk(pTextureF);

        bool notDefault = false;

        if (nameBlk.isValid())
        {
          notDefault = (nameBlk.blockCount() != 0) || (nameBlk.paramCount() != 5) || (nameBlk.getInt("address_u", -1) != 1) ||
                       (nameBlk.getInt("address_v", -1) != 1) || (nameBlk.getInt("hq_mip", -1) != 0) ||
                       (nameBlk.getInt("mq_mip", -1) != 1) || (nameBlk.getInt("lq_mip", -1) != 2);
        }

        if (notDefault)
        {
          String dst(256, "%s/%s.tex.blk", (char *)destFld, res);
          DataBlock out;
          switch (nameBlk.getInt("address_u", 1))
          {
            case 1: out.setStr("addrU", "wrap"); break;
            case 2: out.setStr("addrU", "mirror"); break;
            case 3: out.setStr("addrU", "clamp"); break;
            case 4: out.setStr("addrU", "border"); break;
            case 5: out.setStr("addrU", "mirrorOnce"); break;
          }
          switch (nameBlk.getInt("address_v", 1))
          {
            case 1: out.setStr("addrV", "wrap"); break;
            case 2: out.setStr("addrV", "mirror"); break;
            case 3: out.setStr("addrV", "clamp"); break;
            case 4: out.setStr("addrV", "border"); break;
            case 5: out.setStr("addrV", "mirrorOnce"); break;
          }
          out.setInt("hqMip", nameBlk.getInt("hq_mip", -1));
          out.setInt("mqMip", nameBlk.getInt("mq_mip", -1));
          out.setInt("lqMip", nameBlk.getInt("lq_mip", -1));
          out.setStr("name", String(64, "%s.dds", res));
          out.saveToTextFile(dst);
        }
        else
        {
          if (!texVRB)
          {
            destBlkNeedWrite = true;
            texVRB = destBlk.addNewBlock("virtual_res_blk");
            texVRB->setStr("find", "^(.*)\\.dds$");
            texVRB->setStr("className", "tex");

            DataBlock *subFldBlkNew = texVRB->addBlock("contents");
            subFldBlkNew->setStr("addrU", "wrap");
            subFldBlkNew->setStr("addrV", "wrap");
            subFldBlkNew->setInt("hqMip", 0);
            subFldBlkNew->setInt("mqMip", 1);
            subFldBlkNew->setInt("lqMip", 2);
          }
        }
      }
      else if (stricmp(className, "DynModel") == 0)
      {
        if (convertPass > 0 && usedDmInPhobj.getNameId(res) != -1)
        {
          // printf("skip phobj dynmodel: %s\n", res);
          continue;
        }

        setupTexNameSubst(resdb);
        writeModel("p_DynModel", "DynModel", root_fld, resName, destFld, res, fullReport, {});
        resetTexNameSubst();
      }
      else if (stricmp(className, "RendInst") == 0)
      {
        setupTexNameSubst(resdb);
        writeModel("p_RendInst", "RendInst", root_fld, resName, destFld, res, fullReport, {});
        resetTexNameSubst();
      }
      else if (stricmp(className, "Effects") == 0)
      {
        String src(256, "%s/p_Effects/%s.effect.blk", root_fld, resName);
        String dst(256, "%s%s.fx.blk", (char *)destFld, res);
        DataBlock blk;
        if (blk.load(src))
        {
          int refNameId = blk.getNameId("ref");
          int ref_id = 0;
          for (int bi = 0; bi < blk.blockCount(); ++bi)
          {
            DataBlock &b = *blk.getBlock(bi);
            if (b.getBlockNameId() != refNameId)
              continue;

            ref_id++;
            const char *name = b.getStr("name", NULL);
            if (!name)
              continue;

            const char *typeName = b.getStr("type", NULL);
            if (!typeName)
              continue;

            const char *ref = (ref_id - 1 < resdb.blockCount()) ? resdb.getBlock(ref_id - 1)->getStr("ref", NULL) : NULL;

            if (name && *name)
              b.setStr("slot", name);
            if (stricmp(typeName, "texture") == 0)
            {
              b.setStr("type", "tex");
              if (ref && *ref)
              {
                String texres(ref);
                usedTex.addNameId(texres);
                if (trail_stricmp(texres, "_tex"))
                  erase_items(texres, texres.size() - 5, 4);
                b.setStr("ref", texres);
              }
            }
            else if (stricmp(typeName, "effects") == 0)
            {
              b.setStr("type", "fx");
              if (ref && *ref)
                b.setStr("ref", ref);
            }
            b.removeParam("name");
          }
          blk.saveToTextFile(dst);
        }
        else
          printf("[ERROR] can't read %s\n", src.str());
      }
      else if (stricmp(className, "Anim2Data") == 0)
      {
        String src(256, "%s/p_Anim2Data/%s.a2d", root_fld, resName);
        String dst(256, "%s%s.a2d", (char *)destFld, res);
        if (!copyFileAndTime(src, dst))
          printf("[ERROR] can't copy %s to %s\n", src.str(), dst.str());

        if (!a2dVRB)
        {
          destBlkNeedWrite = true;
          a2dVRB = destBlk.addNewBlock("virtual_res_blk");
          a2dVRB->setStr("find", "^(.*)\\.a2d$");
          a2dVRB->setStr("className", "a2d");
        }
      }
      else if (stricmp(className, "Collision") == 0 || stricmp(className, "Cloud") == 0)
      {
        String src(256, "%s/p_%s/%s.%s.blk", root_fld, className, resName, className);
        String dst(256, "%s%s.%s.blk", destFld.str(), res, className);
        DataBlock blk;
        if (blk.load(src))
          blk.saveToTextFile(dst);
        else
          printf("[ERROR] can't read %s\n", src.str());
      }
      else if (stricmp(className, "GeomNodeTree") == 0)
      {
        String src(256, "%s/p_GeomNodeTree/%s.dat", root_fld, resName);
        String dst(256, "%s%s.dat", (char *)destFld, res);
        if (!copyFileAndTime(src, dst))
          printf("[ERROR] can't copy %s to %s\n", src.str(), dst.str());

        if (!skelVRB)
        {
          destBlkNeedWrite = true;
          skelVRB = destBlk.addNewBlock("virtual_res_blk");
          skelVRB->setStr("find", "^(.*)\\.dat$");
          skelVRB->setStr("className", "skeleton");
        }
      }
      else if (stricmp(className, "PhysObj") == 0)
      {
        if (!resdb.getBlock(0) || !resdb.getBlock(1) || stricmp(resdb.getBlock(0)->getStr("slot", ""), "Model") != 0 ||
            stricmp(resdb.getBlock(1)->getStr("slot", ""), "PhysSys") != 0)
        {
          printf("broken physobj: %s\n", resName);
          continue;
        }

        DataBlock dm_resdb, ps_resdb, ps_blk;
        const char *dm_name = resdb.getBlock(0)->getStr("ref", "");
        const char *ps_name = resdb.getBlock(1)->getStr("ref", "");

        if (!dm_resdb.load(String(256, "%s/resdb/%s.rdbres.blk", root_fld, dm_name)))
        {
          printf("broken ref to model %s in physobj: %s\n", dm_name, resName);
          continue;
        }
        if (!ps_resdb.load(String(256, "%s/resdb/%s.rdbres.blk", root_fld, ps_name)))
        {
          printf("broken ref to physsys %s in physobj: %s\n", ps_name, resName);
          continue;
        }
        String fn(256, "%s/p_PhysSys/%s.physsys.blk", root_fld, ps_resdb.getStr("resName", NULL));
        if (!ps_blk.load(fn))
        {
          printf("broken physsys %s (ref in physobj %s)\n  can't load %s\n", ps_name, resName, fn.str());
          continue;
        }

        // convert physsys ref
        if (!convertPhysSysSimple(ps_blk, String(256, "%s%s.dag", destFld.str(), renamePhysSysRes(res, "_phobj"))))
        {
          printf("skipped not-simple physobj %s\n", resName);
          continue;
        }


        // make virtual rules
        if (!phobjVRB)
        {
          destBlkNeedWrite = true;
          phobjVRB = destBlk.addNewBlock("virtual_res_blk");
          phobjVRB->setStr("find", "^(.*)\\_phobj.dag");
          phobjVRB->setStr("className", "physObj");
          phobjVRB->setStr("name", "$1_phobj");
          DataBlock *b = phobjVRB->addNewBlock("contents");
          b->setStr("dynModel", "$1_model");
          b->setBool("simplePhysObj", true);

          phobjVRB = destBlk.addNewBlock("virtual_res_blk");
          phobjVRB->setStr("find", "^(.*_model)\\.lod[0-9]+\\.dag");
          phobjVRB->setStr("className", "dynModel");
          b = phobjVRB->addNewBlock("contents");
          b->addNewBlock("lod")->setReal("range", 1000);

          DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
          b->saveToTextStream(cwr);
          defPhobjModelContents = make_span_const((char *)cwr.data(), cwr.size());
        }

        const char *dmresName = NULL;
        DataBlock dmresdb;

        {
          String dmblk(256, "%s/resdb/%s.rdbres.blk", root_fld, dm_name);

          if (dmresdb.load(dmblk))
            dmresName = dmresdb.getStr("resName", dm_name);
          else
            printf("could not open resdb: '%s'\n", dmblk.str());
        }

        // convert dynmodel ref
        setupTexNameSubst(dm_resdb);
        writeModel("p_DynModel", "DynModel", root_fld, dmresName, destFld, renamePhysSysRes(res, "_model"), fullReport,
          defPhobjModelContents);
        resetTexNameSubst();

        usedDmInPhobj.addNameId(dm_name);
      }
      else if (stricmp(className, "AnimBnl") == 0)
      {
        String src(256, "%s/p_AnimBnl/%s.AnimBnl.blk", root_fld, resName);
        String dst(256, "%s%s.animTree.blk", destFld.str(), res);
        DataBlock blk;
        if (blk.load(src))
        {
          blk.removeParam("lastUsedModel");
          blk.removeParam("lastUsedSkeleton");
          blk.removeParam("lastUsedCharacter");
          blk.saveToTextFile(dst);
        }
        else
          printf("[ERROR] can't read %s\n", src.str());
      }
      else if (stricmp(className, "AnimGraph") == 0)
      {
        String src(256, "%s/p_AnimGraph/%s.AnimGraph.blk", root_fld, resName);
        String dst(256, "%s%s.shared.blk", destFld.str(), res);
        DataBlock blk;
        if (blk.load(src))
        {
          blk.setStr("sharedType", "stateGraph");
          blk.removeBlock("editorShared");
          blk.saveToTextFile(dst);
        }
        else
          printf("[ERROR] can't read %s\n", src.str());
      }
      else if (stricmp(className, "animchar") == 0)
      {
        if (!resdb.getBlock(0) || !resdb.getBlock(1) || stricmp(resdb.getBlock(0)->getStr("slot", ""), "AnimBnl") != 0 ||
            stricmp(resdb.getBlock(1)->getStr("slot", ""), "AnimGraph") != 0)
        {
          printf("broken animchar: %s\n", resName);
          continue;
        }
        String dst(256, "%s%s.stateGraph.blk", destFld.str(), res);
        DataBlock blk;
        blk.setStr("animTree", resdb.getBlock(0)->getStr("ref", ""));
        if (resdb.getBlock(1)->getStr("ref", "")[0])
          blk.setStr("sharedSG", resdb.getBlock(1)->getStr("ref", ""));
        blk.saveToTextFile(dst);
      }
      else if (stricmp(className, "Character") == 0)
      {
        if (!resdb.getBlock(0) || !resdb.getBlock(1) || !resdb.getBlock(2) ||
            stricmp(resdb.getBlock(0)->getStr("slot", ""), "model") != 0 ||
            stricmp(resdb.getBlock(1)->getStr("slot", ""), "skeleton") != 0 ||
            stricmp(resdb.getBlock(2)->getStr("slot", ""), "animchar") != 0)
        {
          printf("broken character: %s\n", resName);
          continue;
        }

        String src(256, "%s/p_Character/%s.char.blk", root_fld, resName);
        String dst(256, "%s%s.animChar.blk", destFld.str(), res);
        DataBlock blk;
        if (blk.load(src))
        {
          blk.setStr("dynModel", resdb.getBlock(0)->getStr("ref", ""));
          blk.setStr("skeleton", resdb.getBlock(1)->getStr("ref", ""));
          if (resdb.getBlock(2)->getStr("ref", "")[0])
            blk.setStr("stateGraph", resdb.getBlock(2)->getStr("ref", ""));
          blk.saveToTextFile(dst);
        }
        else
          printf("[ERROR] can't read %s\n", src.str());
      }
      /*else
        DAG_FATAL("unsupported class name: '%s'", className);*/
    }

    if (destBlkNeedWrite)
    {
      String destBlkF(256, "%s.folder.blk", (char *)destFld);
      destBlk.saveToTextFile(destBlkF);
    }

    convert(blk->getBlock(i), (char *)newFld, root_fld, (char *)destFld, fullReport, restrict_types);
  }
}


static bool strPars(char *strToPars, const char *prefix, String *str = NULL)
{
  int paramLen = strlen(prefix);
  if (strncmp(strToPars, prefix, paramLen) == 0)
  {
    if (str)
      *str = &strToPars[paramLen];
    return true;
  }
  return false;
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  OAHashNameMap<true> restrict_types, restrict_types2, restrict_types3;
  String srcDir, destDir;
  bool fullReport = false;

  srcDir.reserve(260);
  destDir.reserve(260);
  for (int i = 0; i < __argc; i++)
  {
    if (__argv[i][0] == '-')
    {
      if (strPars(&__argv[i][1], "s:", &srcDir))
        continue;
      if (strPars(&__argv[i][1], "d:", &destDir))
        continue;
      if (strPars(&__argv[i][1], "f"))
      {
        fullReport = true;
        continue;
      }
      if (stricmp(__argv[i], "-c:all") == 0)
      {
        restrict_types.addNameId("Texture");
        restrict_types.addNameId("RendInst");
        restrict_types.addNameId("Effects");
        restrict_types.addNameId("Anim2Data");
        restrict_types.addNameId("Aces");
        restrict_types.addNameId("GeomNodeTree");
        restrict_types.addNameId("PhysObj");
        restrict_types2.addNameId("DynModel");
        restrict_types2.addNameId("AnimBnl");
        restrict_types2.addNameId("AnimGraph");
        restrict_types2.addNameId("AnimChar");
        restrict_types2.addNameId("Character");
      }
      else if (stricmp(__argv[i], "-c:ri") == 0)
      {
        restrict_types.addNameId("RendInst");
        restrict_types2.addNameId("Texture");
      }
      else if (stricmp(__argv[i], "-c:physobj") == 0)
      {
        restrict_types.addNameId("PhysObj");
        restrict_types2.addNameId("Texture");
      }
      else if (stricmp(__argv[i], "-c:fx") == 0)
      {
        restrict_types.addNameId("Effects");
        restrict_types2.addNameId("Texture");
      }
      else if (stricmp(__argv[i], "-c:char") == 0)
      {
        restrict_types.addNameId("Anim2Data");
        restrict_types.addNameId("AnimBnl");
        restrict_types.addNameId("AnimGraph");
        restrict_types.addNameId("AnimChar");
        restrict_types.addNameId("Character");
        restrict_types.addNameId("GeomNodeTree");
        restrict_types.addNameId("DynModel");
        restrict_types2.addNameId("Texture");
      }
    }
  }

  print_title();

  if (((strlen(srcDir) < 1) || (strlen(destDir) < 1)))
  {
    printf("usage: -s:<source dir> -d:<destination dir> [-f - full report] "
           "[-c:{all|ri|physobj}]\n\n");
    return -1;
  }

  dd_append_slash_c(srcDir);
  dd_append_slash_c(destDir);
  srcDir.resize(strlen(srcDir) + 1);
  destDir.resize(strlen(destDir) + 1);

  String rootDir(256, "%s/", (char *)srcDir);
  srcDir.append("/resdb/rdb.folders.blk");

  DataBlock blk(srcDir);
  if (!blk.isValid())
  {
    printf("blk is not found in '%s'", (char *)srcDir);
    return true;
  }

  convertPass = 0;
  convert(&blk, rootDir, rootDir, destDir, fullReport, restrict_types);
  if (restrict_types2.nameCount())
  {
    convertPass = 1;
    convert(&blk, rootDir, rootDir, destDir, fullReport, restrict_types2);
  }
  if (restrict_types3.nameCount())
  {
    convertPass = 2;
    convert(&blk, rootDir, rootDir, destDir, fullReport, restrict_types3);
  }

  printf("\n...complete\n\n");

  return true;
}
