#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/sceneImpIface.h>
#include <libTools/dtx/dtx.h>

#include <coolConsole/coolConsole.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/de_TextureName.h>
#include <regExp/regExp.h>
#include <libTools/util/fileUtils.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>

#include <math/dag_math3d.h>

#include <3d/dag_texMgr.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <texConverter/textureConverterDlg.h>

#include <string.h>
#include <stdio.h>

#include <3d/ddsFormat.h>
#include <direct.h>


//==============================================================================

static RegExp tga_re_;
static RegExp dds_re_;
static bool re_inited_ = false;

//==============================================================================
static String dagutil_last_error_;

const char *get_dagutil_last_error() { return (const char *)dagutil_last_error_; }


//------------------------------------------------------------------------------

bool texture_names_are_equal(const char *name1, const char *name2)
{
  if (!re_inited_)
  {
    tga_re_.compile("(.*)(\\.tga)", "i");
    dds_re_.compile("(.*)(\\.dds)", "i");

    re_inited_ = true;
  }

  int len1 = (int)strlen(name1);
  int len2 = (int)strlen(name2);
  bool rv = false;

  if (len1 == len2)
  {
    if ((tga_re_.test(name1) || dds_re_.test(name1)) && (tga_re_.test(name2) || dds_re_.test(name2)))
    {
      rv = strnicmp(name1, name2, len1 - 4) == 0;
    }
    else
    {
      rv = stricmp(name1, name2) == 0;
    }
  }

  return rv;
}


//------------------------------------------------------------------------------

bool remap_dag_textures(const char *dag_name, const Tab<String> &original_tex, const Tab<String> &actual_tex)
{

  dagutil_last_error_ = "";

  bool success = false;
  file_ptr_t dag_file = df_open(dag_name, DF_READ);

  if (dag_file)
  {
    int id = 0;

    df_read(dag_file, &id, 4);
    if (id == DAG_ID)
    {
      int src_dag_size = 0;

      df_read(dag_file, &src_dag_size, 4);
      df_read(dag_file, &id, 4);

      if (id == DAG_ID)
      {
        int textures_size = 0;

        df_read(dag_file, &textures_size, 4);
        df_read(dag_file, &id, 4);

        if (id == DAG_TEXTURES)
        {
          int count = 0;
          int new_dag_size = src_dag_size;
          int new_textures_size = 0;
          Tab<int> name_index(tmpmem);
          name_index.reserve(128);

          df_read(dag_file, &count, 2);
          for (int i = 0; i < count; ++i)
          {
            int sz = 0;
            char name[256 + 1] = "";

            df_read(dag_file, &sz, 1);
            df_read(dag_file, name, sz);
            name[sz] = '\0';

            int index = -1;

            for (int j = 0; j < original_tex.size(); ++j)
            {
              if (texture_names_are_equal(name, (const char *)(original_tex[j])))
              {
                index = j;
                name_index.push_back(index);
                new_textures_size += actual_tex[j].length() + 1;
                break;
              }
            }

            if (index == -1)
              debug("texture named '%s' not found in original_tex", name);
          }                           // for each texture in src DAG
          new_textures_size += 4 + 2; // block-ID + tex-count


          // compute new DAG size

          new_dag_size -= textures_size;
          new_dag_size += new_textures_size;


          // copy rest of src DAG into buffer

          int sz = src_dag_size + 8 - df_tell(dag_file);
          char *dag = new char[sz];

          df_read(dag_file, dag, sz);


          // write header

          int data = DAG_ID;

          df_close(dag_file);

          dag_file = df_open(dag_name, DF_WRITE);
          df_write(dag_file, &data, 4);

          df_write(dag_file, &new_dag_size, 4);
          df_write(dag_file, &data, 4);


          // write TEXTURES block

          df_write(dag_file, &new_textures_size, 4);
          data = DAG_TEXTURES;
          df_write(dag_file, &data, 4);
          data = name_index.size();
          df_write(dag_file, &data, 2);

          for (int i = 0; i < name_index.size(); ++i)
          {
            int index = name_index[i];
            int name_sz = actual_tex[index].length();
            const char *name = (const char *)actual_tex[index];

            df_write(dag_file, &name_sz, 1);
            df_write(dag_file, name, name_sz);
          }


          // write rest of old DAG

          df_write(dag_file, dag, sz);
          delete[] dag;

          success = true;
        }
        else
        {
          debug("invalid DAG '%s', TEXTURES-block missing", dag_name);
          dagutil_last_error_.printf(0, 128, "invalid DAG \"%s\", no TEXTURES-block found", dag_name);
        }
      }
      else
      {
        debug("invalid DAG '%s', DAG-block missing", dag_name);
        dagutil_last_error_.printf(0, 128, "invalid DAG \"%s\", no DAG-block found", dag_name);
      }
    }
    else
    {
      debug("'%s' is not valid DAG file, aborting", dag_name);
      dagutil_last_error_.printf(0, 128, "\"%s\" is not valid DAG file (DAG_ID misssing)", dag_name);
    }

    df_close(dag_file);
  } // if( dag_file )
  else
  {
    dagutil_last_error_.printf(0, 128, "FAILED to open \"%s\" for reading", dag_name);
  }

  return success;
}


