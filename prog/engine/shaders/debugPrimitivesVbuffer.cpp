#include <shaders/dag_DebugPrimitivesVbuffer.h>

#include <3d/dag_drv3d.h>

#include <EASTL/unique_ptr.h>
#include <generic/dag_sort.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_mathUtils.h>


DebugPrimitivesVbuffer::DebugPrimitivesVbuffer(const char *name) : name(name)
{
  tm = TMatrix::IDENT;
  const char *shader_name = "debug_shader";
  shaderMat = new_shader_material_by_name_optional(shader_name);
  if (!shaderMat)
  {
    logerr("can't create ShaderMaterial for '%s'", shader_name);
  }
  else
  {
    shaderElem = shaderMat->make_elem();
    if (!shaderElem)
      logerr("can't create ShaderElem for '%s'", shader_name);
  }
}


DebugPrimitivesVbuffer::~DebugPrimitivesVbuffer()
{
  invalidate();
  shaderElem = nullptr;
  shaderMat = nullptr;
}


int DebugPrimitivesVbuffer::addVertex(const Point3 &p0)
{
  Point3 p = tm * p0;

  auto entry = vertexMap.find(p);
  if (entry != vertexMap.end())
    return entry->second;

  int index = (int)vertexList.size();
  vertexMap[p] = index;
  vertexList.push_back(p);
  return index;
}


void DebugPrimitivesVbuffer::addLine(const Point3 &p0, const Point3 &p1, E3DCOLOR c)
{
  Line line = {};
  line.id0 = addVertex(p0);
  line.id1 = addVertex(p1);
  line.c = c;
  linesCache.push_back(line);
}

void DebugPrimitivesVbuffer::addSolidBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color)
{
  carray<Point3, 8> boxVertices;
  boxVertices[0] = p;
  boxVertices[1] = p + ax;
  boxVertices[2] = p + ay;
  boxVertices[3] = p + ax + ay;

  boxVertices[4] = p + az;
  boxVertices[5] = p + az + ax;
  boxVertices[6] = p + az + ay;
  boxVertices[7] = p + az + ax + ay;

  carray<int, 8> boxIndices;
  for (int i = 0; i < boxIndices.size(); ++i)
    boxIndices[i] = addVertex(boxVertices[i]);

  trianglesCache.push_back({boxIndices[0], boxIndices[3], boxIndices[1], color});
  trianglesCache.push_back({boxIndices[0], boxIndices[2], boxIndices[3], color});

  trianglesCache.push_back({boxIndices[1], boxIndices[7], boxIndices[5], color});
  trianglesCache.push_back({boxIndices[1], boxIndices[3], boxIndices[7], color});

  trianglesCache.push_back({boxIndices[2], boxIndices[7], boxIndices[3], color});
  trianglesCache.push_back({boxIndices[2], boxIndices[6], boxIndices[7], color});

  trianglesCache.push_back({boxIndices[4], boxIndices[1], boxIndices[5], color});
  trianglesCache.push_back({boxIndices[4], boxIndices[0], boxIndices[1], color});

  trianglesCache.push_back({boxIndices[5], boxIndices[6], boxIndices[4], color});
  trianglesCache.push_back({boxIndices[5], boxIndices[7], boxIndices[6], color});

  trianglesCache.push_back({boxIndices[6], boxIndices[0], boxIndices[4], color});
  trianglesCache.push_back({boxIndices[6], boxIndices[2], boxIndices[0], color});
}

void DebugPrimitivesVbuffer::addTriangle(const Point3 p[3], E3DCOLOR color)
{
  Triangle tri = {};
  tri.id0 = addVertex(p[0]);
  tri.id1 = addVertex(p[1]);
  tri.id2 = addVertex(p[2]);
  tri.c = color;
  trianglesCache.push_back(tri);
}

void DebugPrimitivesVbuffer::addBox(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color)
{
  addLine(p0, p0 + ax, color);
  addLine(p0, p0 + ay, color);
  addLine(p0, p0 + az, color);
  addLine(p0 + ax + ay, p0 + ax, color);
  addLine(p0 + ax + ay, p0 + ay, color);
  addLine(p0 + ax + ay, p0 + ax + ay + az, color);
  addLine(p0 + ax + az, p0 + ax, color);
  addLine(p0 + ax + az, p0 + az, color);
  addLine(p0 + ax + az, p0 + ax + ay + az, color);
  addLine(p0 + ay + az, p0 + ay, color);
  addLine(p0 + ay + az, p0 + az, color);
  addLine(p0 + ay + az, p0 + ax + ay + az, color);
}


void DebugPrimitivesVbuffer::addBox(const BBox3 &box, E3DCOLOR color)
{
  Point3 a = box.width();
  addBox(box.lim[0], Point3(a.x, 0, 0), Point3(0, a.y, 0), Point3(0, 0, a.z), color);
}


void DebugPrimitivesVbuffer::invalidate()
{
  vertexMap.clear();
  clear_and_shrink(vertexList);
  clear_and_shrink(linesCache);
  clear_and_shrink(trianglesCache);
  clear_and_shrink(linesPasses);
  clear_and_shrink(trianglesPasses);
  del_d3dres(vbuffer);
  del_d3dres(ibuffer);
  tm = TMatrix::IDENT;
}


