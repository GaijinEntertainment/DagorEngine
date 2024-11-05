// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>

#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <libTools/dagFileRW/dagMatRemapUtil.h>

#include <stdio.h>

static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();

SimpleString refFile;

static bool write_dag(const char *dest, DagData &data, DataBlock *convData);

bool attach_scene(DagData &data, DagData &data_from);

int _cdecl main(int argc, char **argv)
{
  printf("DAG convert to instance format tool\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  if (argc < 3)
  {
    ::show_usage();
    return 1;
  }

  const char *inFile = argv[1];

  if (!inFile || !*inFile)
  {
    printf("FATAL: Empty input file name\n");
    return 1;
  }

  char *testChar = &argv[1][strlen(argv[1]) - 1];
  if (*testChar == '"')
    *testChar = 0;

  ::init_dagor(argv[0]);

  if (!::dd_file_exist(inFile))
  {
    printf("FATAL: file \"%s\" not exists", inFile);
    return 1;
  }

  printf("Load original file... ");
  debug_flush(true);
  DagData data, data2;
  if (!load_scene(inFile, data, true) || !load_scene(inFile, data2, true))
  {
    printf("\nFATAL: unable to find materials in file \"%s\"\n", inFile);
    return 1;
  }

  printf("OK\n");

  DataBlock convBlock;
  if (!convBlock.load(argv[2]))
  {
    printf("%s is invalid!\n", argv[2]);
    ::shutdown_dagor();
    return 0;
  }

  char outFile[512];
  strcpy(outFile, inFile);
  char *comma = strrchr(outFile, '.');
  if (comma)
    *comma = 0;
  refFile = outFile;
  strcat(outFile, ".dag");

  printf("Save instanced dag %s ... ", outFile);

  DagData instdata;
  if (!load_scene("inst.dag", instdata, true))
  {
    printf("\nFATAL: unable to find materials in file \"%s\"\n", "inst.dag");
    return 1;
  }

  attach_scene(data, instdata);

  DataBlock *instConv = convBlock.getBlockByName("InstRemapBlock");

  printf("Writing dag <%s>...", outFile);
  if (write_dag(outFile, data, instConv))
    printf("OK\n");
  else
    printf("FATAL ERROR writing file.\n");

  strcpy(outFile, inFile);
  comma = strrchr(outFile, '.');
  if (comma)
    *comma = 0;
  strcat(outFile, "_ref.dag");

  printf("Save reference dag %s ... ", outFile);

  DataBlock *refConv = convBlock.getBlockByName("RefRemapBlock");

  printf("Writing dag <%s>...", outFile);
  if (write_dag(outFile, data2, refConv))
    printf("OK\n");
  else
    printf("FATAL ERROR writing file.\n");

  ::shutdown_dagor();
  return 0;
}


//==============================================================================
static void init_dagor(const char *base_path)
{
  ::dd_init_local_conv();
  ::dd_add_base_path("");
}


//==============================================================================
static void shutdown_dagor() {}


//==============================================================================
static void show_usage()
{
  printf("usage: dag_convinst <file name> <convert file path>\n");
  printf("where\n");
  printf("<file name>\t\tpath to DAG file\n");
  printf("<convert file path>\tpath to convert format file (blk)\n");
}


//==============================================================================

static void replaceString(SimpleString &string, const char *strfind, const char *strrepl)
{
  if (strlen(strfind) > 0)
  {
    const char *strin = strstr(string, strfind);
    if (strin)
    {
      String str;
      str.setSubStr(string, strin);
      str += strrepl;
      str += strin + strlen(strfind);
      string = str;
    }
  }
}


static bool write_dag_nodes(FullFileSaveCB &dag, DagData::Node &node, DataBlock *convData)
{
  dag.beginTaggedBlock(DAG_NODE);
  for (int i = 0; i < node.blocks.size(); ++i)
  {
    DagData::Node::NodeBlock &block = node.blocks[i];
    dag.beginTaggedBlock(block.blockType);
    switch (block.blockType)
    {
      case DAG_NODE_DATA:
      {
        DagNodeData nodedata = block.data;
        DataBlock *nodeBlockData = convData->getBlockByName("NodeBlockData");
        if (nodeBlockData && nodeBlockData->paramExists("flgin") && nodedata.flg == nodeBlockData->getInt("flgin", 0))
          nodedata.flg = nodeBlockData->getInt("flgout", 0);
        nodedata.cnum = node.children.size();
        dag.write(&nodedata, sizeof(nodedata));
        dag.write((const char *)block.name, block.name.length());
        break;
      }
      case DAG_NODE_TM:
      {
        dag.write(&block.nodeTm, sizeof(block.nodeTm));
        break;
      }
      case DAG_NODE_SCRIPT:
      {
        SimpleString script(block.script);
        for (int i = 0; i < convData->blockCount(); i++)
        {
          DataBlock *blkScript = convData->getBlock(i);
          if (strcmp(blkScript->getBlockName(), "NodeBlockScript"))
            continue;
          const char *scriptin = blkScript->getStr("scriptin", "");
          if (strncmp(scriptin, "linked_resource:t=", 18) == 0)
          {
            char newstr[512];
            sprintf(newstr, blkScript->getStr("scriptout", ""), (const char *)refFile);
            replaceString(script, scriptin, newstr);
          }
          else
            replaceString(script, scriptin, blkScript->getStr("scriptout", ""));
        }
        dag.write((const char *)script, script.length());
        break;
      }
      case DAG_NODE_MATER:
      {
        dag.writeTabData(block.mater);
        break;
      }
      default:
      {
        dag.write(block.block.data, block.block.end - block.block.start);
        break;
      }
    }
    dag.endBlock();
  }
  if (node.children.size())
  {
    dag.beginTaggedBlock(DAG_NODE_CHILDREN);
    for (int i = 0; i < node.children.size(); ++i)
    {
      DEBUG_CTX("write children dag node");
      write_dag_nodes(dag, node.children[i], convData);
    }
    dag.endBlock();
  }


  dag.endBlock();
  return true;
}

static bool write_dag(const char *dest, DagData &data, DataBlock *convData)
{
  FullFileSaveCB dag(dest, DF_WRITE);

  if (!dag.fileHandle)
    return false;
  dag.writeInt(DAG_ID);
  dag.beginTaggedBlock(DAG_ID);

  dag.beginTaggedBlock(DAG_TEXTURES);
  dag.writeIntP<2>(data.texlist.size());
  for (int i = 0; i < data.texlist.size(); ++i)
  {
    DEBUG_CTX("write texture");

    SimpleString textpath(data.texlist[i]);

    DataBlock *textblk = convData->getBlockByName("Texture");
    if (textblk)
      replaceString(textpath, textblk->getStr("pathin", ""), textblk->getStr("pathout", ""));

    const unsigned char len = textpath.length();
    dag.writeIntP<1>(len);
    dag.write((const char *)textpath, len);
  }
  dag.endBlock();

  for (int i = 0; i < data.matlist.size(); ++i)
  {
    DEBUG_CTX("write material");
    dag.beginTaggedBlock(DAG_MATER);
    unsigned char len = data.matlist[i].name.length();
    dag.writeIntP<1>(len);
    dag.write((const char *)data.matlist[i].name, len);
    dag.write(&data.matlist[i].mater, sizeof(DagMater));

    SimpleString classname(data.matlist[i].classname);
    for (int iblk = 0; iblk < convData->blockCount(); iblk++)
    {
      DataBlock *material = convData->getBlock(iblk);
      if (strcmp(material->getBlockName(), "Material"))
        continue;
      replaceString(classname, material->getStr("classin", ""), material->getStr("classout", ""));
    }
    len = classname.length();
    dag.writeIntP<1>(len);
    dag.write((const char *)classname, len);
    dag.write((const char *)data.matlist[i].script, data.matlist[i].script.length());
    dag.endBlock();
  }

  data.nodes[0].blocks[0].data.id = -1;
  data.nodes[0].blocks[0].data.flg = -1;

  for (int i = 0; i < data.nodes.size(); ++i)
  {
    DEBUG_CTX("write dag node");
    write_dag_nodes(dag, data.nodes[i], convData);
  }

  for (int i = 0; i < data.blocks.size(); ++i)
  {
    dag.beginTaggedBlock(data.blocks[i].tag);
    dag.write(data.blocks[i].data, data.blocks[i].end - data.blocks[i].start);
    dag.endBlock();
  }

  dag.endBlock();

  return true;
}

bool attach_scene(DagData &data, DagData &data_from)
{
  data.matlist.push_back(data_from.matlist[0]);
  data.nodes[0].children.push_back(data_from.nodes[0].children[0]);

  return true;
}