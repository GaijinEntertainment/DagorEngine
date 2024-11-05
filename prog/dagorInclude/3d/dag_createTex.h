//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
class BaseTexture;
struct TextureMetaData;


class ICreateTexFactory
{
public:
  virtual BaseTexture *createTex(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd) = 0;
};

void add_create_tex_factory(ICreateTexFactory *ctf);
void del_create_tex_factory(ICreateTexFactory *ctf);

BaseTexture *create_texture(const char *fn, int flg, int levels, bool fatal_on_err, const char *fnext = NULL);
BaseTexture *create_texture_via_factories(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd,
  ICreateTexFactory *excl_factory);
