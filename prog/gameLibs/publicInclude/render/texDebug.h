//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_tex3d.h>

namespace texdebug
{

/**
 * @brief Wrapper that provides texdebug with external texture.
 */
class ExternalTextureProvider
{
public:
  ExternalTextureProvider() = default;
  ExternalTextureProvider(const ExternalTextureProvider &) = delete;
  ExternalTextureProvider(ExternalTextureProvider &&) = delete;
  virtual ~ExternalTextureProvider() = default;

  virtual void init() {}
  virtual void close() {}
  virtual const char *getName() const = 0;
  virtual TextureInfo getTextureInfo() = 0;
  virtual TEXTUREID getTextureId() const = 0;
};

void init();
void teardown();
void select_texture(const char *name);
void register_external_texture(ExternalTextureProvider &provider);
void unregister_external_texture(ExternalTextureProvider &provider);
} // namespace texdebug