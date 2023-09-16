#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/util/blockFileUtil.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_direct.h>

bool copy_dag_file(const char *src_name, const char *dst_name, Tab<String> &slotNames, Tab<String> &orgTexNames)
{
  // get and patch dag texture names
  BlkFile dag(DAG_ID, true);

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
        orgTexNames.resize(num);
        slotNames.resize(num);

        for (int i = 0; i < orgTexNames.size(); ++i)
        {
          // get tex name
          int len = dag.getchar();
          orgTexNames[i].append(dag.ptr(), len);

          // erase it
          dag.skip(-1);
          dag.erase(1 + len);

          // get new name
          String name, loc;
          ::split_path(orgTexNames[i], loc, name);

          const char *ext = ::dd_get_fname_ext(name);
          if (ext)
            ::remove_trailing_string(name, ext);

          ::make_ident_like_name(name);

          for (;;)
          {
            int j;
            for (j = 0; j < i; ++j)
              if (stricmp(orgTexNames[j], name) == 0)
                break;

            if (j >= i)
              break;

            ::make_next_numbered_name(name);
          }

          // write new name
          slotNames[i] = name;
          dag.putchar(name.length());
          dag.insert(name.length(), &name[0]);
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
