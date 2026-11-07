// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "softRender.h"
#include <dag/dag_vector.h>
#include <image/dag_loadImage.h>
#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <math/dag_bounds3.h>
#include <math/dag_check_nan.h>
#include <math/dag_mathBase.h>
#include <math/dag_mesh.h>
#include <math/dag_Point2.h>
#include <memory/dag_memBase.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/algorithm.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

static constexpr float FOV_Y_DEG = 40;
static constexpr float SUN = 0.75f, AMBIENT = 0.55f, SHADOW = 0.4f;
static constexpr int ENV_WIDTH = 2048, ENV_LEVELS = 5; // reflection levels; one more is the ambient
static constexpr float GRID_LINEAR = 0.02f;
static constexpr float FIT_SHARE = 0.9f; // of the image the bbox corners fill
// linear colors; GROUND also paints the plane
static const Point3 SUN_COLOR(1.f, 0.96f, 0.9f), SKY(0.62f, 0.72f, 0.88f), GROUND(0.36f, 0.33f, 0.29f);
static const Point3 ZENITH(0.25f, 0.42f, 0.75f), HORIZON(0.85f, 0.88f, 0.92f), EARTH(0.12f, 0.11f, 0.1f);
static const Point3 DEFAULT_COLOR(RENDER_GRAY, RENDER_GRAY, RENDER_GRAY);

// The base layer, and a clear coat over it
struct Material
{
  float gloss, reflection;
  bool metal; // the base reflection takes the color
  bool coat;
};

// by SHADE_ value
static const Material MATERIALS[] = {
  {0.5f, 0.1f, false, true},
  {1.f, 0.9f, true, false},
  {0.1f, 0.f, false, false},
  {0.1f, 0.f, false, true},
  {0.45f, 0.6f, true, true},
};
static_assert(countof(MATERIALS) == SHADE_COUNT);
static float shininess(float gloss) { return exp2f(1 + 11 * gloss); }
static const float COAT_SHININESS = shininess(0.95f);

// Equirectangular, linear; each level halves and blurs the last; none: the procedural sky
struct Environment
{
  struct Level
  {
    int w = 0, h = 0;
    dag::Vector<Point3> px;
  };
  dag::Vector<Level> levels;
  float yawCos = 1, yawSin = 0; // the sky turned about +Y, Z toward X
};

static Point3 turn_y(const Point3 &d, float c, float s) { return Point3(d.x * c + d.z * s, d.y, d.z * c - d.x * s); }

struct Camera
{
  Point3 eye, right, up, forward, toSun, center;
  Point3 sunColor = SUN_COLOR, groundLit; // groundLit: the plane without shadow
  const Environment *env = nullptr;
  float radius = 1;
  float tanX = 1, tanY = 1, zNear = 0; // ortho: half extents in meters
  float shiftX = 0, shiftY = 0;        // lens shift, in half images
  bool ortho = false;
  float depthBase = 0; // ortho depth key: depthBase - z
};

struct Vertex
{
  Point3 view; // x right, y up, z depth
  Point3 color, normal;
};

// Attributes over depth: linear on screen
struct ScreenVertex
{
  float x, y, invZ;
  float depth; // linear on screen, bigger is nearer
  Point3 colorZ, normalZ;
};

struct Target
{
  int w = 0, h = 0;
  dag::Vector<Point3> color; // linear
  dag::Vector<float> depth;  // 0: nothing drawn
};

static float srgb_to_linear(float c) { return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f); }

static float linear_to_srgb(float c)
{
  // a color that overflowed to NaN would reach an int conversion
  c = check_nan(c) ? 0.f : eastl::clamp(c, 0.f, 1.f);
  return c <= 0.0031308f ? c * 12.92f : 1.055f * powf(c, 1 / 2.4f) - 0.055f;
}

static Point3 bbox_corner(const BBox3 &b, int i) { return Point3(b.lim[i & 1].x, b.lim[(i >> 1) & 1].y, b.lim[i >> 2].z); }

static Point3 to_view(const Camera &cam, const Point3 &p)
{
  const Point3 d = p - cam.eye;
  return Point3(d * cam.right, d * cam.up, d * cam.forward);
}

// Zoom and lens shift: the eye stays
static void fit_window(const BBox3 &box, Camera &cam)
{
  float lo[2] = {FLT_MAX, FLT_MAX}, hi[2] = {-FLT_MAX, -FLT_MAX};
  for (int i = 0; i < 8; ++i)
  {
    const Point3 v = to_view(cam, bbox_corner(box, i));
    const float ndc[2] = {v.x / (v.z * cam.tanX), v.y / (v.z * cam.tanY)};
    for (int k = 0; k < 2; ++k)
    {
      lo[k] = eastl::min(lo[k], ndc[k]);
      hi[k] = eastl::max(hi[k], ndc[k]);
    }
  }
  const float scale = eastl::max(hi[0] - lo[0], hi[1] - lo[1]) / (2 * FIT_SHARE);
  if (!(scale > 0))
    return;
  cam.tanX *= scale;
  cam.tanY *= scale;
  cam.shiftX = (lo[0] + hi[0]) * 0.5f / scale;
  cam.shiftY = (lo[1] + hi[1]) * 0.5f / scale;
}