//------------------------------------------------------------------------------
static String absolute_path_(const char *path)
{
  String result;

  if (isalpha(path[0]) && path[1] == ':')
  {
    result = path;
  }
  else
  {
    char cwd[512];

    ::getcwd(cwd, countof(cwd));
    result.printf(1024, "%s\\%s", cwd, path);
  }

  return result;
}


//------------------------------------------------------------------------------
bool replace_dag(const char *dag_src, const char *dag_dst, CoolConsole *con)
{
  const String srcFile(absolute_path_(dag_src));
  const String dstFile(absolute_path_(dag_dst));
  const String dstTemp(dstFile + String("-tmp.dag"));
  const bool success = import_dag(dag_src, dstTemp, con);

  if (success)
  {
    ::erase_local_dag_textures(dag_dst);
    ::dd_rename(dstTemp, dstFile);
  }

  return success;
}


//------------------------------------------------------------------------------
bool import_dag(const char *dag_src, const char *dag_dst, CoolConsole *con)
{
  dagutil_last_error_ = "";

  char cwd[256] = ".";
  char dag_dstfile[1024];
  char dag_srcfile[1024];
  bool success = false;
  bool do_remap = true;


  // ensure DAG path & dst_location are absolute pathes

  ::getcwd(cwd, countof(cwd));

  if (isalpha(dag_dst[0]) && dag_dst[1] == ':')
    strcpy(dag_dstfile, dag_dst);
  else
    _snprintf(dag_dstfile, countof(dag_dstfile), "%s\\%s", cwd, dag_dst);

  if (isalpha(dag_src[0]) && dag_src[1] == ':')
    strcpy(dag_srcfile, dag_src);
  else
    _snprintf(dag_srcfile, countof(dag_srcfile), "%s\\%s", cwd, dag_src);


  String dst_location;
  String dst_file;
  String src_dag_location;
  String src_dag_file;

  split_path(dag_srcfile, src_dag_location, src_dag_file);
  split_path(dag_dstfile, dst_location, dst_file);

  if (is_empty_string(dst_location))
  {
    debug("empty dst_location, import of '%s' aborted", dag_srcfile);
    dagutil_last_error_.printf(0, 128, "empty dst_location, import of \"%s\" aborted", dag_srcfile);
  }
  else
  {
    //        if( cb )    cb->beginImport();
    if (con)
      con->setActionDesc("Pre-processing DAG");


    // copy DAG into dst_location

    debug_("copying DAG '%s' -> '%s'...", dag_srcfile, dag_dstfile);
    success = dag_copy_file(dag_srcfile, dag_dstfile);
    debug((success) ? "OK" : "FAILED");

    if (!success)
    {
      debug("copying of '%s' - > '%s' FAILED", dag_srcfile, dag_dstfile);
      dagutil_last_error_.printf(0, 128, "copying of \"%s\" - > \"%s\" FAILED", dag_srcfile, dag_dstfile);
      return success;
    }


    // get all DAG textures

    Tab<String> original_tex(tmpmem);
    original_tex.reserve(16);
    Tab<String> actual_tex(tmpmem);
    actual_tex.reserve(16);
    file_ptr_t dag_file = df_open(dag_dstfile, DF_READ);

    if (dag_file)
    {
      int id = 0;

      df_read(dag_file, &id, 4);
      if (id == DAG_ID)
      {
        int src_dag_size = 0;

        df_read(dag_file, &src_dag_size, 4);
        df_read(dag_file, &id, 4);

        if (id == DAG_ID)
        {
          int textures_size = 0;

          df_read(dag_file, &textures_size, 4);
          df_read(dag_file, &id, 4);

          if (id == DAG_TEXTURES)
          {
            int count = 0;
            int new_dag_size = src_dag_size;
            int new_textures_size = 0;
            Tab<int> name_index(tmpmem);
            name_index.reserve(128);

            df_read(dag_file, &count, 2);

            for (int i = 0; i < count; ++i)
            {
              int sz = 0;
              char name[512 + 1] = "";

              df_read(dag_file, &sz, 1);
              df_read(dag_file, name, sz);
              name[sz] = '\0';

              DETextureName src_name(::make_full_path(src_dag_location, name));

              if (!dd_get_fname_ext(src_name))
                src_name += ".dds";

              DETextureName dst_name(::get_file_name(src_name));
              success = dst_name != "";

              if (success)
              {
                original_tex.push_back() = name;
                actual_tex.push_back("./" + dst_name.getName());
              }

              if (con)
                con->incDone(1);
            } // for each texture in src DAG
          }
          else
          {
            success = true; //-V1048
            do_remap = false;
          }
        }
        else
        {
          debug("invalid DAG '%s', DAG-block missing", dag_dstfile);
          dagutil_last_error_.printf(0, 128, "invalid DAG \"%s\", no DAG-block found", dag_dstfile);
        }
      }
      else
      {
        debug("'%s' is not valid DAG file, aborting", dag_dstfile);
        dagutil_last_error_.printf(0, 128, "\"%s\" is not valid DAG file (DAG_ID misssing)", dag_dstfile);
      }

      df_close(dag_file);
    } // if( dag_file )
    else
    {
      dagutil_last_error_.printf(0, 128, "FAILED top open \"%s\" for reading", dag_dstfile);
    }

    success = false;
    erase_local_dag_textures(dag_dstfile);
    dd_erase(dag_dstfile);
    dagutil_last_error_.printf(0, 128, "CANCELLED import of \"%s\"", dag_srcfile);
  } // dst_location is valid

  return success;
}


