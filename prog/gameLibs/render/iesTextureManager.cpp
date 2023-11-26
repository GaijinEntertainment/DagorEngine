#include <render/iesTextureManager.h>

#include <3d/dag_drv3d.h>
#include <debug/dag_debug3d.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <util/dag_texMetaData.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4D.h>

#include <EASTL/algorithm.h>

static const char *const PHOTOMETRY_TEX_NAME = "photometry_textures";
static const char *const PHOTOMETRY_VAR_NAME = "photometry_textures_tex";

IesTextureCollection *IesTextureCollection::instance = nullptr;
const char *IesTextureCollection::EDITOR_TEXTURE_NAME = "__ies_editor_tex__";

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
  if (strcmp(name, EDITOR_TEXTURE_NAME) == 0)
    return usedTextures.size();
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

IesTextureCollection::PhotometryData IesTextureCollection::getTextureData(int tex_idx) const
{
  if (tex_idx < 0)
    return {};
  // tex_idx == photometryData.size() is a special value, it indicates that this is the edited texture
  // The edited texture currently only supports default photometry parameters
  if (tex_idx >= photometryData.size())
    return {};
  return photometryData[tex_idx];
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

TEXTUREID IesTextureCollection::getTextureArrayId() { return photometryTexId; }

void IesTextureCollection::addTexture(const char *textureName)
{
  // This implementation is not efficient, but it is rarely used
  // It's intended for testing only
  // The released scenes should have all photometry textures listed in the gameparames.blk
  usedTextures.push_back(String(textureName));
  reloadTextures();
}

IesEditor *IesTextureCollection::requireEditor()
{
  if (!editor)
    editor = eastl::make_unique<IesEditor>();
  return editor.get();
}

IPoint3 IesEditor::getTexResolution(BaseTexture *photometry_tex_array)
{
  if (photometry_tex_array == nullptr)
    return IPoint3(0, 0, 0);
  TextureInfo textureInfo;
  photometry_tex_array->getinfo(textureInfo);
  return IPoint3(textureInfo.w, textureInfo.h, textureInfo.a);
}

void IesEditor::ensureTextureCreated(BaseTexture *photometry_tex_array)
{
  IPoint3 photometryResolution = getTexResolution(photometry_tex_array);
  IPoint3 newResolution = IPoint3(photometryResolution.x, photometryResolution.y, photometryResolution.z + 1);
  if (newResolution == currentResolution)
    return;
  dynamicIesTexArray.close();
  currentResolution = newResolution;
  if (photometry_tex_array == nullptr)
    return;
  dynamicIesTexArray = dag::create_array_tex(currentResolution.x, currentResolution.y, currentResolution.z,
    TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE | TEXCF_SRGBREAD | TEXFMT_L8, 1, "ies_editor_tex");
  for (int i = 0; i < photometryResolution.z; ++i)
  {
    dynamicIesTexArray.getArrayTex()->updateSubRegion(photometry_tex_array, i, 0, 0, 0, photometryResolution.x, photometryResolution.y,
      1, i, 0, 0, 0);
  }
  d3d::resource_barrier({dynamicIesTexArray.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  static int photometry_textures_texVarId = get_shader_variable_id("photometry_textures_tex", true);
  ShaderGlobal::set_texture(photometry_textures_texVarId, dynamicIesTexArray.getTexId());
}

IesEditor::IesEditor() : iesGenerator("ies_generator")
{
  IesTextureCollection *photometryTextures = IesTextureCollection::getSingleton();
  if (TEXTUREID photometryTexArrayId = photometryTextures->getTextureArrayId())
  {
    BaseTexture *photometryTexArray = ::acquire_managed_tex(photometryTexArrayId);
    ensureTextureCreated(photometryTexArray);
    ::release_managed_tex(photometryTexArrayId);
  }
  sortedPhotometryControlPointsBuf =
    dag::buffers::create_persistent_sr_structured(sizeof(PhotometryControlPoint), MAX_NUM_CONTROL_POINTS, "ies_control_points_buf");
}

void IesEditor::renderIesTexture()
{
  TIME_D3D_PROFILE(iesTextureGen);
  if (currentResolution.lengthSq() == 0 || dynamicIesTexArray.getBaseTex() == nullptr)
    return;
  if (!needsRender)
    return;
  needsRender = false;

  if (!sortedPhotometryControlPoints.empty())
    sortedPhotometryControlPointsBuf->updateDataWithLock(0,
      sizeof(sortedPhotometryControlPoints[0]) * sortedPhotometryControlPoints.size(), sortedPhotometryControlPoints.data(),
      VBLOCK_WRITEONLY);

  float maxLight = 0;
  for (const auto &controlPoint : sortedPhotometryControlPoints)
    maxLight = eastl::max(maxLight, controlPoint.lightIntensity);
  static int num_control_pointsVarId = get_shader_variable_id("num_control_points", false);
  static int ies_max_light_levelVarId = get_shader_variable_id("ies_max_light_level", false);
  ShaderGlobal::set_int(num_control_pointsVarId, sortedPhotometryControlPoints.size());
  ShaderGlobal::set_real(ies_max_light_levelVarId, maxLight);

  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(dynamicIesTexArray.getBaseTex(), currentResolution.z - 1, 0);
    iesGenerator.render();
  }
}

void IesEditor::clearControlPoints()
{
  sortedPhotometryControlPoints.clear();
  recalcCoefficients();
}

void IesEditor::removeControlPoint(int index)
{
  sortedPhotometryControlPoints.erase(sortedPhotometryControlPoints.begin() + index);
  recalcCoefficients();
}

void IesEditor::recalcCoefficients()
{
  needsRender = true;
  if (sortedPhotometryControlPoints.empty())
    return;
  for (int i = 0; i < sortedPhotometryControlPoints.size(); ++i)
  {
    if (i + 1 < sortedPhotometryControlPoints.size())
    {
      float t0 = sortedPhotometryControlPoints[i].theta;
      float l0 = sortedPhotometryControlPoints[i].lightIntensity;
      float l1 = sortedPhotometryControlPoints[i + 1].lightIntensity;
      float t1 = sortedPhotometryControlPoints[i + 1].theta;

      float y = safediv(l1 - l0, t1 - t0);
      float x = l0 - y * t0;
      sortedPhotometryControlPoints[i].coefficients = float2(x, y);
    }
    else
      sortedPhotometryControlPoints[i].coefficients = float2(sortedPhotometryControlPoints[i].lightIntensity, 0);
  }
}

int IesEditor::addPointWithoutUpdate(float theta, float intensity)
{
  sortedPhotometryControlPoints.resize(sortedPhotometryControlPoints.size() + 1);
  int index = sortedPhotometryControlPoints.size() - 1;
  while (index > 0 && sortedPhotometryControlPoints[index - 1].theta > theta)
  {
    sortedPhotometryControlPoints[index] = sortedPhotometryControlPoints[index - 1];
    index--;
  }
  sortedPhotometryControlPoints[index] = {float2(0, 0), theta, intensity};
  return index;
}

int IesEditor::addPointWithUpdate(float theta, float intensity)
{
  int index = addPointWithoutUpdate(theta, intensity);
  recalcCoefficients();
  return index;
}

float IesEditor::getLightIntensityAt(float theta) const
{
  if (sortedPhotometryControlPoints.empty())
    return 1;
  int a = 0, b = sortedPhotometryControlPoints.size();
  while (a + 1 < b)
  {
    int m = (a + b) / 2;
    if (theta < sortedPhotometryControlPoints[m].theta)
      b = m;
    else
      a = m;
  }
  const auto &coeffs = sortedPhotometryControlPoints[a].coefficients;
  theta = eastl::max(theta, sortedPhotometryControlPoints[a].theta); // clamp on low end
  if (a + 1 < sortedPhotometryControlPoints.size())
    theta = eastl::min(theta, sortedPhotometryControlPoints[a + 1].theta); // clamp on high end

  return coeffs.x + theta * coeffs.y;
}