static void make_camera(const Mesh &m, const RenderView &view, int w, int h, Camera &cam)
{
  BBox3 box;
  for (const Point3 &p : m.vert)
    box += p;
  if (box.isempty())
  {
    box += Point3(-1, -1, -1);
    box += Point3(1, 1, 1);
  }
  const Point3 center = box.center();
  const float radius = eastl::max(length(box.width()) * 0.5f, 1e-4f);
  const float pitchDeg = eastl::clamp(view.pitchDeg, -90.f, 90.f), yaw = DegToRad(view.yawDeg), pitch = DegToRad(pitchDeg);
  // straight up or down the right axis comes from the yaw alone, as the forward one leaves the horizontal plane
  const bool pole = fabsf(pitchDeg) == 90;
  const float flat = pole ? 0 : cosf(pitch);
  const Point3 toEye(flat * sinf(yaw), sinf(pitch), flat * cosf(yaw));
  cam.forward = -toEye;
  cam.right = pole ? Point3(cosf(yaw), 0, -sinf(yaw)) : normalize(cam.forward % Point3(0, 1, 0));
  cam.up = cam.right % cam.forward;
  // 50 degrees up: shadows fall behind
  cam.toSun = normalize(Point3(sinf(yaw), 0, cosf(yaw)) * 0.7f + Point3(0, 1.1f, 0) - cam.right * 0.6f);
  cam.center = center;
  cam.radius = radius;
  cam.ortho = view.ortho;
  Point3 aim = center;
  float dist = 0;
  if (view.ortho)
  {
    if (view.centered)
      aim += cam.right * (view.cx - center * cam.right) + cam.up * (view.cy - center * cam.up);
    cam.tanX = view.orthoWidth > 0 ? view.orthoWidth * 0.5f : 0.05f * radius;
    for (int i = 0; i < 8 && !(view.orthoWidth > 0); ++i)
    {
      const Point3 d = bbox_corner(box, i) - center;
      cam.tanX = eastl::max(cam.tanX, eastl::max(fabsf(d * cam.right), fabsf(d * cam.up) * w / h) / FIT_SHARE);
    }
    cam.tanY = cam.tanX * h / w;
    // the ground grid can lie far from the model
    const float reach = 2.2f * radius + fabsf(center.y) + length(aim - center);
    dist = 2 * reach;
    cam.depthBase = 2 * dist;
  }
  else
  {
    cam.tanY = tanf(DegToRad(FOV_Y_DEG) * 0.5f);
    cam.tanX = cam.tanY * w / h;
    dist = view.dist > 0 ? view.dist : 1.05f * radius / sinf(atanf(eastl::min(cam.tanX, cam.tanY)));
  }
  cam.eye = aim + toEye * dist;
  cam.zNear = dist * 1e-3f;
  if (!view.ortho && !(view.dist > 0))
    fit_window(box, cam);
}

static Vertex lerp_vertex(const Vertex &a, const Vertex &b, float t)
{
  return Vertex{a.view + (b.view - a.view) * t, a.color + (b.color - a.color) * t, a.normal + (b.normal - a.normal) * t};
}

static int clip_near(const Vertex in[3], float z_near, Vertex out[4])
{
  int n = 0;
  for (int i = 0; i < 3; ++i)
  {
    const Vertex &a = in[i], &b = in[(i + 1) % 3];
    const bool aIn = a.view.z >= z_near, bIn = b.view.z >= z_near;
    if (aIn)
      out[n++] = a;
    if (aIn != bIn)
      out[n++] = lerp_vertex(a, b, (z_near - a.view.z) / (b.view.z - a.view.z));
  }
  return n;
}

static ScreenVertex project(const Vertex &v, const Camera &cam, int w, int h)
{
  const float invZ = cam.ortho ? 1.f : 1 / v.view.z;
  return ScreenVertex{((v.view.x * invZ / cam.tanX - cam.shiftX) * 0.5f + 0.5f) * w,
    (0.5f - (v.view.y * invZ / cam.tanY - cam.shiftY) * 0.5f) * h, invZ, cam.ortho ? cam.depthBase - v.view.z : invZ, v.color * invZ,
    v.normal * invZ};
}

static float edge(const ScreenVertex &a, const ScreenVertex &b, float x, float y)
{
  return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}

// pixel(x, y, wa, wb, wc) per covered center
template <class Pixel>
static void scan(int w, int h, const ScreenVertex &a, const ScreenVertex &b, const ScreenVertex &c, Pixel &&pixel)
{
  const float area = edge(a, b, c.x, c.y);
  if (!check_finite(area) || area == 0)
    return;
  // the pixel centers x + 0.5 inside the bbox, within the image
  const int x0 = int(ceilf(eastl::clamp(eastl::min(a.x, eastl::min(b.x, c.x)) - 0.5f, 0.f, float(w))));
  const int x1 = int(floorf(eastl::clamp(eastl::max(a.x, eastl::max(b.x, c.x)) - 0.5f, -1.f, w - 1.f)));
  const int y0 = int(ceilf(eastl::clamp(eastl::min(a.y, eastl::min(b.y, c.y)) - 0.5f, 0.f, float(h))));
  const int y1 = int(floorf(eastl::clamp(eastl::max(a.y, eastl::max(b.y, c.y)) - 0.5f, -1.f, h - 1.f)));
  const float invArea = 1 / area;
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x)
    {
      const float px = x + 0.5f, py = y + 0.5f;
      const float wa = edge(b, c, px, py) * invArea, wb = edge(c, a, px, py) * invArea, wc = 1 - wa - wb;
      if (wa >= 0 && wb >= 0 && wc >= 0)
        pixel(x, y, wa, wb, wc);
    }
}

// Not unit: ray parameter equals view depth
static Point3 pixel_ray(const Camera &cam, float px, float py, int w, int h, Point3 &origin)
{
  const float x = (px / w - 0.5f) * 2 + cam.shiftX, y = (0.5f - py / h) * 2 + cam.shiftY;
  const Point3 side = cam.right * (x * cam.tanX) + cam.up * (y * cam.tanY);
  origin = cam.ortho ? cam.eye + side : cam.eye;
  return cam.ortho ? cam.forward : cam.forward + side;
}

static Point3 environment(const Point3 &r)
{
  const Point3 base = r.y >= 0 ? ZENITH + (HORIZON - ZENITH) * expf(-4 * r.y) : EARTH + (HORIZON * 0.6f - EARTH) * expf(8 * r.y);
  return base + Point3(0.5f, 0.5f, 0.5f) * expf(-25 * fabsf(r.y));
}

static Point3 env_dir(float u, float v)
{
  const float phi = (u - 0.5f) * TWOPI, theta = v * PI;
  return Point3(sinf(theta) * sinf(phi), cosf(theta), -sinf(theta) * cosf(phi));
}

