// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class String;
class DataBlock;

String de3_imgui_file_open_dlg(const char *caption, const char *filter, const char *def_ext, const char *init_path,
  const char *init_fn);

String de3_imgui_file_save_dlg(const char *caption, const char *filter, const char *def_ext, const char *init_path,
  const char *init_fn);
