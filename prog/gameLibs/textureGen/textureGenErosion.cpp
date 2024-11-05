// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <EASTL/vector.h>
#include <EASTL/optional.h>
#include <perfMon/dag_pix.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

static class TextureGenErosion : public TextureGenShader
{
private:
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
          float weight = 1 - len / radius;
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
    var_name = min_val;                                                                                                     \
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

    CHECK_INT_PARAMETERS(erosion_radius, 1, 1, 5)
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
            (*flowMapY)[erodeIndex] = lerp((*flowMapY)[erodeIndex], dir.x, flow_map_inertion * weightedErodeAmount);
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

  void readTexture(BaseTexture *src_texture, eastl::vector<float> &dest_array, size_t map_size)
  {
    size_t map_byte_size = map_size * sizeof(float);
    dest_array.resize(map_size);
    TextureInfo texInfo;
    src_texture->getinfo(texInfo);
    if (texInfo.h != h || texInfo.w != w || (texInfo.cflg & TEXFMT_MASK) != TEXFMT_R32F)
    {
      memset(dest_array.data(), 0, map_byte_size);
      return;
    }
    void *mapData = nullptr;
    int stride = 0;
    if (!src_texture->lockimg(&mapData, stride, 0, TEXLOCK_READ) && mapData)
      return;
    memcpy(dest_array.data(), mapData, map_byte_size);
    src_texture->unlockimg();
  }

  void writeTexture(Texture *dest_texture, const eastl::optional<eastl::vector<float>> &src_array, Texture *temp_tex)
  {
    if (!dest_texture || !src_array)
      return;
    TextureInfo info;
    dest_texture->getinfo(info);
    if (!(info.cflg & TEXFMT_R32F))
      return;
    void *mapData = nullptr;
    int stride = 0;
    if (!temp_tex->lockimg(&mapData, stride, 0, TEXLOCK_WRITE) || !mapData)
      return;
    memcpy(mapData, src_array->data(), src_array->size() * sizeof(float));
    temp_tex->unlockimg();
    dest_texture->update(temp_tex);
  }

  eastl::optional<eastl::vector<float>> prepareCleanMap(bool hasMap, size_t mapSize)
  {
    if (hasMap)
      return eastl::vector<float>(mapSize, 0);
    return eastl::nullopt;
  }

  void cpu_algorithm(dag::ConstSpan<TextureInput> &inputs, dag::ConstSpan<D3dResource *> &outputs)
  {
    size_t mapSize = h * w;

    readTexture(inputs[0].tex, heightMap, mapSize);
    readTexture(inputs[1].tex, hardnessMap, mapSize);

    sedimentMap = prepareCleanMap(outputs[1] != nullptr, mapSize);
    depositMap = prepareCleanMap(outputs[2] != nullptr, mapSize);
    flowMapX = prepareCleanMap(outputs[3] != nullptr, mapSize);
    flowMapY = prepareCleanMap(outputs[4] != nullptr, mapSize);
    waterFlowMap = prepareCleanMap(outputs[5] != nullptr, mapSize);

    set_rnd_seed(630498480);
    process_drops((int)(h * w * drops_multiplier));

    if (flowMapX && flowMapY)
    {
      float maxSqLen = 0;
      for (int i = 0; i < mapSize; i++)
        maxSqLen = max(maxSqLen, (*flowMapX)[i] * (*flowMapX)[i] + (*flowMapY)[i] * (*flowMapY)[i]);
      float maxLenInv = 1 / sqrt(maxSqLen);
      for (int i = 0; i < mapSize; i++)
      {
        Point2 flow = Point2((*flowMapX)[i], (*flowMapY)[i]) * maxLenInv;
        if (flow_map_normalized)
        {
          float flowLen = flow.lengthF();
          float flowLenInv = flowLen < 0.00001f ? 1 / flowLen : 0;
          flow *= flowLenInv;
        }
        if (!flow_map_signed)
        {
          flow = flow * 0.5f + Point2(0.5f, 0.5f);
        }
        (*flowMapX)[i] = flow.x, (*flowMapY)[i] = flow.y;
      }
    }
    // need temp texture because can't write in render target textures. All editors testures are RT
    Texture *temp_tex = d3d::create_tex(NULL, w, h, TEXFMT_R32F, 1, "temp");

    writeTexture((Texture *)outputs[0], heightMap, temp_tex);
    writeTexture((Texture *)outputs[1], sedimentMap, temp_tex);
    writeTexture((Texture *)outputs[2], depositMap, temp_tex);
    writeTexture((Texture *)outputs[3], flowMapX, temp_tex);
    writeTexture((Texture *)outputs[4], flowMapY, temp_tex);
    writeTexture((Texture *)outputs[5], waterFlowMap, temp_tex);

    del_d3dres(temp_tex);
  }

