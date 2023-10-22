#include <3d/fileTexFactory.h>
#include <3d/dag_drv3d.h>
#include "texMgrData.h"

BaseTexture *StubTextureFactory::createTexture(TEXTUREID id)
{
  const char *textureName = get_managed_texture_name(id);
  // debug("stub: %d %s  create", id, textureName);
  if (!textureName)
    return NULL;

  return d3d::create_tex(NULL, 1, 1, 0, 1, textureName);
}

void StubTextureFactory::releaseTexture(BaseTexture *texture, TEXTUREID id)
{
  const char *textureName = get_managed_texture_name(id);
  // debug("stub: %d %s  destroy %p", id, textureName, texture);
  if (!textureName)
    return;

  if (texture)
    texture->destroy();
}

void StubTextureFactory::texFactoryActiveChanged(bool active) { texmgr_internal::auto_add_tex_on_get_id = active; }

StubTextureFactory StubTextureFactory::self;
TextureFactory *get_stub_tex_factory() { return &StubTextureFactory::self; }
