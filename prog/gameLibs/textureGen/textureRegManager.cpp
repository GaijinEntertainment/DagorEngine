// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>
#include <3d/dag_texMgr.h>
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

TextureRegManager::TextureRegManager() : autoShrink(false), currentMemSize(0), maxMemSize(0), needReloadTextures(false)
{
  setLogger(nullptr);
  setEntitiesSaver(nullptr);
}

void TextureRegManager::clearStat()
{
  maxMemSize = currentMemSize;
  stats = Stats();
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].useCount)
      logger->log(LOGLEVEL_REMARK,
        String(128, "register %d <%s> still exist, usage %d", reg, getRegName(reg), textureRegs[reg].useCount));
}

eastl::hash_map<eastl::string, int> TextureRegManager::getRegsAliveMap() const
{
  eastl::hash_map<eastl::string, int> ret;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].isAlive())
      ret[textureRegs[reg].name] = reg;
  return ret;
}

int TextureRegManager::getRegsAlive() const
{
  int ret = 0;
  for (int reg = 0; reg < textureRegs.size(); ++reg)
    if (textureRegs[reg].isAlive())
      ret++;
  return ret;
}

void TextureRegManager::close()
{
  for (int reg = 0; reg < textureRegs.size(); ++reg)
  {
    if (textureRegs[reg].tex)
      stats.resourcesReleased++;
    textureRegs[reg].close(logger);
  }
  clear_and_shrink(textureRegs);
  regNames.clear();
  releaseLoadedFiles();
  releaseTexturePool();
  clear_and_shrink(freeRegs);
  currentMemSize = 0;
  retainedMemSize = 0;
  retainedCount = 0;
  retainHead = retainTail = -1;
}

void TextureRegManager::TexureRegister::close(TextureGenLogger *logger)
{
  G_UNUSED(logger);

  if (owned && texId == BAD_TEXTUREID)
    del_d3dres(tex);
  else if (owned)
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texId, tex);
  else
    release_managed_tex(texId);
  del_d3dres(particlesArgs);
  tex = 0;
  useCount = 0;
  texId = BAD_TEXTUREID;
}