//------------------------------------------------------------------------------

void get_dag_texture_list_(const char *dag_fullpath, Tab<String> *list)
{
  bool success = false;
  file_ptr_t dag_file = df_open(dag_fullpath, DF_READ);

  if (dag_file)
  {
    int id = 0;

    df_read(dag_file, &id, 4);
    if (id == DAG_ID)
    {
      int dag_size = 0;

      df_read(dag_file, &dag_size, 4);
      df_read(dag_file, &id, 4);

      if (id == DAG_ID)
      {
        int textures_size = 0;

        df_read(dag_file, &textures_size, 4);
        df_read(dag_file, &id, 4);

        if (id == DAG_TEXTURES)
        {
          int count = 0;

          clear_and_shrink(*list);
          df_read(dag_file, &count, 2);
          for (int i = 0; i < count; ++i)
          {
            int sz = 0;
            char name[256 + 1] = "";

            df_read(dag_file, &sz, 1);
            df_read(dag_file, name, sz);
            name[sz] = '\0';

            if (name[0] == '.' && (name[1] == '/' || name[1] == '\\'))
            {
              list->push_back(String(name));
            }
          } // for each texture in src DAG
        }
      }
    }

    df_close(dag_file);
  }
}


//------------------------------------------------------------------------------

bool erase_local_dag_textures(const char *dag_wildcard)
{
  String location;
  String file;
  alefind_t find_data;
  Tab<String> matched_dag(tmpmem);
  Tab<String> nonmatched_dag(tmpmem);
  bool found = dd_find_first(dag_wildcard, 0, &find_data);

  split_path(dag_wildcard, location, file);
  if (location.length() == 0)
    location = ".";

  while (found)
  {
    matched_dag.push_back(location + String("\\") + String(find_data.name));
    found = dd_find_next(&find_data);
  }

  dd_find_close(&find_data);

  found = dd_find_first(location + "\\*.dag", 0, &find_data);
  while (found)
  {
    bool unique = true;
    String name(location + String("\\") + String(find_data.name));

    for (int i = 0; i < matched_dag.size(); ++i)
    {
      if (stricmp(name, matched_dag[i]) == 0)
      {
        unique = false;
        break;
      }
    }

    if (unique)
      nonmatched_dag.push_back(name);

    found = dd_find_next(&find_data);
  }

  dd_find_close(&find_data);


  // build list of textures to erase

  Tab<String> erase_tex(tmpmem);
  Tab<String> tex(tmpmem);

  int i = 0;
  for (i = 0; i < matched_dag.size(); ++i)
  {
    get_dag_texture_list_(matched_dag[i], &tex);

    for (int j = 0; j < tex.size(); ++j)
    {
      bool unique = true;

      for (int e = 0; e < erase_tex.size(); ++e)
      {
        if (stricmp(tex[j], erase_tex[e]) == 0)
        {
          unique = false;
          break;
        }
      }

      if (unique)
        erase_tex.push_back(tex[j]);
    }
  }


  // build list of textures NOT to erase

  Tab<String> noterase_tex(tmpmem);

  for (i = 0; i < nonmatched_dag.size(); ++i)
  {
    get_dag_texture_list_(nonmatched_dag[i], &tex);

    for (int j = 0; j < tex.size(); ++j)
    {
      bool unique = true;

      for (int e = 0; e < noterase_tex.size(); ++e)
      {
        if (tex[j] == noterase_tex[e])
        {
          unique = false;
          break;
        }
      }

      if (unique)
        noterase_tex.push_back(tex[j]);
    }
  }


  // check if 'doomed' textures used in others 'non-doomed' DAG in this directory
  i = 0;
  while (i < erase_tex.size())
  {
    bool dont_erase = false;

    for (int j = 0; j < noterase_tex.size(); ++j)
    {
      if (stricmp(erase_tex[i], noterase_tex[j]) == 0)
      {
        dont_erase = true;
        break;
      }
    }

    if (dont_erase)
      erase_items(erase_tex, i, 1);
    else
      ++i;
  }


  // finally, kill'em

  for (int i = 0; i < erase_tex.size(); ++i)
    dd_erase(location + String("\\") + String((const char *)(erase_tex[i]) + 2));

  return true;
}