// Bilinear at the image coordinates of [0, 1]; x wraps, y clamps
static Point3 env_texel(const Environment::Level &l, const Point2 &uv)
{
  const float x = uv.x * l.w - 0.5f, y = eastl::clamp(uv.y * l.h - 0.5f, 0.f, l.h - 1.f);
  // a direction that overflowed to NaN has no texel to convert to
  if (!check_finite(x) || !check_finite(y))
    return l.px[0];
  const int x0 = int(floorf(x)), y0 = int(y), y1 = eastl::min(y0 + 1, l.h - 1);
  const float fx = x - x0, fy = y - y0;
  const auto at = [&](int xx, int yy) { return l.px[yy * l.w + (xx % l.w + l.w) % l.w]; };
  return (at(x0, y0) * (1 - fx) + at(x0 + 1, y0) * fx) * (1 - fy) + (at(x0, y1) * (1 - fx) + at(x0 + 1, y1) * fx) * fy;
}

// level 0 sharp; ENV_LEVELS: the ambient
static Point3 env_sample(const Camera &cam, const Point3 &world, float level)
{
  const dag::Vector<Environment::Level> &lv = cam.env->levels;
  if (lv.empty())
    return environment(world);
  const Point3 d = turn_y(world, cam.env->yawCos, -cam.env->yawSin);
  const Point2 uv(0.5f + atan2f(d.x, -d.z) / TWOPI, acosf(eastl::clamp(d.y, -1.f, 1.f)) / PI);
  level = eastl::clamp(level, 0.f, float(ENV_LEVELS));
  const int i = int(level);
  const float f = level - i;
  const Point3 a = env_texel(lv[i], uv);
  return f > 0 ? a + (env_texel(lv[i + 1], uv) - a) * f : a;
}

static Point3 ambient(const Camera &cam, const Point3 &n)
{
  return cam.env->levels.empty() ? (GROUND + (SKY - GROUND) * (n.y * 0.5f + 0.5f)) * AMBIENT : env_sample(cam, n, ENV_LEVELS);
}

// The first of n texels in cell i of m, texel x lying in cell x * m / n: no texel is left out, and a cell holds n / m
// of them rounded either way
static int first_texel(int i, int m, int n) { return int((int64_t(i) * n + m - 1) / m); }

// Each cell the mean of its texels, an odd side leaving one cell a single texel
static Environment::Level halved(const Environment::Level &s)
{
  Environment::Level l{(s.w + 1) / 2, (s.h + 1) / 2};
  l.px.resize(l.w * l.h);
  for (int y = 0; y < l.h; ++y)
    for (int x = 0; x < l.w; ++x)
    {
      const int x0 = first_texel(x, l.w, s.w), x1 = first_texel(x + 1, l.w, s.w);
      const int y0 = first_texel(y, l.h, s.h), y1 = first_texel(y + 1, l.h, s.h);
      Point3 sum(0, 0, 0);
      for (int yy = y0; yy < y1; ++yy)
        for (int xx = x0; xx < x1; ++xx)
          sum += s.px[yy * s.w + xx];
      l.px[y * l.w + x] = sum * (1.f / ((x1 - x0) * (y1 - y0)));
    }
  return l;
}

static void blur_level(Environment::Level &l, int r)
{
  dag::Vector<Point3> line;
  for (int pass = 0; pass < 2; ++pass)
  {
    const int n = pass ? l.h : l.w, lines = pass ? l.w : l.h, step = pass ? l.w : 1, stride = pass ? 1 : l.w;
    line.resize(n);
    for (int k = 0; k < lines; ++k)
    {
      Point3 *p = l.px.data() + k * stride;
      for (int i = 0; i < n; ++i)
        line[i] = p[i * step];
      for (int i = 0; i < n; ++i)
      {
        Point3 sum(0, 0, 0);
        for (int j = i - r; j <= i + r; ++j)
          sum += line[pass ? eastl::clamp(j, 0, n - 1) : (j % n + n) % n];
        p[i * step] = sum / float(2 * r + 1);
      }
    }
  }
}

// TexImage32 keeps a side in a short: the loader writes past its buffer for a larger image, refused before it loads
static TexImage32 *load_picture(const char *path, const char *what, bool *alpha, eastl::string &err)
{
  int w = 0, h = 0;
  bool mayAlpha = false;
  // the loader reads the source whole, then resizes it to a :W:H suffix: both sizes must fit
  const char *ext = dd_get_fname_ext(path), *suffix = ext ? strchr(ext + 1, ':') : nullptr;
  const eastl::string source(path, suffix ? suffix - path : strlen(path));
  for (const char *name : {source.c_str(), suffix ? path : nullptr})
    if (name && read_image_dimensions(name, w, h, mayAlpha) && (w > SHRT_MAX || h > SHRT_MAX))
      return (err.sprintf("%s %s is %dx%d, past the %d a side the image loader holds", what, path, w, h, SHRT_MAX), nullptr);
  TexImage32 *img = load_image(path, tmpmem, alpha);
  if (!img)
    err.sprintf("cannot read %s %s", what, path);
  return img;
}

