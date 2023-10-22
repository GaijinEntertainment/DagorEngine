#include <render/debugTexOverlay.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <ctype.h>

struct FreeTexAtReturn
{
  TEXTUREID id;
  FreeTexAtReturn(TEXTUREID _id) : id(_id) {}
  ~FreeTexAtReturn() { release_managed_tex(id); }
};

static const float g_target_to_tex_ratio = 0.333f;

static int cube_indexVarId = -1;

static const char g_usage_text[] =
  "usage: show_tex <tex_name size_x size_y offset_x offset_y swizzling range_begin range_end> ; <...> ...\n"
  "     : (without <> brackets and up to 4 texture descriptions)\n"
  "     : order of (size_x,size_y,offset_x,offset_y)/swizzling/(range_begin,range_end) OUTSIDE of () is irrelevant\n"
  "     : to skip specifying a variable, use the character _\n"
  "     : format for range is .1234 that means it must begin with a decimal point (skipping range_end means 1.0)\n"
  "     : tex_name can be a shader_var\n"
  "e.g: show_tex biomeIndicesTex rrr1 512 .0 .5 ; lightmap_tex rgb1 512 _ 512 0 - shows these 2 textures side by side\n"
  "e.g: show_tex foo@2 - shows mip 2 of tex 'foo'\n"
  "e.g: show_tex foo@1#3 - shows mip 1 for face 3 of cubemap 'foo'\n"
  "e.g: show_tex foo#1$32 - shows 32 slices, starting at slice 1*32\n"
  "e.g: show_tex foo$0zoa - shows all slices, filtering Zeroes | Ones | Alpha\n";

DebugTexOverlay::DebugTexOverlay() : renderer(new PostFxRenderer()), targetSize(1024, 768) { init(); }

DebugTexOverlay::DebugTexOverlay(const Point2 &target_size) : renderer(new PostFxRenderer()), targetSize(target_size) { init(); }

void DebugTexOverlay::init()
{
  renderer->init("debug_tex_overlay");
  shaders::OverrideState state;
  state.set(shaders::OverrideState::SCISSOR_ENABLED);
  scissor = shaders::overrides::create(state);
}

DebugTexOverlay::~DebugTexOverlay()
{
  hideTex();
  del_it(renderer);
}

void DebugTexOverlay::setTargetSize(const Point2 &target_size) { targetSize = target_size; }

String DebugTexOverlay::processConsoleCmd(const char *argv[], int argc)
{
  if (!renderer->getMat())
    return String("'debug_tex_overlay' shader is unavailable");
  String result; // returns first error (or empty)
  clear_and_shrink(textures);
  int argOffset = 1; // starts with 1 to skip command name
  for (int i = 0; i < argc; ++i)
  {
    if (argv[i][0] == ';')
    {
      textures.push_back();
      result = textures.back().initFromConsoleCmd(&argv[argOffset], i - argOffset, targetSize);
      if (!result.empty())
        return result;
      argOffset = i + 1;
    }
  }
  if (argOffset <= argc)
  {
    textures.push_back();
    result = textures.back().initFromConsoleCmd(&argv[argOffset], argc - argOffset, targetSize);
  }
  return result;
}

static int convert_int_param(const char *str, int def) { return (str && str[0] != '_') ? atoi(str) : def; }
static float convert_float_param(const char *str, float def) { return (str && str[0] != '_') ? atof(str) : def; }

DebugTexOverlay::TextureWrapper::TextureWrapper() : texId(BAD_TEXTUREID), swizzling(), modifiers() {}

DebugTexOverlay::TextureWrapper::~TextureWrapper()
{
  if (needToReleaseTex)
    release_managed_tex(texId);
}

void DebugTexOverlay::TextureWrapper::reset()
{
  texId = BAD_TEXTUREID;
  // the rest is not important (can be garbage)
}

