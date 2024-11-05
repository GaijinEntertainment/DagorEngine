// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_mainCon.inc.cpp>

#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <util/dag_bitArray.h>
#include <ioSys/dag_fileIo.h>
#include <libTools/util/strUtil.h>
#include <stdio.h>

//== imported from "dagor2\prog\tools\sharedInclude\sceneLighter\nlt_geomScene.h"
// -> lt.spt.bin
struct LtSamplePoint
{
  Point3 pos, norm;         // sample position and direction
  int skipObjId;            // object id to skip
  int skipLodId : 8;        // id of top LOD object to skip LOD group
  unsigned faceDuDvId : 24; // if of corresponding face tangent space basis
};

// -> vlt.bind.bin
struct VltBind
{
  int sampleIdx; // index in sample array
  int targetIdx; // index in vertex array
};
// -> lm.bind.bin
struct LmBind
{
  int targetIdx; // index in lightmap array
  union
  {
    int targetIdx2;
    struct
    {
      unsigned idxNum : 31;
      unsigned idxNumUsed : 1;
    };
  };
};


struct LtBindHeader
{
  int dataValidLabel;
  int sptNum; // total number of sample points to compute

  int vltNum; // total number of simple VLT bind recs
  int vssNum; // total number of VSS bind recs

  int lmStartSptIdx; // starting index for lightmap samples
  int lmRleRecNum;   // total number of lightmap bind recs

  int faceDuDvNum; // total number of

  int reserved[1];
};
//==


bool detect_client_point(const char *cl_fname, NameMap &clients, Tab<signed char> &client_idx)
{
  FILE *fp = fopen(cl_fname, "rt");
  char buf[512], *p;
  int idx, cnt = 0, rec_cnt = 0;

  if (!fp)
  {
    printf("ERROR: can't read %s\n", cl_fname);
    return false;
  }

  // find data start and samples count
  int sample_count = -1;
  while (fgets(buf, 512, fp))
  {
    if (strncmp(buf, "started pass ", 13) == 0)
    {
      p = strstr(buf + 13, "units done");
      if (p)
      {
        p = strchr(p + 10, '/');
        if (p)
          sample_count = atoi(p + 1);
      }
      break;
    }
  }

  if (sample_count == -1)
  {
    printf("ERROR: can't find start of pass in %s\n", cl_fname);
    return false;
  }

  printf("found pass start, %d samples\n", sample_count);
  client_idx.resize(sample_count);
  mem_set_ff(client_idx);

  // process all logs until end of pass
  while (fgets(buf, 512, fp))
  {
    if (strncmp(buf, "client ", 7) == 0)
    {
      p = buf + 7;
      while (!strchr(" \t\n\r", *p))
        p++;
      *p = 0;
      idx = clients.addNameId(buf + 7);
      p = strchr(p + 1, '[');
      if (!p)
        continue;

      int start = atoi(p + 1);
      p = strchr(p + 1, ',');
      int end = atoi(p + 1);

      // printf ( "client <%s>: range=[%d,%d]\n", clients.getName(idx), start, end );
      for (int i = start; i <= end; i++)
        client_idx[i] = idx;
      cnt += end - start + 1;
      rec_cnt++;
    }
    if (strncmp(buf, "finished pass", 13) == 0)
      break;
  }

  printf("processed %d records, %d samples\n", rec_cnt, cnt);

  return true;
}

bool samples_to_lmap(const char *spt_fname, dag::ConstSpan<signed char> client_idx, Tab<signed char> &lmap, int w, int h)
{
  FullFileLoadCB crd(spt_fname);
  if (!crd.fileHandle)
  {
    printf("ERROR: can't read %s\n", spt_fname);
    return false;
  }

  LtBindHeader hdr;

  memset(&hdr, 0, sizeof(hdr));
  crd.read(&hdr, sizeof(hdr));
  if (hdr.dataValidLabel != _MAKE4C('lbOK'))
    return false;
  if (crd.beginTaggedBlock() != _MAKE4C('SPT'))
  {
    crd.endBlock();
    return false;
  }
  crd.endBlock();

  // ltm first
  lmap.resize(w * h);
  memset(lmap.data(), 0xFE, data_size(lmap));

  if (crd.beginTaggedBlock() != _MAKE4C('LM'))
  {
    crd.endBlock();
    return false;
  }
  int spt_idx = hdr.lmStartSptIdx;
  for (int i = 0; i < hdr.lmRleRecNum; i++)
  {
    LmBind lmb;
    crd.read(&lmb, sizeof(lmb));
    if (!lmb.idxNumUsed)
    {
      lmap[lmb.targetIdx] = client_idx[spt_idx++];
      lmap[lmb.targetIdx2] = client_idx[spt_idx++];
    }
    else
      for (int j = 0; j < lmb.idxNum; j++)
        lmap[lmb.targetIdx + j] = client_idx[spt_idx++];
  }
  crd.endBlock();

  return true;
}
bool write_tga_mask(const char *fn, int idx, dag::ConstSpan<signed char> lmap, int w, int h, int m)
{
  TexPixel8a *pix = new TexPixel8a[w * h];
  for (int i = 0; i < w * h; i++)
  {
    pix[i].l = lmap[i] < 0 ? 255 - lmap[i] : lmap[i] * m;
    pix[i].a = lmap[i] == idx ? 255 : 0;
  }

  int ret = save_tga8a(fn, pix, w, h, w * 2);
  delete[] pix;
  return ret;
}

void showUsage()
{
  printf("\nUsage: detClientMask.exe <compute.log> <samplePoints.dat> <width> <height>\n");
  printf("  tool outputs Targa 8-bit images with name <client>-mask.tga for all lightmap\n");
}


int DagorWinMain(bool debugmode)
{
  printf("Client lightmap-mask detection tool v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n");

  // get options
  if (__argc < 5)
  {
    showUsage();
    return 0;
  }

  if (stricmp(__argv[1], "-h") == 0 || stricmp(__argv[1], "-H") == 0 || stricmp(__argv[1], "/?") == 0)
  {
    showUsage();
    return 0;
  }

  Tab<signed char> clientIdx(tmpmem), lmap(tmpmem);
  NameMap clients;
  int w = atoi(__argv[3]), h = atoi(__argv[4]), m;

  if (!detect_client_point(__argv[1], clients, clientIdx))
    return 13;

  if (!samples_to_lmap(__argv[2], clientIdx, lmap, w, h))
    return 13;

  m = 240 / clients.nameCount();
  printf("using scale=%d\n", m);
  for (int i = -2; i < clients.nameCount(); i++)
  {
    String fn(128, "%s-mask.tga", i == -2 ? "$white$" : (i == -1 ? "$none$" : clients.getName(i)));

    if (i >= 0)
      printf("%s - col %d\n", clients.getName(i), i * m);
    if (!write_tga_mask(fn, i, lmap, w, h, m))
      printf("ERROR: cannot write <%s>\n", (char *)fn);
  }
  return 0;
}
