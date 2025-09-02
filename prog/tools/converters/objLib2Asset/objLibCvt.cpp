// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/fileUtils.h>
#include <math/dag_TMatrix.h>
#include <util/dag_oaHashNameMap.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/dagFileRW/textureNameResolver.h>

static OAHashNameMap<true> otherDag, otherDds;
static OAHashNameMap<true> destDdsPathNm, destDdsFileNm;
static Tab<int> destDdsFileId(midmem);
static bool fullReport = false;
static int skipCnt = 0;
static int copyCnt = 0;

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("Object Library to Asset converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

static int getLastSmbPos(const char *str, const char *smbs)
{
  const int len = strlen(str);
  const int smbLen = strlen(smbs);
  for (int i = len - 1; i > -1; i--)
    for (int j = 0; j < smbLen; j++)
      if (str[i] == smbs[j])
        return i;
  return 0;
}

static int getFileName(const char *path)
{
  for (int i = strlen(path) - 1; i > 0; i--)
  {
    if ((path[i] == '\\') || (path[i] == '/'))
      return i + 1;
  }
  return 0;
}

static int getStripFileName(const char *path)
{
  for (int i = strlen(path) - 1; i > 0; i--)
  {
    if (path[i] == '.')
      return i;
  }
  return 0;
}

static inline bool copyLinkForDag(const char *dag_file, const char *dest_path)
{
  class NoChangeResolver : public ITextureNameResolver
  {
  public:
    bool resolveTextureName(const char *src_name, String &out_str) override { return false; }
  } resolveCb;

  StaticGeometryContainer sgc;

  set_global_tex_name_resolver(&resolveCb);
  bool ret = sgc.loadFromDag(dag_file, NULL);
  set_global_tex_name_resolver(NULL);
  if (!ret)
    return false;

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
        localNm.addNameId(mats[j]->textures[l]->fileName.str());
      }
    }
  }

  for (int i = localNm.nameCount() - 1; i >= 0; i--)
  {
    const char *file = localNm.getName(i);
    file = &file[getFileName(file)];
    static char fileName[255];
    _snprintf(fileName, 255, "%.*s.dds", getStripFileName(file), file);

    static char dstF[255];
    _snprintf(dstF, 255, "%s%s", dest_path, fileName);

    if (destDdsPathNm.nameCount() == destDdsPathNm.addNameId(dstF))
    {
      const int cnt = destDdsFileNm.nameCount();
      const int id = destDdsFileNm.addNameId(fileName);
      if (cnt != id)
      {
        for (int i = destDdsFileId.size() - 1; i >= 0; i--)
          if (destDdsFileId[i] == id)
          {
            destDdsFileId[i] = -destDdsFileId[i] - 2;
            break;
          }
        destDdsFileId.push_back(-1);
      }
      else
        destDdsFileId.push_back(id);
    }
  }

  return true;
}

static inline void delBlkId(DataBlock *blk)
{
  blk->removeParam("id");
  for (int i = 0; i < blk->blockCount(); i++)
    delBlkId(blk->getBlock(i));
}

static void copyFile(const char *src_name, const char *dst_path, const char *dest_name)
{
  int pos = getLastSmbPos(src_name, ".");
  if (pos > 0)
  {
    const char *msk = &src_name[pos];

    const int slashPos = getLastSmbPos(src_name, "/\\");
    pos -= slashPos + 1;
    char *name = new char[pos + 1];
    strncpy(name, &src_name[slashPos + 1], pos);

    int id = 0;
    int filesCnt = 0;

    enum
    {
      TYPE_DDS,
      TYPE_DAG,

      TYPE_OTHER,
    };
    int type = TYPE_OTHER;

    if (stricmp(msk, ".dds") == 0)
    {
      type = TYPE_DDS;
      filesCnt = otherDds.nameCount();
      id = otherDds.addNameId(&src_name[slashPos + 1]);
    }
    else /*if (stricmp(msk, ".dag") == 0)*/
    {
      type = TYPE_DAG;
      filesCnt = otherDag.nameCount();
      id = otherDag.addNameId(&src_name[slashPos + 1]);
    }

    if (id != filesCnt)
    {
      if (fullReport)
        printf("\n");

      printf("[warning] duplicated reference: '%s' to '%s' [SKIPPED]\n", dest_name, &src_name[slashPos + 1]);

      if (fullReport)
        printf("\n");

      skipCnt++;
      return;
    }

    name[pos] = '\0';

    if (!dest_name)
      dest_name = name;

    if (type == TYPE_DAG)
      copyLinkForDag(src_name, dst_path);

    char *destName = new char[strlen(msk) + strlen(dest_name) + 1];
    sprintf(destName, "%s%s", dest_name, msk);

    if (fullReport)
      printf(" [copy] '%s%s' -> '%s'\t", name, msk, destName);

    char *dstF = new char[strlen(dst_path) + strlen(destName) + 1];
    sprintf(dstF, "%s%s", dst_path, destName);

    if (dag_copy_file(src_name, dstF, true))
    {
      if (fullReport)
        printf("[complete]\n");
      copyCnt++;
    }
    else
    {
      if (!fullReport)
        printf(" [copy] '%s' -> '%s'\t[ERROR]\n", src_name, dstF);
      else
        printf("[ERROR]\n");
    }

    delete[] dstF;
    delete[] name;
    delete[] destName;
  }
}

