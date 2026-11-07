// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      "*"

#include "softRender.h"
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/strUtil.h>
#include <math/dag_check_nan.h>
#include <math/dag_mesh.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_mainCon.inc.cpp>
#include <startup/dag_startupTex.h>
#include <EASTL/string_view.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// a face's material look picks the shading by SHADE_ value
static const char *const SHADE_NAMES[] = {"paint", "chrome", "matte", "glass", "metallic"};

struct Pose
{
  String node;
  int axis = 0;
  float deg = 0;
};

struct Options
{
  String dag, png, ref, refDiff, env, dots;
  dag::Vector<Pose> poses;
  bool sizeGiven = false;
  RenderView view;
  RenderOverlay overlay;
  int width = 1024, height = 768;
};

static void print_usage()
{
  printf("usage: dagrender <in.dag> -png:<out.png> [-view:<yaw>,<pitch>,<dist>] [-size:<w>x<h>] [-ref:<image>[,<alpha>]]\n"
         "                 [-ortho:<width>[,<cx>,<cy>]] [-grid] [-bg:<r>,<g>,<b>] [-env:<image>[,<yaw>]] [-ss:<n>]\n"
         "                 [-ref-diff:<out.png>] [-dots:<file>] [-pose:<node>,<x|y|z>,<deg>] [-quiet]\n"
         "  every renderable mesh node of the dag with its transform; normals from the dag, else from the smoothing groups\n"
         "  -png        software render: sun, sky and ground light, gloss and reflection by the material's look, the last\n"
         "              look=<name> script line, else its name (paint, chrome, matte, glass, metallic; another or no name renders as\n"
         "              paint), the base color times the material's diffuse, a ground plane at y = 0 with the model's soft shadow\n"
         "  -view       camera in degrees and meters: yaw from +Z toward +X, pitch up, dist 0 puts the bbox\n"
         "              corners within 90%% of the image; default 40,25,0; -ortho and -ref ignore dist\n"
         "  -size       png size, default 1024x768; with -ref the image's size, or a size of its aspect to a pixel\n"
         "  -ref        png or jpeg image under the png, the render blended over it with alpha (default 0.5);\n"
         "              the projection is orthographic, no ground plane\n"
         "  -ref-diff   with -ref: the model's silhouette against the photo's (its alpha when any pixel is not\n"
         "              opaque, else what differs from its corners' color): a png, red model only, blue photo only,\n"
         "              the faded photo under both, the -bg color outside both, a ring at the largest gap each way;\n"
         "              a line with the symmetric difference in m2 and the largest outline gap each way in meters,\n"
         "              placed as -ortho's cx, cy\n"
         "  -ortho      orthographic: <width> meters across, centered at cx, cy along the view's right and up axes\n"
         "              (dot products with them, meters); default the bbox center and a width that fits the bbox;\n"
         "              yaw 90, pitch 0 is the side view from +X\n"
         "  -grid       0.1 m lines on the ground plane y = 0, for models under 500 m within 100 km of the origin\n"
         "  -bg         background, sRGB 0..1, default 0.85,0.85,0.85\n"
         "  -env        an equirectangular png or jpeg for the reflections and the ambient light; its brightest\n"
         "              spot above the horizon is the sun, its lower half's mean the ground color; yaw turns the sky\n"
         "              about +Y in degrees, +Z toward +X like -view, to put the sun where the light should come from\n"
         "  -ss         supersampling per axis, 1..8, default 2; 4 for final shots\n"
         "  -dots       a text file, one dot per line: x y z r g b (meters, linear color 0..1), drawn over the image\n"
         "  -pose       turn the named node and its children about the node's own x, y or z axis, in degrees by the\n"
         "              right-hand rule as csg rot does; repeat it for more nodes\n"
         "  -quiet      never open a dialog\n");
}

static const char *option_value(const char *arg, const char *name)
{
  const size_t len = strlen(name);
  return strncmp(arg, name, len) == 0 ? arg + len : nullptr;
}

// The three below advance s past what they read; a space or a '+' before a number is no number, a '-' is one where
// the range admits it
static bool take_float(const char *&s, float &out)
{
  char *end = nullptr;
  out = isdigit((unsigned char)*s) || *s == '.' || *s == '-' ? strtof(s, &end) : 0;
  const bool ok = end && end != s && check_finite(out);
  s = end ? end : s;
  return ok;
}

