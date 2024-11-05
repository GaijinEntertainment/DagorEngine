//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forwards for used classes
class String;


class IShaderMatVdataTexLoadCtrl
{
public:
  //! receives original requested texture name, context (obj type/name) and string storage;
  //! shall return one of:
  //! 1) original_tex_name - when no texture subst/ignore not needed;
  //! 2) NULL - when texture must be ignored (not loaded);
  //! 3) tmp_storage.str() - when substituting texture name with another, written to tmp_storage
  virtual const char *preprocessMatVdataTexName(const char *original_tex_name, unsigned ctx_obj_type, const char *ctx_obj_name,
    String &tmp_storage) = 0;

  static const char *preprocess_tex_name(const char *original_tex_name, String &tmp_storage);
};

extern IShaderMatVdataTexLoadCtrl *dagor_sm_tex_load_controller;

void dagor_set_sm_tex_load_ctx_type(unsigned obj_type);
void dagor_set_sm_tex_load_ctx_name(const char *obj_name);
void dagor_reset_sm_tex_load_ctx();
