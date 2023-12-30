#include <libTools/renderUtil/dynRenderBuf.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_DynamicShadersBuffer.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <3d/dag_materialData.h>
#include <gui/dag_stdGuiRender.h>

#include <debug/dag_debug.h>


#define INC_SH_VERTEX  4096
#define INC_SH_FACES   4096
#define MAX_VERT_COUNT 0xFFFFFF
#define MAX_FACE_COUNT (MAX_VERT_COUNT / 3)


DynRenderBuffer::DynRenderBuffer(const char *class_name) : edVerts(midmem), edFaces(midmem), vertexMaxCount(2048), faceMaxCount(1024)
{
  channels[0].t = SCTYPE_FLOAT3;
  channels[0].vbu = SCUSAGE_POS;
  channels[0].vbui = 0;
  channels[0].streamId = 0;

  channels[1].t = SCTYPE_E3DCOLOR;
  channels[1].vbu = SCUSAGE_VCOL;
  channels[1].vbui = 0;
  channels[1].streamId = 0;

  channels[2].t = SCTYPE_FLOAT2;
  channels[2].vbu = SCUSAGE_TC;
  channels[2].vbui = 0;
  channels[2].streamId = 0;

  Ptr<MaterialData> matNull = new (tmpmem) MaterialData;
  matNull->className = class_name;
  edMat = new_shader_material(*matNull);
  matNull = NULL;
  if (!edMat)
  {
    texVarId = -1;
    edBuffer = NULL;
    logerr("failed to get shader <%s>", class_name);
    return;
  }

  texVarId = get_shader_variable_id("tex");

  if (!edMat->checkChannels(channels, sizeof(channels) / sizeof(channels[0])))
    DAG_FATAL("Invalid channels for shader '%s'!", (char *)edMat->getShaderClassName());

  edShader = edMat->make_elem();
  G_ASSERT(edShader);

  createBuffer(vertexMaxCount, faceMaxCount);
  edMat->set_texture_param(texVarId, BAD_TEXTUREID);
}

void DynRenderBuffer::createBuffer(int max_verts, int max_faces)
{
  if (!edMat)
    return;
  edBuffer = new DynamicShadersBuffer(channels, sizeof(channels) / sizeof(channels[0]), max_verts, max_faces);

  edBuffer->setCurrentShader(edShader);
}

void DynRenderBuffer::clearBuf()
{
  edVerts.clear();
  edFaces.clear();
}


void DynRenderBuffer::drawCustom(dag::ConstSpan<Vertex> vertex_tab, dag::ConstSpan<int> face_tab)
{
  append_items(edVerts, vertex_tab.size(), vertex_tab.data());
  append_items(edFaces, face_tab.size(), face_tab.data());
}

DynRenderBuffer::Vertex *DynRenderBuffer::drawNetSurface(int w, int h)
{
  G_ASSERT(w > 1 && h > 1);
  int vi = append_items(edVerts, w * h);
  int ii = append_items(edFaces, (w - 1) * (h - 1) * 2 * 3);

  for (int y = 1; y < h; y++)
    for (int x = 1; x < w; x++, ii += 6)
    {
      int bi = vi + y * w + x;
      edFaces[ii + 0] = bi - 1;
      edFaces[ii + 1] = bi - w - 1;
      edFaces[ii + 2] = bi - w;
      edFaces[ii + 3] = bi - 1;
      edFaces[ii + 4] = bi - w;
      edFaces[ii + 5] = bi;
    }
  return &edVerts[vi];
}

void DynRenderBuffer::drawQuad(const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3, E3DCOLOR color, float u,
  float v)
{
  int pos = append_items(edVerts, 4);
  int ipos = append_items(edFaces, 6);

  edVerts[pos].color = color;
  edVerts[pos].pos = p0;
  edVerts[pos].tc = Point2(0, 0);

  edVerts[pos + 1].color = color;
  edVerts[pos + 1].pos = p1;
  edVerts[pos + 1].tc = Point2(0, v);

  edVerts[pos + 2].color = color;
  edVerts[pos + 2].pos = p2;
  edVerts[pos + 2].tc = Point2(u, v);

  edVerts[pos + 3].color = color;
  edVerts[pos + 3].pos = p3;
  edVerts[pos + 3].tc = Point2(u, 0);

  edFaces[ipos] = pos + 0;
  edFaces[ipos + 1] = pos + 1;
  edFaces[ipos + 2] = pos + 2;
  edFaces[ipos + 3] = pos + 0;
  edFaces[ipos + 4] = pos + 2;
  edFaces[ipos + 5] = pos + 3;
}

