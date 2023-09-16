#include <rendInst/impostorTextureMgr.h>

void init_impostor_texture_mgr() {}
void close_impostor_texture_mgr() {}
ImpostorTextureManager *get_impostor_texture_mgr() { return nullptr; }
bool ImpostorTextureManager::update_shadow(RenderableInstanceLodsResource *, const ShadowGenInfo &, int, int, int, const UniqueTex &)
{
  return false;
}
UniqueTex ImpostorTextureManager::renderDepthAtlasForShadow(RenderableInstanceLodsResource *) { return {}; }
bool ImpostorTextureManager::hasBcCompression() const { return false; }
int ImpostorTextureManager::getPreferredShadowAtlasMipOffset() const { return 0; }
