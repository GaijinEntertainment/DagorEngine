#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/blockFileUtil.h>
#include <libTools/util/strUtil.h>
#include <util/dag_globDef.h>
#include <debug/dag_except.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>

bool copy_dag_file_tex_subst(const char *src_name, const char *dst_name, dag::ConstSpan<const char *> srcTexName,
  dag::ConstSpan<const char *> destTexName)
{
  // get and patch dag texture names
  BlkFile dag(DAG_ID, true);
  G_ASSERT(srcTexName.size() == destTexName.size());

  DAGOR_TRY
  {
    dag.loadfile(src_name, 0);

    if (dag.getint() != DAG_ID)
      DAGOR_THROW(BlkFileException("Invalid DAG file"));

    for (; dag.rest();)
    {
      dag.getblk();
      int id = dag.getint();

      if (id == DAG_END)
      {
        break;
      }
      else if (id == DAG_TEXTURES)
      {
        int num = dag.getshort();
        String texname;

        for (int i = 0; i < num; ++i)
        {
          // get tex name
          int len = dag.getchar();
          texname.printf(len + 2, "%.*s", len, dag.ptr());

          // erase it
          dag.skip(-1);
          dag.erase(1 + len);

          // get new name
          bool found = false;
          for (int j = 0; j < srcTexName.size(); j++)
            if (stricmp(texname, srcTexName[j]) == 0)
            {
              texname = destTexName[j];
              found = true;
            }
          if (!found)
            debug("not found %s in %s", texname.str(), src_name);

          // write new name
          dag.putchar(texname.length());
          dag.insert(texname.length(), &texname[0]);
        }

        break;
      }

      dag.goup();
    }

    dag.savefile(dst_name, DF_CREATE | DF_WRITE);
  }
  DAGOR_CATCH(BlkFileException e) { return false; }

  dag.clearfile();

  return true;
}
