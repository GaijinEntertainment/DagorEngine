#include "gpuGrassSrv.h"
#include <de3_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <oldEditor/de_interface.h>

bool RandomGPUGrassRenderHelper::isValid() const { return hmap != nullptr; }

bool RandomGPUGrassRenderHelper::beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &tm)
{
  this->box = box;
  if (!hmap)
    for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
    {
      IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
      IRenderingService *iface = p->queryInterface<IRenderingService>();
      if (stricmp(p->getInternalName(), "heightmapLand") == 0)
        hmap = iface;
    }

  if (!hmap)
    return false;
  hmap->prepare(center_pos, box);
  return true;
}

void RandomGPUGrassRenderHelper::endRender() {}

void RandomGPUGrassRenderHelper::renderColor()
{
  if (!hmap)
    return;
  hmap->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP);
}

void RandomGPUGrassRenderHelper::renderMask()
{
  if (!hmap)
    return;
  hmap->renderGeometry(IRenderingService::STG_RENDER_GRASS_MASK);
}

void GPUGrassService::closeGrass() { grass.reset(); }

void GPUGrassService::createGrass(DataBlock &grass_settings)
{
  closeGrass();
  grass = eastl::make_unique<GPUGrass>();
  grass->init(grass_settings);
  enumerateGrassTypes(grass_settings);
  enumerateGrassDecals(grass_settings);
}

DataBlock *GPUGrassService::createDefaultGrass()
{
  enabled = false;
  DataBlock *result = new DataBlock();
  result->addReal("grass_grid_size", 0.125);
  result->addReal("grass_distance", 150);
  result->addInt("grassMaskResolution", 1024);
  result->addReal("hor_size_mul", 1.0);
  DataBlock *types = result->addBlock("grass_types");
  DataBlock *nograss = types->addBlock("nograss");
  nograss->addReal("density_from_weight_mul", 0.0);
  nograss->addReal("density_from_weight_add", 0.0);
  result->addBlock("decals");
  enumerateGrassTypes(*result);
  enumerateGrassDecals(*result);
  return result;
}

void GPUGrassService::enableGrass(bool flag) { enabled = flag; }

void GPUGrassService::beforeRender(Stage stage)
{
  if (!grass || !enabled)
    return;

  const TMatrix &itm = ::grs_cur_view.itm;
  grass->generate(itm.getcol(3), itm.getcol(2), grassHelper, {});
}

void GPUGrassService::renderGeometry(Stage stage)
{
  if (!grass || !enabled)
    return;

  grass->render(grass->GRASS_NO_PREPASS);
}

BBox3 *GPUGrassService::getGrassBbox() { return &grassHelper.box; }

bool GPUGrassService::isGrassEnabled() const { return enabled; }

void GPUGrassService::enumerateGrassTypes(DataBlock &grassBlk)
{
  grassTypes.clear();
  DataBlock *types = grassBlk.getBlockByName("grass_types");
  if (!types)
    return;
  int i = 0;
  while (DataBlock *type = types->getBlock(i))
  {
    grassTypes.push_back({});
    grassTypes.back().name = type->getBlockName();
    grassTypes.back().diffuse = type->getStr("diffuse", "");
    grassTypes.back().normal = type->getStr("normal", "");
    grassTypes.back().variations = type->getInt("variations", 0);
    grassTypes.back().height = type->getReal("height", 1.0);
    grassTypes.back().size_lod_mul = type->getReal("size_lod_mul", 1.3);
    grassTypes.back().ht_rnd_add = type->getReal("ht_rnd_add", 0.0);
    grassTypes.back().hor_size = type->getReal("hor_size", 1.0);
    grassTypes.back().hor_size_rnd_add = type->getReal("hor_size_rnd_add", 0.0);
    grassTypes.back().color_mask_r_from = type->getE3dcolor("color_mask_r_from", {0, 0, 0, 0});
    grassTypes.back().color_mask_r_to = type->getE3dcolor("color_mask_r_to", {0, 0, 0, 0});
    grassTypes.back().color_mask_g_from = type->getE3dcolor("color_mask_g_from", {0, 0, 0, 0});
    grassTypes.back().color_mask_g_to = type->getE3dcolor("color_mask_g_to", {0, 0, 0, 0});
    grassTypes.back().color_mask_b_from = type->getE3dcolor("color_mask_b_from", {0, 0, 0, 0});
    grassTypes.back().color_mask_b_to = type->getE3dcolor("color_mask_b_to", {0, 0, 0, 0});
    grassTypes.back().height_from_weight_mul = type->getReal("height_from_weight_mul", 0.75);
    grassTypes.back().height_from_weight_add = type->getReal("height_from_weight_add", 0.25);
    grassTypes.back().density_from_weight_mul = type->getReal("density_from_weight_mul", 0.999);
    grassTypes.back().density_from_weight_add = type->getReal("density_from_weight_add", 0.0);
    grassTypes.back().vertical_angle_add = type->getReal("vertical_angle_add", -0.1);
    grassTypes.back().vertical_angle_mul = type->getReal("vertical_angle_mul", 0.2);

    grassTypes.back().stiffness = type->getReal("stiffness", 1.0);
    grassTypes.back().horizontal_grass = type->getBool("horizontal_grass", false);
    grassTypes.back().underwater = type->getBool("underwater", false);
    ++i;
  }
}