// Box-filtered to ENV_WIDTH; the sun is the brightest texel above the horizon, the plane the lower half's mean
static bool load_environment(const char *path, float yaw_deg, Environment &e, Camera &cam, eastl::string &err)
{
  TexImage32 *img = load_picture(path, "the environment", nullptr, err);
  if (img && (img->w < 8 || img->h < 4))
  {
    err.sprintf("the environment %s is %dx%d, under the 8x4 it needs", path, img->w, img->h);
    tmpmem->free(img);
    img = nullptr;
  }
  if (!img)
    return false;
  float lut[256];
  for (int i = 0; i < 256; ++i)
    lut[i] = srgb_to_linear(i / 255.f);
  // the top level keeps two rows at least: one per hemisphere; texel x in cell x * top.w / w, each cell the mean of its
  // texels, so a side that f does not divide loses none
  const int f = eastl::clamp(img->w / ENV_WIDTH, 1, img->h / 2);
  Environment::Level top{(img->w + f - 1) / f, (img->h + f - 1) / f};
  top.px.assign(top.w * top.h, Point3(0, 0, 0));
  for (int cy = 0; cy < top.h; ++cy)
    for (int cx = 0; cx < top.w; ++cx)
    {
      const int x0 = first_texel(cx, top.w, img->w), x1 = first_texel(cx + 1, top.w, img->w);
      const int y0 = first_texel(cy, top.h, img->h), y1 = first_texel(cy + 1, top.h, img->h);
      for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
          const TexPixel32 &p = img->getPixels()[int64_t(y) * img->w + x];
          top.px[cy * top.w + cx] += Point3(lut[p.r], lut[p.g], lut[p.b]) / float((x1 - x0) * (y1 - y0));
        }
    }
  tmpmem->free(img);
  e.levels.push_back(eastl::move(top));
  while (e.levels.size() < ENV_LEVELS)
  {
    e.levels.push_back(halved(e.levels.back()));
    blur_level(e.levels.back(), 2);
  }
  Environment::Level amb = e.levels.back();
  while (amb.w > 32)
    amb = halved(amb);
  for (int i = 0; i < 3; ++i)
    blur_level(amb, 4);
  e.levels.push_back(eastl::move(amb));

  // the blurred levels mix the hemispheres near the horizon: the sun takes level 2 with four rows a hemisphere past its
  // blur, else the sharp level, and the ground the sharp level's lower half
  const Environment::Level &sky = e.levels[e.levels[2].h >= 8 ? 2 : 0], &low = e.levels[0];
  int best = 0;
  for (int i = 0; i < sky.w * (sky.h / 2); ++i)
    best = sky.px[i].x + sky.px[i].y + sky.px[i].z > sky.px[best].x + sky.px[best].y + sky.px[best].z ? i : best;
  const Point3 &c = sky.px[best];
  e.yawCos = cosf(DegToRad(yaw_deg));
  e.yawSin = sinf(DegToRad(yaw_deg));
  cam.toSun = turn_y(env_dir((best % sky.w + 0.5f) / sky.w, (best / sky.w + 0.5f) / sky.h), e.yawCos, e.yawSin);
  cam.sunColor = c / eastl::max(eastl::max(c.x, c.y), eastl::max(c.z, 1e-6f));
  // a sun below 8 degrees would cast endless shadows
  cam.toSun = normalize(Point3(cam.toSun.x, eastl::max(cam.toSun.y, 0.14f), cam.toSun.z));
  Point3 sum(0, 0, 0);
  float weight = 0;
  for (int y = low.h / 2; y < low.h; ++y)
  {
    const float w = sinf((y + 0.5f) / low.h * PI);
    for (int x = 0; x < low.w; ++x)
      sum += low.px[y * low.w + x] * w;
    weight += w * low.w;
  }
  cam.groundLit = sum / eastl::max(weight, 1e-6f);
  printf("env: the sun at %.1f degrees up, the ground linear %.3f %.3f %.3f\n", RadToDeg(asinf(cam.toSun.y)), cam.groundLit.x,
    cam.groundLit.y, cam.groundLit.z);
  return true;
}

// Blinn-Phong with Schlick on h.v; v from the eye
static Point3 highlight(const Camera &cam, const Point3 &n, const Point3 &v, float power, const Point3 &f0)
{
  const Point3 h = normalize(cam.toSun - v);
  const float schlick = powf(1 - eastl::max(-(h * v), 0.f), 5);
  const float spec = (power + 8) / 8 * powf(eastl::max(n * h, 0.f), power) * eastl::max(n * cam.toSun, 0.f) * SUN;
  return mul(cam.sunColor, f0 + (Point3(1, 1, 1) - f0) * schlick) * spec;
}

// v: unit, from the eye
static Point3 shade(const Camera &cam, const Material &mat, float power, const Point3 &matDiffuse, const Point3 &base, const Point3 &n,
  const Point3 &v)
{
  const Point3 color = mul(base, matDiffuse), r = v - n * (2 * (v * n)), dielectric(0.04f, 0.04f, 0.04f);
  const float nv = -(n * v), front = nv > 0 ? 1.f : 0.f, fresnel = powf(1 - eastl::max(nv, 0.f), 5);
  const Point3 diffuse = mul(color, ambient(cam, n) + cam.sunColor * (SUN * eastl::max(n * cam.toSun, 0.f)));
  const Point3 sky = mat.reflection > 0 ? env_sample(cam, r, (1 - mat.gloss) * (ENV_LEVELS - 1)) : Point3(0, 0, 0);
  const float kr = front * mat.reflection * (mat.metal ? 1.f : 0.25f + 0.75f * fresnel);
  const Point3 baseLayer = diffuse * (1 - kr) + (mat.metal ? mul(sky, color) : sky) * kr +
                           highlight(cam, n, v, power, mat.metal ? color : dielectric) * front;
  if (!mat.coat)
    return baseLayer;
  const float kc = front * (0.04f + 0.96f * fresnel);
  return baseLayer * (1 - kc) + env_sample(cam, r, 0) * kc + highlight(cam, n, v, COAT_SHININESS, dielectric) * front;
}

// Misoriented back faces get no highlight
static void raster(Target &t, const Camera &cam, const Material &mat, const Point3 &diffuse, const ScreenVertex &a,
  const ScreenVertex &b, const ScreenVertex &c)
{
  const float power = shininess(mat.gloss);
  const Point3 orthoView = normalize(cam.forward);
  scan(t.w, t.h, a, b, c, [&](int x, int y, float wa, float wb, float wc) {
    const float depth = wa * a.depth + wb * b.depth + wc * c.depth;
    float &stored = t.depth[y * t.w + x];
    if (depth <= stored)
      return;
    stored = depth;
    const float z = 1 / (wa * a.invZ + wb * b.invZ + wc * c.invZ);
    const Point3 color = (a.colorZ * wa + b.colorZ * wb + c.colorZ * wc) * z;
    const Point3 n = normalize(a.normalZ * wa + b.normalZ * wb + c.normalZ * wc);
    Point3 origin, v = cam.ortho ? orthoView : normalize(pixel_ray(cam, x + 0.5f, y + 0.5f, t.w, t.h, origin));
    t.color[y * t.w + x] = shade(cam, mat, power, diffuse, color, n, v);
  });
}