bool TextureRegManager::shrinkTex(int reg)
{
  logger->log(LOGLEVEL_REMARK, String(128, "shrink #%d, usage = %d", reg, textureRegs[reg].useCount));
  if (textureRegs[reg].useCount || !textureRegs[reg].tex)
    return false;
  logger->log(LOGLEVEL_REMARK, String(128, "push #%d", reg));
  freeRegs.push_back(reg);
  unretain(reg);
  stats.resourcesReleased++;
  // A pooled texture stays in currentMemSize until the pool hands it out or frees it.
  const uint64_t size = textureRegs[reg].tex->getSize();
  if (!poolTexture(reg))
  {
    currentMemSize -= size;
    textureRegs[reg].close(logger);
  }
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
  if (regNo < 0 || !textureRegs[regNo].isAlive())
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
  const bool nearest = gradient_str[0] == 'N';
  Tab<float> arr;
  const char *ptr = gradient_str + (nearest ? 1 : 0);
  double val = 0.0;

  while (*ptr)
  {
    while (isspace(*ptr) || *ptr == ',')
      ptr++;
    if (!*ptr || arr.size() >= 200)
      break;
    char *next = nullptr;
    val = strtod(ptr, &next);
    if (next == ptr)
      break;
    ptr = next;
    arr.push_back(float(val));
  }
  if (*ptr)
    logger->log(LOGLEVEL_ERR, String(128, "gradient: text after item %d is not read: %s", arr.size(), gradient_str));
  else if (arr.size() % 5 != 0)
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
      else
      {
        for (int i = 1; i < len; i++)
          if (t < arr[i * 5])
          {
            float k = nearest ? 0.f : (t - arr[(i - 1) * 5]) / (arr[i * 5] - arr[(i - 1) * 5] + 1e-6f);
            float invK = 1.0f - k;

            float res[4] = {0};
            for (int n = 0; n < 4; n++)
              res[n] = clamp(arr[(i - 1) * 5 + n + 1] * invK + arr[i * 5 + n + 1] * k, 0.0f, 1.0f);

            r = res[0];
            g = res[1];
            b = res[2];
            a = res[3];
            break;
          }
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
  const auto last = lastFileInfo.find_as(name);
  if (last != lastFileInfo.end() && last->second.mtime == stat.mtime && last->second.size == stat.size)
    return false;
  changedFileInfo[name] = FileInfo(stat.mtime, stat.size);
  return true;
}

bool TextureRegManager::markFileChanged(const char *name)
{
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

namespace
{
bool is_generated_reg_name(const char *name)
{
  return strstr(name, GRADIENT_STR) || strstr(name, CURVE_STR) || strstr(name, CONST_STR) || strstr(name, COLOR_STR);
}
} // namespace

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

  if (strstr(name, "name:t="))
  {
    texWithParams = name;
    if (!textureRootDir.empty())
      texWithParams.replaceAll("name:t=", String(128, "name:t=%s/", textureRootDir.str()));
    const char *namePos = strstr(texWithParams, "name:t=");
    texname = namePos + strlen("name:t=");
  }
  else
  {
    if (textureRootDir.empty())
      texname = name;
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

namespace
{
BaseTexture *acquire_file_texture(const char *file, TEXTUREID &tex_id)
{
  tex_id = add_managed_texture(file);
  BaseTexture *tex = tex_id != BAD_TEXTUREID ? acquire_managed_tex(tex_id) : nullptr;
  if (!tex && tex_id != BAD_TEXTUREID)
  {
    if (get_managed_texture_refcount(tex_id) > 0)
    {
      release_managed_tex(tex_id);
    }
    tex_id = BAD_TEXTUREID;
  }
  return tex;
}
} // namespace

bool TextureRegManager::isFileLoadPending(const char *reg_name) const
{
  return strstr(reg_name, TEX_STR) == reg_name && !is_generated_reg_name(reg_name) && getRegNo(reg_name) < 0 &&
         loadedFiles.find_as(reg_name) == loadedFiles.end();
}

bool TextureRegManager::prepareFileLoad(const char *reg_name, FileLoad &load)
{
  if (!isFileLoadPending(reg_name))
  {
    return false;
  }
  String texname;
  String texWithParams;
  getRelativeTexName(reg_name, texname, texWithParams);
  DagorStat stat;
  if (texname.length() <= 1 || df_stat(texname, &stat) == -1)
  {
    return false;
  }
  load.regName = reg_name;
  load.texWithParams = texWithParams.str();
  load.info = FileInfo(stat.mtime, stat.size);
  // validateFile's test, without scheduling the change
  const auto last = lastFileInfo.find_as(texname.str());
  load.changed = changedFileInfo.find_as(texname.str()) != changedFileInfo.end() || last == lastFileInfo.end() ||
                 last->second.mtime != stat.mtime || last->second.size != stat.size;
  return true;
}

void TextureRegManager::loadFile(FileLoad &load)
{
  d3d::LoadingAutoLock loadingLock;
  load.texId = get_managed_texture_id(load.texWithParams.c_str());
  if (load.texId != BAD_TEXTUREID)
  {
    if (get_managed_texture_refcount(load.texId) > 0)
    {
      load.texId = BAD_TEXTUREID;
      return;
    }
    if (load.changed)
    {
      discard_unused_managed_texture(load.texId);
    }
  }
  if (acquire_file_texture(load.texWithParams.c_str(), load.texId))
  {
    ddsx::tex_pack2_perform_delayed_data_loading();
  }
}

void TextureRegManager::releaseLoad(const FileLoad &load) { release_managed_tex(load.texId); }

void TextureRegManager::addLoadedFile(const FileLoad &load)
{
  if (!isFileLoadPending(load.regName.c_str()))
  {
    release_managed_tex(load.texId);
    return;
  }
  const BaseTexture *tex = D3dResManagerData::getBaseTex(load.texId);
  const uint64_t memSize = tex ? tex->getSize() : 0;
  currentMemSize += memSize;
  maxMemSize = max(maxMemSize, currentMemSize);
  loadedFiles.emplace(load.regName, LoadedFile{load.texId, load.info, memSize});
}

void TextureRegManager::releaseLoadedFiles()
{
  for (const auto &file : loadedFiles)
  {
    currentMemSize -= file.second.memSize;
    release_managed_tex(file.second.texId);
  }
  loadedFiles.clear();
}

TEXTUREID TextureRegManager::takeLoadedFile(const char *reg_name, const char *tex_name)
{
  const auto loaded = loadedFiles.find_as(reg_name);
  if (loaded == loadedFiles.end() || loaded->second.texId == BAD_TEXTUREID) // a failed load stays marked
  {
    return BAD_TEXTUREID;
  }
  const LoadedFile file = loaded->second;
  loadedFiles.erase(loaded);
  // createTexture counts the texture again when it takes it.
  currentMemSize -= file.memSize;
  DagorStat stat;
  if (df_stat(tex_name, &stat) == -1 || stat.mtime != file.info.mtime || stat.size != file.info.size)
  {
    release_managed_tex(file.texId);
    return BAD_TEXTUREID;
  }
  return file.texId;
}

int TextureRegManager::pooledTargetCount(uint32_t fmt, int w, int h) const
{
  int count = 0;
  for (const PooledTexture &pooled : texturePool)
  {
    count += pooled.flags == (TEXCF_RTARGET | fmt) && pooled.w == w && pooled.h == h;
  }
  return count;
}

bool TextureRegManager::isGeneratedTexturePending(const char *reg_name) const
{
  const int regNo = findRegNo(reg_name);
  return strstr(reg_name, TEX_STR) == reg_name && is_generated_reg_name(reg_name) && (regNo < 0 || !textureRegs[regNo].tex);
}

bool TextureRegManager::poolTexture(int reg)
{
  TexureRegister &r = textureRegs[reg];
  if (!r.targetFlags || !r.tex || r.tex->getSize() > texturePoolLimit || get_managed_texture_refcount(r.texId) != 1)
  {
    return false;
  }
  BaseTexture *tex = static_cast<BaseTexture *>(r.tex);
  TextureInfo info;
  tex->getinfo(info, 0);
  ShaderGlobal::reset_from_vars(r.texId);
  change_managed_texture(r.texId, nullptr);
  release_managed_tex(r.texId);
  texturePool.push_back({tex, r.targetFlags, info.w, info.h});
  texturePoolSize += tex->getSize();
  r.clear();
  while (texturePoolSize > texturePoolLimit)
  {
    const uint64_t size = texturePool.front().tex->getSize();
    texturePoolSize -= size;
    currentMemSize -= size;
    del_d3dres(texturePool.front().tex);
    texturePool.erase(texturePool.begin());
  }
  return true;
}

int TextureRegManager::findPooledTarget(uint32_t flags, int w, int h) const
{
  for (int i = static_cast<int>(texturePool.size()) - 1; i >= 0; --i)
  {
    if (texturePool[i].flags == flags && texturePool[i].w == w && texturePool[i].h == h)
    {
      return i;
    }
  }
  return -1;
}

BaseTexture *TextureRegManager::takePooledTexture(uint32_t flags, int w, int h)
{
  const int i = findPooledTarget(flags, w, h);
  if (i < 0)
  {
    return nullptr;
  }
  BaseTexture *tex = texturePool[i].tex;
  texturePoolSize -= tex->getSize();
  currentMemSize -= tex->getSize();
  texturePool.erase(texturePool.begin() + i);
  return tex;
}

void TextureRegManager::releaseTexturePool()
{
  for (PooledTexture &pooled : texturePool)
  {
    del_d3dres(pooled.tex);
  }
  texturePool.clear();
  currentMemSize -= texturePoolSize;
  texturePoolSize = 0;
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
      // A tex: register keeps the size it was made with, as its creation below ignores the request.
      const bool reuseAnySize = strstr(name, TEX_STR) == name;
      bool canReuse = reuseAnySize || !textureRegs[regNo].retained;
      if (canReuse && textureRegs[regNo].tex->getType() != D3DResourceType::TEX)
      {
        logger->log(LOGLEVEL_ERR, String(128, "tex reg  <%s> differs from previous use, it is not a texture", name));
        return -1;
      }
      if (!reuseAnySize && textureRegs[regNo].tex->getType() == D3DResourceType::TEX)
      {
        TextureInfo tinfo;
        ((BaseTexture *)textureRegs[regNo].tex)->getinfo(tinfo, 0);
        // fmt may carry TEXCF_UNORDERED (is_uav outputs): a UAV texture and a plain one are not interchangeable
        canReuse = tinfo.w == w && tinfo.h == h && ((tinfo.cflg ^ fmt) & (TEXFMT_MASK | TEXCF_UNORDERED)) == 0;
        if (!canReuse && !textureRegs[regNo].retained)
        {
          logger->log(LOGLEVEL_ERR, String(128, "tex reg  <%s> differs from previous use %dx%d(0x%X) != %dx%d(0x%X)", name, w, h, fmt,
                                      tinfo.w, tinfo.h, tinfo.cflg & (TEXFMT_MASK | TEXCF_UNORDERED)));
          return -1;
        }
      }
      if (canReuse)
      {
        unretain(regNo);
        textureRegs[regNo].useCount = use_count;
        return regNo;
      }
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
    unretain(regNo);
    currentMemSize -= textureRegs[regNo].tex->getSize();
    stats.resourcesReleased++;
    textureRegs[regNo].close(logger);
  }
  if (strstr(name, TEX_STR) != name && findPooledTarget(TEXCF_RTARGET | fmt, w, h) < 0)
    evictRetained(uint64_t(w) * h * get_tex_format_desc(fmt & TEXFMT_MASK).bytesPerElement);
  BaseTexture *tex = 0;
  TEXTUREID texId = BAD_TEXTUREID;
  bool owned = false;
  uint32_t targetFlags = 0;

  if (strstr(name, TEX_STR) == name)
  {
    if (const char *gradientStr = strstr(name, GRADIENT_STR))
    {
      gradientStr += strlen(GRADIENT_STR);
      fmt = TEXFMT_A16B16G16R16;
      tex = d3d::create_tex(NULL, GRADIENT_TEXTURE_WIDTH, 1, fmt, 1, name, RESTAG_TEXGEN); //|TEXCF_UNORDERED
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
      tex = d3d::create_tex(NULL, GRADIENT_TEXTURE_WIDTH, 1, fmt, 1, name, RESTAG_TEXGEN); //|TEXCF_UNORDERED
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
      tex = d3d::create_tex(NULL, 1, 1, fmt, 1, name, RESTAG_TEXGEN); //|TEXCF_UNORDERED
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
      tex = d3d::create_tex(NULL, 2, 2, fmt, 1, name, RESTAG_TEXGEN); //|TEXCF_UNORDERED
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
      const TEXTUREID loadedId = takeLoadedFile(name, texname);
      const bool loadedAhead = loadedId != BAD_TEXTUREID;
      texId = loadedAhead ? loadedId : get_managed_texture_id(texWithParams);

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

        if (texId != BAD_TEXTUREID && !loadedAhead)
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
          tex = acquire_file_texture(texWithParams, texId);
          if (!tex)
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
        // a load ahead hands its reference to the register
        tex = loadedAhead ? D3dResManagerData::getBaseTex(texId) : acquire_managed_tex(texId);
        if (!tex)
        {
          if (loadedAhead)
          {
            release_managed_tex(texId);
          }
          logger->log(LOGLEVEL_ERR, String(128, "can not create texture <%s>, although file exist", texname));
          return -1;
        }
      }

      if (texId != BAD_TEXTUREID && !loadedAhead)
        ddsx::tex_pack2_perform_delayed_data_loading();
    }
  }
  else
  {
    String texName(128, "texgen_%s", name);
    targetFlags = TEXCF_RTARGET | fmt;
    tex = takePooledTexture(targetFlags, w, h);
    if (!tex)
    {
      tex = d3d::create_tex(NULL, w, h, targetFlags, 1, texName, RESTAG_TEXGEN); //|TEXCF_UNORDERED
      if (!tex && !texturePool.empty())
      {
        // The pool can hold the memory a full device needs for this texture.
        releaseTexturePool();
        tex = d3d::create_tex(NULL, w, h, targetFlags, 1, texName, RESTAG_TEXGEN);
      }
    }
    if (tex)
    {
      // Blending nodes read the target first: a pooled texture holds old pixels, a new one undefined ones.
      d3d::clear_rt({tex, 0, 0});
    }
    owned = true;
    if (tex)
      texId = register_managed_tex(texName, tex);
  }
  if (!tex)
  {
    logger->log(LOGLEVEL_ERR, String(128, "can not create texture reg <%s> with %dx%d resolution and 0x%X format", name, w, h, fmt));
    return -1;
  }

  currentMemSize += tex->getSize();
  maxMemSize = max(maxMemSize, currentMemSize);
  stats.resourcesCreated++;
  if (owned)
    stats.bytesCreated += tex->getSize();
  evictRetained(0);

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
  textureRegs[regNo].targetFlags = targetFlags;
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
      const bool sbuf = textureRegs[regNo].tex->getType() == D3DResourceType::SBUF;
      const int maxParticlesCnt = sbuf ? textureRegs[regNo].tex->getSize() / PARTICLE_SIZE : 0;
      if (!textureRegs[regNo].retained)
      {
        if (!sbuf)
        {
          logger->log(LOGLEVEL_ERR, String(128, "sbuffer reg  <%s> differs from previous definition (not sbuffer)", name));
          return -1;
        }
        if (maxParticlesCnt < max_count)
        {
          logger->log(LOGLEVEL_ERR,
            String(128, "sbuffer reg  <%s> differs from previous use, count = %d, needed = %d", name, maxParticlesCnt, max_count));
          return -1;
        }
      }
      if (sbuf && maxParticlesCnt >= max_count)
      {
        unretain(regNo);
        textureRegs[regNo].useCount = use_count;
        return regNo;
      }
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
    unretain(regNo);
    currentMemSize -= textureRegs[regNo].tex->getSize();
    stats.resourcesReleased++;
    textureRegs[regNo].close(logger);
  }
  evictRetained(uint64_t(PARTICLE_SIZE) * max_count);
  Sbuffer *particles = 0;
  String texName(128, "texgen_%s", name);
  particles = d3d::buffers::create_ua_sr_structured(PARTICLE_SIZE, max_count, texName.str(), d3d::buffers::Init::No, RESTAG_TEXGEN); //
  Sbuffer *args =
    particles ? d3d::buffers::create_ua_indirect(d3d::buffers::Indirect::Draw, 1, String(128, "%s_args", texName.str()), RESTAG_TEXGEN)
              : nullptr;
  if (!args)
    del_d3dres(particles);
  if (particles)
  {
    currentMemSize += particles->getSize();
    maxMemSize = max(maxMemSize, currentMemSize);
    stats.resourcesCreated++;
    stats.bytesCreated += particles->getSize();
    evictRetained(0);
  }
  if (!particles)
  {
    logger->log(LOGLEVEL_ERR, String(128, "can not create sbuffer reg <%s> with %d count", name, max_count));
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
  textureRegs[regNo].particlesArgs = args;
  textureRegs[regNo].useCount = use_count;
  textureRegs[regNo].texId = BAD_TEXTUREID;
  textureRegs[regNo].owned = true;
  textureRegs[regNo].targetFlags = 0;
  return regNo;
}


void TextureRegManager::setUseCount(int reg, unsigned cnt)
{
  if (reg < 0 || reg >= textureRegs.size())
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg#%d is invalid", reg));
    return;
  }
  if (!textureRegs[reg].isAlive())
    logger->log(LOGLEVEL_ERR, String(128, "we can't safely add usage to unused reg! reg#%d %s", reg, getRegName(reg)));
  unretain(reg);
  textureRegs[reg].useCount = cnt;
  if (autoShrink && !textureRegs[reg].useCount)
    releaseUnused(reg);
}

