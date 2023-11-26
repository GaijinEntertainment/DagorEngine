#include "gpuGrassPanel.h"
#include "hmlCm.h"
#include "de3_gpuGrassService.h"
#include <ioSys/dag_dataBlock.h>
#include "GPUGrassTypePID.h"
#include "GPUGrassDecalPID.h"
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <libTools/util/strUtil.h>
#include <coolConsole/coolConsole.h>
#include <oldEditor/de_interface.h>
#include <EASTL/unique_ptr.h>

using hdpi::_pxScaled;

namespace
{
Tab<String> getDecalTypeNames(const GPUGrassDecal &decal)
{
  Tab<String> types;
  for (const auto &channel : decal.channels)
    types.push_back(String(channel.first.c_str()));
  return types;
}

void removeDecalType(GPUGrassDecal &decal, const char *name, DataBlock *blk)
{
  if (!decal.channels.empty())
    decal.channels.erase(
      eastl::remove_if(decal.channels.begin(), decal.channels.end(), [name](const auto &channel) { return channel.first == name; }),
      decal.channels.end());

  DataBlock *decalsBlk = blk->getBlockByName("decals");
  DataBlock *decalBlk = decalsBlk->getBlockByName(decal.name.c_str());
  decalBlk->removeParam(name);
}
} // namespace
void GPUGrassPanel::fillPanel(IGPUGrassService *service, DataBlock *grass_blk, PropPanel2 &panel)
{
  G_ASSERT_RETURN(service, );
  G_ASSERT_RETURN(grass_blk, );
  srv = service;
  blk = grass_blk;
  grassPanel = panel.createGroup(PID_GPU_GRASS_GROUP, "Grass");
  grassPanel->createCheckBox(PID_GPU_GRASS_ENABLE, "Enable grass", srv->isGrassEnabled());
  grassPanel->createButton(PID_GPU_GRASS_LOADFROM_LEVELBLK, "Load from level.blk");
  grassPanel->createFileEditBox(PID_GPU_GRASS_TYPES, "Types", blk->getStr("grass_types", ""));
  grassPanel->createFileEditBox(PID_GPU_GRASS_MASKS, "Masks", blk->getStr("grass_masks", ""));
  grassPanel->createTrackFloat(PID_GPU_GRASS_GRID_SIZE, "Grid size", blk->getReal("grass_grid_size"), 0.0, 1.0, 0.025);
  grassPanel->createEditFloat(PID_GPU_GRASS_DISTANCE, "Distance", blk->getReal("grass_distance"));
  grassPanel->createEditInt(PID_GPU_GRASS_MASK_RESOLUTION, "Mask resolution", blk->getInt("grassMaskResolution"));
  grassPanel->createEditFloat(PID_GPU_GRASS_HOR_SIZE_MUL, "Horizontal size mul", blk->getReal("hor_size_mul"));
  grassPanel->createStatic(0, "Type");
  grassPanel->createSeparator(0, false);
  grassPanel->createButton(PID_GPU_GRASS_ADD_TYPE, "Add...", srv->getTypeCount() < GRASS_MAX_TYPES);
  grassPanel->createButton(PID_GPU_GRASS_REMOVE_TYPE, "Remove...", srv->getTypeCount(), false);
  grassPanel->createButton(PID_GPU_GRASS_RENAME_TYPE, "Rename...", srv->getTypeCount(), false);

  for (int i = 0; i < srv->getTypeCount(); ++i)
  {
    GPUGrassType &type = srv->getType(i);
    if (type.name == "nograss")
      continue;
    int basePID = PID_GPU_GRASS_TYPE_START + i * (int)GPUGrassTypePID::COUNT;
    PropertyContainerControlBase *typePanel = grassPanel->createGroup(basePID + (int)GPUGrassTypePID::NAME, type.name.c_str());
    typePanel->createFileButton(basePID + (int)GPUGrassTypePID::DIFFUSE, "Diffuse", type.diffuse.c_str());
    typePanel->createFileButton(basePID + (int)GPUGrassTypePID::NORMAL, "Normal", type.normal.c_str());
    typePanel->createEditInt(basePID + (int)GPUGrassTypePID::VARIATIONS, "Variations", type.variations);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HEIGHT, "Height", type.height, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::SIZE_LOD_MUL, "Size multiplier per lod", type.size_lod_mul, 0.5f, 2.0f,
      0.01f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HT_RND_ADD, "Height random add", type.ht_rnd_add, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HOR_SIZE, "Horizontal size", type.hor_size, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HOR_SIZE_RND_ADD, "Horizontal size random add", type.hor_size_rnd_add,
      0.0f, 1.0f, 0.05f);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_R_FROM, "Color mask red from", type.color_mask_r_from);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_R_TO, "Color mask red to", type.color_mask_r_to);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_G_FROM, "Color mask green from", type.color_mask_g_from);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_G_TO, "Color mask green to", type.color_mask_g_to);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_B_FROM, "Color mask blue from", type.color_mask_b_from);
    typePanel->createColorBox(basePID + (int)GPUGrassTypePID::COLOR_MASK_B_TO, "Color mask blue to", type.color_mask_b_to);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HEIGHT_FROM_WEIGHT_MUL, "Height from weight mul",
      type.height_from_weight_mul, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::HEIGHT_FROM_WEIGHT_ADD, "Height from weight add",
      type.height_from_weight_add, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::DENSITY_FROM_WEIGHT_MUL, "Density from weight mul",
      type.density_from_weight_mul, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::DENSITY_FROM_WEIGHT_ADD, "Density from weight add",
      type.density_from_weight_add, 0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::VERTICAL_ANGLE_MUL, "Vertical angle mul", type.vertical_angle_mul,
      0.0f, 1.0f, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::VERTICAL_ANGLE_ADD, "Vertical angle add", type.vertical_angle_add, -PI,
      PI, 0.05f);
    typePanel->createTrackFloat(basePID + (int)GPUGrassTypePID::STIFFNESS, "Stiffness", type.stiffness, 0.0f, 1.0f, 0.05f);
    typePanel->createCheckBox(basePID + (int)GPUGrassTypePID::HORIZONTAL_GRASS, "Horizontal grass", type.horizontal_grass);
    typePanel->createCheckBox(basePID + (int)GPUGrassTypePID::UNDERWATER, "Underwater", type.underwater);
  }

  grassPanel->createStatic(0, "Decal");
  grassPanel->createSeparator(0, false);
  grassPanel->createButton(PID_GPU_GRASS_ADD_DECAL, "Add...", srv->getDecalCount() < GRASS_MAX_CHANNELS);
  grassPanel->createButton(PID_GPU_GRASS_REMOVE_DECAL, "Remove...", srv->getDecalCount(), false);
  grassPanel->createButton(PID_GPU_GRASS_RENAME_DECAL, "Rename...", srv->getDecalCount(), false);
  for (int i = 0; i < srv->getDecalCount(); ++i)
  {
    GPUGrassDecal &decal = srv->getDecal(i);
    int basePID = PID_GPU_GRASS_DECAL_START + i * (int)GPUGrassDecalPID::COUNT;
    PropertyContainerControlBase *decalPanel = grassPanel->createGroup(basePID + (int)GPUGrassDecalPID::NAME, decal.name.c_str());
    decalPanel->createEditInt(basePID + (int)GPUGrassDecalPID::ID, "Id", decal.id);
    decalPanel->createButton(basePID + (int)GPUGrassDecalPID::ADD_TYPE, "Add...", decal.channels.size() < MAX_TYPES_PER_CHANNEL);
    decalPanel->createButton(basePID + (int)GPUGrassDecalPID::REMOVE_TYPE, "Remove...", !decal.channels.empty(), false);
    int j = 0;
    for (auto &channel : decal.channels)
    {
      decalPanel->createEditFloat(basePID + (int)GPUGrassDecalPID::TYPES_START + j, channel.first.c_str(), channel.second, 0);
      ++j;
    }
  }
  panel.setBool(PID_GPU_GRASS_GROUP, true);
}