static void box_blur(dag::Vector<float> &v, int w, int h, int r)
{
  dag::Vector<float> line;
  for (int pass = 0; pass < 2; ++pass)
  {
    const int n = pass ? h : w, lines = pass ? w : h, step = pass ? w : 1, stride = pass ? 1 : w;
    line.resize(n);
    for (int l = 0; l < lines; ++l)
    {
      float *p = v.data() + l * stride, sum = 0;
      for (int i = 0; i < n; ++i)
        line[i] = p[i * step];
      // taps past the border repeat the border sample
      for (int i = -r - 1; i < r; ++i)
        sum += line[eastl::clamp(i, 0, n - 1)];
      for (int i = 0; i < n; ++i)
      {
        sum += line[eastl::min(i + r, n - 1)];
        sum -= line[eastl::max(i - r - 1, 0)];
        p[i * step] = sum / (2 * r + 1);
      }
    }
  }
}

// Image resolution, not the supersampled target
static dag::Vector<float> ground_shadow(const Mesh &m, const Camera &cam, int w, int h)
{
  dag::Vector<float> shadow(w * h, 0.f);
  const Point3 &l = cam.toSun;
  for (uint32_t f = 0; f < m.face.size(); ++f)
  {
    // the face above the ground alone: z holds the height, so clip_near cuts at y = 0
    Vertex world[3], above[4];
    for (uint32_t k = 0; k < 3; ++k)
    {
      const Point3 &p = m.vert[m.face[f].v[k]];
      world[k] = Vertex{Point3(p.x, p.z, p.y), Point3(), Point3()};
    }
    const int na = clip_near(world, 0, above);
    for (int t = 1; t + 1 < na; ++t)
    {
      Vertex corners[3], clipped[4];
      for (int k = 0; k < 3; ++k)
      {
        const Point3 &q = above[k ? t + k - 1 : 0].view, p(q.x, q.z, q.y);
        corners[k] = Vertex{to_view(cam, p - l * (p.y / l.y)), Point3(), Point3()};
      }
      ScreenVertex s[4];
      const int n = clip_near(corners, cam.zNear, clipped);
      for (int i = 0; i < n; ++i)
        s[i] = project(clipped[i], cam, w, h);
      for (int i = 1; i + 1 < n; ++i)
        scan(w, h, s[0], s[i], s[i + 1], [&](int x, int y, float, float, float) { shadow[y * w + x] = 1; });
    }
  }
  box_blur(shadow, w, h, eastl::max(1, int(w * 0.01f)));
  return shadow;
}

static void draw_ground(Target &t, const Camera &cam, dag::ConstSpan<float> shadow, int w, const Point3 &bg)
{
  for (int y = 0; y < t.h; ++y)
    for (int x = 0; x < t.w; ++x)
    {
      Point3 origin;
      const Point3 d = pixel_ray(cam, x + 0.5f, y + 0.5f, t.w, t.h, origin);
      const float k = d.y < 0 ? -origin.y / d.y : -1;
      const float depth = cam.ortho ? cam.depthBase - k : 1 / k;
      if (!(k > cam.zNear) || !(depth > t.depth[y * t.w + x]))
        continue;
      t.depth[y * t.w + x] = depth;
      const Point3 p = origin + d * k;
      const float s = shadow[(y * w / t.w) * w + x * w / t.w];
      const Point3 lit = cam.groundLit * (1 - SHADOW * s);
      const float far = eastl::clamp((hypotf(p.x - cam.center.x, p.z - cam.center.z) / cam.radius - 2) / 4, 0.f, 1.f);
      t.color[y * t.w + x] = lit + (bg - lit) * far;
    }
}

// One target pixel wide, depth-tested, clipped to the screen
static void draw_line(Target &t, const Camera &cam, const Point3 &from, const Point3 &to, const Point3 &color)
{
  Vertex v[2] = {{to_view(cam, from), color, color}, {to_view(cam, to), color, color}};
  if (v[0].view.z < cam.zNear && v[1].view.z < cam.zNear)
    return;
  for (int i = 0; i < 2; ++i)
    if (v[i].view.z < cam.zNear)
      v[i] = lerp_vertex(v[i], v[1 - i], (cam.zNear - v[i].view.z) / (v[1 - i].view.z - v[i].view.z));
  const ScreenVertex a = project(v[0], cam, t.w, t.h), b = project(v[1], cam, t.w, t.h);
  const float d[2] = {b.x - a.x, b.y - a.y}, s[2] = {a.x, a.y}, size[2] = {float(t.w), float(t.h)};
  float k0 = 0, k1 = 1;
  for (int i = 0; i < 2; ++i)
  {
    if (d[i] == 0 && (s[i] < 0 || s[i] >= size[i]))
      return;
    if (d[i] == 0)
      continue;
    const float e0 = -s[i] / d[i], e1 = (size[i] - s[i]) / d[i];
    k0 = eastl::max(k0, eastl::min(e0, e1));
    k1 = eastl::min(k1, eastl::max(e0, e1));
  }
  if (!(k0 < k1))
    return;
  const int steps = int(eastl::max(fabsf(d[0]), fabsf(d[1])) * (k1 - k0)) + 1;
  for (int i = 0; i <= steps; ++i)
  {
    const float k = k0 + (k1 - k0) * i / steps;
    const int x = int(s[0] + d[0] * k), y = int(s[1] + d[1] * k);
    if (x < 0 || y < 0 || x >= t.w || y >= t.h)
      continue;
    // ortho samples the plane as faces do
    Point3 origin;
    const Point3 ray = pixel_ray(cam, x + 0.5f, y + 0.5f, t.w, t.h, origin);
    const float lineDepth = cam.ortho && ray.y != 0 ? cam.depthBase + origin.y / ray.y : a.depth + (b.depth - a.depth) * k;
    // the bias wins over a face lying in the grid plane
    const float depth = cam.ortho ? lineDepth + 1e-3f : lineDepth * 1.001f;
    if (depth <= t.depth[y * t.w + x])
      continue;
    t.depth[y * t.w + x] = depth;
    t.color[y * t.w + x] = color;
  }
}

