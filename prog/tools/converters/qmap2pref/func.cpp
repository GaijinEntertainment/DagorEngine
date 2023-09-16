#include "func.h"

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>

#include <libTools/util/strUtil.h>
#include <regExp/regExp.h>

#include <ioSys/dag_dataBlock.h>
#include <memory/dag_mem.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>

#include <stdio.h>
#include <ctype.h>


#define POSITION_SCALE 0.0254


//==================================================================================================
void show_usage()
{
  printf("\nUsage: qmap2pref <map file> <lib path> [output path] [options]\n");
  printf("where\n");
  printf("map file\tQuake map file\n");
  printf("lib path\tpath to objects library file relative to develop/library (or equivalent) "
         "folder\n");
  printf("output path\tPath to save result staticobjects.plugin.blk. By default it saves "
         "in the same folder with Quake map file\n");
  printf("options\tadditional options:\n");
  printf("  -d\tshow debug information (off by default):\n");
  printf("  -c\tdo not check entity type (off by default):\n");
  printf("\nexamle:\n");
  printf("qmap2pref d:/dagor2/temp/qmap.map objects/objects.blk\n");
}


//==================================================================================================
void init_dagor(const char *base_path)
{
  ::dd_init_local_conv();

  String dir(base_path);
  ::location_from_path(dir);
  ::append_slash(dir);

  ::dd_add_base_path("");
  ::dd_add_base_path(dir);
}


//==================================================================================================
bool create_prefabs_list(const char *map, Tab<Prefab> &prefabs, bool check_models)
{
  RegExp entityExp;
  RegExp angleExp;
  RegExp posExp;
  RegExp nameExp;
  RegExp classExp;

  if (!entityExp.compile("\\/\\/ entity [0-9]*"))
    return false;

  if (!angleExp.compile("\"angle\""))
    return false;

  if (!posExp.compile("\"origin\""))
    return false;

  if (!nameExp.compile("\"model\""))
    return false;

  if (!classExp.compile("\"classname\""))
    return false;

  String content;
  String nameBuff;

  while (entityExp.test(map))
  {
    const RegExp::Match *match = entityExp.pmatch + 0;
    map += match->rm_eo;

    const char *start = strchr(map, '{');
    if (start)
    {
      while (*start && isspace(*(++start)))
        ;

      const char *end = strchr(start, '}');
      if (end)
      {
        Prefab object;

        content.setSubStr(start, end);

        if (angleExp.test(content))
        {
          match = angleExp.pmatch + 0;
          sscanf(content.data() + match->rm_so, "\"angle\" \"%f\"", &object.angle);
          object.angle = -DegToRad(object.angle);
        }

        if (posExp.test(content))
        {
          match = posExp.pmatch + 0;
          sscanf(content.data() + match->rm_so, "\"origin\" \"%f %f %f\"", &object.pos.x, &object.pos.z, &object.pos.y);
          object.pos.z = -object.pos.z;
          object.pos *= POSITION_SCALE;
        }

        if (nameExp.test(content))
        {
          nameBuff.resize(content.length());
          mem_set_0(nameBuff);

          match = nameExp.pmatch + 0;
          sscanf(content.data() + match->rm_so, "\"model\" \"%s\"", nameBuff.data());

          object.name = ::get_file_name_wo_ext(nameBuff);
        }

        bool doPush = true;

        if (check_models)
        {
          if (classExp.test(content))
          {
            nameBuff.resize(content.length());
            mem_set_0(nameBuff);

            match = classExp.pmatch + 0;
            sscanf(content.data() + match->rm_so, "\"classname\" \"%s\"", nameBuff.data());

            if (stricmp(nameBuff, "misc_model\"") && stricmp(nameBuff, "misc_model") && stricmp(nameBuff, "\"misc_model") &&
                stricmp(nameBuff, "\"misc_model\""))
              doPush = false;
          }
          else
          {
            printf("error: couldn't check entity type\n");
            return false;
          }
        }

        if (doPush)
          prefabs.push_back(object);
      }
    }
  }

  return true;
}


//==================================================================================================
bool generate_file(const char *path, const Tab<Prefab> &prefabs, const char *lib_path, bool show_debug)
{
  DataBlock blk;
  blk.setBool("useVisRange", true);

  for (int i = 0; i < prefabs.size(); ++i)
  {
    DataBlock *objBlk = blk.addNewBlock("staticObject");

    if (objBlk)
    {
      if (show_debug)
        printf("\"%s\"\t\tpos = (%g, %g, %g)\t\tangle = %g\n", (const char *)prefabs[i].name, P3D(prefabs[i].pos), prefabs[i].angle);

      objBlk->setStr("name", prefabs[i].name);
      objBlk->setStr("libPath", lib_path);
      objBlk->setStr("objNameInLib", prefabs[i].name);
      objBlk->setBool("useDefault", true);

      TMatrix tm = TMatrix::IDENT;
      if (prefabs[i].angle)
        tm *= rotyTM(prefabs[i].angle);

      tm.setcol(3, prefabs[i].pos);

      TMatrix mirror = TMatrix::IDENT;
      mirror.setcol(2, Point3(0, 0, -1));

      tm *= mirror;

      DataBlock *tmBlk = objBlk->addNewBlock("tm");

      if (tmBlk)
      {
        tmBlk->setPoint3("col0", tm.getcol(0));
        tmBlk->setPoint3("col1", tm.getcol(1));
        tmBlk->setPoint3("col2", tm.getcol(2));
        tmBlk->setPoint3("col3", tm.getcol(3));
      }
    }
  }

  return blk.saveToTextFile(path);
}
