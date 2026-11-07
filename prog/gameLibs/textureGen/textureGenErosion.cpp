// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_rwResource.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

static class TextureGenErosion : public TextureGenShader
{
private:
  static constexpr int OLD_HARDNESS_INPUT = 1, HARDNESS_INPUT = 4;
  int h, w;
  eastl::vector<float> hardnessMap, heightMap;
  eastl::optional<eastl::vector<float>> sedimentMap, depositMap, waterFlowMap, flowMapX, flowMapY;
  int erosion_radius, drop_lifetime;
  float inertia, sediment_capacity_factor, min_sediment_capacity, deposit_speed, erosion_speed, evaporate_speed, gravity,
    start_drop_speed, start_drop_water, flow_map_inertion, drops_multiplier;
  bool flow_map_normalized, flow_map_signed;
  eastl::vector<float> erosion_weights;
  eastl::vector<IPoint2> erosion_index;

  void calculate_conus_weights(int radius, eastl::vector<float> &conus_weight, eastl::vector<IPoint2> &conus_index)
  {
    radius -= 1;
    float sum_weights = 0;
    conus_weight.clear();
    conus_index.clear();
    size_t reserveSize = (radius * 2 + 1) * (radius * 2 + 1);
    conus_weight.reserve(reserveSize);
    conus_index.reserve(reserveSize);
    for (int dy = -radius; dy <= radius; dy++)
    {
      for (int dx = -radius; dx <= radius; dx++)
      {
        float len = sqrt(dx * dx + dy * dy);
        if (len <= radius)
        {
          float weight = radius > 0 ? 1 - len / radius : 1;
          conus_weight.push_back(weight);
          sum_weights += weight;
          conus_index.push_back(IPoint2(dx, dy));
        }
      }
    }
    for (int i = 0; i < conus_weight.size(); ++i)
      conus_weight[i] /= sum_weights;
  }