String DebugTexOverlay::TextureWrapper::initFromConsoleCmd(const char *argv[], int argc, const Point2 &target_size)
{
  reset();
  TEXTUREID id = BAD_TEXTUREID;
  Point2 offs(-1, -1);
  Point2 sz(-1, -1);
  int m = 0;
  int f = 0;
  int num = -1;
  int filter = 0;

  carray<Point4, 4> swz = {Point4(1, 0, 0, 0), Point4(0, 1, 0, 0), Point4(0, 0, 1, 0), Point4(0, 0, 0, 1)};

  carray<Point2, 4> mod = {Point2(1, 0), Point2(1, 0), Point2(1, 0), Point2(1, 0)};

  if (argc < 1)
    return String(g_usage_text);

  // offset_x, offset_y, width, height, swizzling, range_0, range_1
  const char *params[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

  for (int i = 1; i < argc; ++i)
  {
    const char *v = argv[i];
    // swizzling (caveat: cannot use 0 or 1 in first channel)
    if (v[0] == 'r' || v[0] == 'g' || v[0] == 'b' || v[0] == 'a')
      params[4] = v;
    // range begin/end: float
    else if (strstr(v, "."))
    {
      if (!params[5])
        params[5] = v;
      else if (!params[6])
        params[6] = v;
    }
    // others: order of parameters must be: width, height, offset_x, offset_y
    else if (!params[2])
      params[2] = v;
    else if (!params[3])
      params[3] = v;
    else if (!params[0])
      params[0] = v;
    else if (!params[1])
      params[1] = v;
  }

  G_ASSERT(strlen(argv[0]) < 128);

  char name[128];
  strcpy(name, argv[0]);

  if (strcmp("off", name) == 0 || strcmp("none", name) == 0 || strcmp("0", name) == 0)
    return String();

  char *cur = strchr(name, '@');
  if (cur && strlen(cur) > 1)
  {
    m = cur[1] - '0';
    int ofs = cur - name;
    memmove(cur, cur + 2, strlen(name) - ofs - 1);
  }

  cur = strchr(name, '#');
  if (!cur)
    cur = strchr(name, '!');
  if (cur && strlen(cur) > 1)
  {
    cur++;
    int cnt = 0;
    while (isdigit(cur[cnt]))
      cnt++;

    int oldChar = cur[cnt];
    cur[cnt] = 0;
    f = atoi(cur);
    cur[cnt] = oldChar;
    cur--;

    int ofs = cur - name;
    memmove(cur, cur + cnt + 1, strlen(name) - ofs - 1);
  }

  cur = strrchr(name, '$');
  if (cur && cur > name && strlen(cur) > 1)
  {
    cur++;
    int cnt = 0;
    while (isdigit(cur[cnt]))
      cnt++;

    char *next = cur + cnt;
    while (true)
    {
      if (*next == 'z')
        filter |= Zeroes;
      else if (*next == 'o')
        filter |= Ones;
      else if (*next == 'a')
        filter |= Alpha;
      else
        break;
      next++;
    }

    cur[cnt] = 0;
    num = atoi(cur);
    cur--;
    cur[0] = 0;
  }

  if (name[0] == '<' || name[0] == '>' || name[0] == '%')
  {
    logwarn("support for special marker \'%c\' in \"%s\" dropped, ignored", name[0], name);
    memmove(name, name + 1, strlen(name));
  }
  id = get_managed_texture_id(name);

  if (id == BAD_TEXTUREID) // worth trying to check shader_vars as well
    id = ShaderGlobal::get_tex_fast(::get_shader_variable_id(name, true));

  if (id == BAD_TEXTUREID)
  {
    id = ::get_tex_gameres(name);
    if (id != BAD_TEXTUREID)
      needToReleaseTex = true;
  }

  if (id == BAD_TEXTUREID)
    return String(128, "unknown managed texture: %s", name);

  BaseTexture *tex = acquire_managed_tex(id);
  FreeTexAtReturn ftat(id);

  if (m >= tex->level_count())
    return String(128, "incorrect mip: %d, tex has only: %d", m, tex->level_count());

  if (f != 0 && tex->restype() == RES3D_CUBETEX && f >= 6)
    return String(128, "incorrect cube face: %d, tex has only: 6", f);

  if (f != 0 && tex->restype() == RES3D_VOLTEX)
  {
    TextureInfo info;
    tex->getinfo(info);
    if (f >= info.d)
      return String(128, "incorrect face: %d, tex has only: %d", f, info.d);
  }

  if (f != 0 && (tex->restype() == RES3D_ARRTEX || tex->restype() == RES3D_CUBEARRTEX))
  {
    TextureInfo info;
    tex->getinfo(info);
    if (f >= info.a)
      return String(128, "incorrect face: %d, tex has only: %d", f, info.a);
  }

  if (f != 0 && (tex->restype() != RES3D_CUBETEX && tex->restype() != RES3D_ARRTEX && tex->restype() != RES3D_VOLTEX &&
                  tex->restype() != RES3D_CUBEARRTEX))
    return String(128, "trying to use face id for non-cubic/non-array/non-voltex tex");

  offs.x = convert_int_param(params[0], 0);
  offs.y = convert_int_param(params[1], 0);
  sz.x = convert_int_param(params[2], -1);
  sz.y = convert_int_param(params[3], -1);

  if (params[4])
  {
    const char *str = params[4];

    swz[0] = swz[1] = swz[2] = swz[3] = Point4(0, 0, 0, 0);
    mod[3] = Point2(0, 1);

    for (int i = 0; i < 4 && i < strlen(str); ++i)
    {
      switch (str[i])
      {
        case 'r':
          swz[i] = Point4(1, 0, 0, 0);
          mod[i] = Point2(1, 0);
          break;

        case 'g':
          swz[i] = Point4(0, 1, 0, 0);
          mod[i] = Point2(1, 0);
          break;

        case 'b':
          swz[i] = Point4(0, 0, 1, 0);
          mod[i] = Point2(1, 0);
          break;

        case 'a':
          swz[i] = Point4(0, 0, 0, 1);
          mod[i] = Point2(1, 0);
          break;

        case '0':
          swz[i] = Point4(0, 0, 0, 0);
          mod[i] = Point2(0, 0);
          break;

        case '1':
          swz[i] = Point4(0, 0, 0, 0);
          mod[i] = Point2(0, 1);
          break;

        default: return String(128, "unknown component: %c, must be one of: r/g/b/0/1", str[i]);
      }
    }
  }

  float ch_range0 = convert_float_param(params[5], 0);
  float ch_range1 = convert_float_param(params[6], 1);
  float chRangeMul = 1.0f / max(ch_range1 - ch_range0, 0.0001f);

  for (int i = 0; i < 3; ++i) // don't touch alpha
  {
    mod[i].x *= chRangeMul;
    mod[i].y -= ch_range0 * chRangeMul;
  }

  setTexEx(target_size, id, offs, sz, swz, mod, m, f, num, filter);
  return String();
}


void DebugTexOverlay::TextureWrapper::setTexEx(const Point2 &target_size_, TEXTUREID texId_, const Point2 &offset_,
  const Point2 &size_, const carray<Point4, 4> &swz_, const carray<Point2, 4> &mod_, int mip_, int face_, int num_slices_, int filter_)
{
  texId = texId_;
  mip = mip_;
  face = face_;
  offset = offset_;
  size = size_;
  numSlices = num_slices_;
  dataFilter = filter_;
  swizzling = swz_;
  modifiers = mod_;
  if (texId != BAD_TEXTUREID && (size.x < 0 || size.y < 0))
    fixAspectRatio(target_size_);
}

void DebugTexOverlay::hideTex() { clear_and_shrink(textures); }

void DebugTexOverlay::render()
{
  shaders::overrides::set(scissor);
  for (TextureWrapper &texture : textures)
  {
    texture.render(targetSize, *renderer);
  }
  shaders::overrides::reset();
}

void DebugTexOverlay::TextureWrapper::render(const Point2 &target_size, const PostFxRenderer &renderer)
{
  if (texId == BAD_TEXTUREID)
    return;
  if (!get_managed_texture_name(texId))
  {
    reset();
    return;
  }
  static int texVarId = get_shader_variable_id("swizzled_texture");
  static int texSizeVarId = get_shader_variable_id("swizzled_texture_size");
  static int texTypeVarId = get_shader_variable_id("swizzled_texture_type");
  static int texFaceMipVarId = get_shader_variable_id("swizzled_texture_face_mip");
  cube_indexVarId = get_shader_variable_id("cube_index", true);
  static int swzVarIds[4] = {get_shader_variable_id("tex_swizzling_r"), get_shader_variable_id("tex_swizzling_g"),
    get_shader_variable_id("tex_swizzling_b"), get_shader_variable_id("tex_swizzling_a")};
  static int modVarIds[4] = {get_shader_variable_id("tex_mod_r"), get_shader_variable_id("tex_mod_g"),
    get_shader_variable_id("tex_mod_b"), get_shader_variable_id("tex_mod_a")};

  BaseTexture *tex = acquire_managed_tex(texId);
  FreeTexAtReturn ftat(texId);
  if (!tex)
    return;

#if DAGOR_DBGLEVEL > 0
  int curTexFilter = tex->getTexfilter();
  if (curTexFilter == TEXFILTER_COMPARE)
    tex->texfilter(TEXFILTER_POINT);
#endif

  ShaderGlobal::set_texture(texVarId, texId);

  for (int i = 0; i < 4; ++i)
  {
    ShaderGlobal::set_color4(swzVarIds[i], (Color4 &)swizzling[i]);
    ShaderGlobal::set_color4(modVarIds[i], Color4(modifiers[i].x, modifiers[i].y, 0, 0)); // needs padding
  }

  float faceUse = face;
  if (tex->restype() == RES3D_TEX)
  {
    ShaderGlobal::set_int(texTypeVarId, 0);
  }
  else if (tex->restype() == RES3D_CUBETEX)
  {
    ShaderGlobal::set_int(texTypeVarId, 1);
  }
  else if (tex->restype() == RES3D_VOLTEX)
  {
    TextureInfo info;
    tex->getinfo(info, mip);
    ShaderGlobal::set_int(texTypeVarId, 3);
    faceUse = (faceUse + .5f) / info.d;
  }
  else if (tex->restype() == RES3D_ARRTEX)
  {
    if (tex->isCubeArray())
    {
      ShaderGlobal::set_real_fast(cube_indexVarId, static_cast<int>(faceUse) / 6);
      faceUse = static_cast<float>(static_cast<int>(faceUse) % 6);
      ShaderGlobal::set_int(texTypeVarId, 6);
    }
    else
      ShaderGlobal::set_int(texTypeVarId, 2);
  }

  ShaderGlobal::set_color4(texFaceMipVarId, faceUse, mip, face | dataFilter, numSlices);

  Point2 o = Point2(offset.x / target_size.x, offset.y / target_size.y);
  Point2 s = Point2(size.x / target_size.x, size.y / target_size.y);
  ShaderGlobal::set_color4(texSizeVarId, Color4(s.x, s.y, s.x - 1.f + o.x * 2, 1.f - s.y - o.y * 2));
  if (size.x <= 0 || size.y <= 0 || offset.x >= target_size.x || offset.y >= target_size.y)
  {
    logerr("invalid scissor %@ .. %@", offset, size);
  }
  else
  {
    d3d::setscissor(offset.x, offset.y, size.x, size.y);
    renderer.render();
  }

  ShaderGlobal::set_texture(texVarId, BAD_TEXTUREID);

#if DAGOR_DBGLEVEL > 0
  if (curTexFilter == TEXFILTER_COMPARE)
    tex->texfilter(curTexFilter);
#endif
}

void DebugTexOverlay::TextureWrapper::fixAspectRatio(const Point2 &targetSize)
{
  if (size.x < 0)
    size.x = targetSize.x * g_target_to_tex_ratio;

  BaseTexture *tex = acquire_managed_tex(texId);

  if (tex->restype() == RES3D_TEX || tex->restype() == RES3D_VOLTEX || tex->restype() == RES3D_ARRTEX)
  {
    TextureInfo ti;
    tex->getinfo(ti);
    float ratio = (float)ti.h / (float)ti.w;
    size.y = size.x * ratio;
  }
  else
  {
    size.y = size.x;
  }

  release_managed_tex(texId);
}
