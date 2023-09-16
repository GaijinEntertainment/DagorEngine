#pragma once


class String;


class ITextureNameResolver
{
public:
  virtual bool resolveTextureName(const char *src_name, String &out_name) = 0;

  static const char *getCurrentDagName();
};


void set_global_tex_name_resolver(ITextureNameResolver *cb);
ITextureNameResolver *get_global_tex_name_resolver();