bool GPUGrassPanel::isGrassValid() const
{
  if (srv->getTypeCount() == 0 || (srv->getTypeCount() == 1 && srv->getType(0).name == "nograss"))
    return false;
  for (int i = 0; i < srv->getDecalCount(); ++i)
  {
    const GPUGrassDecal &decal = srv->getDecal(i);
    if (decal.channels.empty())
      continue;
    for (const auto &channel : decal.channels)
    {
      if (channel.second < 1.0)
        continue;
      const GPUGrassType *type = srv->getTypeByName(channel.first.c_str());
      if (!type)
        continue;
      if (!type->diffuse.empty() && !type->normal.empty())
        return true;
    }
  }
  return false;
}

void GPUGrassPanel::reload()
{
  if (isGrassValid())
    srv->createGrass(*blk);
  else
    srv->closeGrass();
}

void GPUGrassPanel::onChange(int pcb_id, PropPanel2 *panel)
{
  switch (pcb_id)
  {
    case PID_GPU_GRASS_ENABLE: srv->enableGrass(panel->getBool(pcb_id)); break;
    case PID_GPU_GRASS_MASKS:
    case PID_GPU_GRASS_TYPES:
    {
      SimpleString fname = panel->getText(pcb_id);
      String resName = ::get_file_name_wo_ext(fname);
      panel->setText(pcb_id, resName);
      blk->setStr(pcb_id == PID_GPU_GRASS_TYPES ? "grass_types" : "grass_masks", resName.c_str());
    }
    break;
    case PID_GPU_GRASS_GRID_SIZE: blk->setReal("grass_grid_size", panel->getFloat(PID_GPU_GRASS_GRID_SIZE)); break;
    case PID_GPU_GRASS_DISTANCE: blk->setReal("grass_distance", panel->getFloat(PID_GPU_GRASS_DISTANCE)); break;
    case PID_GPU_GRASS_MASK_RESOLUTION: blk->setInt("grassMaskResolution", panel->getInt(PID_GPU_GRASS_MASK_RESOLUTION)); break;
    case PID_GPU_GRASS_HOR_SIZE_MUL: blk->setReal("hor_size_mul", panel->getFloat(PID_GPU_GRASS_HOR_SIZE_MUL)); break;
  }
  if (pcb_id > PID_GPU_GRASS_ENABLE && pcb_id <= PID_GPU_GRASS_HOR_SIZE_MUL)
    reload();
  else if (pcb_id >= PID_GPU_GRASS_TYPE_START && pcb_id <= PID_GPU_GRASS_TYPE_END)
  {
    int basePid = pcb_id - PID_GPU_GRASS_TYPE_START;
    int index = basePid / (int)GPUGrassTypePID::COUNT;
    basePid -= index * (int)GPUGrassTypePID::COUNT;
    G_ASSERT(index < srv->getTypeCount());
    GPUGrassType &type = srv->getType(index);
    DataBlock *grassTypesBlk = blk->getBlockByName("grass_types");
    G_ASSERT_RETURN(grassTypesBlk, );
    DataBlock *typeBlk = grassTypesBlk->getBlockByName(type.name.c_str());
    G_ASSERT_RETURN(typeBlk, );
    switch (basePid)
    {
      case (int)GPUGrassTypePID::NORMAL:
      case (int)GPUGrassTypePID::DIFFUSE:
      {
        SimpleString fname = panel->getText(pcb_id);
        if (fname.empty())
          fname = (basePid == (int)GPUGrassTypePID::DIFFUSE) ? type.diffuse.c_str() : type.normal.c_str();
        String resName = ::get_file_name_wo_ext(fname);
        for (int i = 0; i < srv->getTypeCount(); ++i)
        {
          if (srv->getType(i).diffuse == resName.c_str() || srv->getType(i).normal == resName.c_str())
          {
            resName = (basePid == (int)GPUGrassTypePID::DIFFUSE) ? type.diffuse.c_str() : type.normal.c_str();
            wingw::message_box(wingw::MBS_EXCL, "Error",
              "Currently diffuse and normal textures supposed to be unique for each grass. check grass <%s> ",
              srv->getType(i).name.c_str());
            break;
          }
        }
        panel->setText(pcb_id, resName);
        typeBlk->setStr(basePid == (int)GPUGrassTypePID::DIFFUSE ? "diffuse" : "normal", resName.c_str());
        if (basePid == (int)GPUGrassTypePID::DIFFUSE)
          type.diffuse = resName;
        else
          type.normal = resName;
      }
      break;
      case (int)GPUGrassTypePID::VARIATIONS: typeBlk->setInt("variations", type.variations = panel->getInt(pcb_id)); break;
      case (int)GPUGrassTypePID::HEIGHT: typeBlk->setReal("height", type.height = panel->getFloat(pcb_id)); break;
      case (int)GPUGrassTypePID::SIZE_LOD_MUL: typeBlk->setReal("size_lod_mul", type.size_lod_mul = panel->getFloat(pcb_id)); break;
      case (int)GPUGrassTypePID::HT_RND_ADD: typeBlk->setReal("ht_rnd_add", type.ht_rnd_add = panel->getFloat(pcb_id)); break;
      case (int)GPUGrassTypePID::HOR_SIZE: typeBlk->setReal("hor_size", type.hor_size = panel->getFloat(pcb_id)); break;
      case (int)GPUGrassTypePID::HOR_SIZE_RND_ADD:
        typeBlk->setReal("hor_size_rnd_add", type.hor_size_rnd_add = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_R_FROM:
        typeBlk->setE3dcolor("color_mask_r_from", type.color_mask_r_from = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_R_TO:
        typeBlk->setE3dcolor("color_mask_r_to", type.color_mask_r_to = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_G_FROM:
        typeBlk->setE3dcolor("color_mask_g_from", type.color_mask_g_from = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_G_TO:
        typeBlk->setE3dcolor("color_mask_g_to", type.color_mask_g_to = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_B_FROM:
        typeBlk->setE3dcolor("color_mask_b_from", type.color_mask_b_from = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::COLOR_MASK_B_TO:
        typeBlk->setE3dcolor("color_mask_b_to", type.color_mask_b_to = panel->getColor(pcb_id));
        break;
      case (int)GPUGrassTypePID::HEIGHT_FROM_WEIGHT_MUL:
        typeBlk->setReal("height_from_weight_mul", type.height_from_weight_mul = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::HEIGHT_FROM_WEIGHT_ADD:
        typeBlk->setReal("height_from_weight_add", type.height_from_weight_add = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::DENSITY_FROM_WEIGHT_MUL:
        typeBlk->setReal("density_from_weight_mul", type.density_from_weight_mul = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::DENSITY_FROM_WEIGHT_ADD:
        typeBlk->setReal("density_from_weight_add", type.density_from_weight_add = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::VERTICAL_ANGLE_MUL:
        typeBlk->setE3dcolor("vertical_angle_mul", type.vertical_angle_mul = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::VERTICAL_ANGLE_ADD:
        typeBlk->setReal("vertical_angle_add", type.vertical_angle_add = panel->getFloat(pcb_id));
        break;
      case (int)GPUGrassTypePID::STIFFNESS: typeBlk->setReal("stiffness", type.stiffness = panel->getFloat(pcb_id)); break;
      case (int)GPUGrassTypePID::HORIZONTAL_GRASS:
        typeBlk->setBool("horizontal_grass", type.horizontal_grass = panel->getBool(pcb_id));
        break;
      case (int)GPUGrassTypePID::UNDERWATER: typeBlk->setBool("underwater", type.underwater = panel->getBool(pcb_id)); break;
    }
    reload();
  }
  else if (pcb_id >= PID_GPU_GRASS_DECAL_START && pcb_id <= PID_GPU_GRASS_DECAL_END)
  {
    int basePid = pcb_id - PID_GPU_GRASS_DECAL_START;
    int index = basePid / (int)GPUGrassDecalPID::COUNT;
    basePid -= index * (int)GPUGrassDecalPID::COUNT;
    G_ASSERT(index < srv->getDecalCount());
    GPUGrassDecal &decal = srv->getDecal(index);
    DataBlock *decalsBlk = blk->getBlockByName("decals");
    G_ASSERT_RETURN(decalsBlk, );
    DataBlock *decalBlk = decalsBlk->getBlockByName(decal.name.c_str());
    G_ASSERT_RETURN(decalBlk, );
    if (basePid == (int)GPUGrassDecalPID::ID)
    {
      int id = panel->getInt(pcb_id);
      if (srv->findDecalId(id))
        id = decal.id;
      if (id >= GRASS_MAX_CHANNELS)
        id = decal.id;
      panel->setInt(pcb_id, id);
      decalBlk->setInt("id", id);
    }
    else if (basePid >= (int)GPUGrassDecalPID::TYPES_START && basePid <= (int)GPUGrassDecalPID::TYPES_END)
    {
      int basePid2 = basePid - (int)GPUGrassDecalPID::TYPES_START;
      G_ASSERT_RETURN(basePid2 < decal.channels.size(), );
      auto &pair = decal.channels[basePid2];
      float val = panel->getFloat(pcb_id);
      if (val < 1.0)
        panel->setFloat(pcb_id, val = 1.0);
      decalBlk->setReal(pair.first.c_str(), pair.second = val);
    }
    reload();
  }
}

void GPUGrassPanel::onClick(int pcb_id, PropPanel2 *panel, const eastl::function<void()> &loadGPUGrassFromLevelBlk,
  const eastl::function<void()> &fillPanel)
{
  switch (pcb_id)
  {
    case PID_GPU_GRASS_LOADFROM_LEVELBLK:
      if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Overwrite grass settings?") == wingw::MB_ID_YES)
        loadGPUGrassFromLevelBlk();
      break;
    case PID_GPU_GRASS_ADD_TYPE:
    {
      eastl::string name;
      if (showNameDialog("Add type", name, [this](const char *name) { return !!srv->getTypeByName(name); }))
      {
        GPUGrassType &type = srv->addType(name.c_str());
        addGrassType(type);
        fillPanel();
      }
    }
    break;
    case PID_GPU_GRASS_REMOVE_TYPE:
    {
      Tab<String> types = getGrassTypeNames();
      Tab<int> result;
      if (showMultiListDialog("Remove type", types, result))
        if (!result.empty() &&
            wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Remove grass type(s)?") == wingw::MB_ID_YES)
        {
          for (int i = 0; i < result.size(); ++i)
          {
            const char *name = types[result[i]].c_str();
            srv->removeType(name);
            DataBlock *typesBlk = blk->getBlockByName("grass_types");
            typesBlk->removeBlock(name);
            for (int j = 0; j < srv->getDecalCount(); ++j)
              removeDecalType(srv->getDecal(j), name, blk);
          }
          fillPanel();
          reload();
        }
    }
    break;
    case PID_GPU_GRASS_RENAME_TYPE:
    {
      Tab<String> types = getGrassTypeNames();
      int result;
      if (showListDialog("Rename type", types, result))
      {
        eastl::string name = types[result].c_str();
        if (showNameDialog("Rename type", name, [this](const char *name) { return !!srv->getTypeByName(name); }))
        {
          GPUGrassType &type = srv->getType(result);
          for (int i = 0; i < srv->getDecalCount(); ++i)
          {
            GPUGrassDecal &decal = srv->getDecal(i);
            for (auto &channel : decal.channels)
              if (channel.first == type.name)
                channel.first = name.c_str();
          }
          DataBlock *decals = blk->getBlockByName("decals");
          int i = 0;
          while (DataBlock *decal = decals->getBlock(i))
          {
            int j = 0;
            while (const char *typeName = decal->getParamName(j))
            {
              if (type.name == typeName)
              {
                float val = decal->getReal(typeName);
                decal->removeParam(j);
                decal->addReal(name.c_str(), val);
                break;
              }
              else
                ++j;
            }
            ++i;
          }
          DataBlock *types = blk->getBlockByName("grass_types");
          types->removeBlock(type.name.c_str());
          type.name = name;
          addGrassType(type);
          fillPanel();
        }
      }
    }
    break;
    case PID_GPU_GRASS_ADD_DECAL:
    {
      eastl::string name;
      if (showNameDialog("Add decal", name, [this](const char *name) { return !!srv->getDecalByName(name); }))
      {
        GPUGrassDecal &decal = srv->addDecal(name.c_str());
        addGrassDecal(decal);
        fillPanel();
        reload();
      }
    }
    break;
    case PID_GPU_GRASS_REMOVE_DECAL:
    {
      Tab<String> decals = getGrassDecalNames();
      Tab<int> result;
      if (showMultiListDialog("Remove decal", decals, result))
        if (!result.empty() &&
            wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Remove grass decal(s)?") == wingw::MB_ID_YES)
        {
          for (int i = 0; i < result.size(); ++i)
          {
            const char *name = decals[result[i]].c_str();
            srv->removeDecal(name);
            DataBlock *decalBlks = blk->getBlockByName("decals");
            decalBlks->removeBlock(name);
          }
          fillPanel();
          reload();
        }
    }
    break;
    case PID_GPU_GRASS_RENAME_DECAL:
    {
      Tab<String> decals = getGrassDecalNames();
      int result;
      if (showListDialog("Rename decal", decals, result))
      {
        eastl::string name = decals[result].c_str();
        if (showNameDialog("Rename decal", name, [this](const char *name) { return !!srv->getDecalByName(name); }))
        {
          GPUGrassDecal &decal = srv->getDecal(result);
          DataBlock *decals = blk->getBlockByName("decals");
          decals->removeBlock(decal.name.c_str());
          decal.name = name;
          addGrassDecal(decal);
          fillPanel();
        }
      }
    }
    break;
  }
  int basePid = pcb_id - PID_GPU_GRASS_DECAL_START;
  int index = basePid / (int)GPUGrassDecalPID::COUNT;
  basePid -= index * (int)GPUGrassDecalPID::COUNT;
  if (basePid >= (int)GPUGrassDecalPID::ADD_TYPE && basePid <= (int)GPUGrassDecalPID::REMOVE_TYPE)
  {
    GPUGrassDecal &decal = srv->getDecal(index);
    switch (basePid)
    {
      case (int)GPUGrassDecalPID::ADD_TYPE:
      {
        Tab<String> types = getGrassTypeNames();
        types.push_back(String("nograss"));
        for (const auto &channel : decal.channels)
        {
          intptr_t ptr = find_value_idx(types, String{channel.first.c_str()});
          if (ptr >= 0)
            erase_items(types, ptr, 1);
        }
        int result;
        if (showListDialog("Add type", types, result))
        {
          const float defVal = 1.0;
          decal.channels.emplace_back(eastl::string{types[result].c_str()}, defVal);
          DataBlock *decalsBlk = blk->getBlockByName("decals");
          DataBlock *decalBlk = decalsBlk->getBlockByName(decal.name.c_str());
          decalBlk->addReal(types[result].c_str(), defVal);
          fillPanel();
          reload();
        }
      }
      break;
      case (int)GPUGrassDecalPID::REMOVE_TYPE:
      {
        Tab<String> types = getDecalTypeNames(decal);
        Tab<int> result;
        if (showMultiListDialog("Remove type", types, result))
          if (!result.empty() &&
              wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Remove grass type(s)?") == wingw::MB_ID_YES)
          {
            for (int i = 0; i < result.size(); ++i)
              removeDecalType(decal, types[result[i]].c_str(), blk);
            fillPanel();
            reload();
          }
      }
      break;
    }
  }
}

Tab<String> GPUGrassPanel::getGrassTypeNames() const
{
  Tab<String> types;
  for (int i = 0; i < srv->getTypeCount(); ++i)
  {
    if (srv->getType(i).name == "nograss")
      continue;
    types.push_back(String(srv->getType(i).name.c_str()));
  }
  return types;
}

Tab<String> GPUGrassPanel::getGrassDecalNames() const
{
  Tab<String> decals;
  for (int i = 0; i < srv->getDecalCount(); ++i)
    decals.push_back(String(srv->getDecal(i).name.c_str()));
  return decals;
}

void GPUGrassPanel::addGrassType(const GPUGrassType &type)
{
  DataBlock *types = blk->getBlockByName("grass_types");
  G_ASSERT_RETURN(types, );
  DataBlock *typeBlk = types->addBlock(type.name.c_str());
  typeBlk->addStr("diffuse", type.diffuse.c_str());
  typeBlk->addStr("normal", type.normal.c_str());
  typeBlk->addInt("variations", type.variations);
  typeBlk->addReal("height", type.height);
  typeBlk->addReal("size_lod_mul", type.size_lod_mul);
  typeBlk->addReal("ht_rnd_add", type.ht_rnd_add);
  typeBlk->addReal("hor_size", type.hor_size);
  typeBlk->addReal("hor_size_rnd_add", type.hor_size_rnd_add);
  typeBlk->addE3dcolor("color_mask_r_from", type.color_mask_r_from);
  typeBlk->addE3dcolor("color_mask_r_to", type.color_mask_r_to);
  typeBlk->addE3dcolor("color_mask_g_from", type.color_mask_g_from);
  typeBlk->addE3dcolor("color_mask_g_to", type.color_mask_g_to);
  typeBlk->addE3dcolor("color_mask_b_from", type.color_mask_b_from);
  typeBlk->addE3dcolor("color_mask_b_to", type.color_mask_b_to);
  typeBlk->addReal("height_from_weight_mul", type.height_from_weight_mul);
  typeBlk->addReal("height_from_weight_add", type.height_from_weight_add);
  typeBlk->addReal("density_from_weight_mul", type.density_from_weight_mul);
  typeBlk->addReal("density_from_weight_add", type.density_from_weight_add);
  typeBlk->addReal("vertical_angle_add", type.vertical_angle_add);
  typeBlk->addReal("vertical_angle_mul", type.vertical_angle_mul);

  typeBlk->addReal("stiffness", type.stiffness);
  typeBlk->addBool("horizontal_grass", type.horizontal_grass);
  typeBlk->addBool("underwater", type.underwater);
}

void GPUGrassPanel::addGrassDecal(const GPUGrassDecal &decal)
{
  DataBlock *decals = blk->getBlockByName("decals");
  G_ASSERT_RETURN(decals, );
  DataBlock *decalBlk = decals->addBlock(decal.name.c_str());
  decalBlk->addInt("id", decal.id);
  for (const auto &channel : decal.channels)
    decalBlk->addReal(channel.first.c_str(), channel.second);
}

bool GPUGrassPanel::showNameDialog(const char *title, eastl::string &res, const eastl::function<bool(const char *)> &findName)
{
  eastl::unique_ptr<CDialogWindow> dialog(DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), title));
  dialog->setInitialFocus(DIALOG_ID_NONE);
  PropPanel2 *dlgPanel = dialog->getPanel();
  enum
  {
    PCB_EDIT,
  };
  dlgPanel->createEditBox(PCB_EDIT, "Name");
  dlgPanel->setText(PCB_EDIT, res.c_str());
  dlgPanel->setFocusById(PCB_EDIT);
  while (true)
  {
    int ret = dialog->showDialog();
    if (ret == DIALOG_ID_OK)
    {
      SimpleString name = dlgPanel->getText(PCB_EDIT);
      if (name.empty())
        continue;
      if (findName(name.c_str()))
        wingw::message_box(wingw::MBS_EXCL, "Error", "Name already exists");
      else
      {
        res = name;
        return true;
      }
    }
    else
      break;
  }
  return false;
}

bool GPUGrassPanel::showMultiListDialog(const char *title, const Tab<String> &entries, Tab<int> &res)
{
  eastl::unique_ptr<CDialogWindow> dialog(DAGORED2->createDialog(_pxScaled(250), _pxScaled(620), title));
  PropPanel2 *dlgPanel = dialog->getPanel();
  enum
  {
    PCB_LIST,
  };
  clear_and_shrink(res);
  dlgPanel->createMultiSelectList(PCB_LIST, entries, _pxScaled(600));
  if (dialog->showDialog() == DIALOG_ID_OK)
  {
    dlgPanel->getSelection(PCB_LIST, res);
    return true;
  }
  return false;
}

bool GPUGrassPanel::showListDialog(const char *title, const Tab<String> &entries, int &res)
{
  eastl::unique_ptr<CDialogWindow> dialog(DAGORED2->createDialog(_pxScaled(250), _pxScaled(200), title));
  PropPanel2 *dlgPanel = dialog->getPanel();
  enum
  {
    PCB_LIST,
  };
  res = -1;
  dlgPanel->createList(PCB_LIST, "Name", entries, 0);
  if (dialog->showDialog() == DIALOG_ID_OK)
  {
    res = dlgPanel->getInt(PCB_LIST);
    return true;
  }
  return false;
}