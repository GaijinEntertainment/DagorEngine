// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_debug.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* qdox @page Image

.. _image_rasterization:

Image Rasterization
--------------------

Images can be of several formats, most common TIFF, PNG, JPG and SVG.


There can also be suffixes that allow to specify parameters of rasterization, separated with ``:``::

  <image>[<:W>[:H][:k|f|p]]

  W - width
  H - height
  K - keep aspect
  F - force aspect
  P - premultiplied alpha

Width and Height can be used for svg and other format for better down/upsampling quality.
When both Width and Height are 0 then resampling is skipped (it may be useful to force premultiplied alpha leaving source picture
resolution as is using :0:P suffix). For vector graphics (such as .SVG) resolution (at least Width) is mandatory.

Sample usage (in daRg for example)::

  Picture("image.svg:256:256:K:P")
  or
  Picture("b64://<base64 encoded string of svg or png>:256:256:K:P")
  or
  Picture("image.png:0:P")
  or
  Picture("image.png:512:256:P:F")

*/

static bool decode_fname_suffix(const char *fn_ext, int &out_w, int &out_h, bool &out_keep_ar, bool &out_premul,
  const char *fn_ext_orig)
{
  out_w = atoi(fn_ext);
  out_h = 0;
  fn_ext = strchr(fn_ext, ':');
  if (fn_ext)
  {
    out_h = atoi(fn_ext + 1);
    const char *fn_ext_last = fn_ext;
    fn_ext = strchr(fn_ext + 1, ':');
    if (!fn_ext)
      fn_ext = fn_ext_last;

    if (fn_ext[1] == 'P' || fn_ext[1] == 'p') // check before
    {
      out_premul = true;
      fn_ext = strchr(fn_ext + 2, ':');
      if (!fn_ext)
        return true;
    }

    if (fn_ext[1] == 'K' || fn_ext[1] == 'k')
      out_keep_ar = true;
    else if (fn_ext[1] == 'F' || fn_ext[1] == 'f')
      out_keep_ar = false;
    else if ((fn_ext != fn_ext_last || !isdigit(fn_ext[1])) && fn_ext[1])
    {
      logerr("unrecognized %c in %s", fn_ext[1], fn_ext_orig);
      return false;
    }

    fn_ext = strchr(fn_ext + 2, ':');
    if (fn_ext && (fn_ext[1] == 'P' || fn_ext[1] == 'p')) // check after
      out_premul = true;
  }
  return true;
}

static inline void pic_adjust_dim(float &dim, float &other_dim, float max_dim, float src_aspect, bool keep_aspect_ratio)
{
  if (dim > max_dim)
  {
    dim = max_dim;
    if (keep_aspect_ratio)
      other_dim = max_dim * src_aspect;
  }
}
static inline void pic_set_dim(float &dim, float &other_dim, float target_dim, float src_aspect, bool keep_aspect_ratio)
{
  dim = target_dim;
  if (keep_aspect_ratio)
    other_dim = src_aspect * dim;
}
