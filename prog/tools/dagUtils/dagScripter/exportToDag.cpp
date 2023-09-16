#include <math/dag_TMatrix.h>
#include "impBlkReader.h"
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_memIo.h>

static bool save_node(ImpBlkReader &rdr, FullFileSaveCB &dag, DataBlock &blk, int node_nid)
{
  int childId = 0;
  while (childId < blk.blockCount() && blk.getBlock(childId)->getBlockNameId() != node_nid)
    childId++;

  while (rdr.getBlockRest() > 0)
  {
    BEGIN_BLOCK;
    dag.beginTaggedBlock(rdr.getBlockTag());

    switch (rdr.getBlockTag())
    {
      case DAG_NODE_CHILDREN:
      {
        while (rdr.getBlockRest() > 0)
        {
          BEGIN_BLOCK;
          dag.beginTaggedBlock(rdr.getBlockTag());
          DataBlock *cb = blk.addNewBlock("node");
          if (rdr.getBlockTag() == DAG_NODE)
          {
            if (childId > blk.blockCount() - 1)
            {
              printf("[ERROR] BLK doesn't according to input DAG file");
              return false;
            }

            if (!save_node(rdr, dag, *blk.getBlock(childId), node_nid))
              return false;

            childId++;
          }
          else
          {
            printf("[ERROR]: error while parsing DAG file\n\n");
            return false;
          }
          END_BLOCK;
          dag.endBlock();
        }

        break;
      }

      case DAG_NODE_DATA:
      {
        DagNodeData nodeData;
        READ(&nodeData, sizeof(nodeData));

        unsigned &flags = nodeData.flg;
        flags = 0;

        if (blk.getBool("Renderable", false))
          flags |= DAG_NF_RENDERABLE;
        if (blk.getBool("Castshadow", false))
          flags |= DAG_NF_CASTSHADOW;
        if (blk.getBool("Collidable", false))
          flags |= DAG_NF_RCVSHADOW;

        dag.write(&nodeData, sizeof(nodeData));

        int l = rdr.getBlockRest();
        if (l > 255)
          l = 255;
        char str[256];
        READ(str, l);
        str[l] = 0;

        const char *nodeName = blk.getStr("name", NULL);
        if (nodeName)
          dag.write(nodeName, strlen(nodeName));
        else
          dag.write(str, l);

        break;
      }

      case DAG_NODE_TM:
      {
        TMatrix tm;
        READ(&tm, sizeof(tm));

        int param = blk.findParam("tm");
        if (param != -1 && blk.getParamType(param) == DataBlock::TYPE_MATRIX)
          tm = blk.getTm(param);

        dag.write(&tm, sizeof(tm));

        break;
      }

      case DAG_NODE_SCRIPT:
      {
        int l = rdr.getBlockLen();
        SimpleString script;
        script.allocBuffer(l + 1);
        if (rdr.tryRead(&script[0], l) != l)
        {
          rdr.read_err();
          return 0;
        }
        script[l] = 0;

        DataBlock *scr = blk.getBlockByName("script");
        if (scr && (scr->paramCount() || scr->blockCount()))
        {
          int param = scr->findParam("node_script_str");
          if (param != -1 && scr->getParamType(param) == DataBlock::TYPE_STRING)
            script = scr->getStr(param);
          else
          {
            DataBlock nblk;
            nblk.setParamsFrom(scr);

            DynamicMemGeneralSaveCB cwr(tmpmem);
            nblk.saveToTextStream(cwr);
            cwr.write("\0", 1);
            script = (char *)cwr.data();
          }

          dag.write(script, script.length());
        }
        else
        {
          while (l > 0 && strchr(" \t\r\n", script[l]))
            l--;
          if (l > 0 && script[l] != '\0')
          {
            script[l] = '\n';
            l++;
          }

          dag.write(&script[0], l);
        }

        break;
      }

      default:
      {
        int size = rdr.getBlockRest();
        void *blData = memalloc(size, tmpmem);

        READ(blData, size);
        dag.write(blData, size);

        memfree(blData, tmpmem);
        break;
      }
    }
    END_BLOCK;
    dag.endBlock();
  }
  return true;

err:
  return false;
}

bool save_scene(const char *in_fname, DataBlock &blk, const char *out_fname)
{
  ImpBlkReader rdr(in_fname);
  if (!rdr.fileHandle)
  {
    printf("[ERROR] can't read from %s\n", in_fname);
    return false;
  }

  if (stricmp(in_fname, out_fname) == 0)
  {
    printf("[ERROR] input file = output file!\n");
    return false;
  }

  FullFileSaveCB dag(out_fname, DF_WRITE);
  if (!dag.fileHandle)
  {
    printf("[ERROR] can't write to %s\n", out_fname);
    return false;
  }

  if (!rdr.seektoSafe(4))
  {
    rdr.read_err();
    printf("[ERROR] can't read from %s\n", in_fname);
    return false;
  }
  dag.writeInt(DAG_ID);

  BEGIN_BLOCK;
  dag.beginTaggedBlock(DAG_ID);
  int node_nid = blk.getNameId("node");

  for (; rdr.getBlockRest() > 0;)
  {
    BEGIN_BLOCK;
    dag.beginTaggedBlock(rdr.getBlockTag());

    if (rdr.getBlockTag() == DAG_NODE)
    {
      if (!save_node(rdr, dag, blk, node_nid))
      {
        printf("[ERROR] can't save node\n");
        return false;
      }
    }
    else
    {
      int size = rdr.getBlockRest();
      void *blData = memalloc(size, tmpmem);

      READ(blData, size);
      dag.write(blData, size);

      memfree(blData, tmpmem);
    }
    END_BLOCK;
    dag.endBlock();
  }
  END_BLOCK;
  dag.endBlock();
  return true;

err:
  printf("[ERROR] i/o error\n");
  return false;
}