void TextureRegManager::discard(int reg)
{
  textureRegs[reg].useCount = 0;
  if (autoShrink || textureRegs[reg].retained)
    shrinkTex(reg);
}

void TextureRegManager::setRetainBudget(uint64_t bytes)
{
  retainBudget = bytes;
  evictRetained(0);
}

void TextureRegManager::releaseUnused(int reg)
{
  TexureRegister &r = textureRegs[reg];
  // "tex:" registers load from files and constants; they are cheap to make again and must see file changes.
  if (!retainBudget || r.useCount || !r.tex || strstr(r.name.c_str(), TEX_STR) == r.name.c_str())
  {
    shrinkTex(reg);
    return;
  }
  if (!r.retained)
  {
    r.retained = true;
    retainedMemSize += r.tex->getSize();
    retainedCount++;
    r.retainPrev = retainTail;
    r.retainNext = -1;
    (retainTail >= 0 ? textureRegs[retainTail].retainNext : retainHead) = reg;
    retainTail = reg;
  }
  evictRetained(0);
}

void TextureRegManager::unretain(int reg)
{
  TexureRegister &r = textureRegs[reg];
  if (!r.retained)
    return;
  r.retained = false;
  retainedMemSize -= r.tex->getSize();
  retainedCount--;
  (r.retainPrev >= 0 ? textureRegs[r.retainPrev].retainNext : retainHead) = r.retainNext;
  (r.retainNext >= 0 ? textureRegs[r.retainNext].retainPrev : retainTail) = r.retainPrev;
}

