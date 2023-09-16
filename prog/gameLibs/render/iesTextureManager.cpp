#include <render/iesTextureManager.h>

#include <3d/dag_drv3d.h>
#include <debug/dag_debug3d.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <util/dag_texMetaData.h>
#include <3d/dag_resPtr.h>

#include <EASTL/algorithm.h>

static const char *const PHOTOMETRY_TEX_NAME = "photometry_textures";
static const char *const PHOTOMETRY_VAR_NAME = "photometry_textures_tex";

IesTextureCollection *IesTextureCollection::instance = nullptr;

IesTextureCollection *IesTextureCollection::getSingleton() { return instance; }

IesTextureCollection *IesTextureCollection::acquireRef()
{
  if (!instance)
  {
    eastl::vector<String> textures;
    for (TEXTUREID i = first_managed_texture(0); i != BAD_TEXTUREID; i = next_managed_texture(i, 0))
    {
      const char *textureName = get_managed_texture_name(i);
      const char *const IES_PREFIX = "ies_";
      if (strncmp(textureName, IES_PREFIX, strlen(IES_PREFIX)) == 0)
      {
        unsigned int size = strlen(IES_PREFIX); // skip the prefix
        const size_t len = strlen(textureName);
        while (size < len && textureName[size] != '*')
          size++;
        textures.emplace_back(textureName, size);
      }
    }
    instance = new IesTextureCollection(eastl::move(textures));
  }
  instance->refCount++;
  return instance;
}

void IesTextureCollection::releaseRef()
{
  G_ASSERT_RETURN(instance, );
  if (--instance->refCount == 0)
  {
    delete instance;
    instance = nullptr;
  }
}

IesTextureCollection::IesTextureCollection(eastl::vector<String> &&textures) : usedTextures(eastl::move(textures))
{
  reloadTextures();
}

IesTextureCollection::~IesTextureCollection() { close(); }

void IesTextureCollection::close()
{
  ShaderGlobal::reset_from_vars(photometryTexId);
  evict_managed_tex_id(photometryTexId);
}

int IesTextureCollection::getTextureIdx(const char *name)
{
  int ret = textureNames.getNameId(name);
  if (ret == -1)
  {
#if DAGOR_DBGLEVEL > 0
    debug("Loading an ies texture after initialization: %s", name);
#endif
    addTexture(name);
    return textureNames.getNameId(name);
  }
  return ret;
}

IesTextureCollection::PhotometryData IesTextureCollection::getTextureData(int texIdx) const
{
  if (texIdx < 0)
    return {};
  return photometryData[texIdx];
}

void IesTextureCollection::reloadTextures()
{
  textureNames.reset();
  photometryData.clear();
  if (usedTextures.size() == 0)
    return;
  photometryData.reserve(usedTextures.size());
  eastl::vector<const char *> cstrVec(usedTextures.size());
  eastl::transform(eastl::begin(usedTextures), eastl::end(usedTextures), eastl::begin(cstrVec), [this](const String &s) {
    textureNames.addNameId(s.c_str());
    return s.c_str();
  });

  if (photometryTexId != BAD_TEXTUREID)
  {
    ShaderGlobal::reset_from_vars(photometryTexId);
    photometryTexId = update_managed_array_texture(PHOTOMETRY_TEX_NAME, cstrVec);
  }
  else
  {
    photometryTexId = ::add_managed_array_texture(PHOTOMETRY_TEX_NAME, cstrVec);
    if (BaseTexture *photometryTex = D3dResManagerData::getD3dTex<RES3D_TEX>(photometryTexId))
      photometryTex->texaddr(TEXADDR_CLAMP);
  }
  ShaderGlobal::set_texture(::get_shader_variable_id(PHOTOMETRY_VAR_NAME), photometryTexId);
  for (uint32_t i = 0; i < usedTextures.size(); ++i)
  {
    SharedTex tex = dag::get_tex_gameres(cstrVec[i]);
    G_ASSERT_CONTINUE(tex);

    TextureMetaData tmd;
    tmd.decode(tex->getTexName());
    PhotometryData data;
    data.rotated = bool(tmd.flags & tmd.FLG_IES_ROT);
    data.zoom = tmd.getIesScale();
    photometryData.push_back(data);
  }
}

void IesTextureCollection::addTexture(const char *textureName)
{
  // This implementation is not efficient, but it is rarely used
  // It's intended for testing only
  // The released scenes should have all photometry textures listed in the gameparames.blk
  usedTextures.push_back(String(textureName));
  reloadTextures();
}
