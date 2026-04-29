// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiBlurTexHelper.h"

static BaseTexture *ui_tex_ptr = nullptr;

void set_ui_tex_for_blur(BaseTexture *tex) { ui_tex_ptr = tex; }
BaseTexture *get_ui_tex_for_blur() { return ui_tex_ptr; }