static void draw_grid(Target &t, const Camera &cam, const Mesh &m)
{
  BBox3 box;
  for (const Point3 &p : m.vert)
    box += p;
  const float margin = box.isempty() ? 0 : 0.5f * eastl::max(box.width().x, box.width().z);
  // above 500 m the lines fill the image
  if (box.isempty() || !(margin < 250))
    return;
  // past 100 km a float holds no 10 cm line, and the line numbers stay far inside int
  const float reach = eastl::max(eastl::max(fabsf(box[0].x), fabsf(box[1].x)), eastl::max(fabsf(box[0].z), fabsf(box[1].z)));
  if (!(reach + margin < 1e5f))
  {
    printf("grid: none, the model reaches %g m from the origin, past the 100 km of 10 cm lines\n", reach);
    return;
  }
  const int x0 = int(floorf((box[0].x - margin) * 10)), x1 = int(ceilf((box[1].x + margin) * 10));
  const int z0 = int(floorf((box[0].z - margin) * 10)), z1 = int(ceilf((box[1].z + margin) * 10));
  // whole meters darker, so the author can count
  const Point3 tenth(GRID_LINEAR * 10, GRID_LINEAR * 10, GRID_LINEAR * 10), meter(GRID_LINEAR, GRID_LINEAR, GRID_LINEAR);
  for (int i = x0; i <= x1; ++i)
    draw_line(t, cam, Point3(i * 0.1f, 0, z0 * 0.1f), Point3(i * 0.1f, 0, z1 * 0.1f), i % 10 ? tenth : meter);
  for (int i = z0; i <= z1; ++i)
    draw_line(t, cam, Point3(x0 * 0.1f, 0, i * 0.1f), Point3(x1 * 0.1f, 0, i * 0.1f), i % 10 ? tenth : meter);
}

// soft shoulder: highlights do not clip flat
static float tone(float c) { return c < 0.8f ? c : 1 - 0.2f * expf((0.8f - c) / 0.2f); }

static Point3 linear_color(const TexPixel32 &p)
{
  return Point3(srgb_to_linear(p.r / 255.f), srgb_to_linear(p.g / 255.f), srgb_to_linear(p.b / 255.f));
}

// The reference pixel under output pixel (x, y) of w x h, its products in 64 bits
static const TexPixel32 &ref_pixel(const dag::Vector<TexPixel32> &ref, int refW, int refH, int x, int y, int w, int h)
{
  return ref[int64_t(2 * y + 1) * refH / (2 * h) * refW + int64_t(2 * x + 1) * refW / (2 * w)];
}

static void put_color(TexPixel32 &px, const Point3 &linear)
{
  px.r = uint8_t(linear_to_srgb(linear.x) * 255 + 0.5f);
  px.g = uint8_t(linear_to_srgb(linear.y) * 255 + 0.5f);
  px.b = uint8_t(linear_to_srgb(linear.z) * 255 + 0.5f);
  px.a = 255;
}

// Background not reachable from the border joins the mask: silhouettes, not spoke gaps
static void fill_holes(dag::Vector<uint8_t> &mask, int w, int h)
{
  dag::Vector<uint8_t> outside(w * h, 0);
  dag::Vector<int> stack;
  for (int i = 0; i < w * h; ++i)
    if ((i % w == 0 || i % w == w - 1 || i < w || i >= w * (h - 1)) && !mask[i])
    {
      outside[i] = 1;
      stack.push_back(i);
    }
  while (!stack.empty())
  {
    const int i = stack.back();
    stack.pop_back();
    const int next[4] = {i % w ? i - 1 : -1, i % w < w - 1 ? i + 1 : -1, i - w, i + w};
    for (int n : next)
      if (n >= 0 && n < w * h && !mask[n] && !outside[n])
      {
        outside[n] = 1;
        stack.push_back(n);
      }
  }
  for (int i = 0; i < w * h; ++i)
    mask[i] = !outside[i];
}

// Mask pixels with a 4-neighbour outside the mask or the image
static void mask_outline(const dag::Vector<uint8_t> &mask, int w, int h, dag::Vector<int> &out)
{
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
    {
      const int i = y * w + x;
      if (mask[i] && (x == 0 || y == 0 || x == w - 1 || y == h - 1 || !mask[i - 1] || !mask[i + 1] || !mask[i - w] || !mask[i + w]))
        out.push_back(i);
    }
}

// Squared pixel distance of every pixel to its nearest seed (Felzenszwalb and Huttenlocher): columns, then rows by the
// lower envelope of parabolas; exact integers, as a side is 32767 at most (a -ref image's), so they stay under
// 2 * 32766^2, far below UINT32_MAX, the mark of a pixel with no seed
static dag::Vector<uint32_t> squared_distances(const dag::Vector<int> &seeds, int w, int h)
{
  dag::Vector<uint32_t> g(w * h, UINT32_MAX), d(w * h, UINT32_MAX);
  dag::Vector<double> z(w + 1);
  dag::Vector<int> v(w);
  for (int p : seeds)
    g[p] = 0;
  for (int x = 0; x < w; ++x)
    for (int pass = 0; pass < 2; ++pass)
      for (int i = 1, last = -1; i <= h; ++i)
      {
        const int y = pass ? h - i : i - 1;
        last = g[y * w + x] == 0 ? y : last;
        if (last >= 0)
          g[y * w + x] = eastl::min(g[y * w + x], uint32_t(sqr(y - last)));
      }
  for (int y = 0; y < h; ++y)
  {
    const uint32_t *f = g.data() + y * w;
    int k = -1;
    for (int q = 0; q < w; ++q)
    {
      if (f[q] == UINT32_MAX)
        continue;
      double s = 0;
      while (k >= 0 && (s = (double(f[q]) + sqr(q) - double(f[v[k]]) - sqr(v[k])) / (2.0 * (q - v[k]))) <= z[k])
        --k;
      v[++k] = q;
      z[k] = k ? s : -DBL_MAX;
      z[k + 1] = DBL_MAX;
    }
    for (int q = 0, j = 0; k >= 0 && q < w; ++q)
    {
      while (z[j + 1] < q)
        ++j;
      d[y * w + q] = uint32_t(sqr(q - v[j])) + f[v[j]];
    }
  }
  return d;
}