static inline const char *resolveMaterial(int id, const DataBlock *blk = NULL)
{
  if (id == -1)
    return NULL;

  if (blk->getInt("id", -1) == id)
    return blk->getStr("name", NULL);

  for (int i = blk->blockCount() - 1; i >= 0; i--)
  {
    const char *tmp = resolveMaterial(id, blk->getBlock(i));
    if (tmp)
      return tmp;
  }

  return NULL;
}

static inline bool procEntry(const DataBlock *library_blk, const DataBlock *blk, const char *now_in, const char *root_fld,
  const char *dst_fld, DataBlock *dest_blk)
{
  enum
  {
    PRS_CLASS_NONE,
    PRS_CLASS_TEXTURE,
    PRS_CLASS_PREFAB,
    PRS_CLASS_MATERIAL,
  };

  static int entryId = blk->getNameId("Entry");
  bool globCompare = false;
  bool compare = false;
  int classType = PRS_CLASS_NONE;

  for (int i = 0; i < blk->blockCount(); i++)
  {
    const DataBlock *subBlock = blk->getBlock(i);
    if (subBlock->getBlockNameId() != entryId)
      continue;

    const char *type = subBlock->getStr("type", "");
    const char *file = NULL;
    const char *name = subBlock->getStr("name", NULL);
    if (stricmp(type, "StaticObject") == 0)
    {
      classType = PRS_CLASS_PREFAB;
      file = subBlock->getStr("dag_file", NULL);
      type = "prefab";

      /*compare = subBlock->getBool( "clip_in_game"  , false) &&
                subBlock->getBool( "clip_in_editor", false) &&
                (subBlock->getReal("sample_size"   , 0 ) == 10.0) &&
                (subBlock->getInt( "lighting_type" , -1) == 0   ) &&
                (subBlock->getTm(  "tm", TMatrix::ZERO)  == TMatrix::IDENT);

      globCompare |= compare;*/
      compare = true;
      globCompare = true;
    }
    else if (stricmp(type, "Texture") == 0)
    {
      classType = PRS_CLASS_TEXTURE;
      file = subBlock->getStr("texture", NULL);

      /*compare = subBlock->getPoint2("tile", Point2(0, 0)) == Point2(1, 1);

      globCompare |= compare;*/
      compare = true;
      globCompare = true;
    }
    else if (stricmp(type, "Material") == 0)
    {
      classType = PRS_CLASS_MATERIAL;

      const char *matdir = "mat/";
      const char *matblkSuffix = ".mat.blk";
      char *matBlkF = new char[strlen(root_fld) + strlen(matdir) + strlen(name) + 1];
      char *destBlk = new char[strlen(dst_fld) + strlen(name) + strlen(matblkSuffix) + 1];
      sprintf(matBlkF, "%s%s%s", root_fld, matdir, name);
      sprintf(destBlk, "%s%s%s", dst_fld, name, matblkSuffix);

      DataBlock oldMat;
      oldMat.load(matBlkF);
      DataBlock newMat;

      static char texBuf[16];

      DataBlock *newtex = newMat.addBlock("textures");
      DataBlock *oldtex = oldMat.getBlockByName("textures");
      const int cnt = oldtex->blockCount();
      for (int i = 0; i < cnt; i++)
      {
        const int texId = oldtex->getBlock(i)->getInt("id", -1);
        const char *tmp = resolveMaterial(texId, library_blk);
        if (tmp)
        {
          sprintf(texBuf, "tex%d", i);
          newtex->addStr(texBuf, tmp);
        }
        else if (texId != -1)
          printf("[ERROR] can not resolve texture id %d in file '%s'\n", texId, matBlkF);
      }

      newMat.addNewBlock(oldMat.getBlockByName("defined_textures"));
      newMat.addNewBlock(oldMat.getBlockByName("params"));
      const int matCnt = newMat.blockCount();
      for (int i = 0; i < matCnt; i++)
      {
        DataBlock *subBlk = newMat.getBlock(i);
        const int subCnt = subBlk->blockCount();
        for (int j = 0; j < subCnt; j++)
        {
          DataBlock *subBlk2 = subBlk->getBlock(j);

          const int texId = subBlk2->getInt("tex_id", -1);
          const char *tmp = resolveMaterial(texId, library_blk);

          subBlk2->setStr("texture", tmp ? tmp : "");
          subBlk2->removeParam("tex_id");

          if (!tmp && (texId != -1))
            printf("[ERROR] can not resolve texture id %d in file '%s'\n", texId, matBlkF);
        }
      }

      const char *tmp = oldMat.getStr("class_name", NULL);
      if (tmp)
        newMat.setStr("class_name", tmp);
      tmp = oldMat.getStr("script", NULL);
      if (tmp)
        newMat.setStr("script", tmp);
      const int flags = oldMat.getInt("flags", -777);
      if (flags != -777)
        newMat.setInt("flags", flags);

      newMat.saveToTextFile(destBlk);

      delete[] matBlkF;
      delete[] destBlk;

      compare = true;
    }
    else
    {
      if (fullReport)
        printf("\n");
      printf("[warning] unsupported type: '%s'\n", type);
      if (fullReport)
        printf("\n");
    }

    if (!compare)
    {
      const char *blkSuffix = ".res.blk";
      char *blkFile = new char[strlen(dst_fld) + strlen(name) + strlen(blkSuffix) + 1];
      sprintf(blkFile, "%s%s%s", dst_fld, name, blkSuffix);
      DataBlock tmpBlk;
      tmpBlk.setParamsFrom(subBlock);
      tmpBlk.removeParam("name");
      tmpBlk.removeParam("thumb");
      tmpBlk.setStr("className", type);
      tmpBlk.removeParam("type");
      delBlkId(&tmpBlk);

      if (!tmpBlk.saveToTextFile(blkFile))
      {
        if (fullReport)
          printf("\n");
        printf("[mkBlk]: '%s'\t[ERROR]\n", blkFile);
        if (fullReport)
          printf("\n");
      }
      delete[] blkFile;
    }

    if (file)
    {
      char *srcFile = new char[strlen(file) + strlen(root_fld) + 1];
      sprintf(srcFile, "%s%s", root_fld, file);
      copyFile(srcFile, dst_fld, name);
      delete[] srcFile;
      continue;
    }
  }

  if (globCompare)
  {
    DataBlock *virtBlk = NULL;
    switch (classType)
    {
      case PRS_CLASS_PREFAB:
        virtBlk = dest_blk->addNewBlock("virtual_res_blk");

        virtBlk->setStr("find", "^(.*)\\.dag$");
        virtBlk->setStr("exclude", "\\.lod[0-9]+.dag$");

        virtBlk->setStr("className", "prefab");

        virtBlk = virtBlk->addBlock("contents");
        virtBlk->setBool("clip_in_game", true);
        virtBlk->setBool("clip_in_editor", true);
        virtBlk->setReal("sample_size", 10.0);
        virtBlk->setInt("lighting_type", 0);
        virtBlk->setTm("tm", TMatrix::IDENT);

        // break;

      case PRS_CLASS_TEXTURE:
        virtBlk = dest_blk->addNewBlock("virtual_res_blk");

        virtBlk->setStr("find", "^(.*)\\.dds$");

        virtBlk->setStr("className", "tex");

        break;

      case PRS_CLASS_MATERIAL:
      case PRS_CLASS_NONE: break;
    }
  }

  return globCompare;
}

