#include <libTools/util/de_TextureName.h>
#include <debug/dag_debug.h>

//============================================================================================
bool DETextureName::makeRelativePath(const char *base_path)
{
  if (!::is_full_path(*this))
  {
    debug("makeRelativePath: Texture path is already relative [%s]", (const char *)*this);
    return true;
  }

  if (base_path)
    return FilePathName::makeRelativePath(base_path);

  return false;
}
