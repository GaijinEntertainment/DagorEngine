#include <math/dag_TMatrix.h>
#include <math/dag_mesh.h>
#include "impBlkReader.h"

static bool load_node(ImpBlkReader &rdr, DataBlock &blk, bool load_tm)
{
  while (rdr.getBlockRest() > 0)
  {
    BEGIN_BLOCK;
    switch (rdr.getBlockTag())
    {
      case DAG_NODE_CHILDREN:
        while (rdr.getBlockRest() > 0)
        {
          BEGIN_BLOCK;
          DataBlock *cb = blk.addNewBlock("node");
          if (rdr.getBlockTag() == DAG_NODE)
          {
            if (!load_node(rdr, *cb, load_tm))
              return false;
          }
          else
          {
            printf("[ERROR]: error while parsing DAG file\n\n");
            return false;
          }
          END_BLOCK;
        }
        break;

      case DAG_NODE_DATA:
      {
        DagNodeData nodeData;
        READ(&nodeData, sizeof(nodeData));

        unsigned &flags = nodeData.flg;

        blk.setBool("Renderable", flags & DAG_NF_RENDERABLE);
        blk.setBool("Castshadow", flags & DAG_NF_CASTSHADOW);
        blk.setBool("Collidable", flags & DAG_NF_RCVSHADOW);

        int l = rdr.getBlockRest();
        if (l > 255)
          l = 255;
        char str[256];
        READ(str, l);
        str[l] = 0;
        blk.setStr("name", str);
        break;
      }

      case DAG_NODE_TM:
      {
        TMatrix tm;
        READ(&tm, sizeof(tm));

        if (load_tm)
          blk.setTm("tm", tm);
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

        if (script.length())
        {
          DataBlock *scr = blk.addNewBlock("script");

          DataBlock sb;
          if (!dblk::load_text(sb, make_span_const(script), dblk::ReadFlag::ROBUST))
          {
            scr->addStr("node_script_str", script);
          }
          else
            scr->setFrom(&sb);
        }
        break;
      }

      case DAG_NODE_OBJ:
        while (rdr.getBlockRest() > 0)
        {
          BEGIN_BLOCK;
          if (rdr.getBlockTag() == DAG_OBJ_BIGMESH)
          {
            int vnum = 0, fnum = 0, numch = 0, bad_numch = 0;
            READ(&vnum, 4);
            rdr.seekrelSafe(vnum * 12);
            READ(&fnum, 4);
            rdr.seekrelSafe(fnum * sizeof(DagBigFace));
            READ(&numch, 1);

            String ch_details;
            for (int i = 0; i < numch; i++)
            {
              int tn = 0, co = 0, id = 0;
              READ(&tn, 4);
              READ(&co, 1);
              READ(&id, 1);

              if (id == 0)
                ch_details.aprintf(0, "vc0 %d,%d;  ", tn, fnum);
              else if (id >= 1 && id <= NUMMESHTCH)
                ch_details.aprintf(0, "tc%d %d,%d; ", id - 1, tn, fnum);
              else
              {
                ch_details.aprintf(0, "[id=%d %d*%d]; ", id, co, tn);
                bad_numch++;
              }
              rdr.seekrelSafe(co * 4 * tn + fnum * sizeof(TFace));
            }
            if (ch_details.suffix(";  "))
              erase_items(ch_details, ch_details.length() - 3, 3);
            else if (ch_details.suffix("; "))
              erase_items(ch_details, ch_details.length() - 2, 2);

            String bad_ch(0, " (%d bad)", bad_numch);
            blk.addStr("obj", String(128, "bigMesh %d verts, %d faces, %d%s channels (%s)", vnum, fnum, numch,
                                bad_numch > 0 ? bad_ch.str() : "", ch_details));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_MESH)
          {
            int vnum = 0, fnum = 0, numch = 0, bad_numch = 0;
            READ(&vnum, 2);
            rdr.seekrelSafe(vnum * 12);
            READ(&fnum, 2);
            rdr.seekrelSafe(fnum * sizeof(DagFace));
            READ(&numch, 1);

            String ch_details;
            for (int i = 0; i < numch; i++)
            {
              int tn = 0, co = 0, id = 0;
              READ(&tn, 2);
              READ(&co, 1);
              READ(&id, 1);

              if (id == 0)
                ch_details.aprintf(0, "vc0 %d,%d;  ", tn, fnum);
              else if (id >= 1 && id <= NUMMESHTCH)
                ch_details.aprintf(0, "tc%d %d,%d; ", id - 1, tn, fnum);
              else
              {
                ch_details.aprintf(0, "[id=%d %d*%d]; ", id, co, tn);
                bad_numch++;
              }
              rdr.seekrelSafe(co * 4 * tn + fnum * sizeof(DagTFace));
            }
            if (ch_details.suffix(";  "))
              erase_items(ch_details, ch_details.length() - 3, 3);
            else if (ch_details.suffix("; "))
              erase_items(ch_details, ch_details.length() - 2, 2);

            String bad_ch(0, " (%d bad)", bad_numch);
            blk.addStr("obj", String(128, "mesh %d verts, %d faces, %d%s channels (%s)", vnum, fnum, numch,
                                bad_numch > 0 ? bad_ch.str() : "", ch_details));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_LIGHT)
          {
            blk.addStr("obj", "light");
          }
          else if (rdr.getBlockTag() == DAG_OBJ_SPLINES)
          {
            int seg;
            READ(&seg, 0);
            blk.addStr("obj", String(64, "spline %d segs", seg));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_BONES)
          {
            int bnum = 0;
            READ(&bnum, 2);
            blk.addStr("obj", String(64, "bones: %d", bnum));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_MORPH)
          {
            int tnum = 0;
            READ(&tnum, 2);
            blk.addStr("obj", String(64, "morph: %d", tnum));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_FACEFLG)
          {
            int n = rdr.getBlockRest() / sizeof(uint8_t);
            blk.addStr("obj", String(64, "faceflg: %d", n));
          }
          else if (rdr.getBlockTag() == DAG_OBJ_NORMALS)
          {
            int nrmnum;
            READ(&nrmnum, 4);
            blk.addStr("obj", String(64, "vertnorms: %d", nrmnum));
          }
          END_BLOCK;
        }
        break;

      default:
      {
        rdr.seekrelSafe(rdr.getBlockRest());
        break;
      }
    }
    END_BLOCK;
  }
  return true;

err:
  return false;
}

bool load_scene(const char *fname, DataBlock &blk, bool load_tm)
{
  ImpBlkReader rdr(fname);
  if (!rdr.fileHandle)
  {
    printf("[ERROR] can't read from %s\n", fname);
    return false;
  }
  if (!rdr.seektoSafe(4))
  {
    rdr.read_err();
    printf("[ERROR] can't read from %s\n", fname);
    return false;
  }
  BEGIN_BLOCK;
  for (; rdr.getBlockRest() > 0;)
  {
    BEGIN_BLOCK;

    if (rdr.getBlockTag() == DAG_NODE)
    {
      int blStart = rdr.tell();
      rdr.seekrelSafe(rdr.getBlockRest());
      int blEnd = rdr.tell();
      rdr.seekto(blStart);

      if (!load_node(rdr, blk, load_tm))
        rdr.seekto(blEnd);
    }
    else
    {
      rdr.seekrelSafe(rdr.getBlockRest());
    }
    END_BLOCK;
  }
  END_BLOCK;
  return true;

err:
  printf("[ERROR] i/o error\n");
  return false;
}