static void copyOtherFiles(const char *src, Tab<int> &files, OAHashNameMap<true> &file_nm, OAHashNameMap<true> &path_nm,
  const char *mask)
{
  for (const alefind_t &ff : dd_find_iterator(String(260, "%s/%s", src, mask), DA_FILE))
  {
    const int cnt = file_nm.nameCount();
    const int id = file_nm.addNameId(ff.name);
    if (cnt == id)
      files.push_back(path_nm.addNameId(src));
    else
    {
      printf("[warning] duplicated file: '%s' in '%s' [SKIPPED]\n", ff.name, src);
      skipCnt++;
    }
  }

  for (const alefind_t &ff : dd_find_iterator(String(260, "%s/%s", src, "*.*"), DA_SUBDIR))
  {
    const char *dir = ff.name;
    if ((dir[0] == '.') || !(ff.attr & DA_SUBDIR))
      continue;

    char newDir[255];
    _snprintf(newDir, 255, "%s%s/", src, dir);

    copyOtherFiles(newDir, files, file_nm, path_nm, mask);
  }

  return;
}

static void createPath(const char *path)
{
  if (fullReport)
    printf("\n[mkdir]: '%s'\t", path);

  if (!dd_mkpath(path))
  {
    if (!fullReport)
      printf("[mkdir]: '%s'\t[ERROR]\n", path);
    else
      printf("[ERROR]\n\n");
  }
  else if (fullReport)
    printf("\n");
}

