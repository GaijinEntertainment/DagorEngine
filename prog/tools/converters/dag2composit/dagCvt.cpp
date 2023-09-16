#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>

#define DAG_FILE_MASK "*.dag"

struct DagReourceRec
{
  String name;
  TMatrix tm;
};

void __cdecl ctrl_break_handler(int) { quit_game(0); }


void getNode(Node *node, Tab<DagReourceRec *> &res_tab)
{
  if (!node)
    return;

  bool needRem = false;
  if (node->script)
  {
    DataBlock blk;
    dblk::load_text(blk, make_span_const(node->script), dblk::ReadFlag::ROBUST);

    const char *linkedRes = blk.getStr("linked_resource", "");
    if (strlen(linkedRes) > 0)
    {
      DagReourceRec *rec = new DagReourceRec;
      rec->name = linkedRes;
      rec->tm = node->wtm;
      res_tab.push_back(rec);

      bool renderable = blk.getBool("renderable", false);
      bool collidable = blk.getBool("collidable", false);

      if (!renderable && !collidable)
        needRem = true;
    }
  }

  for (int i = 0; i < node->child.size(); ++i)
    getNode(node->child[i], res_tab);

  if (needRem)
  {
    Node *prt = node->parent;
    if (prt)
    {
      for (int i = 0; i < prt->child.size(); i++)
        if (prt->child[i] == node)
        {
          erase_ptr_items(prt->child, i, 1);
          break;
        }
    }
  }
}


int convertFile(const char *in_filename, const char *out_filename = NULL)
{
  if (!in_filename || !*in_filename)
    return -1;

  // loading AScene
  AScene sc;
  if (!::load_ascene(in_filename, sc, 0, false))
    return -1;

  Tab<DagReourceRec *> dagRes(tmpmem);

  // getting dag linked resources
  sc.root->calc_wtm();
  getNode(sc.root, dagRes);

  if (!dagRes.size())
    return 0;

  // getting out filename without extension
  String outNameWithoutExt(out_filename ? out_filename : in_filename);
  remove_trailing_string(outNameWithoutExt, ".dag");

  // filling out DataBlock
  DataBlock blk;

  if (!sc.root->child.size() && dagRes.size() == 1) // there is no geometry and one resource in DAG file
  {
    blk.setStr("className", "resource");
    blk.addStr("resName", dagRes[0]->name);

    blk.saveToTextFile(outNameWithoutExt + ".blk");
    return 0;
  }

  // filling composit BLK
  blk.setStr("className", "composit");

  for (int i = 0; i < dagRes.size(); i++)
  {
    DataBlock *resBlk = blk.addNewBlock("Resource");
    resBlk->addStr("res", dagRes[i]->name);
    resBlk->addTm("tm", dagRes[i]->tm);
  }

  clear_all_ptr_items(dagRes);

  // exporting geometry
  if (sc.root->child.size())
  {
    DagExporter::exportGeometry(outNameWithoutExt + ".comp.dag", sc.root);

    DataBlock *prefBlk = blk.addNewBlock("prefab");
    prefBlk->addStr("res", String(128, "./%s", dd_get_fname(outNameWithoutExt + ".comp.dag")));
  }

  blk.saveToTextFile(outNameWithoutExt + ".blk");
  return 0;
}


int convertDir(const char *in_dir, const char *out_dir)
{
  alefind_t ff;
  String mask(in_dir);
  append_slash(mask);
  mask += DAG_FILE_MASK;
  if (::dd_find_first(mask, DA_FILE, &ff))
  {
    String outDir(out_dir);
    append_slash(outDir);
    dd_mkpath(outDir);
    do
    {
      String filename(in_dir);
      append_slash(filename);
      filename += ff.name;

      String outfname(outDir + ff.name);
      if (convertFile(filename, outfname) == 0)
        printf("processed: %s\n", (char *)filename);
    } while (::dd_find_next(&ff));
    ::dd_find_close(&ff);
  }

  return 0;
}


int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  printf("DAG to daEditor3 composit converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  if (__argc < 3)
  {
    printf("usage #1: <source dir> <destination dir>\n");
    printf("usage #2: -f <source file>\n\n");
    return -1;
  }

  if (stricmp(__argv[1], "-f") == 0)
  {
    if (convertFile(__argv[2]) == 0)
      printf("processed: %s\n", __argv[2]);

    return 0;
  }

  return convertDir(__argv[1], __argv[2]);
}