// decimal digits only, at most hi; a sign, a space or an overflow is no count
static bool take_count(const char *&s, int hi, int &out)
{
  char *end = nullptr;
  errno = 0;
  const unsigned long long n = isdigit((unsigned char)*s) ? strtoull(s, &end, 10) : 0;
  if (!end || errno == ERANGE || n > (unsigned long long)hi)
    return false;
  out = int(n);
  s = end;
  return true;
}

static bool take(const char *&s, char c) { return *s == c && (++s, true); }

static bool parse_float(const char *s, float &out) { return take_float(s, out) && !*s; }

// an empty path, or one too long to make absolute, is an error, not an option ignored
static bool parse_path(const char *arg, const char *v, String &out)
{
  out = *v ? make_path_absolute(v) : String();
  if (!out.empty())
    return true;
  printf("%s needs a path\n", arg);
  return false;
}

// nan and inf tails are no path
static bool parse_path_num(const char *arg, const char *v, String &path, float &num)
{
  const char *comma = strrchr(v, ',');
  char *end = nullptr;
  if (comma)
    strtof(comma + 1, &end);
  const bool tail = end && end != comma + 1 && !*end;
  return parse_path(arg, (tail ? String(v, int(comma - v)) : String(v)).str(), path) && (!tail || parse_float(comma + 1, num));
}

static bool parse_args(Options &o)
{
  for (int i = 1; i < dgs_argc; ++i)
  {
    const char *arg = dgs_argv[i], *v = nullptr;
    if ((v = option_value(arg, "-png:")) != nullptr)
    {
      if (!parse_path(arg, v, o.png))
        return false;
    }
    else if ((v = option_value(arg, "-view:")) != nullptr)
    {
      RenderView &w = o.view;
      const char *p = v;
      if (!take_float(p, w.yawDeg) || !take(p, ',') || !take_float(p, w.pitchDeg) || !take(p, ',') || !take_float(p, w.dist) || *p ||
          w.dist < 0)
      {
        printf("-view needs <yaw>,<pitch>,<dist> with dist 0 or more: %s\n", arg);
        return false;
      }
    }
    else if ((v = option_value(arg, "-size:")) != nullptr)
    {
      const char *p = v;
      if (!take_count(p, 8192, o.width) || !take(p, 'x') || !take_count(p, 8192, o.height) || *p || o.width < 1 || o.height < 1)
      {
        printf("-size needs <w>x<h> in 1..8192: %s\n", arg);
        return false;
      }
      o.sizeGiven = true;
    }
    else if ((v = option_value(arg, "-ref-diff:")) != nullptr)
    {
      if (!parse_path(arg, v, o.refDiff))
        return false;
    }
    else if ((v = option_value(arg, "-ref:")) != nullptr)
    {
      o.overlay.refAlpha = 0.5f;
      if (!parse_path_num(arg, v, o.ref, o.overlay.refAlpha) || !(o.overlay.refAlpha >= 0 && o.overlay.refAlpha <= 1))
      {
        printf("-ref needs <image>[,<alpha 0..1>]: %s\n", arg);
        return false;
      }
    }
    else if ((v = option_value(arg, "-ortho:")) != nullptr)
    {
      RenderView &w = o.view;
      const char *p = v;
      const bool width = take_float(p, w.orthoWidth) && w.orthoWidth > 0;
      w.centered = width && take(p, ',');
      if (!width || (w.centered && (!take_float(p, w.cx) || !take(p, ',') || !take_float(p, w.cy))) || *p)
      {
        printf("-ortho needs <width>[,<cx>,<cy>] with width above zero: %s\n", arg);
        return false;
      }
      w.ortho = true;
    }
    else if (strcmp(arg, "-grid") == 0)
      o.overlay.grid = true;
    else if ((v = option_value(arg, "-bg:")) != nullptr)
    {
      Point3 &c = o.overlay.background;
      const char *p = v;
      if (!take_float(p, c.x) || !take(p, ',') || !take_float(p, c.y) || !take(p, ',') || !take_float(p, c.z) || *p ||
          !(c.x >= 0 && c.x <= 1) || !(c.y >= 0 && c.y <= 1) || !(c.z >= 0 && c.z <= 1))
      {
        printf("-bg needs <r>,<g>,<b> in 0..1: %s\n", arg);
        return false;
      }
    }
    else if ((v = option_value(arg, "-env:")) != nullptr)
    {
      o.overlay.envYaw = 0;
      if (!parse_path_num(arg, v, o.env, o.overlay.envYaw))
      {
        printf("-env needs <image>[,<yaw>]: %s\n", arg);
        return false;
      }
    }
    else if ((v = option_value(arg, "-ss:")) != nullptr)
    {
      const char *p = v;
      if (!take_count(p, 8, o.overlay.supersample) || *p || o.overlay.supersample < 1)
      {
        printf("-ss needs a factor in 1..8: %s\n", arg);
        return false;
      }
    }
    else if ((v = option_value(arg, "-dots:")) != nullptr)
    {
      if (!parse_path(arg, v, o.dots))
        return false;
    }
    else if ((v = option_value(arg, "-pose:")) != nullptr)
    {
      const char *comma = strrchr(v, ',');
      const char *axis = comma && comma - v >= 3 && comma[-2] == ',' ? strchr("xyz", comma[-1]) : nullptr;
      Pose &p = o.poses.push_back();
      if (!axis || !parse_float(comma + 1, p.deg))
      {
        printf("-pose needs <node>,<x|y|z>,<deg>: %s\n", arg);
        return false;
      }
      p.node = String(v, int(comma - v - 2));
      p.axis = comma[-1] - 'x';
    }
    else if (strcmp(arg, "-quiet") == 0 || strcmp(arg, "-noeh") == 0 || strcmp(arg, "-debug") == 0)
      continue;
    else if (arg[0] == '-')
    {
      printf("unknown option %s\n", arg);
      return false;
    }
    else if (o.dag.empty())
    {
      if (!parse_path("the dag", arg, o.dag))
        return false;
    }
    else
    {
      printf("one dag only: %s\n", arg);
      return false;
    }
  }
  if (o.dag.empty() || o.png.empty())
  {
    print_usage();
    return false;
  }
  if (!o.refDiff.empty() && o.ref.empty())
  {
    printf("-ref-diff needs -ref\n");
    return false;
  }
  o.view.ortho |= !o.ref.empty();
  return true;
}

