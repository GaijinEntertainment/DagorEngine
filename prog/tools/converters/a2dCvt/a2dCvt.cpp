#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_fileIo.h>
#include "old_animChannels.h"
#include <ioSys/dag_chainedMemIo.h>
#include <util/dag_globDef.h>
#include <util/dag_oaHashNameMap.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>


void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title()
{
  printf("a2d V150 to V200 converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

using namespace AnimV20;

// struct declaration
struct DataTypeHeaderCvt
{
  unsigned dataType;
  unsigned offsetInKeyPool;
  unsigned offsetInTimePool;
  unsigned channelNum;
};

struct AnimDataHeaderCvt
{
  unsigned label;    // MAKE4C('A','N','I','M')
  unsigned ver;      // 0x200
  unsigned hdrSize;  // =sizeof(AnimDataHeader)
  unsigned reserved; // 0x00000000
};
struct AnimDataHeaderV200Cvt
{
  unsigned namePoolSize;      // size of name pool, in bytes
  unsigned timeNum;           // number of entries in time pool
  unsigned keyPoolSize;       // size of key pool, in bytes
  unsigned totalAnimLabelNum; // number of keys in note track (time labels)
  unsigned dataTypeNum;       // number of data type records
};
//

template <class T>
static void writeChannelDataInGen(dag::ConstSpan<AnimDataChan<T>> channels, DataTypeHeaderCvt &dataTypeHeader, Tab<int> &timeOffset,
  Tab<int> &keyOffset, NameMap &nameMap, Tab<int> nameOffset, int offsetInd, IGenSave &cwr, unsigned DataType)
{
  dataTypeHeader.dataType = DataType;
  dataTypeHeader.offsetInKeyPool = keyOffset[offsetInd];
  dataTypeHeader.offsetInTimePool = timeOffset[offsetInd];
  dataTypeHeader.channelNum = channels.size();

  cwr.write(&dataTypeHeader, sizeof(dataTypeHeader));

  for (int i = 0; i < channels.size(); i++)
  {
    cwr.write(&channels[i].channelType, sizeof(unsigned));
    cwr.write(&channels[i].nodeNum, sizeof(unsigned));

    Tab<unsigned> keyNums(tmpmem);
    Tab<int> nodeNameIds(tmpmem);
    Tab<float> nodeWeights(tmpmem);

    keyNums.resize(channels[i].nodeNum);
    nodeNameIds.resize(channels[i].nodeNum);
    nodeWeights.resize(channels[i].nodeNum);

    for (int k = 0; k < channels[i].nodeNum; k++)
    {
      keyNums[k] = channels[i].nodeAnim[k].keyNum;
      nodeNameIds[k] = nameOffset[nameMap.getNameId(channels[i].nodeName[k])];
      nodeWeights[k] = channels[i].nodeWt[k];
    }

    cwr.write(&keyNums[0], sizeof(unsigned) * channels[i].nodeNum);
    cwr.write(&nodeNameIds[0], sizeof(int) * channels[i].nodeNum);
    cwr.write(&nodeWeights[0], sizeof(float) * channels[i].nodeNum);
  }
}

template <class T, class Type>
static bool writeChannelDataInMem(dag::ConstSpan<AnimDataChan<T>> channels, NameMap &nameMap, MemorySaveCB &mcbTimePool,
  MemorySaveCB &mcbKeyPool)
{
  for (int i = 0; i < channels.size(); i++)
    for (int j = 0; j < channels[i].nodeNum; j++)
    {
      nameMap.addNameId(channels[i].nodeName[j]);

      int size = channels[i].nodeAnim[j].keyNum;
      mcbTimePool.write(channels[i].nodeAnim[j].keyTime, sizeof(int) * size);
      mcbKeyPool.write(channels[i].nodeAnim[j].key, sizeof(Type) * size);
    }
  return channels.size();
}

static void saveToSaveCB(MemoryChainedData *mem, IGenSave &cwr)
{
  while (mem)
  {
    if (!mem->used)
      break;
    cwr.write(mem->data, mem->used);
    mem = mem->next;
  }
}

static bool saveV200(IGenSave &cwr, AnimData *animData)
{
  AnimDataHeaderCvt hdr;

  hdr.label = MAKE4C('A', 'N', 'I', 'M');
  hdr.ver = 0x200;
  hdr.hdrSize = sizeof(AnimDataHeaderCvt) + sizeof(AnimDataHeaderV200Cvt);
  hdr.reserved = 0x00000000;

  cwr.write(&hdr, sizeof(hdr));

  AnimDataHeaderV200Cvt hdrV200;

  MemorySaveCB mcbNamePool;
  MemorySaveCB mcbTimePool;
  MemorySaveCB mcbKeyPool;

  NameMap nameMap;
  Tab<int> nameOffset(tmpmem);
  Tab<int> timeOffset(tmpmem);
  Tab<int> keyOffset(tmpmem);

  int dataTypeNum = 0;

  timeOffset.push_back(mcbTimePool.tell() / 4);
  keyOffset.push_back(mcbKeyPool.tell());

  if (writeChannelDataInMem<AnimChanPoint3, AnimKeyPoint3>(animData->animDataChannelsPoint3, nameMap, mcbTimePool, mcbKeyPool))
    dataTypeNum++;

  timeOffset.push_back(mcbTimePool.tell() / 4);
  keyOffset.push_back(mcbKeyPool.tell());

  if (writeChannelDataInMem<AnimChanQuat, AnimKeyQuat>(animData->animDataChannelsQuat, nameMap, mcbTimePool, mcbKeyPool))
    dataTypeNum++;

  timeOffset.push_back(mcbTimePool.tell() / 4);
  keyOffset.push_back(mcbKeyPool.tell());

  if (writeChannelDataInMem<AnimChanReal, AnimKeyReal>(animData->animDataChannelsReal, nameMap, mcbTimePool, mcbKeyPool))
    dataTypeNum++;


  for (int i = 0; i < animData->noteTrack.num; i++)
    nameMap.addNameId(animData->noteTrack.pool[i].name);

  nameOffset.resize(nameMap.nameCount());

  for (int i = 0; i < nameMap.nameCount(); i++)
  {
    nameOffset[i] = mcbNamePool.tell();
    mcbNamePool.write(nameMap.getName(i), strlen(nameMap.getName(i)) + 1);
  }

  MemorySaveCB noteTrackPool;
  for (int i = 0; i < animData->noteTrack.num; i++)
  {
    char *poolName = ((char *)animData->noteTrack.pool[i].name);
    int offset = nameOffset[nameMap.getNameId(poolName)];

    noteTrackPool.writeInt(offset);
    noteTrackPool.writeInt(animData->noteTrack.pool[i].time);
  }


  hdrV200.namePoolSize = mcbNamePool.getMem()->calcTotalUsedSize();
  hdrV200.timeNum = mcbTimePool.tell() / 4;
  hdrV200.keyPoolSize = mcbKeyPool.getMem()->calcTotalUsedSize();
  hdrV200.totalAnimLabelNum = animData->noteTrack.num;
  hdrV200.dataTypeNum = dataTypeNum;

  cwr.write(&hdrV200, sizeof(AnimDataHeaderV200Cvt));

  saveToSaveCB(mcbNamePool.takeMem(), cwr);
  saveToSaveCB(mcbTimePool.takeMem(), cwr);
  saveToSaveCB(mcbKeyPool.takeMem(), cwr);

  saveToSaveCB(noteTrackPool.takeMem(), cwr);


  DataTypeHeaderCvt dataTypeHeader;

  if (animData->animDataChannelsPoint3.size())
    writeChannelDataInGen(make_span_const(animData->animDataChannelsPoint3), dataTypeHeader, timeOffset, keyOffset, nameMap,
      nameOffset, 0, cwr, DATATYPE_POINT3);
  if (animData->animDataChannelsQuat.size())
    writeChannelDataInGen(make_span_const(animData->animDataChannelsQuat), dataTypeHeader, timeOffset, keyOffset, nameMap, nameOffset,
      1, cwr, DATATYPE_QUAT);
  if (animData->animDataChannelsReal.size())
    writeChannelDataInGen(make_span_const(animData->animDataChannelsReal), dataTypeHeader, timeOffset, keyOffset, nameMap, nameOffset,
      2, cwr, DATATYPE_REAL);
  return true;
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

static unsigned getAnimDataVer(IGenLoad &cb)
{
  AnimDataHeaderCvt hdr;
  cb.read(&hdr, sizeof(hdr));
  cb.seekto(0);
  if (hdr.label != MAKE4C('A', 'N', 'I', 'M'))
  {
    printf("unrecognized label\n");
    return 0;
  }
  return hdr.ver;
}

static bool convertFile(char *source, const char *dest)
{
  dd_simplify_fname_c(source);

  FullFileLoadCB crd(source);

  if (!crd.fileHandle)
  {
    printf("can't open source file: %s\n", (char *)source);
    return false;
  }

  unsigned ver = getAnimDataVer(crd);
  // printf("converting file: %s\tver: 0x%03X\n", source, ver);
  printf(" [ver: 0x%03X]", ver);
  if ((ver >= 0x200) || (ver == 0))
  {
    printf("[SKIP] file: %s\n", source);
    return false;
  }
  printf("[Convert] file: %s\n", source);

  AnimData *tmpAnimData = new AnimData;

  if (tmpAnimData->load(crd, tmpmem))
  {
    crd.close();

    FullFileSaveCB cwrI(dest);
    if (!cwrI.fileHandle)
    {
      printf("can't open destination file: %s\n", (char *)dest);
      return false;
    }

    saveV200(cwrI, tmpAnimData);
  }
  else
  {
    printf("error loading AnimData\n");
    return false;
  }
  return true;
}

static void getAllSubDir(const char *path, Tab<SimpleString> *dirList)
{
  dirList->push_back() = path;
  alefind_t ff;
  String mask(path);
  mask += "*.*";
  if (dd_find_first(mask, DA_SUBDIR, &ff))
  {
    do
    {
      if (ff.attr & DA_SUBDIR)
      {
        String dirname(path);
        dirname += ff.name;
        if (!::dd_dir_exist(dirname))
          continue;

        dirname += "/";
        getAllSubDir(dirname, dirList);
      }
    } while (dd_find_next(&ff));
    dd_find_close(&ff);
  }
}

static int scanAndCvt(const char *srcDir, const char *destDir)
{
  alefind_t ff;
  if (!srcDir || !destDir)
    return -1;

  dd_mkdir(destDir);
  Tab<SimpleString> files(tmpmem);

  for (bool scan = dd_find_first(String(260, "%s/*.a2d", srcDir), 0, &ff); scan; scan = dd_find_next(&ff))
    files.push_back() = ff.name;
  dd_find_close(&ff);

  if (!files.size())
  {
    printf(" [no files to convert]\n");
    return 0;
  }

  int filesDecoded = 0;
  for (int i = 0; i < files.size(); i++)
    if (convertFile(String(260, "%s/%s", srcDir, (char *)files[i]), String(60, "%s/%s", destDir, (char *)files[i])))
      filesDecoded++;

  return filesDecoded;
}

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  String source, dest, srcDir, destDir;
  bool scanSubFolders = false;

  for (int i = 0; i < __argc; i++)
  {
    if (__argv[i][0] == '-')
    {
      if (strPars(&__argv[i][1], "s:", &source))
        continue;
      if (strPars(&__argv[i][1], "d:", &dest))
        continue;
      if (strPars(&__argv[i][1], "sd:", &srcDir))
        continue;
      if (strPars(&__argv[i][1], "dd:", &destDir))
        continue;
      if (strPars(&__argv[i][1], "sif"))
      {
        scanSubFolders = true;
        continue;
      }
    }
  }

  print_title();

  if (((strlen(source) < 1) || (strlen(dest) < 1)) && (strlen(srcDir) < 1))
  {
    printf("usage: -s:<source> -d:<destination> | -sd:<source dir> [-dd:<destination dir>] [-sif]\n\n sif - scan include folders\n");
    return -1;
  }

  int convFilesC = 0;
  if (strlen(srcDir) > 0)
  {
    if (scanSubFolders)
    {
      Tab<SimpleString> dirList(tmpmem);
      printf("Creating folder list...");
      getAllSubDir(srcDir, &dirList);
      printf("complete\n");
      if (dirList.size())
      {
        printf("\n");
        for (int i = 0; i < dirList.size(); i++)
        {
          printf("\nchecking dir: <%s>\n", (char *)dirList[i]);
          convFilesC += scanAndCvt(dirList[i], strlen(destDir) < 1 ? dirList[i].str() : destDir);
        }
      }
    }
    else
      convFilesC = scanAndCvt(srcDir, strlen(destDir) < 1 ? srcDir : destDir);
  }
  else
    convertFile(source, dest);

  printf("\n...completed...\n\n converted  %d  files", convFilesC);
  return true;
}