static void processOtherFiles(const char *src_path, const char *dest_path)
{
  G_ASSERT(destDdsPathNm.nameCount() == destDdsFileId.size());

  int duplicateLinkCnt = 0;

  static Tab<int> files(midmem);
  static OAHashNameMap<true> fileNm, pathNm;
  files.clear();
  pathNm.reset();
  fileNm.reset();

  copyOtherFiles(src_path, files, fileNm, pathNm, "*.dds");

  static char srcF[255];
  static char dstF[255];

  for (int i = destDdsFileId.size() - 1; i >= 0; i--)
  {
    bool copyInCommon = false;
    if (destDdsFileId[i] == -1)
      continue;
    else if (destDdsFileId[i] < -1)
    {
      copyInCommon = true;
      duplicateLinkCnt++;
      destDdsFileId[i] = -(destDdsFileId[i] + 2);
    }

    const char *fn = destDdsFileNm.getName(destDdsFileId[i]);
    if (otherDds.nameCount() != otherDds.addNameId(fn))
      continue;

    const int id = fileNm.getNameId(fn);
    if (id == -1)
      // error
      continue;

    const char *fp = pathNm.getName(files[id]);
    _snprintf(srcF, 255, "%s/%s", fp, fn);

    const char *df = NULL;
    static char destF[255];
    if (copyInCommon)
    {
      if (duplicateLinkCnt == 1)
      {
        _snprintf(destF, 255, "%scommonDds/", dest_path);
        createPath(destF);

        const char *blkName = ".folder.blk";
        sprintf(destF, "%scommonDds/%s", dest_path, blkName);

        DataBlock destBlk;
        destBlk.setBool("exported", true);

        DataBlock *virtBlk = destBlk.addBlock("virtual_res_blk");
        virtBlk->setStr("find", "^(.*)\\.dds$");

        virtBlk->setStr("className", "tex");

        destBlk.saveToTextFile(destF);
      }

      _snprintf(destF, 255, "%scommonDds/%s", dest_path, fn);
      df = destF;
    }
    else
      df = destDdsPathNm.getName(i);

    if (fullReport)
      printf(" [copy] '%s' -> '%s'\t", srcF, df);

    if (dag_copy_file(srcF, df, true))
    {
      if (fullReport)
        printf("[complete]\n");

      copyCnt++;
    }
    else
    {
      if (!fullReport)
        printf(" [copy] '%s' -> '%s'\t[ERROR]\n", srcF, df);
      else
        printf("[ERROR]\n");
    }
  }

  static char needMkPath[255];
  bool first = true;
  for (int i = files.size() - 1; i >= 0; i--)
  {
    if (otherDds.nameCount() == otherDds.addNameId(fileNm.getName(i)))
      _snprintf(dstF, 255, "%sunusedDds/%s", dest_path, fileNm.getName(i));
    else
      continue;

    _snprintf(srcF, 255, "%s/%s", pathNm.getName(files[i]), fileNm.getName(i));

    otherDds.addNameId(fileNm.getName(i));

    if (first)
    {
      _snprintf(needMkPath, 255, "%sunusedDds/", dest_path);
      createPath(needMkPath);

      first = false;
    }

    if (fullReport)
      printf(" [copy] '%s' -> '%s'\t", srcF, dstF);

    if (dag_copy_file(srcF, dstF, true))
    {
      if (fullReport)
        printf("[complete]\n");

      copyCnt++;
    }
    else
    {
      if (!fullReport)
        printf(" [copy] '%s' -> '%s'\t[ERROR]\n", srcF, dstF);
      else
        printf("[ERROR]\n");
    }
  }

  printf("duplicated path texture link count: %d\n", duplicateLinkCnt);
}

