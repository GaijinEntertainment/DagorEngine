#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/strUtil.h>
#include <math/dag_TMatrix.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_logSys.h>

#define PREFABS_FILE "StaticObjects.plugin.blk"
#define RESOBJ_FILE  "res_object_placer.plugin.blk"

static String developDir, libraryDir;

static FastNameMap libEntryNameMap;
static Tab<String> prefDags(midmem);
/*
static Tab<ObjectLibrary*> libs(midmem);

ObjectLibrary *getLibraryByName(const char *name)
{
  for (int i = 0; i < libs.size(); i++)
    if (strcmp(libs[i]->getFileName(), name)==0)
      return libs[i];

  ObjectLibrary *ol = new ObjectLibrary;
  libs.push_back(ol);
  return ol;
}
*/

extern int _ctl_main(int, char **) { return 0; }

void __cdecl ctrl_break_handler(int) { quit_game(0); }

int convertFile(const char *in_filename, DataBlock &blk)
{
  if (!in_filename || !*in_filename)
    return -1;

  int entCnt = 0, errCnt = 0;

  DataBlock oldblk;
  if (!oldblk.load(in_filename))
    return -1;

  int prefabsNameId = oldblk.getNameId("staticObject");
  int resourceNameId = oldblk.getNameId("object");

  String libEntryPath;

  for (int i = 0; i < oldblk.blockCount(); i++)
  {
    if (oldblk.getBlock(i)->getBlockNameId() == prefabsNameId)
    {
      DataBlock &cb = *oldblk.getBlock(i);
      DataBlock *entBlock = blk.addNewBlock("entity");
      entBlock->addStr("name", cb.getStr("name", ""));

      const char *objNameInLib = cb.getStr("objNameInLib", "");
      libEntryPath.printf(256, "%s::%s", cb.getStr("libPath", ""), objNameInLib);

      /*
            const char *dagFile = libEntryPath;

            int id = libEntryNameMap.addNameId(libEntryPath);

            if (id < prefDags.size())
              dagFile = prefDags[id];
            else
            {
              prefDags.resize(id + 1);

              String libPath = make_full_path(libraryDir, cb.getStr("libPath", ""));

              char location[256];
              dd_get_fname_location(location, cb.getStr("libPath", ""));

              ObjectLibrary *ol = getLibraryByName(libPath);

              if (ol && ol->openFile(libPath))
              {
                const ObjectLibrary::Entry *entry = ol->findEntry(objNameInLib);

                if (entry)
                {
                  ObjectLibrary::StatObjEntry *libEntry = entry->data.staticObject;
                  if (libEntry)
                  {
                    prefDags[id].printf(260, "./%s",
                      (char*)make_path_relative(make_full_path(libraryDir + String(location),
                        libEntry->dagFile), developDir));
                    dagFile = prefDags[id];
                  }
                }
                else
                  errCnt++;
              }
              else
                errCnt++;
            }
      */
      entBlock->addStr("entName", objNameInLib);
      entBlock->addBool("place_on_collision", false);
      // entBlock->addStr("entClass", "prefab");

      TMatrix tm = TMatrix::IDENT;
      DataBlock *tmBlk = cb.getBlockByName("tm");
      if (tmBlk)
      {
        tm.setcol(0, tmBlk->getPoint3("col0", Point3(1, 0, 0)));
        tm.setcol(1, tmBlk->getPoint3("col1", Point3(0, 1, 0)));
        tm.setcol(2, tmBlk->getPoint3("col2", Point3(0, 0, 1)));
        tm.setcol(3, tmBlk->getPoint3("col3", Point3(0, 0, 0)));
      }

      entBlock->addTm("tm", tm);
      entCnt++;
    }
    else if (oldblk.getBlock(i)->getBlockNameId() == resourceNameId)
    {
      DataBlock &cb = *oldblk.getBlock(i);
      DataBlock *entBlock = blk.addNewBlock("entity");
      entBlock->addStr("name", cb.getStr("numericName", ""));
      entBlock->addStr("entName", cb.getStr("name", ""));
      // entBlock->addStr("entClass", "resource");
      entBlock->addBool("place_on_collision", false);
      entBlock->addTm("tm", cb.getTm("matrix", TMatrix::IDENT));
      entCnt++;
    }
  }

  printf("processed: %s, entities: %d", (char *)in_filename, entCnt);
  if (errCnt)
    printf(", %d entities NOT FOUND", errCnt);
  printf("\n");
  return 0;
}


int convertDir(const char *in_dir, DataBlock &blk)
{
  alefind_t ff;
  String mask(in_dir);
  append_slash(mask);
  mask += "*";
  if (::dd_find_first(mask, DA_SUBDIR, &ff))
  {
    do
    {
      String filename(in_dir);
      append_slash(filename);
      filename += ff.name;

      if (ff.attr & DA_SUBDIR)
        convertDir(filename, blk);
      else
      {
        if (stricmp(ff.name, PREFABS_FILE) == 0 || stricmp(ff.name, RESOBJ_FILE) == 0)
          convertFile(filename, blk);
      }
    } while (::dd_find_next(&ff));
    ::dd_find_close(&ff);
  }

  return 0;
}


int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  printf("Prefabs and Resources to daEditor3 entities converter v2.1\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  if (__argc < 4)
  {
    printf("usage: <source level dir> <path to application.blk or dir with application.blk>"
           "<destination asset file>\n");
    terminate_process(0);
    return -1;
  }

  // libs.setmem(midmem);
  dag::set_allocator(prefDags, midmem);

  String appBlk(__argv[2]);
  if (!strstr(appBlk, "application.blk"))
  {
    append_slash(appBlk);
    appBlk += "application.blk";
  }

  DataBlock appblk;
  if (!appblk.load(appBlk))
  {
    printf("\nERROR: can't load blk from %s\n", (char *)appBlk);
    terminate_process(0);
    return -1;
  }

  DataBlock *cb = appblk.getBlockByName("SDK");
  if (!cb)
  {
    printf("\nERROR: can't read SDK paths from %s\n", (char *)appBlk);
    terminate_process(0);
    return -1;
  }

  location_from_path(appBlk);
  appBlk.pop_back();
  developDir = appBlk + cb->getStr("sdk_folder", "");
  libraryDir = appBlk + cb->getStr("lib_folder", "");

  DataBlock blk;
  convertDir(__argv[1], blk);

  blk.saveToTextFile(__argv[3]);

  // clear_all_ptr_items(libs);
  clear_and_shrink(prefDags);

  start_classic_debug_system(NULL);
  quit_game(0);
  return 0;
}