static eastl::string_view material_look(const MaterialData &md)
{
  eastl::string_view look = md.matName.str();
  const char *script = md.matScript.str();
  for (const char *s = strstr(script, "look="); s; s = strstr(s + 5, "look="))
    if (s == script || s[-1] == '\r' || s[-1] == '\n')
      look = eastl::string_view(s + 5, strcspn(s + 5, "\r\n"));
  return look;
}

static uint16_t shading(const MaterialData *md)
{
  const eastl::string_view look = md ? material_look(*md) : "";
  for (uint16_t i = 0; i < countof(SHADE_NAMES); ++i)
    if (look == SHADE_NAMES[i])
      return i;
  return SHADE_PAINT;
}

static void gray_colors(Mesh &m)
{
  m.cvert.assign(1, Color4(RENDER_GRAY, RENDER_GRAY, RENDER_GRAY, 1));
  m.cface.resize(m.face.size());
  for (TFace &t : m.cface)
    t.t[0] = t.t[1] = t.t[2] = 0;
}

// Every renderable mesh node in world space as one Mesh, face mat a SHADE_ value; per face its material's diffuse, 1 with none
static bool load_dag(const char *path, dag::ConstSpan<Pose> poses, Mesh &out, dag::Vector<Point3> &diffuse)
{
  AScene scene;
  if (!load_ascene(path, scene, LASF_MATNAMES | LASF_NULLMATS, false) || !scene.root)
  {
    printf("cannot read %s\n", path);
    return false;
  }
  for (const Pose &p : poses)
  {
    Node *n = scene.root->find_node(p.node);
    if (!n)
    {
      printf("-pose: %s has no node %s\n", path, p.node.str());
      return false;
    }
    const TMatrix turn = makeTM(point3(normalize(dpoint3(n->tm.getcol(p.axis)))), DegToRad(fmodf(p.deg, 360.f)));
    for (int c = 0; c < 3; ++c)
      n->tm.setcol(c, turn % n->tm.getcol(c));
  }
  scene.root->invalidate_wtm();
  scene.root->calc_wtm();
  dag::Vector<Node *> stack;
  stack.push_back(scene.root);
  while (!stack.empty())
  {
    Node *n = stack.back();
    stack.pop_back();
    for (Node *c : n->child)
      stack.push_back(c);
    const MeshHolderObj *holder = n->obj && n->obj->isSubOf(OCID_MESHHOLDER) ? static_cast<MeshHolderObj *>(n->obj) : nullptr;
    if (!(n->flags & NODEFLG_RENDERABLE) || !holder || !holder->mesh)
      continue;
    Mesh part = *holder->mesh;
    for (Face &f : part.face)
    {
      const MaterialData *md = n->mat ? n->mat->getSubMat(f.mat) : nullptr;
      diffuse.push_back(md ? Point3(md->mat.diff.r, md->mat.diff.g, md->mat.diff.b) : Point3(1, 1, 1));
      f.mat = shading(md);
    }
    if (part.facengr.size() != part.face.size() || part.vertnorm.empty())
    {
      part.calc_ngr();
      part.calc_vertnorms();
    }
    // an identity transform would still renormalize the stored normals
    if (n->wtm != TMatrix::IDENT)
      part.transform(n->wtm);
    // attach gives the faces of a mesh without colors the other one's first color
    if (out.face.size() && (out.cface.size() == out.face.size()) != (part.cface.size() == part.face.size()))
      gray_colors(out.cface.size() == out.face.size() ? part : out);
    if (out.face.empty())
      out = eastl::move(part);
    else
      out.attach(part);
  }
  if (!out.face.size())
  {
    printf("%s has no renderable mesh\n", path);
    return false;
  }
  return true;
}

