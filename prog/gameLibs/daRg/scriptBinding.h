// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <daRg/dag_picture.h>

class SqModules;

namespace darg
{

struct GuiSceneScriptHandlers
{
  Sqrat::Function onUpdate;
  Sqrat::Function onShutdown;
  Sqrat::Function onHotkeysNav;

  void releaseFunctions()
  {
    onUpdate.Release();
    onShutdown.Release();
    onHotkeysNav.Release();
  }
};


namespace binding
{

template <typename PictureClass>
inline SQInteger picture_script_ctor(HSQUIRRELVM vm)
{
  SQInteger nParams = sq_gettop(vm);
  if (nParams > 3)
    return sq_throwerror(vm, "Too many arguments");

  const char *name = "";
  if (nParams >= 2 && sq_gettype(vm, 2) == OT_STRING)
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &name)));

  int texFormat = Picture::def_tex_format;
  if (nParams >= 3 && sq_gettype(vm, 3) == OT_TABLE)
  {
    Sqrat::Var<Sqrat::Table> params(vm, 3);
    texFormat = params.value.GetSlotValue<int>("texFormat", Picture::def_tex_format);
    bool isTexFmtValid = (texFormat == TexFormat::SRGB_IN_UNORM || texFormat == TexFormat::UNORM || texFormat == TexFormat::SRGB);
    if (!isTexFmtValid)
      return sqstd_throwerrorf(vm, "Invalid tex format %d", texFormat);
  }

  PictureClass *inst = new PictureClass(vm, name);
  inst->texFormat = static_cast<TexFormat>(texFormat);

  Sqrat::ClassType<PictureClass>::SetManagedInstance(vm, 1, inst);
  return 0;
}

void bind_script_classes(SqModules *module_mgr, Sqrat::Table &exports);

} // namespace binding

} // namespace darg
