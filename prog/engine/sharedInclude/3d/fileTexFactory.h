// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  create texture from file
************************************************************************/
#ifndef __FILETEXTUREFACTORY_H
#define __FILETEXTUREFACTORY_H


#include <3d/dag_texMgr.h>


/*********************************
 *
 * class FileTextureFactory
 *
 *********************************/
class FileTextureFactory : public TextureFactory
{
private:
  TEXTUREID missingTexId;
  BaseTexture *missingTex;
  bool useMissingTex;

  // cannot create factories - use FileTextureFactory::getStatic()
  FileTextureFactory();
  //~FileTextureFactory() { onTexFactoryDeleted(this); }

public:
  static FileTextureFactory self;

  // Create texture, be sure to store it to release it in releaseTexture()
  virtual BaseTexture *createTexture(TEXTUREID id);

  // Release created texture
  virtual void releaseTexture(BaseTexture *texture, TEXTUREID id);

  bool scheduleTexLoading(TEXTUREID tid, TexQL req_ql) override;

  void setMissingTexture(const char *tex_name, bool replace_bad_shader_textures);
  TEXTUREID getReplaceBadTexId();
  void setMissingTextureUsage(bool use = true);
};


class SymbolicTextureFactory : public TextureFactory
{
private:
  SymbolicTextureFactory() {}
  //~SymbolicTextureFactory() { onTexFactoryDeleted(this); }

public:
  static SymbolicTextureFactory self;

  virtual BaseTexture *createTexture(TEXTUREID /*id*/) { return NULL; }

  virtual void releaseTexture(BaseTexture * /*texture*/, TEXTUREID /*id*/) {}
};


class StubTextureFactory : public TextureFactory
{
public:
  static StubTextureFactory self;

  //~StubTextureFactory() { onTexFactoryDeleted(this); }

  virtual BaseTexture *createTexture(TEXTUREID id);
  virtual void releaseTexture(BaseTexture *texture, TEXTUREID id);

  virtual void texFactoryActiveChanged(bool active);
};


#endif //__FILETEXTUREFACTORY_H