void DynRenderBuffer::drawLine(const Point3 &from, const Point3 &to, real width, E3DCOLOR color)
{
  const Point3 v = Point3(::grs_cur_view.tm.m[0][2], ::grs_cur_view.tm.m[1][2], ::grs_cur_view.tm.m[2][2]);
  const Point3 halfWidth = normalize((to - from) % v) * width;

  const Point3 lt = to - halfWidth;
  const Point3 rt = to + halfWidth;
  const Point3 lb = from - halfWidth;
  const Point3 rb = from + halfWidth;

  drawQuad(lt, rt, rb, lb, color);
  drawQuad(lt, lb, rb, rt, color);
}

void DynRenderBuffer::drawSquare(const Point3 &p, real radius, E3DCOLOR color)
{
  TMatrix vtm;
  d3d::getm2vtm(vtm);
  vtm = inverse(vtm);

  const Point3 rv = vtm % Point3(radius, 0, 0);
  const Point3 tv = vtm % Point3(0, radius, 0);
  const Point3 lt = p - rv + tv;
  const Point3 rt = p + rv + tv;
  const Point3 lb = p - rv - tv;
  const Point3 rb = p + rv - tv;

  drawQuad(lt, rt, rb, lb, color);
}

#define SPHERE_SEGS 24
void DynRenderBuffer::drawWireSphere(const Point3 &pos, real radius, real width, E3DCOLOR color)
{
  if (radius < 0)
    return;

  Tab<Point3> p(tmpmem);
  p.resize(SPHERE_SEGS * SPHERE_SEGS * 2);
  const real PSI_STEP = PI / SPHERE_SEGS;
  const real FI_STEP = 2 * PI / SPHERE_SEGS;
  for (int psi = 0; psi < SPHERE_SEGS; psi++)
  {
    for (int fi = 0; fi < SPHERE_SEGS * 2; fi++)
    {
      p[SPHERE_SEGS * psi + fi] = Point3(radius * cos(psi * PSI_STEP - PI / 2) * cos(fi * FI_STEP),
                                    radius * cos(psi * PSI_STEP - PI / 2) * sin(fi * FI_STEP), radius * sin(psi * PSI_STEP - PI / 2)) +
                                  pos;
    }
  }

  for (int psi = 0; psi < SPHERE_SEGS - 1; psi++)
    for (int fi = 0; fi < SPHERE_SEGS * 2 - 1; fi++)
    {
      drawLine(p[SPHERE_SEGS * psi + fi], p[SPHERE_SEGS * psi + fi + 1], width, color);
      drawLine(p[SPHERE_SEGS * psi + fi], p[SPHERE_SEGS * (psi + 1) + fi], width, color);
    }
  clear_and_shrink(p);
  return;
}

void DynRenderBuffer::drawDebugSphere(const Point3 &c, real radius, real width, E3DCOLOR color)
{
  if (radius < 0)
    return;

  int i;
  real an;
  static Point3 v[SPHERE_SEGS + 1];
  for (an = 0, i = 0; i <= SPHERE_SEGS; ++i, an += TWOPI / SPHERE_SEGS)
  {
    v[i] = Point3(radius * cosf(an) + c.x, c.y, radius * sinf(an) + c.z);
    if (i > 0)
      drawLine(v[i], v[i - 1], width, color);
  }
  for (an = 0, i = 0; i <= SPHERE_SEGS; ++i, an += TWOPI / SPHERE_SEGS)
  {
    v[i] = Point3(c.x, radius * cosf(an) + c.y, radius * sinf(an) + c.z);
    if (i > 0)
      drawLine(v[i], v[i - 1], width, color);
  }
  for (an = 0, i = 0; i <= SPHERE_SEGS; ++i, an += TWOPI / SPHERE_SEGS)
  {
    v[i] = Point3(radius * cosf(an) + c.x, radius * sinf(an) + c.y, c.z);
    if (i > 0)
      drawLine(v[i], v[i - 1], width, color);
  }
  v[0] = c - Point3(radius, 0, 0);
  v[1] = c + Point3(radius, 0, 0);
  drawLine(v[0], v[1], width, color);
  v[0] = c - Point3(0, radius, 0);
  v[1] = c + Point3(0, radius, 0);
  drawLine(v[0], v[1], width, color);
  v[0] = c - Point3(0, 0, radius);
  v[1] = c + Point3(0, 0, radius);
  drawLine(v[0], v[1], width, color);
}

