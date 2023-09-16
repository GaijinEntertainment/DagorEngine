#include <libTools/util/de_TextureName.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/dtx/dtxHeader.h>
#include <debug/dag_debug.h>
#include <3d/dag_tex3d.h>
#include <libTools/util/iLogWriter.h>

static ILogWriter *log_writer = NULL;


//================================================================================
//  DDS pathname-suit
//================================================================================
DDSPathName DDSPathName::findExisting(const char *base_path) const
{
  DDSPathName name = *this;
  name.makeFullPath(base_path);

  if (name.getExt() == "dtx")
    name.setExt("dds");

  if (name == "")
    return name;

  // check many textures for display warning
  {
    DDSPathName tmp = name;
    tmp.simplify();

    Tab<String> tmp_list(tmpmem);
    if (::dd_file_exist(tmp))
      tmp_list.push_back(tmp);

    tmp.setName(tmp.getName(false) + "@*.dds");

    // find some file_name@[].dds - file
    alefind_t find;
    if (::dd_find_first(tmp, 0, &find))
      tmp_list.push_back(tmp.setName(find.name));
    ::dd_find_close(&find);

    if (tmp_list.size() > 1)
    {
      String log(0, "found many textures for %s:", name);
      debug(log);
      if (log_writer)
        log_writer->addMessage(ILogWriter::WARNING, log);
      for (int i = 0; i < tmp_list.size(); i++)
      {
        debug(tmp_list[i]);
        if (log_writer)
          log_writer->addMessage(ILogWriter::WARNING, tmp_list[i]);
      }
    }
  }

  if (::dd_file_exist(name))
    return name;

  name.simplify();
  if (::dd_file_exist(name))
    return name;

  name.setExt("dtx");
  if (!::dd_file_exist(name))
  {
    name.setName(name.getName(false) + "@*.dds");

    // find some file_name@[].dds - file
    alefind_t find;
    if (::dd_find_first(name, 0, &find))
      name.setName(find.name);
    else
      name = "";
    ::dd_find_close(&find);
  }

  return name;
}


//======================================================================================================================
DDSPathName &DDSPathName::simplify()
{
  String name(getName(true));
  const char *info = name.find('@');
  if (!info)
    info = name.find('.');

  if (info)
    *this = DDSPathName(getPath(), String::mk_sub_str(name, info), "dds");
  return *this;
}


//======================================================================================================================
namespace ddstexture
{
template <class T>
int getId(T value, const T *vector)
{
  for (int i = 0; vector[i]; i++)
    if (value == vector[i])
      return i;
  return 0; // set to default
}

//======================================================================================================================
static inline char toHex(int number) { return number < 10 ? '0' + number : 'a' + number - 10; }


//======================================================================================================================
String Header::makeName(const char *path_name_param)
{
  DDSPathName path_name = path_name_param;
  DDSPathName name = path_name.getName(false);
  if (name.find('@'))
    name = String::mk_sub_str(name.begin(), name.find('@'));

  // scan presets
  Tab<Header> &pres = presets();
  for (int i = 0; i < pres.size(); i++)
    if (pres[i] == *this)
      return path_name.setName(String(512, "%s@P%d.dds", (char *)name, i));

  char tex_addr[] = "wmcb";
  unsigned int tex_ids[] = {TEXADDR_WRAP, TEXADDR_MIRROR, TEXADDR_CLAMP, TEXADDR_BORDER, 0};

  String str("@");

  if (addr_mode_u == addr_mode_v)
  {
    if (addr_mode_u != TEXADDR_WRAP)
    {
      str += (str == "@" ? "A" : "_A");
      str += tex_addr[getId(addr_mode_u, tex_ids)];
    }
  }
  else
  {
    str += (str == "@" ? "A" : "_A");
    str += tex_addr[getId(addr_mode_u, tex_ids)];
    str += tex_addr[getId(addr_mode_v, tex_ids)];
  }

  if (HQ_mip != 0 || MQ_mip != 1 || LQ_mip != 2)
  {
    str += (str == "@" ? "Q" : "_Q");
    str += toHex(HQ_mip);
    if (MQ_mip != 1 || LQ_mip != 2)
    {
      str += toHex(MQ_mip);
      if (LQ_mip != 2)
        str += toHex(LQ_mip);
    }
  }

  if (str == "@")
    str = "";

  return path_name.setName(name + str + ".dds");
}
} // namespace ddstexture

//================================================================================
//  dagored texture pathname-suit
//================================================================================
bool DETextureName::findExisting(const char *base_path)
{
  if (*this == "")
    return false;

  if (::is_full_path(*this) && ::dd_file_exist(*this))
    return true;

  if (base_path)
  {
    for (const char *p = end(); p >= begin(); --p)
      if (p == begin() || *(p - 1) == '/' || *(p - 1) == '\\')
      {
        DDSPathName newName(base_path, p);

        if (newName.getExt() == "dds" || newName.getExt() == "dtx")
          newName = newName.findExisting();

        if (::dd_file_exist(newName))
        {
          *this = newName;
          return true;
        }
      }

    debug("Texture [%s] not found at base path \"%s\"", (const char *)*this, base_path);
  }

  debug("Texture [%s] not found", (const char *)*this);
  return false;
}


//================================================================================
void DETextureName::setLogWriter(ILogWriter *writer) { log_writer = writer; }
