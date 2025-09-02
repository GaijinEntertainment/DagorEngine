// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <debug/dag_debug.h>

class AnyLoadImageFactory : public ILoadImageFactory
{
public:
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    if (fn_ext)
      return false;

    for (int i = 0; i < useExt.size(); i++)
    {
      String new_fn(0, "%s%s", fn, useExt[i]);
      if (dd_file_exists(new_fn))
        return read_image_dimensions(new_fn, out_w, out_h, out_may_have_alpha, useExt[i]);
    }
    return false;
  }

  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    if (fn_ext)
      return NULL;

    for (int i = 0; i < useExt.size(); i++)
    {
      String new_fn(0, "%s%s", fn, useExt[i]);
      if (dd_file_exists(new_fn))
        return load_image(new_fn, mem, out_used_alpha, useExt[i]);
    }

    return NULL;
  }

  bool supportLoadImage2() override { return true; }
  void *loadImage2(const char *fn, IAllocImg &a, const char *fn_ext) override
  {
    if (fn_ext)
      return NULL;

    for (int i = 0; i < useExt.size(); i++)
    {
      String new_fn(0, "%s%s", fn, useExt[i]);
      if (dd_file_exists(new_fn))
        return load_image2(new_fn, a, useExt[i]);
    }

    return NULL;
  }

  int init(const char *try_order) { return parse_string_list(useExt, try_order, "."); }

  Tab<String> useExt;
};

class UrlAuxLoadImageFactory : public ILoadImageFactory
{
public:
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    String real_fn;
    bool ret = false;
    if (file_ptr_t fp = resolveFn(fn, fn_ext, real_fn))
    {
      ret = read_image_dimensions(real_fn, out_w, out_h, out_may_have_alpha, fn_ext);
      df_close(fp);
    }
    return ret;
  }
  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    TexImage32 *img = NULL;
    String real_fn;
    if (file_ptr_t fp = resolveFn(fn, fn_ext, real_fn))
    {
      img = load_image(real_fn, mem, out_used_alpha, fn_ext);
      df_close(fp);
    }
    return img;
  }

  bool supportLoadImage2() override { return true; }
  void *loadImage2(const char *fn, IAllocImg &a, const char *fn_ext) override
  {
    void *img = NULL;
    String real_fn;
    if (file_ptr_t fp = resolveFn(fn, fn_ext, real_fn))
    {
      img = load_image2(real_fn, a, fn_ext);
      df_close(fp);
    }
    return img;
  }

  bool init(user_get_file_data_t cb_get, void *cb_arg, const char *url, const char *ext)
  {
    if (!cb_get)
      return false;
    if (!parse_string_list(suppUrl, url, "", "://"))
      return false;
    if (!parse_string_list(suppExt, ext, "."))
      return false;

    getDataFile = cb_get;
    getDataFileArg = cb_arg;
    return true;
  }
  void term()
  {
    clear_and_shrink(suppExt);
    clear_and_shrink(suppUrl);
    getDataFile = NULL;
    getDataFileArg = NULL;
  }
  file_ptr_t resolveFn(const char *fn, const char *fn_ext, String &out_fn)
  {
    if (!fn_ext || !getDataFile)
      return NULL;
    for (int i = 0; i < suppExt.size(); i++)
      if (strcmp(suppExt[i], fn_ext) == 0)
        for (int j = 0; j < suppUrl.size(); j++)
          if (strncmp(suppUrl[j], fn, suppUrl[j].length()) == 0)
          {
            int out_base_ofs = 0;
            if (getDataFile(fn, NULL, out_fn, out_base_ofs, true, false, "", getDataFileArg))
            {
              if (out_base_ofs != 0)
              {
                logerr("<%s> resolved to <%s> baseOfs=%d != 0", fn, out_fn, out_base_ofs);
                return NULL;
              }
              if (file_ptr_t fp = df_open(out_fn, DF_READ | DF_IGNORE_MISSING))
                return fp;
              logerr("<%s> resolved to <%s>, but df_open() failed", fn, out_fn);
            }
            return NULL;
          }
    return NULL;
  }

  Tab<String> suppExt, suppUrl;
  user_get_file_data_t getDataFile = NULL;
  void *getDataFileArg = NULL;
};

static AnyLoadImageFactory any_load_image_factory;
static UrlAuxLoadImageFactory url_load_image_factory;

void register_any_tex_load_factory(const char *try_order)
{
  if (any_load_image_factory.init(try_order))
    add_load_image_factory(&any_load_image_factory);
}

void register_url_tex_load_factory(user_get_file_data_t cb_get, void *cb_arg, const char *url, const char *ext)
{
  if (!cb_get)
  {
    del_load_image_factory(&url_load_image_factory);
    url_load_image_factory.term();
    return;
  }
  if (url_load_image_factory.init(cb_get, cb_arg, url, ext))
    add_load_image_factory(&url_load_image_factory, /*add_first*/ true);
}
