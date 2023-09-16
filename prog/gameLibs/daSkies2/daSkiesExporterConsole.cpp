#include <util/dag_console.h>
#include <daSkies2/daScattering.h>
#include <daSkies2/daSkies.h>
#include <EASTL/string.h>
#include <osApiWrappers/dag_direct.h>
#include "daSkiesBlkGetSet.h"

struct NamedDataBlock
{
  eastl::string name;
  DataBlock blk;
};

namespace skies_utils
{
void get_rand_value(Point2 &v, const DataBlock &blk, const char *name, float def)
{
  int paramIndex = blk.findParam(name);
  if (blk.getParamType(paramIndex) == DataBlock::TYPE_POINT2)
    v = blk.getPoint2(name);
  else if (blk.getParamType(paramIndex) == DataBlock::TYPE_REAL)
  {
    float val = blk.getReal(name);
    v = Point2(val, val);
  }
  else
    v = Point2(def, def);
}
} // namespace skies_utils

static bool is_layer(const char *name) { return strstr(name, "layer"); }
static int get_layer_index(const char *name)
{
  if (strstr(name, "layers[0]."))
    return 0;
  G_ASSERT(strstr(name, "layers[1]."));
  return 1;
}
static const char *get_name_in_layer(const char *name)
{
  const char *dot = strstr(name, ".");
  G_ASSERT_RETURN(dot, nullptr);
  return dot + 1;
}

template <typename T>
static void set_to_entity(DataBlock &entity_blk, DataBlock *layer0, DataBlock *layer1, const eastl::string &prefix, const char *name,
  const T &result)
{
  if (is_layer(name))
  {
    G_ASSERT(layer0 && layer1);
    skies_utils::set_value(get_layer_index(name) == 0 ? *layer0 : *layer1, get_name_in_layer(name), result);
  }
  else
  {
    eastl::string fullName = prefix + name;
    skies_utils::set_value(entity_blk, fullName.c_str(), result);
  }
}


#define _PARAM_RAND(tp, name, def_value)                             \
  {                                                                  \
    Point2 result;                                                   \
    skies_utils::get_rand_value(result, blk, #name, def_value);      \
    set_to_entity(entityBlk, layer0, layer1, prefix, #name, result); \
  }
#define _PARAM_RAND_CLAMP(tp, name, def_value, min, max) _PARAM_RAND(tp, name, def_value)
#define _PARAM(tp, name, def_value)                                  \
  {                                                                  \
    tp result;                                                       \
    skies_utils::get_value(result, blk, #name, def_value);           \
    set_to_entity(entityBlk, layer0, layer1, prefix, #name, result); \
  }

static DataBlock convert_to_entity(const NamedDataBlock &weather_blk)
{
  DataBlock weatherTemplate;
  DataBlock &entityBlk = *weatherTemplate.addBlock(weather_blk.name.c_str());
  entityBlk.setStr("_extends", "skies_settings");

  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("clouds_rendering");
    DataBlock *layer0 = nullptr;
    DataBlock *layer1 = nullptr;
    eastl::string prefix = "clouds_rendering__";
    CLOUDS_RENDERING_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("strata_clouds");
    DataBlock *layer0 = nullptr;
    DataBlock *layer1 = nullptr;
    eastl::string prefix = "strata_clouds__";
    STRATA_CLOUDS_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("clouds_form");
    DataBlock *array = entityBlk.addBlock("clouds_form__layers:array");
    DataBlock *layer0 = array->addNewBlock("clouds_form__layers:object");
    DataBlock *layer1 = array->addNewBlock("clouds_form__layers:object");
    eastl::string prefix = "clouds_form__";
    CLOUDS_FORM_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("clouds_settings");
    DataBlock *layer0 = nullptr;
    DataBlock *layer1 = nullptr;
    eastl::string prefix = "clouds_settings__";
    CLOUDS_SETTINGS_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("clouds_weather_gen");
    DataBlock *array = entityBlk.addBlock("clouds_weather_gen__layers:array");
    DataBlock *layer0 = array->addNewBlock("clouds_weather_gen__layers:object");
    DataBlock *layer1 = array->addNewBlock("clouds_weather_gen__layers:object");
    eastl::string prefix = "clouds_weather_gen__";
    CLOUDS_WEATHER_GEN_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("ground");
    DataBlock *layer0 = nullptr;
    DataBlock *layer1 = nullptr;
    eastl::string prefix = "sky_atmosphere__";
    GROUND_PARAMS
  }
  {
    const DataBlock &blk = *weather_blk.blk.getBlockByNameEx("sky");
    DataBlock *layer0 = nullptr;
    DataBlock *layer1 = nullptr;
    eastl::string prefix = "sky_atmosphere__";
    SKY_PARAMS
  }
  return weatherTemplate;
}

#undef _PARAM
#undef _PARAM_RAND
#undef _PARAM_RAND_CLAMP

static bool convert_weather_blk_to_entity_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("skies", "convert_weather_blk_to_entity", 2, 2)
  {
    char folderBuf[260];
    strcpy(folderBuf, argv[1]);
    dd_append_slash_c(folderBuf);
    eastl::string folder = folderBuf;
    eastl::string mask = folder + "*.blk";
    eastl::vector<NamedDataBlock> weatherBlks;

    alefind_t fh;
    if (dd_find_first(mask.c_str(), 0, &fh))
    {
      do
      {
        char buf[260];
        const char *fname = dd_get_fname_without_path_and_ext(buf, countof(buf), fh.name);
        eastl::string filePath = folder + fh.name;
        DataBlock weatherBlk;
        weatherBlk.load(filePath.c_str());
        // It's considered a weather blk, if it contains at least one of these blocks.
        if (weatherBlk.blockCountByName("clouds_rendering") > 0 || weatherBlk.blockCountByName("strata_clouds") > 0 ||
            weatherBlk.blockCountByName("clouds_form") > 0 || weatherBlk.blockCountByName("clouds_settings") > 0 ||
            weatherBlk.blockCountByName("clouds_weather_gen") > 0 || weatherBlk.blockCountByName("ground") > 0 ||
            weatherBlk.blockCountByName("sky") > 0)
          weatherBlks.push_back({eastl::string(fname), eastl::move(weatherBlk)});
      } while (dd_find_next(&fh));
      dd_find_close(&fh);
    }

    for (auto &weatherBlk : weatherBlks)
    {
      DataBlock weatherTemplate = convert_to_entity(weatherBlk);
      eastl::string outputFile = folder + weatherBlk.name + "_converted.blk";
      weatherTemplate.saveToTextFile(outputFile.c_str());
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(convert_weather_blk_to_entity_console_handler);