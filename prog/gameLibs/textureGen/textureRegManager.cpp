#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_buffers.h>
#include <3d/dag_texPackMgr2.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <textureGen/textureRegManager.h>
#include <shaders/dag_shaders.h>
#include <textureGen/textureGenLogger.h>
#include <webui/editorCurves.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <ctype.h>
#include <math/dag_mathUtils.h>


#define GRADIENT_TEXTURE_WIDTH 512


static TextureGenLogger defaultLogger;
void TextureRegManager::setLogger(TextureGenLogger *logger_) { logger = logger_ ? logger_ : &defaultLogger; }
// void TextureRegManager::setLogger(TextureGenLogger *) {logger = &defaultLogger;defaultLogger.setStartLevel(LOGLEVEL_REMARK);}

TextureRegManager::TextureRegManager() : autoShrink(false), currentMemSize(0), maxMemSize(0), needReloadTextures(false)
{
  setLogger(nullptr);
  setEntitiesSaver(nullptr);
}

void TextureRegManager::clearStat()
{
  maxMemSize = currentMemSize;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].useCount)
      logger->log(LOGLEVEL_REMARK,
        String(128, "register %d <%s> still exist, usage %d", reg, getRegName(reg), textureRegs[reg].useCount));
}

eastl::hash_map<eastl::string, int> TextureRegManager::getRegsAliveMap() const
{
  eastl::hash_map<eastl::string, int> ret;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].useCount)
      ret[textureRegs[reg].name] = reg;
  return ret;
}

int TextureRegManager::getRegsAlive() const
{
  int ret = 0;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].useCount)
      ret++;
  return ret;
}

void TextureRegManager::close()
{
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    textureRegs[reg].close(logger);
  clear_and_shrink(textureRegs);
  regNames.clear();
  clear_and_shrink(freeRegs);
  currentMemSize = 0;
}

void TextureRegManager::TexureRegister::close(TextureGenLogger *logger)
{
  G_UNUSED(logger);

  if (owned)
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texId, tex);
  else
    release_managed_tex(texId);
  tex = 0;
  useCount = 0;
  texId = BAD_TEXTUREID;
}

bool TextureRegManager::shrinkTex(int reg)
{
  logger->log(LOGLEVEL_REMARK, String(128, "shrink #%d, usage = %d", reg, textureRegs[reg].useCount));
  if (textureRegs[reg].useCount)
    return false;
  logger->log(LOGLEVEL_REMARK, String(128, "push #%d", reg));
  freeRegs.push_back(reg);
  if (textureRegs[reg].tex)
    currentMemSize -= textureRegs[reg].tex->ressize();
  textureRegs[reg].close(logger);
  return true;
}

bool TextureRegManager::shrinkMem()
{
  bool ret = false;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    ret |= shrinkTex(reg);
  return ret;
}

int TextureRegManager::findRegNo(const char *name) const
{
  auto it = regNames.find_as(name);
  if (it == regNames.end())
    return -1;
  G_ASSERTF(strcmp(textureRegs[it->second].name.c_str(), name) == 0, "name = %s cached_name = %s", name,
    textureRegs[it->second].name.c_str());
  return it->second;
}

int TextureRegManager::getRegNo(const char *name) const
{
  int regNo = findRegNo(name);
  if (regNo < 0)
    return -1;
  if (textureRegs[regNo].useCount == 0)
    return -1;
  return regNo;
}