void DebugPrimitivesVbuffer::beginCache() { invalidate(); }

template <typename T>
static int compare_primitive_by_color(const T *a, const T *b)
{
  return sign(((int64_t)a->c.u - (int64_t)b->c.u));
}


void DebugPrimitivesVbuffer::endCache()
{
  G_ASSERT(vbuffer == nullptr);
  G_ASSERT(ibuffer == nullptr);

  if (linesCache.size() == 0 && trianglesCache.size() == 0)
    return;

  sort(linesCache, &compare_primitive_by_color<Line>);
  sort(trianglesCache, &compare_primitive_by_color<Triangle>);

  clear_and_shrink(linesPasses);
  clear_and_shrink(trianglesPasses);
  Tab<int> indices;
  indices.reserve(2 * linesCache.size() + 3 * trianglesCache.size());

  Pass new_pass;
  for (int i = 0; i < linesCache.size(); i++)
  {
    indices.push_back(linesCache[i].id0);
    indices.push_back(linesCache[i].id1);
    if (i == 0 || linesCache[i - 1].c.u != linesCache[i].c.u)
    {
      if (i > 0)
      {
        new_pass.lastPrimitive = i;
        linesPasses.push_back(new_pass);
      }

      new_pass.color = linesCache[i].c;
      new_pass.firstPrimitive = i;
    }
  }
  if (linesCache.size())
  {
    new_pass.lastPrimitive = (int)linesCache.size();
    linesPasses.push_back(new_pass);
  }

  trianglesStartIndex = indices.size();
  for (int i = 0; i < trianglesCache.size(); i++)
  {
    indices.push_back(trianglesCache[i].id0);
    indices.push_back(trianglesCache[i].id1);
    indices.push_back(trianglesCache[i].id2);
    if (i == 0 || trianglesCache[i - 1].c.u != trianglesCache[i].c.u)
    {
      if (i > 0)
      {
        new_pass.lastPrimitive = i;
        trianglesPasses.push_back(new_pass);
      }

      new_pass.color = trianglesCache[i].c;
      new_pass.firstPrimitive = i;
    }
  }
  if (trianglesCache.size())
  {
    new_pass.lastPrimitive = (int)trianglesCache.size();
    trianglesPasses.push_back(new_pass);
  }

  const size_t vbuffer_size = vertexList.size() * sizeof(Point3);
  vbuffer = d3d::create_vb((int)(vbuffer_size), SBCF_MAYBELOST, name);
  vbuffer->updateData(0, (uint32_t)vbuffer_size, (void *)vertexList.data(), 0);

  String ibName = String(0, "%s_ib", name);
  const size_t ibuffer_size = indices.size() * sizeof(int);
  ibuffer = d3d::create_ib((int)ibuffer_size, SBCF_INDEX32 | SBCF_MAYBELOST, ibName);
  ibuffer->updateData(0, (uint32_t)ibuffer_size, (void *)indices.data(), 0);

  clear_and_shrink(linesCache);
}

void DebugPrimitivesVbuffer::renderEx(bool z_test, bool z_write, bool z_func_less, Color4 color_multiplier)
{
  if (shaderElem == nullptr || vbuffer == nullptr || ibuffer == nullptr)
    return;

  renderStates.setState(debug3d::StateKey(z_test, z_write, z_func_less ? debug3d::ZFUNC_LESS : debug3d::ZFUNC_GREATER_EQUAL));

  d3d::setvsrc_ex(0, vbuffer, 0, sizeof(Point3));
  d3d::setind(ibuffer);

  TMatrix4_vec4 gtm;
  d3d::settm(TM_WORLD, &Matrix44::IDENT);
  d3d::getglobtm(gtm);

  process_tm_for_drv_consts(gtm);
  d3d::set_vs_const(0, gtm[0], 4);

  // draw triangles before lines to make lines visible over triangles
  for (Pass &pass : trianglesPasses)
  {
    Color4 color = Color4(pass.color) * color_multiplier;
    ShaderGlobal::set_color4(::get_shader_variable_id("debug_line_color"), color);
    shaderElem->setStates();

    const int startIndex = trianglesStartIndex + 3 * pass.firstPrimitive;
    const int numPrim = pass.lastPrimitive - pass.firstPrimitive;
    d3d::drawind(PRIM_TRILIST, startIndex, numPrim, 0);
  }
  for (Pass &pass : linesPasses)
  {
    Color4 color = Color4(pass.color) * color_multiplier;
    ShaderGlobal::set_color4(::get_shader_variable_id("debug_line_color"), color);
    shaderElem->setStates();

    const int startIndex = 2 * pass.firstPrimitive;
    const int numPrim = pass.lastPrimitive - pass.firstPrimitive;
    d3d::drawind(PRIM_LINELIST, startIndex, numPrim, 0);
  }
  renderStates.resetState();
}


void DebugPrimitivesVbuffer::renderOverrideColor(Color4 color_mult) { renderEx(true, false, false, color_mult); }

void DebugPrimitivesVbuffer::render() { renderEx(true, false, false, Color4(1.0, 1.0, 1.0, 1.0)); }

void DebugPrimitivesVbuffer::setTm(const TMatrix &tm) { this->tm = tm; }