void DynRenderBuffer::drawSphere(const TMatrix &tm, E3DCOLOR color, int segs)
{
  if (segs == -1)
    segs = SPHERE_SEGS;
  Tab<Point3> p(tmpmem);
  p.resize(segs * segs * 2);
  const real PSI_STEP = PI / segs;
  const real FI_STEP = 2 * PI / segs;
  for (int psi = 0; psi < segs; psi++)
  {
    for (int fi = 0; fi < segs * 2; fi++)
    {
      p[segs * psi + fi] = tm * Point3(cos(psi * PSI_STEP - PI / 2) * cos(fi * FI_STEP),
                                  cos(psi * PSI_STEP - PI / 2) * sin(fi * FI_STEP), sin(psi * PSI_STEP - PI / 2));
    }
  }
  for (int psi = 0; psi < segs - 1; psi++)
    for (int fi = 0; fi < segs * 2 - 1; fi++)
      drawQuad(p[segs * psi + fi], p[segs * psi + fi + 1], p[segs * (psi + 1) + fi + 1], p[segs * (psi + 1) + fi], color);
  clear_and_shrink(p);
}

void DynRenderBuffer::drawWireBox(const TMatrix &tm, real width, E3DCOLOR color)
{
  Point3 p[8];
  p[0] = tm * Point3(-1, -1, -1);
  p[1] = tm * Point3(-1, -1, 1);
  p[2] = tm * Point3(-1, 1, -1);
  p[3] = tm * Point3(-1, 1, 1);
  p[4] = tm * Point3(1, -1, -1);
  p[5] = tm * Point3(1, -1, 1);
  p[6] = tm * Point3(1, 1, -1);
  p[7] = tm * Point3(1, 1, 1);
  drawLine(p[0], p[1], width, color);
  drawLine(p[1], p[3], width, color);
  drawLine(p[3], p[2], width, color);
  drawLine(p[2], p[0], width, color);
  drawLine(p[4], p[5], width, color);
  drawLine(p[5], p[7], width, color);
  drawLine(p[7], p[6], width, color);
  drawLine(p[6], p[4], width, color);
  drawLine(p[0], p[4], width, color);
  drawLine(p[1], p[5], width, color);
  drawLine(p[2], p[6], width, color);
  drawLine(p[3], p[7], width, color);
}


void DynRenderBuffer::drawBox(const TMatrix &tm, E3DCOLOR color)
{
  Point3 p[8];
  p[0] = tm * Point3(-1, -1, -1);
  p[1] = tm * Point3(-1, -1, 1);
  p[2] = tm * Point3(-1, 1, -1);
  p[3] = tm * Point3(-1, 1, 1);
  p[4] = tm * Point3(1, -1, -1);
  p[5] = tm * Point3(1, -1, 1);
  p[6] = tm * Point3(1, 1, -1);
  p[7] = tm * Point3(1, 1, 1);
  drawQuad(p[0], p[1], p[3], p[2], color);
  drawQuad(p[4], p[5], p[7], p[6], color);
  drawQuad(p[0], p[1], p[5], p[4], color);
  drawQuad(p[3], p[2], p[6], p[7], color);
  drawQuad(p[0], p[4], p[6], p[2], color);
  drawQuad(p[1], p[5], p[7], p[3], color);
}


void DynRenderBuffer::flushToBuffer(TEXTUREID tid) { addFaces(tid); }


void DynRenderBuffer::addFaces(TEXTUREID tid)
{
  const int faceCnt = edFaces.size() / 3;
  if (!edBuffer)
    return;

  if (edVerts.size() > vertexMaxCount || faceCnt > faceMaxCount)
  {
    del_it(edBuffer);

    while (edVerts.size() > vertexMaxCount || faceCnt > faceMaxCount)
    {
      if (edVerts.size() > vertexMaxCount)
      {
        vertexMaxCount += INC_SH_VERTEX;
        if (vertexMaxCount > MAX_VERT_COUNT)
          vertexMaxCount = edVerts.size();
      }

      if (faceCnt > faceMaxCount)
      {
        faceMaxCount += INC_SH_FACES;
        if (faceMaxCount > MAX_FACE_COUNT)
          faceMaxCount = faceCnt;
      }
    }

    G_ASSERTF(vertexMaxCount <= MAX_VERT_COUNT && faceMaxCount <= MAX_FACE_COUNT,
      "vertexMaxCount=%d (MAX=%d) faceMaxCount=%d (MAX=%d)", vertexMaxCount, MAX_VERT_COUNT, faceMaxCount, MAX_FACE_COUNT);

    createBuffer(vertexMaxCount, faceMaxCount);
  }

  G_ASSERT(edMat);

  edMat->set_texture_param(texVarId, tid);
  if (edFaces.size() >= 3)
  {
    edBuffer->addFaces(&edVerts[0], edVerts.size(), &edFaces[0], edFaces.size() / 3);
    edBuffer->flush();
  }
}


void DynRenderBuffer::flush()
{
  if (edBuffer)
    edBuffer->flush();
}