  void read_parameters(const DataBlock &data, TextureGenerator &gen)
  {
#define CHECK_BOUNDS(var_name, min_val, max_val)                                                                            \
  if (var_name < min_val)                                                                                                   \
  {                                                                                                                         \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "the %s must be greater than or equal %f", #var_name, min_val)); \
    var_name = min_val;                                                                                                     \
  }                                                                                                                         \
  else if (max_val < var_name)                                                                                              \
  {                                                                                                                         \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "the %s must be less than or equal %f", #var_name, max_val));    \
    var_name = max_val;                                                                                                     \
  }
#define CHECK_FLOAT_PARAMETERS(var_name, def_value, min_val, max_val) \
  var_name = data.getReal(#var_name, def_value);                      \
  CHECK_BOUNDS(var_name, min_val, max_val)
#define CHECK_INT_PARAMETERS(var_name, def_value, min_val, max_val) \
  var_name = data.getInt(#var_name, def_value);                     \
  CHECK_BOUNDS(var_name, min_val, max_val)

#define CHECK_BOOL_PARAMETERS(var_name, def_value) var_name = data.getBool(#var_name, def_value);

    CHECK_FLOAT_PARAMETERS(inertia, 0.5f, 0.f, 1.f)
    CHECK_FLOAT_PARAMETERS(sediment_capacity_factor, 1.f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(min_sediment_capacity, 0.01f, 0.f, 1.f)
    CHECK_FLOAT_PARAMETERS(deposit_speed, 0.1f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(erosion_speed, 0.1f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(evaporate_speed, 0.1f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(gravity, 1.f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(start_drop_speed, 1.f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(start_drop_water, 1.f, 0.f, 100.f)
    CHECK_FLOAT_PARAMETERS(flow_map_inertion, 0.5f, 0.f, 1.f)
    CHECK_FLOAT_PARAMETERS(drops_multiplier, 1.f, 0.f, 100.f)

    CHECK_INT_PARAMETERS(erosion_radius, 1, 1, 7)
    CHECK_INT_PARAMETERS(drop_lifetime, 1, 1, 70)

    CHECK_BOOL_PARAMETERS(flow_map_normalized, false)
    CHECK_BOOL_PARAMETERS(flow_map_signed, false)

    calculate_conus_weights(erosion_radius, erosion_weights, erosion_index);
  }

  int index(int x, int y)
  {
    x = clamp(x, 0, w - 1);
    y = clamp(y, 0, h - 1);
    return y * w + x;
  }

  int index(IPoint2 p) { return index(p.x, p.y); }

  Point3 CalculateHeightAndGradient(float posX, float posY)
  {
    int coordX = (int)posX;
    int coordY = (int)posY;

    float x = posX - coordX;
    float y = posY - coordY;

    float heightNW = heightMap[index(coordX, coordY)];
    float heightNE = heightMap[index(coordX + 1, coordY)];
    float heightSW = heightMap[index(coordX, coordY + 1)];
    float heightSE = heightMap[index(coordX + 1, coordY + 1)];

    float gradientX = (heightNE - heightNW) * (1 - y) + (heightSE - heightSW) * y;
    float gradientY = (heightSW - heightNW) * (1 - x) + (heightSE - heightNE) * x;

    float height = heightNW * (1 - x) * (1 - y) + heightNE * x * (1 - y) + heightSW * (1 - x) * y + heightSE * x * y;

    return Point3(gradientX, gradientY, height);
  }

  bool InsideMap(float x, float y)
  {
    const float border = 1.5;
    return border <= x && x < w - border && border <= y && y < h - border;
  }

  void Deposition(float posX, float posY, float amountToDeposit)
  {
    int coordX = (int)posX;
    int coordY = (int)posY;
    float cellOffsetX = posX - coordX;
    float cellOffsetY = posY - coordY;
    heightMap[index(coordX, coordY)] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
    heightMap[index(coordX + 1, coordY)] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
    heightMap[index(coordX, coordY + 1)] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
    heightMap[index(coordX + 1, coordY + 1)] += amountToDeposit * cellOffsetX * cellOffsetY;
    if (depositMap)
    {
      (*depositMap)[index(coordX, coordY)] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
      (*depositMap)[index(coordX + 1, coordY)] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
      (*depositMap)[index(coordX, coordY + 1)] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
      (*depositMap)[index(coordX + 1, coordY + 1)] += amountToDeposit * cellOffsetX * cellOffsetY;
    }
  }

  void process_drops(int iterations)
  {
    for (int iteration = 0; iteration < iterations; ++iteration)
    {
      float x = rnd_float(0, w);
      float y = rnd_float(0, h);
      Point2 position = Point2(x, y);
      float speed = start_drop_speed;
      float water = start_drop_water;
      float sediment = 0;
      Point2 dir(0, 0);
      IPoint2 node(0, 0);
      for (int lifetime = 0; lifetime < drop_lifetime; lifetime++)
      {
        node = IPoint2((int)position.x, (int)position.y);
        Point2 cellOffset = position - node;
        Point3 heightAndGradient = CalculateHeightAndGradient(position.x, position.y);
        Point2 grad = Point2(heightAndGradient.x, heightAndGradient.y);
        dir = (dir * inertia - grad * (1 - inertia));
        if (flowMapY && flowMapX)
        {
          for (int i = 0, n = erosion_index.size(); i < n; ++i)
          {
            int erodeIndex = index(node + erosion_index[i]);
            float weightedErodeAmount = erosion_weights[i];
            (*flowMapY)[erodeIndex] = lerp((*flowMapY)[erodeIndex], dir.y, flow_map_inertion * weightedErodeAmount);
            (*flowMapX)[erodeIndex] = lerp((*flowMapX)[erodeIndex], dir.x, flow_map_inertion * weightedErodeAmount);
          }
        }
        if (waterFlowMap)
        {
          (*waterFlowMap)[index(node)] += water;
        }
        dir /= max(0.01f, length(dir));
        position += dir;
        if (length(dir) < 0.01f || !InsideMap(position.x, position.y))
        {
          break;
        }
        float newHeight = CalculateHeightAndGradient(position.x, position.y).z;
        float deltaHeight = newHeight - heightAndGradient.z;
        float sedimentCapacity = max(-deltaHeight * speed * water * sediment_capacity_factor, min_sediment_capacity);
        if (sediment > sedimentCapacity || deltaHeight > 0)
        {
          float amountToDeposit = (deltaHeight > 0) ? min(deltaHeight, sediment) : (sediment - sedimentCapacity) * deposit_speed;

          sediment -= amountToDeposit;
          Deposition(node.x + cellOffset.x, node.y + cellOffset.y, amountToDeposit);
        }
        else
        {
          float amountToErode = min((sedimentCapacity - sediment) * erosion_speed, -deltaHeight) * (1 - hardnessMap[index(node)]);
          for (int i = 0, n = erosion_index.size(); i < n; ++i)
          {
            int erodeIndex = index(node + erosion_index[i]);
            float weightedErodeAmount = amountToErode * erosion_weights[i];
            float deltaSediment = min(heightMap[erodeIndex], weightedErodeAmount);
            heightMap[erodeIndex] -= deltaSediment;
            if (sedimentMap)
              (*sedimentMap)[erodeIndex] += deltaSediment;
            sediment += deltaSediment;
          }
        }

        speed = sqrt(max(0.f, speed * speed + deltaHeight * gravity));
        water *= (1 - evaporate_speed);
      }
    }
  }

  bool toNodeMap(BaseTexture *&src_texture, Texture *&converted, bool any_format = false, uint32_t fmt = TEXFMT_R32F,
    bool one_texel_is_no_map = false)
  {
    TextureInfo texInfo;
    src_texture->getinfo(texInfo);
    if ((texInfo.h == h && texInfo.w == w && (any_format || (texInfo.cflg & TEXFMT_MASK) == fmt)) ||
        (one_texel_is_no_map && texInfo.w == 1 && texInfo.h == 1))
      return true;
    converted = d3d::create_tex(NULL, w, h, fmt | TEXCF_RTARGET, 1, "erosion_input", RESTAG_TEXGEN);
    if (!converted)
      return false;
    d3d::stretch_rect(src_texture, converted);
    src_texture = converted;
    return true;
  }

  bool readTexture(BaseTexture *src_texture, eastl::vector<float> &dest_array)
  {
    dest_array.resize(h * w);
    if (!src_texture)
    {
      memset(dest_array.data(), 0, h * w * sizeof(float));
      return true;
    }
    Texture *converted = nullptr;
    if (!toNodeMap(src_texture, converted))
      return false;
    void *mapData = nullptr;
    int stride = 0;
    const bool locked = src_texture->lockimg(&mapData, stride, 0, TEXLOCK_READ);
    if (locked && mapData)
      for (int row = 0; row < h; row++)
        memcpy(dest_array.data() + size_t(row) * w, (uint8_t *)mapData + size_t(row) * stride, w * sizeof(float));
    if (locked)
      src_texture->unlockimg();
    del_d3dres(converted);
    return locked && mapData;
  }

  static bool mayBeAboveZero(BaseTexture *tex)
  {
    TextureInfo info;
    tex->getinfo(info);
    Texture *converted = nullptr;
    if ((info.cflg & TEXFMT_MASK) != TEXFMT_R32F)
    {
      converted = d3d::create_tex(NULL, info.w, info.h, TEXFMT_R32F | TEXCF_RTARGET, 1, "erosion_old_hardness", RESTAG_TEXGEN);
      if (!converted)
        return true;
      d3d::stretch_rect(tex, converted);
      tex = converted;
    }
    void *mapData = nullptr;
    int stride = 0;
    const bool locked = tex->lockimg(&mapData, stride, 0, TEXLOCK_READ);
    bool zero = locked && mapData;
    for (int row = 0; zero && row < info.h; row++)
    {
      const float *texels = (const float *)((const uint8_t *)mapData + size_t(row) * stride);
      zero = eastl::all_of(texels, texels + info.w, [](float v) { return v <= 0; });
    }
    if (locked)
      tex->unlockimg();
    del_d3dres(converted);
    return !zero;
  }

  static TextureInput hardnessInput(dag::ConstSpan<TextureInput> inputs)
  {
    return inputs.size() > HARDNESS_INPUT ? inputs[HARDNESS_INPUT] : TextureInput();
  }

  bool writeTexture(Texture *dest_texture, const eastl::vector<float> *src_array, Texture *temp_tex)
  {
    if (!dest_texture || !src_array)
      return true;
    void *mapData = nullptr;
    int stride = 0;
    if (!temp_tex->lockimg(&mapData, stride, 0, TEXLOCK_WRITE) || !mapData)
      return false;
    for (int row = 0; row < h; row++)
      memcpy((uint8_t *)mapData + size_t(row) * stride, src_array->data() + size_t(row) * w, w * sizeof(float));
    temp_tex->unlockimg();
    dest_texture->update(temp_tex);
    return true;
  }

  eastl::optional<eastl::vector<float>> prepareCleanMap(bool hasMap, size_t mapSize)
  {
    if (hasMap)
      return eastl::vector<float>(mapSize, 0);
    return eastl::nullopt;
  }

  bool cpu_algorithm(TextureGenerator &gen, dag::ConstSpan<TextureInput> &inputs, dag::ConstSpan<D3dResource *> &outputs)
  {
    size_t mapSize = h * w;
    auto outAt = [&](int i) -> Texture * { return i < outputs.size() ? (Texture *)outputs[i] : nullptr; };

    if (!inputs[0].tex || !readTexture(inputs[0].tex, heightMap) || !readTexture(hardnessInput(inputs).tex, hardnessMap))
      return false;

    sedimentMap = prepareCleanMap(outAt(1) != nullptr, mapSize);
    depositMap = prepareCleanMap(outAt(2) != nullptr, mapSize);
    flowMapX = prepareCleanMap(outAt(3) != nullptr, mapSize);
    flowMapY = prepareCleanMap(outAt(4) != nullptr, mapSize);
    waterFlowMap = prepareCleanMap(outAt(5) != nullptr, mapSize);

    set_rnd_seed(630498480);
    process_drops((int)(h * w * drops_multiplier));

    if (flowMapX && flowMapY)
    {
      float maxSqLen = 0;
      for (int i = 0; i < mapSize; i++)
        maxSqLen = max(maxSqLen, (*flowMapX)[i] * (*flowMapX)[i] + (*flowMapY)[i] * (*flowMapY)[i]);
      float maxLenInv = maxSqLen > 0 ? 1 / sqrt(maxSqLen) : 0;
      for (int i = 0; i < mapSize; i++)
      {
        Point2 flow = Point2((*flowMapX)[i], (*flowMapY)[i]) * maxLenInv;
        if (flow_map_normalized)
        {
          float flowLenSq = flow.lengthSq();
          flow *= flowLenSq < 1e-10f ? 0.f : fastinvsqrt(flowLenSq);
        }
        if (!flow_map_signed)
        {
          flow = flow * 0.5f + Point2(0.5f, 0.5f);
        }
        (*flowMapX)[i] = flow.x, (*flowMapY)[i] = flow.y;
      }
    }
    // need temp texture because can't write in render target textures. All editors testures are RT
    Texture *temp_tex = d3d::create_tex(NULL, w, h, TEXFMT_R32F, 1, "temp", RESTAG_TEXGEN);
    if (!temp_tex)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "%s can not create temp texture %dx%d", getName(), w, h));
      return false;
    }

    auto ptr = [](const eastl::optional<eastl::vector<float>> &map) { return map ? &*map : nullptr; };
    bool written = writeTexture(outAt(0), &heightMap, temp_tex);
    written = writeTexture(outAt(1), ptr(sedimentMap), temp_tex) && written;
    written = writeTexture(outAt(2), ptr(depositMap), temp_tex) && written;
    written = writeTexture(outAt(3), ptr(flowMapX), temp_tex) && written;
    written = writeTexture(outAt(4), ptr(flowMapY), temp_tex) && written;
    written = writeTexture(outAt(5), ptr(waterFlowMap), temp_tex) && written;

    del_d3dres(temp_tex);
    return written;
  }

  bool gpu_algorithm(const TextureGenNodeData &data, TextureGenerator &gen, TextureRegManager &regs,
    dag::ConstSpan<TextureInput> &inputs, dag::ConstSpan<D3dResource *> &outputs)
  {
    if (!inputs[0].tex)
      return false;
#define REQUIRE(a)                                                                                            \
  TextureGenShader *a = texgen_get_shader(&gen, #a);                                                          \
  if (!a)                                                                                                     \
  {                                                                                                           \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for %s", #a, getName())); \
    return false;                                                                                             \
  }
    REQUIRE(erosion_shader);
    // The GPU path has no CPU upper limits.
    const int dropLifetime = data.params.getInt("drop_lifetime", 1), erosionRadius = data.params.getInt("erosion_radius", 1);
    int cellRadius = max(1, dropLifetime + erosionRadius);
    int cellSize = 2 * cellRadius;
    int cellCount = (w + cellSize - 1) / cellSize;
    DataBlock params = data.params;
    params.setInt("drop_lifetime", dropLifetime);
    params.setInt("erosion_radius", erosionRadius);
    params.setInt("mapSize", w);
    params.setInt("cellCount", cellCount);
    params.setInt("cellSize", cellSize);

    Texture *tempOutputTex = d3d::create_tex(NULL, w, h, TEXFMT_R32F | TEXCF_UNORDERED | TEXCF_RTARGET, 1, "temp_out0", RESTAG_TEXGEN);
    if (!tempOutputTex)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "%s can not create temp texture %dx%d", getName(), w, h));
      return false;
    }
    d3d::stretch_rect(inputs[0].tex, tempOutputTex);

    int iterations = (int)(cellSize * cellSize * data.params.getReal("drops_multiplier", 1.f));
    set_rnd_seed(630498480);
    eastl::vector<Point2> offsets(cellSize * cellSize);
    for (int i = 0; i < cellSize; i++)
      for (int j = 0; j < cellSize; j++)
        offsets[i * cellSize + j] = Point2(i, j);
    for (int i = offsets.size() - 1; i > 0; i--)
      eastl::swap(offsets[i], offsets[rnd_int(0, i - 1)]);

    eastl::fixed_vector<TextureInput, 4> shaderInputs(inputs.begin(), inputs.begin() + min<int>(inputs.size(), 4));
    shaderInputs[1] = hardnessInput(inputs);
    Texture *maps[3] = {};
    const auto freeTextures = [&] {
      for (Texture *&map : maps)
        del_d3dres(map);
      del_d3dres(tempOutputTex);
    };
    // Shader loads any format by node pixel.
    const uint32_t mapFormats[3] = {TEXFMT_R32F, TEXFMT_R32F, TEXFMT_G32R32F};
    for (int i = 1; i < shaderInputs.size(); i++)
      if (shaderInputs[i].tex && !toNodeMap(shaderInputs[i].tex, maps[i - 1], true, mapFormats[i - 1], i > 1))
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR,
          String(128, "%s node <%s> can not create input %d map %dx%d", getName(), data.nodeName, i == 1 ? HARDNESS_INPUT : i, w, h));
        freeTextures();
        return false;
      }
    // The drops add into these outputs, and a new texture may hold old data.
    const float zero[4] = {0, 0, 0, 0};
    eastl::vector<D3dResource *> writableOutputs = {tempOutputTex};
    for (int i = 1; i < outputs.size(); i++)
    {
      if (outputs[i])
        d3d::clear_rwtexf((Texture *)outputs[i], zero, 0, 0);
      writableOutputs.push_back(outputs[i]);
    }

    TextureGenNodeData nodeData(cellCount, cellCount, NO_BLENDING, false, params, shaderInputs, writableOutputs, NULL);
    for (int i = 0; i < iterations; i++)
    {
      Point2 offset = offsets[i % offsets.size()] + Point2(rnd_float(0, 1), rnd_float(0, 1));
      nodeData.params.setReal("cellOffsetX", offset.x);
      nodeData.params.setReal("cellOffsetY", offset.y);

      if (!erosion_shader->processAll(gen, regs, nodeData))
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "%s: erosion_shader failed", getName()));
        freeTextures();
        return false;
      }
    }