// pixels from the farthest point of a, at, to its nearest point of b
static double farthest(const dag::Vector<int> &a, const dag::Vector<int> &b, int w, int h, int &at)
{
  const dag::Vector<uint32_t> d = squared_distances(b, w, h);
  double worst = -1;
  at = 0;
  for (int p : a)
    if (d[p] > worst)
    {
      worst = d[p];
      at = p;
    }
  return sqrt(eastl::max(worst, 0.0));
}

static TexPixel32 pixel(uint8_t r, uint8_t g, uint8_t b)
{
  TexPixel32 p;
  p.r = r;
  p.g = g;
  p.b = b;
  p.a = 255;
  return p;
}

static void draw_ring(dag::Vector<TexPixel32> &image, int w, int h, int at, const TexPixel32 &color)
{
  const int r = eastl::max(w / 80, 8), cx = at % w, cy = at / w;
  for (int y = eastl::max(cy - r - 2, 0); y <= eastl::min(cy + r + 2, h - 1); ++y)
    for (int x = eastl::max(cx - r - 2, 0); x <= eastl::min(cx + r + 2, w - 1); ++x)
      if (abs(sqr(x - cx) + sqr(y - cy) - sqr(r)) <= 3 * r)
        image[y * w + x] = color;
}

// The photo's alpha is its mask; an opaque photo: off the corners' color
static bool write_ref_diff(const char *path, dag::Vector<uint8_t> &model, const dag::Vector<TexPixel32> &photo, bool alpha,
  const Camera &cam, int w, int h, const Point3 &background, eastl::string &err)
{
  const TexPixel32 c[4] = {photo[0], photo[w - 1], photo[(h - 1) * w], photo[h * w - 1]};
  const int back[3] = {
    (c[0].r + c[1].r + c[2].r + c[3].r) / 4, (c[0].g + c[1].g + c[2].g + c[3].g) / 4, (c[0].b + c[1].b + c[2].b + c[3].b) / 4};
  dag::Vector<uint8_t> mask(w * h);
  for (int i = 0; i < w * h; ++i)
  {
    const TexPixel32 &p = photo[i];
    mask[i] = alpha ? p.a >= 128 : abs(p.r - back[0]) > 8 || abs(p.g - back[1]) > 8 || abs(p.b - back[2]) > 8;
  }
  fill_holes(mask, w, h);
  fill_holes(model, w, h);
  TexPixel32 bg;
  put_color(bg, background);
  dag::Vector<TexPixel32> image(w * h, bg);
  uint32_t onlyModel = 0, onlyPhoto = 0, inPhoto = 0;
  for (int i = 0; i < w * h; ++i)
  {
    inPhoto += mask[i];
    onlyModel += model[i] && !mask[i];
    onlyPhoto += mask[i] && !model[i];
    TexPixel32 &o = image[i];
    if (model[i] != mask[i])
      o = model[i] ? pixel(220, 40, 40) : pixel(40, 90, 230);
    else if (mask[i])
    {
      const TexPixel32 &p = photo[i];
      o = pixel(255 - (255 - p.r) * 0.55f, 255 - (255 - p.g) * 0.55f, 255 - (255 - p.b) * 0.55f);
      // a mask pixel never reads as background
      if (o.u == bg.u)
        o.g ^= 1;
    }
  }
  dag::Vector<int> modelEdge, photoEdge;
  mask_outline(model, w, h, modelEdge);
  mask_outline(mask, w, h, photoEdge);
  const double mpp = 2.0 * cam.tanX / w, m2 = sqr(mpp);
  printf("ref diff: symmetric difference %.4f m2, %.1f%% of the photo mask (%.4f m2); ", (onlyModel + onlyPhoto) * m2,
    inPhoto ? 100.0 * (onlyModel + onlyPhoto) / inPhoto : 0.0, inPhoto * m2);
  if (modelEdge.empty() || photoEdge.empty())
    printf("the %s silhouette is missing\n", modelEdge.empty() ? "model" : "photo");
  else
  {
    int at[2];
    const double gap[2] = {farthest(modelEdge, photoEdge, w, h, at[0]), farthest(photoEdge, modelEdge, w, h, at[1])};
    const char *const way[2] = {"model to photo", "photo to model"};
    const int big = gap[1] > gap[0];
    auto place = [&](int i) {
      return Point2(cam.eye * cam.right + ((i % w + 0.5f) * 2 / w - 1) * cam.tanX,
        cam.eye * cam.up + (1 - (i / w + 0.5f) * 2 / h) * cam.tanY);
    };
    const Point2 bigAt = place(at[big]), otherAt = place(at[!big]);
    printf("largest outline gap %.4f m at (%.3f, %.3f), %s; %s %.4f m at (%.3f, %.3f)\n", gap[big] * mpp, bigAt.x, bigAt.y, way[big],
      way[!big], gap[!big] * mpp, otherAt.x, otherAt.y);
    draw_ring(image, w, h, at[0], pixel(150, 0, 0));
    draw_ring(image, w, h, at[1], pixel(0, 20, 160));
  }
  if (save_png32(path, image.data(), w, h, w * sizeof(TexPixel32), nullptr, 0, false))
    return true;
  err.sprintf("cannot write the ref diff %s", path);
  return false;
}