int GPUGrassService::getTypeCount() const { return (int)grassTypes.size(); }

GPUGrassType &GPUGrassService::getType(int index)
{
  G_ASSERT(index < grassTypes.size());
  return grassTypes[index];
}

GPUGrassType &GPUGrassService::addType(const char *name)
{
  grassTypes.push_back({});
  grassTypes.back().name = name;
  return grassTypes.back();
}

void GPUGrassService::removeType(const char *name)
{
  grassTypes.erase(eastl::remove_if(grassTypes.begin(), grassTypes.end(), [&](const auto &type) { return type.name == name; }),
    grassTypes.end());
}

GPUGrassType *GPUGrassService::getTypeByName(const char *name)
{
  for (auto &type : grassTypes)
    if (type.name == name)
      return &type;
  return nullptr;
}

void GPUGrassService::enumerateGrassDecals(DataBlock &grassBlk)
{
  grassDecals.clear();
  DataBlock *decals = grassBlk.getBlockByName("decals");
  if (!decals)
    return;
  int i = 0;
  while (DataBlock *decal = decals->getBlock(i))
  {
    grassDecals.push_back({});
    grassDecals.back().name = decal->getBlockName();
    grassDecals.back().id = decal->getInt("id", 0);
    int j = 0;
    while (const char *name = decal->getParamName(j))
    {
      if (!strcmp(name, "id"))
      {
        ++j;
        continue;
      }
      grassDecals.back().channels.emplace_back(eastl::string{name}, decal->getReal(name));
      ++j;
    }
    ++i;
  }
}

int GPUGrassService::getDecalCount() const { return (int)grassDecals.size(); }

GPUGrassDecal &GPUGrassService::getDecal(int index)
{
  G_ASSERT(index < grassDecals.size());
  return grassDecals[index];
}

bool GPUGrassService::findDecalId(int id) const
{
  return eastl::find_if(grassDecals.begin(), grassDecals.end(), [id](const auto &decal) { return decal.id == id; }) !=
         grassDecals.end();
}

int GPUGrassService::findFreeDecalId() const
{
  int id = 1;
  while (
    eastl::find_if(grassDecals.begin(), grassDecals.end(), [id](const auto &decal) { return decal.id == id; }) != grassDecals.end())
    ++id;
  return id;
}

GPUGrassDecal &GPUGrassService::addDecal(const char *name)
{
  int id = findFreeDecalId();
  grassDecals.push_back({});
  grassDecals.back().name = name;
  grassDecals.back().id = id;
  return grassDecals.back();
}

void GPUGrassService::removeDecal(const char *name)
{
  grassDecals.erase(eastl::remove_if(grassDecals.begin(), grassDecals.end(), [&](const auto &decal) { return decal.name == name; }),
    grassDecals.end());
}

GPUGrassDecal *GPUGrassService::getDecalByName(const char *name)
{
  for (auto &decal : grassDecals)
    if (decal.name == name)
      return &decal;
  return nullptr;
}

void GPUGrassService::invalidate()
{
  if (grass)
    grass->invalidate();
}

static GPUGrassService srv;

void setup_gpu_grass_service(const DataBlock &app_blk)
{
  srv.srvEnabled = app_blk.getBlockByNameEx("projectDefaults")->getBool("gpuGrass", false);
}
void *get_gpu_grass_service()
{
  if (!srv.srvEnabled)
    return NULL;
  return &srv;
}