void TextureRegManager::evictRetained(uint64_t bytes_to_add)
{
  // Pooled textures have their own limit, and evicting into the pool frees nothing.
  while (retainHead >= 0 && currentMemSize - texturePoolSize + bytes_to_add > retainBudget)
  {
    const int reg = retainHead;
    logger->log(LOGLEVEL_REMARK, String(128, "evict retained reg#%d <%s>", reg, getRegName(reg)));
    unretain(reg);
    shrinkTex(reg);
  }
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
    releaseUnused(reg);
}

bool TextureRegManager::keepAllOrNone(dag::ConstSpan<int> regs)
{
  bool all = true;
  for (int reg : regs)
    all &= reg >= 0;
  if (!all)
    for (int reg : regs)
      if (reg >= 0)
        textureUsed(reg);
  return all;
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
  if (res->getType() > D3DResourceType::ARRTEX)
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg #%d <%s> is not a texture", reg, getRegName(reg)));
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
  if (res->getType() != D3DResourceType::SBUF)
  {
    logger->log(LOGLEVEL_ERR, String(128, "reg %d <%s> is not a buffer, it is #%d", reg, getRegName(reg), int(res->getType())));
    return 0;
  }
  return (Sbuffer *)res;
}

Sbuffer *TextureRegManager::getParticlesArgs(const D3dResource *particles) const
{
  for (const TexureRegister &r : textureRegs)
    if (r.particlesArgs && r.tex == particles)
      return r.particlesArgs;
  return nullptr;
}

const char *TextureRegManager::getRegName(int reg) const
{
  if (reg < 0 || reg >= textureRegs.size())
    return "___UNDEFINED___ and INVALID";
  return textureRegs[reg].name.c_str();
}