  void gpu_algorithm(const TextureGenNodeData &data, TextureGenerator &gen, TextureRegManager &regs,
    dag::ConstSpan<TextureInput> &inputs, dag::ConstSpan<D3dResource *> &outputs)
  {
    if (!inputs[0].tex)
      return;
#define REQUIRE(a)                                                                                            \
  TextureGenShader *a = texgen_get_shader(&gen, #a);                                                          \
  if (!a)                                                                                                     \
  {                                                                                                           \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for %s", #a, getName())); \
    return;                                                                                                   \
  }
    REQUIRE(erosion_shader);
    int cellRadius = data.params.getInt("drop_lifetime") + data.params.getInt("erosion_radius");
    int cellSize = 2 * cellRadius;
    int cellCount = (w + cellSize - 1) / cellSize;
    DataBlock params = data.params;
    params.setInt("mapSize", w);
    params.setInt("cellCount", cellCount);
    params.setInt("cellSize", cellSize);
    params.setInt("weightsCount", erosion_weights.size());

    Texture *tempOutputTex = d3d::create_tex(NULL, w, h, TEXFMT_R32F | TEXCF_UNORDERED | TEXCF_RTARGET, 1, "temp_out0");
    d3d::stretch_rect(inputs[0].tex, tempOutputTex);

    int iterations = (int)(cellSize * cellSize * data.params.getReal("drops_multiplier"));
    set_rnd_seed(630498480);
    eastl::vector<Point2> offsets(cellSize * cellSize);
    for (int i = 0; i < cellSize; i++)
      for (int j = 0; j < cellSize; j++)
        offsets[i * cellSize + j] = Point2(i, j);
    for (int i = offsets.size() - 1; i > 0; i--)
      eastl::swap(offsets[i], offsets[rnd_int(0, i - 1)]);
#if DAGOR_DBGLEVEL > 1
    PIX_GPU_BEGIN_CAPTURE(D3D11X_PIX_CAPTURE_API, L"D:\\YourName.xpix");
#endif

    eastl::vector<D3dResource *> writableOutputs = {tempOutputTex};
    for (int i = 1; i < outputs.size(); i++)
      writableOutputs.push_back(outputs[i]);

    for (int i = 0; i < iterations; i++)
    {
      Point2 offset = offsets[i % offsets.size()] + Point2(rnd_float(0, 1), rnd_float(0, 1));
      params.setReal("cellOffsetX", offset.x);
      params.setReal("cellOffsetY", offset.y);

      erosion_shader->processAll(gen, regs,
        TextureGenNodeData(cellCount, cellCount, NO_BLENDING, false, params, inputs, writableOutputs, NULL));
    }

    if (outputs[0])
      d3d::stretch_rect(tempOutputTex, (Texture *)outputs[0]);

    del_d3dres(tempOutputTex);

#if DAGOR_DBGLEVEL > 1
    PIX_GPU_END_CAPTURE();
#endif
  }

public:
  virtual ~TextureGenErosion() {}
  virtual void destroy() {}

  virtual const char *getName() const { return "erosion"; }
  virtual int getInputParametersCount() const { return 6; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 4 : 6; }
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
    erosion_radius = data.params.getInt("erosion_radius");
    drop_lifetime = data.params.getInt("drop_lifetime");

    if (!data.params.getBool("use_gpu", false))
    {
      read_parameters(data.params, gen);
      cpu_algorithm(inputs, outputs);
    }
    else
    {
      gpu_algorithm(data, gen, regs, inputs, outputs);
    }
    return true;
  }
} texgen_erosion;

void init_texgen_erosion_shader(TextureGenerator *tex_gen) { texgen_add_shader(tex_gen, texgen_erosion.getName(), &texgen_erosion); }