void TextureRegManager::fillCurveTexture(Texture *tex, const char *curve_str)
{
  EditorCurve curve;
  if (!curve.parse(curve_str))
  {
    logger->log(LOGLEVEL_ERR, String(128, "cannot parse curve : %s", curve_str));
    return;
  }

  TextureInfo info;
  tex->getinfo(info);
  int width = info.w;
  G_ASSERT(width > 1);

  unsigned short *data = NULL;
  int stride = 0;
  if (tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    for (int x = 0; x < width; x++)
    {
      float t = float(x) / (width - 1);
      float r, g, b, a;

      r = g = b = a = curve.getValue(t);

      data[x * 4 + 0] = clamp(int(r * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 1] = clamp(int(g * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 2] = clamp(int(b * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 3] = clamp(int(a * 65535 + 0.5f), 0, 65535);
    }
    tex->unlockimg();
  }
  else
  {
    logger->log(LOGLEVEL_ERR, String(128, "cannot lock curve texture"));
  }
}

void TextureRegManager::fillGradientTexture(Texture *tex, const char *gradient_str)
{
  bool nearest = gradient_str[0] == 'N';
  if (nearest)
    gradient_str++;

  Tab<float> arr;
  const char *end = gradient_str + strlen(gradient_str);
  char *ptr = (char *)gradient_str;
  double val = 0.0f;

  while (isspace(*ptr))
    ptr++;

  while (ptr < end)
  {
    val = strtod(ptr, &ptr);
    arr.push_back(float(val));
    while (isspace(*ptr) || *ptr == ',')
      ptr++;

    if (arr.size() > 200)
    {
      logger->log(LOGLEVEL_ERR, String(128, "too many items in gradient"));
      return;
    }
  }


  if (arr.size() % 5 != 0)
    logger->log(LOGLEVEL_ERR, String(128, "invalid number of items in gradient (%d): %s", arr.size(), gradient_str));

  TextureInfo info;
  tex->getinfo(info);
  int width = info.w;
  G_ASSERT(width > 1);

  unsigned short *data = NULL;
  int stride = 0;
  if (tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    for (int x = 0; x < width; x++)
    {
      float r = 0.5;
      float g = 0.5;
      float b = 0.5;
      float a = 1.0;

      float t = float(x) / (width - 1);

      int len = arr.size() / 5;

      if (len == 0)
      {
        r = g = b = t;
        a = 1.0;
      }
      else if (len == 1 || t <= arr[0])
      {
        r = arr[1];
        g = arr[2];
        b = arr[3];
        a = arr[4];
      }
      else if (t >= arr[(len - 1) * 5])
      {
        r = arr[(len - 1) * 5 + 1];
        g = arr[(len - 1) * 5 + 2];
        b = arr[(len - 1) * 5 + 3];
        a = arr[(len - 1) * 5 + 4];
      }

      for (int i = 1; i < len; i++)
        if (t < arr[i * 5])
        {
          float k = nearest ? 0.f : (t - arr[(i - 1) * 5]) / (arr[i * 5] - arr[(i - 1) * 5] + 1e-6f);
          float invK = 1.0 - k;

          float res[4] = {0};
          for (int n = 0; n < 4; n++)
            res[n] = clamp(arr[(i - 1) * 5 + n + 1] * invK + arr[i * 5 + n + 1] * k, 0.0f, 1.0f);

          r = res[0];
          g = res[1];
          b = res[2];
          a = res[3];
          break;
        }

      data[x * 4 + 0] = clamp(int(r * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 1] = clamp(int(g * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 2] = clamp(int(b * 65535 + 0.5f), 0, 65535);
      data[x * 4 + 3] = clamp(int(a * 65535 + 0.5f), 0, 65535);
    }
    tex->unlockimg();
  }
  else
  {
    logger->log(LOGLEVEL_ERR, String(128, "cannot lock gradient texture"));
  }
}

void TextureRegManager::fillConstTexture(Texture *tex, const char *const_str)
{
  float val = atof(const_str);
  float *data = NULL;
  int stride = 0;
  if (tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    data[0] = data[1] = data[2] = data[3] = val;
    tex->unlockimg();
  }
  else
  {
    logger->log(LOGLEVEL_ERR, String(128, "cannot lock const color texture"));
  }
}

void TextureRegManager::fillSolidColorTexture(Texture *tex, const char *color_str, int width, int height)
{
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 1.f;
  sscanf(color_str, "%f,%f,%f,%f", &r, &g, &b, &a);
  float *data = NULL;
  int stride = 0;
  if (tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    G_ASSERT(stride % sizeof(float) == 0);
    stride /= sizeof(float);
    for (int y = 0; y < height; y++)
      for (int x = 0; x < width; x++)
      {
        int idx = stride * y + x * 4;
        data[idx + 0] = r;
        data[idx + 1] = g;
        data[idx + 2] = b;
        data[idx + 3] = a;
      }

    tex->unlockimg();
  }
  else
  {
    logger->log(LOGLEVEL_ERR, String(128, "cannot lock const color texture (2)"));
  }
}

bool TextureRegManager::validateFile(const char *name)
{
  const auto fi = changedFileInfo.find_as(name);
  if (fi != changedFileInfo.end()) // already scheduled
    return true;
  DagorStat stat;
  if (df_stat(name, &stat) == -1)
    return false;
  changedFileInfo[name] = FileInfo(stat.mtime, stat.size);
  return true;
}

void TextureRegManager::validateFilesData()
{
  for (const auto &fi : lastFileInfo)
    validateFile(fi.first.c_str());
}

#define TEX_STR      "tex:"
#define CONST_STR    "const:"
#define COLOR_STR    "color:"
#define GRADIENT_STR "gradient:"
#define CURVE_STR    "curve:"

void TextureRegManager::getRelativeTexName(const char *name, String &texname, String &texWithParams)
{
  if (strstr(name, TEX_STR) != name)
  {
    texname.setStr(name);
    texWithParams = texname;
    return;
  }

  name += strlen(TEX_STR);
  if (strstr(name, CONST_STR) || strstr(name, GRADIENT_STR) || strstr(name, COLOR_STR))
  {
    texname.setStr(name);
    texWithParams = texname;
    return;
  }

  int len = strlen(name);
  if (
    len > 4 && (stricmp(name + len - 4, ".raw") == 0 || stricmp(name + len - 4, ".r16") == 0 || stricmp(name + len - 4, ".r32") == 0))
  {
    texWithParams = name;
    if (!textureRootDir.empty())
      texWithParams.replaceAll("name:t=", String(128, "name:t=%s/", textureRootDir.str()));
    const char *namePos = strstr(texWithParams, "name:t=");
    if (namePos)
    {
      texname = namePos + strlen("name:t=");
    }
    else
    {
      logger->log(LOGLEVEL_ERR,
        String(128, "RAW texture '%s' has no params. You should use node 'raw texture'.", texWithParams.str()));
      return;
    }
  }
  else
  {
    if (textureRootDir.empty())
    {
      texname = name;
      texWithParams = texname;
    }
    else
      texname.printf(128, "%s/%s", textureRootDir.str(), name);

    texWithParams = texname;
  }


  return;
}

bool TextureRegManager::texHasChanged(const char *name)
{
  String texname;
  String texWithParams;
  getRelativeTexName(name, texname, texWithParams);
  return changedFileInfo.find_as(texname.str()) != changedFileInfo.end();
}


int TextureRegManager::createTexture(const char *name, uint32_t fmt, int w, int h, int use_count)
{
  int regNo = findRegNo(name);
  logger->log(LOGLEVEL_REMARK, String(128, "look for <%s>, found = %d", name, regNo));
  if (regNo >= 0)
  {
    for (int i = 0; i < freeRegs.size(); ++i)
    {
      if (freeRegs[i] == regNo)
      {
        logger->log(LOGLEVEL_REMARK, String(128, "remove unused reg#%d from freeList", regNo));
        erase_items(freeRegs, i, 1);
        break;
      }
    }

    if (textureRegs[regNo].useCount != 0)
    {
      logger->log(LOGLEVEL_ERR, String(128, "tex reg name <%s> has already been used, %d to use", name, textureRegs[regNo].useCount));
      return -1;
    }
    logger->log(LOGLEVEL_REMARK, String(128, "reg#%d: tex=%p owned=%d", regNo, textureRegs[regNo].tex, textureRegs[regNo].owned));
    if (textureRegs[regNo].tex && textureRegs[regNo].owned)
    {
      if (textureRegs[regNo].tex->restype() == RES3D_TEX)
      {
        TextureInfo tinfo;
        ((BaseTexture *)textureRegs[regNo].tex)->getinfo(tinfo, 0);
        if (tinfo.w != w || tinfo.h != h || (tinfo.cflg & TEXFMT_MASK) != fmt)
        {
          logger->log(LOGLEVEL_ERR, String(128, "tex reg  <%s> differs from previous use %dx%d(%d) != %dx%d(%d)", name, w, h, fmt,
                                      tinfo.w, tinfo.h, (tinfo.cflg & TEXFMT_MASK)));
          return -1;
        }
      }
      textureRegs[regNo].useCount = use_count;
      return regNo;
    }
  }
  if (regNo < 0 && freeRegs.size())
  {
    regNo = freeRegs.back();
    freeRegs.pop_back();
    logger->log(LOGLEVEL_REMARK, String(128, "pop free reg#%d", regNo));
    G_ASSERT(textureRegs[regNo].tex == 0);
  }
  if (regNo >= 0 && textureRegs[regNo].tex)
  {
    logger->log(LOGLEVEL_REMARK, String(128, "close reg %d", regNo));
    textureRegs[regNo].close(logger);
  }
  BaseTexture *tex = 0;
  TEXTUREID texId = BAD_TEXTUREID;
  bool owned = false;

  if (strstr(name, TEX_STR) == name)
  {
    if (const char *gradientStr = strstr(name, GRADIENT_STR))
    {
      gradientStr += strlen(GRADIENT_STR);
      fmt = TEXFMT_A16B16G16R16;
      tex = d3d::create_tex(NULL, GRADIENT_TEXTURE_WIDTH, 1, fmt, 1, name); //|TEXCF_UNORDERED
      owned = true;
      if (tex)
      {
        fillGradientTexture((Texture *)tex, gradientStr);
        texId = register_managed_tex(name, tex);
      }
    }
    else if (const char *curveStr = strstr(name, CURVE_STR))
    {
      curveStr += strlen(CURVE_STR);
      fmt = TEXFMT_A16B16G16R16;
      tex = d3d::create_tex(NULL, GRADIENT_TEXTURE_WIDTH, 1, fmt, 1, name); //|TEXCF_UNORDERED
      owned = true;
      if (tex)
      {
        fillCurveTexture((Texture *)tex, curveStr);
        texId = register_managed_tex(name, tex);
      }
    }
    else if (const char *constStr = strstr(name, CONST_STR))
    {
      constStr += strlen(CONST_STR);
      fmt = TEXFMT_A32B32G32R32F;
      tex = d3d::create_tex(NULL, 1, 1, fmt, 1, name); //|TEXCF_UNORDERED
      owned = true;
      if (tex)
      {
        fillConstTexture((Texture *)tex, constStr);
        texId = register_managed_tex(name, tex);
      }
    }
    else if (const char *colorStr = strstr(name, COLOR_STR))
    {
      colorStr += strlen(COLOR_STR);
      fmt = TEXFMT_A32B32G32R32F;
      tex = d3d::create_tex(NULL, 2, 2, fmt, 1, name); //|TEXCF_UNORDERED
      owned = true;
      if (tex)
      {
        fillSolidColorTexture((Texture *)tex, colorStr, 2, 2);
        texId = register_managed_tex(name, tex);
      }
    }
    else
    {
      String texname;
      String texWithParams;
      getRelativeTexName(name, texname, texWithParams);
      texId = get_managed_texture_id(texWithParams);

      if (texname.length() > 1)
        inputFiles.insert(texname.str());

      if (validateFile(texname))
      {
        auto it = lastFileInfo.find_as(texname.str());
        logger->log(LOGLEVEL_REMARK, String(128, "texture <%s> %s", texname.str(), it == lastFileInfo.end() ? "used" : "changed"));
        auto changedIt = changedFileInfo.find_as(texname.str());
        if (changedIt != changedFileInfo.end())
        {
          lastFileInfo[texname.str()] = changedIt->second;
          changedFileInfo.erase(changedIt);
        }

        if (texId != BAD_TEXTUREID)
        {
          int refCnt = get_managed_texture_refcount(texId);
          while (get_managed_texture_refcount(texId))
            release_managed_tex(texId);
          discard_unused_managed_texture(texId);
          if (refCnt)
          {
            texId = add_managed_texture(texWithParams);
            if (texId != BAD_TEXTUREID)
              tex = acquire_managed_tex(texId);
            for (int i = 0; i < refCnt; ++i)
              acquire_managed_tex(texId);
          }
          else
            texId = BAD_TEXTUREID;
        }
      }

      if (texId == BAD_TEXTUREID)
      {
        if (dd_file_exist(texname))
        {
          texId = add_managed_texture(texWithParams);
          if (texId != BAD_TEXTUREID)
          {
            tex = acquire_managed_tex(texId);
            if (!tex)
            {
              logger->log(LOGLEVEL_ERR, String(128, "can not create texture <%s>, although file exist", texWithParams));
              return -1;
            }
          }
          else
          {
            logger->log(LOGLEVEL_ERR, String(128, "can not load texture <%s>, although file exist", texWithParams));
            return -1;
          }
        }
        else
        {
          logger->log(LOGLEVEL_ERR, String(128, "file doesn't exist <%s>", texname));
          return -1;
        }
      }
      else
      {
        tex = acquire_managed_tex(texId);
        if (!tex)
        {
          logger->log(LOGLEVEL_ERR, String(128, "can not create texture <%s>, although file exist", texname));
          return -1;
        }
      }

      if (texId != BAD_TEXTUREID)
        ddsx::tex_pack2_perform_delayed_data_loading();
    }
  }
  else
  {
    String texName(128, "texgen_%s", name);
    tex = d3d::create_tex(NULL, w, h, TEXCF_RTARGET | fmt, 1, texName); //|TEXCF_UNORDERED
    owned = true;
    if (tex)
      texId = register_managed_tex(texName, tex);
  }
  if (!tex)
  {
    logger->log(LOGLEVEL_ERR, String(128, "can not create texture reg <%s> with %dx%d resolution and 0x%X format", name, w, h, fmt));
    return -1;
  }

  if (tex)
  {
    currentMemSize += tex->ressize();
    maxMemSize = max(maxMemSize, currentMemSize);
  }

  if (regNo < 0)
    regNo = append_items(textureRegs, 1);
  logger->log(LOGLEVEL_REMARK,
    String(128, "create texture reg#%d <%s> %dx%d fmt = 0x%X texId = %d, to be used %d", regNo, name, w, h, fmt, texId, use_count));

  auto it = regNames.find_as(textureRegs[regNo].name.c_str());
  if (it != regNames.end())
    regNames.erase(it);

  regNames[name] = regNo;
  textureRegs[regNo].name = name;
  textureRegs[regNo].tex = tex;
  textureRegs[regNo].useCount = use_count;
  textureRegs[regNo].texId = texId;
  textureRegs[regNo].owned = owned;
  return regNo;
}

int TextureRegManager::createParticlesBuffer(const char *name, int max_count, int use_count)
{
  int regNo = findRegNo(name);
  logger->log(LOGLEVEL_REMARK, String(128, "look for <%s>, found = %d", name, regNo));
  if (regNo >= 0)
  {
    for (int i = 0; i < freeRegs.size(); ++i)
    {
      if (freeRegs[i] == regNo)
      {
        logger->log(LOGLEVEL_REMARK, String(128, "remove unused reg#%d from freeList", regNo));
        erase_items(freeRegs, i, 1);
        break;
      }
    }

    if (textureRegs[regNo].useCount != 0)
    {
      logger->log(LOGLEVEL_ERR, String(128, "tex reg name <%s> has already been used, %d to use", name, textureRegs[regNo].useCount));
      return -1;
    }
    logger->log(LOGLEVEL_REMARK, String(128, "%d: %p %d", regNo, textureRegs[regNo].tex, textureRegs[regNo].owned));
    if (textureRegs[regNo].tex && textureRegs[regNo].owned)
    {
      if (textureRegs[regNo].tex->restype() == RES3D_SBUF)
      {
        int maxParticlesCnt = textureRegs[regNo].tex->ressize() / PARTICLE_SIZE;
        if (maxParticlesCnt < max_count)
        {
          logger->log(LOGLEVEL_ERR,
            String(128, "sbuffer reg  <%s> differs from previous use, count = %d, needed = %d", name, maxParticlesCnt, max_count));
          return -1;
        }
      }
      else
        logger->log(LOGLEVEL_ERR, String(128, "sbuffer reg  <%s> differs from previous definition (not sbuffer)", name));
      textureRegs[regNo].useCount = use_count;
      return regNo;
    }
  }
  if (regNo < 0 && freeRegs.size())
  {
    regNo = freeRegs.back();
    freeRegs.pop_back();
    logger->log(LOGLEVEL_REMARK, String(128, "pop free reg#%d", regNo));
    G_ASSERT(textureRegs[regNo].tex == 0);
  }
  if (regNo >= 0 && textureRegs[regNo].tex)
  {
    logger->log(LOGLEVEL_REMARK, String(128, "close reg %d", regNo));
    textureRegs[regNo].close(logger);
  }
  Sbuffer *particles = 0;
  bool owned = false;
  String texName(128, "texgen_%s", name);
  particles = d3d_buffers::create_ua_sr_structured(PARTICLE_SIZE, max_count, texName.str()); //
  owned = true;
  if (particles)
  {
    currentMemSize += particles->ressize();
    maxMemSize = max(maxMemSize, currentMemSize);
  }
  if (!particles)
  {
    logger->log(LOGLEVEL_ERR,
      String(128, "can not create sbuffer reg <%s> with %d count resolution and 0x%X format", name, max_count));
    return -1;
  }
  if (regNo < 0)
    regNo = append_items(textureRegs, 1);
  logger->log(LOGLEVEL_REMARK,
    String(128, "create sbuffer reg#%d <%s> max_count = %d, to be used %d", regNo, name, max_count, use_count));
  auto it = regNames.find_as(textureRegs[regNo].name.c_str());
  if (it != regNames.end())
    regNames.erase(it);
  regNames[name] = regNo;
  textureRegs[regNo].name = name;
  textureRegs[regNo].tex = particles;
  textureRegs[regNo].useCount = use_count;
  textureRegs[regNo].texId = BAD_TEXTUREID;
  textureRegs[regNo].owned = owned;
  return regNo;
}


void TextureRegManager::setUseCount(int reg, unsigned cnt)
{
  if (reg < 0 || reg >= textureRegs.size())
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg#%d is invalid", reg));
    return;
  }
  if (textureRegs[reg].useCount == 0)
    logger->log(LOGLEVEL_ERR, String(128, "we can't safely add usage to unused reg! reg#%d %s", reg, getRegName(reg)));
  textureRegs[reg].useCount = cnt;
  if (autoShrink && !textureRegs[reg].useCount)
    shrinkTex(reg);
}

int TextureRegManager::getRegUsageLeft(int reg) const
{
  if (reg < 0 || reg >= textureRegs.size())
    return -1;
  return textureRegs[reg].useCount;
}

void TextureRegManager::textureUsed(int reg)
{
  logger->log(LOGLEVEL_REMARK, String(128, "use reg#%d", reg));
  if (!validateReg(reg, true))
    return;
  textureRegs[reg].useCount--;
  if (autoShrink)
    shrinkTex(reg);
}

bool TextureRegManager::validateReg(int reg, bool log) const
{
  if (reg < 0 || reg >= textureRegs.size())
  {
    if (log)
      logger->log(LOGLEVEL_ERR, String(128, "invalid reg#%d", reg));
    return false;
  }
  if (textureRegs[reg].useCount == 0)
  {
    if (log)
      logger->log(LOGLEVEL_ERR, String(128, "texture with name <%s> has already fully used", getRegName(reg)));
    return false;
  }
  if (textureRegs[reg].tex == 0)
  {
    if (log)
      logger->log(LOGLEVEL_ERR, String(128, "texture with name <%s> is not initialized", getRegName(reg)));
    return false;
  }
  return true;
}

D3dResource *TextureRegManager::getResource(int reg) const
{
  logger->log(LOGLEVEL_REMARK, String(128, "get reg#%d", reg));
  if (!validateReg(reg, true))
    return 0;
  return textureRegs[reg].tex;
}


BaseTexture *TextureRegManager::getTexture(int reg) const
{
  D3dResource *res = getResource(reg);
  if (!res)
    return 0;
  if (res->restype() > RES3D_ARRTEX)
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg #d <%s> is not a texture", reg, getRegName(reg)));
    return 0;
  }
  return (BaseTexture *)res;
}

TEXTUREID TextureRegManager::getTextureId(int reg) const
{
  logger->log(LOGLEVEL_REMARK, String(128, "get reg#%d textureId", reg));
  if (!validateReg(reg, true))
    return BAD_TEXTUREID;
  return textureRegs[reg].texId;
}

Sbuffer *TextureRegManager::getParticlesBuffer(int reg) const
{
  D3dResource *res = getResource(reg);
  if (!res)
    return 0;
  if (res->restype() != RES3D_SBUF)
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg %d <%s> is not a buffer, it is #%d", reg, getRegName(reg), res->restype()));
    return 0;
  }
  return (Sbuffer *)res;
}

const char *TextureRegManager::getRegName(int reg) const
{
  if (reg < 0 || reg >= textureRegs.size())
    return "___UNDEFINED___ and INVALID";
  return textureRegs[reg].name.c_str();
}