static void convert_library(const DataBlock *blk, const char *now_in, const char *root_fld, const char *dst_fld)
{
  static int groupId = blk->getNameId("Group");
  for (int i = 0; i < blk->blockCount(); i++)
  {
    const DataBlock *subBlock = blk->getBlock(i);
    if (subBlock->getBlockNameId() != groupId)
      continue;

    const char *tmp = subBlock->getStr("name", NULL);
    if (!tmp)
      DAG_FATAL("not found 'name' in block '%s'", subBlock->getBlockName());

    String destFld(256, "%s%s/", dst_fld, tmp);
    String newFld(256, "%s%s/", now_in, tmp);

    createPath(destFld.str());

    const char *blkName = ".folder.blk";
    char *destBlkF = new char[strlen(destFld) + strlen(blkName) + 1];
    sprintf(destBlkF, "%s%s", (char *)destFld, blkName);

    DataBlock destBlk;
    destBlk.setBool("exported", true); //???

    if (procEntry(blk, subBlock, newFld, root_fld, destFld, &destBlk))
      destBlk.saveToTextFile(destBlkF);
    delete[] destBlkF;

    convert_library(subBlock, (char *)newFld, root_fld, (char *)destFld);
  }
}

static void convert(const DataBlock *blk, const char *now_in, const char *root_fld, const char *dst_fld)
{
  createPath(dst_fld);

  char buf[260];
  _snprintf(buf, 260, "%s.folder.blk", dst_fld);

  DataBlock destBlk;
  destBlk.setBool("exported", true); //???

  if (procEntry(blk, blk, now_in, root_fld, dst_fld, &destBlk))
    destBlk.saveToTextFile(buf);

  convert_library(blk, now_in, root_fld, dst_fld);

  if (fullReport)
    printf("\n[copy linked dds]\n");

  processOtherFiles(root_fld, dst_fld);

  /*_snprintf(destFld, 255, "%s%s/", dst_fld, "otherDag");
  cnt = copyOtherFiles(now_in, destFld, "*.dag", otherDag);
  copyCnt += cnt;
  if (cnt > 0)
  {
    const char *blkName = ".folder.blk";
    char *destBlkF = new char[strlen(destFld) + strlen(blkName) + 1];
    sprintf(destBlkF, "%s%s", (char *) destFld, blkName);

    DataBlock destBlk;
    destBlk.setBool("exported", true);

    DataBlock *virtBlk = destBlk.addBlock("virtual_res_blk");
    virtBlk->setStr("find", "^(.*)\\.dag$");
    virtBlk->setStr("exclude", "\\.lod[0-9]+.dag$");

    virtBlk->setStr("className", "prefab");

    virtBlk = virtBlk->addBlock("contents");
    virtBlk->setBool("clip_in_game", true);
    virtBlk->setBool("clip_in_editor", true);
    virtBlk->setReal("sample_size", 10.0);
    virtBlk->setInt("lighting_type", 0);
    virtBlk->setTm("tm", TMatrix::IDENT);

    destBlk.saveToTextFile(destBlkF);
  }
  if (fullReport)
    printf("\ncopyed %d files.\n", cnt);*/
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

  String srcFile, destDir;

  for (int i = 0; i < __argc; i++)
  {
    if (__argv[i][0] == '-')
    {
      if (strPars(&__argv[i][1], "sf:", &srcFile))
        continue;
      if (strPars(&__argv[i][1], "d:", &destDir))
        continue;
      if (strPars(&__argv[i][1], "f"))
      {
        fullReport = true;
        continue;
      }
    }
  }

  print_title();

  const int destDirLen = strlen(destDir);
  if (((strlen(srcFile) < 1) || (destDirLen < 1)))
  {
    printf("usage: -sf:<source file> -d:<destination dir> [-f - full report]\n\n");
    return -1;
  }
  if ((destDir[destDirLen - 1] != '\\') && (destDir[destDirLen - 1] != '/'))
    destDir += '\\';

  const char *dirStr = srcFile;
  int len = strlen(dirStr);
  int point = -len;
  while (len > 0)
  {
    if ((dirStr[len] == '\\') || (dirStr[len] == '/'))
      break;
    if ((point < 0) && (dirStr[len] == '.'))
      point = len;
    len--;
  }
  if (point < 0)
    point = -point;

  String rootDir(256, "%.*s\\", len, dirStr);

  DataBlock blk(srcFile);
  if (!blk.isValid())
  {
    printf("blk is not found in '%s'", (char *)srcFile);
    return true;
  }

  DataBlock *rootBlk = blk.getBlockByName("root");
  if (!rootBlk)
  {
    printf("not found 'root' in blk file: '%s'", (char *)srcFile);
    return false;
  }
  if (strnicmp(blk.getStr("name", ""), &srcFile[++len], point - len) != 0)
  {
    printf("incorrect library blk file: '%s'", (char *)srcFile);
    return false;
  }

  convert(rootBlk, rootDir, rootDir, destDir);

  printf("\ncopyed files: %d  skipped files: %d\n", copyCnt, skipCnt);
  printf("...complete\n\n");

  return true;
}