    if (outputs[0])
      d3d::stretch_rect(tempOutputTex, (Texture *)outputs[0]);

    freeTextures();
    return true;
  }

public:
  virtual ~TextureGenErosion() {}
  virtual void destroy() {}

  virtual const char *getName() const { return "erosion"; }
  virtual bool mapsResourcesOnCpu(int) const { return true; }
  virtual int getInputParametersCount() const { return 6; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? HARDNESS_INPUT + 1 : 6; }
  virtual bool isRegOptional(TShaderRegType tp, int reg) const { return tp == TSHADER_REG_TYPE_INPUT ? reg != 0 : false; }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int)
  {
    w = data.nodeW, h = data.nodeH;
    dag::ConstSpan<TextureInput> inputs = data.inputs;
    dag::ConstSpan<D3dResource *> outputs = data.outputs;
    if (inputs.size() < 2)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "not enough inputs for %s", getName()));
      return false;
    }
    if (outputs.size() < 1)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for %s", getName()));
      return true;
    }
    for (int i = 0; i < outputs.size(); i++)
    {
      if (!outputs[i])
        continue;
      if (outputs[i]->getType() != D3DResourceType::TEX)
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "%s output #%d must be a texture", getName(), i));
        return false;
      }
      TextureInfo info;
      ((Texture *)outputs[i])->getinfo(info);
      if ((info.cflg & TEXFMT_MASK) != TEXFMT_R32F)
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "%s output #%d must be R32F", getName(), i));
        return false;
      }
    }
    if (BaseTexture *oldHardness = inputs[OLD_HARDNESS_INPUT].tex; oldHardness && mayBeAboveZero(oldHardness))
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR,
        String(0, "%s node <%s>: its 'hardness map' input may be above 0 and is not read; connect it to the 'hardness' input",
          getName(), data.nodeName));
    if (!data.params.getBool("use_gpu", false))
    {
      read_parameters(data.params, gen);
      return cpu_algorithm(gen, inputs, outputs);
    }
    return gpu_algorithm(data, gen, regs, inputs, outputs);
  }
} texgen_erosion;

void init_texgen_erosion_shader(TextureGenerator *tex_gen) { texgen_add_shader(tex_gen, texgen_erosion.getName(), &texgen_erosion); }