bool render_png(const Mesh &m, dag::ConstSpan<Point3> diffuse, const RenderView &view, const RenderOverlay &overlay, int &width,
  int &height, const char *path, eastl::string &err)
{
  dag::Vector<TexPixel32> ref;
  int refW = 0, refH = 0;
  bool refAlpha = false;
  if (overlay.ref)
  {
    TexImage32 *img = load_picture(overlay.ref, "the image", &refAlpha, err);
    if (!img)
      return false;
    refW = img->w;
    refH = img->h;
    ref.assign(img->getPixels(), img->getPixels() + refW * refH);
    tmpmem->free(img);
    refAlpha = refAlpha && eastl::any_of(ref.begin(), ref.end(), [](const TexPixel32 &p) { return p.a < 255; });
    if (!width)
    {
      width = refW;
      height = refH;
    }
    else if (fabs(height - double(width) * refH / refW) > 1)
    {
      err.sprintf("the size %dx%d stretches the %dx%d image: keep its aspect to a pixel", width, height, refW, refH);
      return false;
    }
  }
  Camera cam;
  make_camera(m, view, width, height, cam);
  Environment env;
  cam.env = &env;
  cam.groundLit = mul(GROUND, SKY * AMBIENT + SUN_COLOR * (SUN * cam.toSun.y));
  if (overlay.env && !load_environment(overlay.env, overlay.envYaw, env, cam, err))
    return false;
  Target t;
  const int ss = overlay.supersample;
  // the target is indexed by int
  if (int64_t(width) * height * ss * ss > INT_MAX)
  {
    err.sprintf("%dx%d at supersample %d is %lld target pixels, more than %d", width, height, ss,
      (long long)(int64_t(width) * height * ss * ss), INT_MAX);
    return false;
  }
  t.w = width * ss;
  t.h = height * ss;
  t.color.assign(t.w * t.h, Point3(0, 0, 0));
  t.depth.assign(t.w * t.h, 0.f);

  const bool colors = m.cface.size() == m.face.size();
  for (uint32_t f = 0; f < m.face.size(); ++f)
  {
    Vertex corners[3], clipped[4];
    for (uint32_t k = 0; k < 3; ++k)
    {
      const Color4 color = colors ? m.cvert[m.cface[f].t[k]] : Color4();
      corners[k].view = to_view(cam, m.vert[m.face[f].v[k]]);
      corners[k].color = colors ? Point3(color.r, color.g, color.b) : DEFAULT_COLOR;
      corners[k].normal = m.vertnorm[m.facengr[f][k]];
    }
    const int n = clip_near(corners, cam.zNear, clipped);
    ScreenVertex s[4];
    for (int i = 0; i < n; ++i)
      s[i] = project(clipped[i], cam, t.w, t.h);
    const Material &mat = MATERIALS[m.face[f].mat < SHADE_COUNT ? m.face[f].mat : SHADE_PAINT];
    const Point3 faceDiffuse = f < diffuse.size() ? diffuse[f] : Point3(1, 1, 1);
    for (int i = 1; i + 1 < n; ++i)
      raster(t, cam, mat, faceDiffuse, s[0], s[i], s[i + 1]);
  }
  dag::Vector<uint8_t> coverage;
  dag::Vector<TexPixel32> photo;
  for (int i = 0; overlay.refDiff && !ref.empty() && i < width * height; ++i)
  {
    const int x = i % width, y = i / width;
    int covered = 0;
    for (int k = 0; k < ss * ss; ++k)
      covered += t.depth[(y * ss + k / ss) * t.w + x * ss + k % ss] > 0;
    coverage.push_back(2 * covered >= ss * ss);
    photo.push_back(ref_pixel(ref, refW, refH, x, y, width, height));
  }
  const Point3 &bgSrgb = overlay.background;
  const Point3 bg(srgb_to_linear(bgSrgb.x), srgb_to_linear(bgSrgb.y), srgb_to_linear(bgSrgb.z));
  if (!coverage.empty() && !write_ref_diff(overlay.refDiff, coverage, photo, refAlpha, cam, width, height, bg, err))
    return false;
  if (!overlay.ref)
    draw_ground(t, cam, make_span_const(ground_shadow(m, cam, width, height)), width, bg);
  if (overlay.grid)
    draw_grid(t, cam, m);

  dag::Vector<TexPixel32> image(width * height);
  const float norm = 1.f / (ss * ss), alpha = ref.empty() ? 1.f : overlay.refAlpha;
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
    {
      Point3 sum(0, 0, 0);
      float covered = 0;
      for (int sy = 0; sy < ss; ++sy)
        for (int sx = 0; sx < ss; ++sx)
        {
          const int i = (y * ss + sy) * t.w + x * ss + sx;
          sum += Point3(tone(t.color[i].x), tone(t.color[i].y), tone(t.color[i].z));
          covered += t.depth[i] > 0 ? norm : 0;
        }
      // a photo pixel lies over -bg by its own alpha
      Point3 back = bg;
      if (!ref.empty())
      {
        const TexPixel32 &p = photo.empty() ? ref_pixel(ref, refW, refH, x, y, width, height) : photo[y * width + x];
        back = p.a == 255 ? linear_color(p) : bg + (linear_color(p) - bg) * (p.a / 255.f);
      }
      put_color(image[y * width + x], sum * (norm * alpha) + back * (1 - covered * alpha));
    }
  for (const RenderDot &d : overlay.dots)
  {
    const Vertex v{to_view(cam, d.pos), d.color, d.color};
    if (v.view.z < cam.zNear)
      continue;
    const ScreenVertex s = project(v, cam, width, height);
    for (int i = 0; i < 9; ++i)
    {
      const float x = s.x + i % 3 - 1, y = s.y + i / 3 - 1;
      if (x >= 0 && y >= 0 && x < width && y < height)
        put_color(image[int(y) * width + int(x)], d.color);
    }
  }
  if (!save_png32(path, image.data(), width, height, width * sizeof(TexPixel32), nullptr, 0, false))
  {
    err = "save_png32 failed";
    return false;
  }
  return true;
}