// x y z r g b per line
static bool read_dots(const char *path, dag::Vector<RenderDot> &dots)
{
  FILE *f = fopen(path, "rt");
  RenderDot d = {};
  bool ok = f != nullptr;
  char line[512];
  while (ok && fgets(line, sizeof(line), f))
  {
    int end = 0;
    ok =
      sscanf(line, "%f %f %f %f %f %f %n", &d.pos.x, &d.pos.y, &d.pos.z, &d.color.x, &d.color.y, &d.color.z, &end) == 6 && !line[end];
    for (float v : {d.pos.x, d.pos.y, d.pos.z, d.color.x, d.color.y, d.color.z})
      ok &= check_finite(v);
    dots.push_back(d);
  }
  ok = ok && feof(f);
  if (f)
    fclose(f);
  if (!ok)
    printf("cannot read the dots %s: a line is not x y z r g b of finite numbers\n", path);
  return ok;
}

int DagorWinMain(bool)
{
  Options o;
  if (!parse_args(o))
    return 1;
  register_png_tex_load_factory();
  register_jpeg_tex_load_factory();
  Mesh mesh;
  dag::Vector<Point3> diffuse;
  dag::Vector<RenderDot> dots;
  if (!load_dag(o.dag, make_span_const(o.poses), mesh, diffuse) || (!o.dots.empty() && !read_dots(o.dots, dots)))
    return 1;
  for (const String *path : {&o.png, &o.refDiff})
    if (!path->empty())
      dd_mkpath(*path);
  o.overlay.ref = o.ref.empty() ? nullptr : o.ref.str();
  o.overlay.refDiff = o.refDiff.empty() ? nullptr : o.refDiff.str();
  o.overlay.env = o.env.empty() ? nullptr : o.env.str();
  o.overlay.dots = make_span_const(dots);
  int width = o.ref.empty() || o.sizeGiven ? o.width : 0, height = o.height;
  eastl::string err;
  if (!render_png(mesh, diffuse, o.view, o.overlay, width, height, o.png, err))
  {
    printf("cannot render %s: %s\n", o.png.str(), err.c_str());
    return 1;
  }
  printf("wrote .png %s\npng size %dx%d\n", o.png.str(), width, height);
  return 0;
}
