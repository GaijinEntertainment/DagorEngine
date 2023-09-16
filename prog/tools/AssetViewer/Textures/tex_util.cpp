#include "tex_util.h"

#include "../av_appwnd.h"

#include <winGuiWrapper/wgw_dialogs.h>
#include <texConverter/textureConverterDlg.h>


//==================================================================================================
String import_texture(const char *path)
{
  const String importDir = ::make_full_path(::get_app().getLibDir(), "tex/");

  String importPath(512, "%s%s.dds", importDir.str(), ::get_file_name_wo_ext(path).str());

  if (!::dd_dir_exist(importDir))
    if (!::dd_mkdir(importDir))
    {
      wingw::message_box(wingw::MBS_EXCL, "Texture import error", "Could not create folder \"%s\"", (const char *)importDir);

      return "";
    }

  const char *ext = ::get_file_ext(path);

  // DDS or DTX texture
  if (!stricmp(ext, "dds") || !stricmp(ext, "dtx"))
  {
    if (!stricmp(ext, "dtx"))
    {
      ddstexture::Header ddsHeader;
      ddsHeader.getFromFile(path);
      importPath = ddsHeader.makeName(importPath);
    }

    return ::_ctl_BinFileCopy(importPath, path) ? ::get_file_name(importPath) : "";
  }
  // other textures
  else
  {
    ddstexture::ConverterDlg ddsConverter(&get_app());
    ddsConverter.noChangeDstName();

    DDSPathName newLocation = ddsConverter.convert(path, importPath);

    while (newLocation == "")
    {
      const int result = wingw::message_box(wingw::MBS_YESNOCANCEL, "Texture convert", "Do you want to abort texture import?");

      switch (result)
      {
        case CTL_IDYES: newLocation = ddsConverter.convert(path, importPath); break;

        case CTL_IDNO: newLocation = path = ::make_full_path(_ctl_StartDirectory, "../commonData/tex/default.dds"); break;

        case CTL_IDCANCEL: return "";
      }
    }
  }

  return ::get_file_name(importPath);
}
