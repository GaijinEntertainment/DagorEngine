#include <libTools/ObjCreator3d/objCreator3d.h>

#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

#include <math/dag_math3d.h>
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>
#include <math/dag_bezier.h>


unsigned ObjCreator3d::sphereIdx = 0;
unsigned ObjCreator3d::planeIdx = 0;
unsigned ObjCreator3d::boxIdx = 0;


//==============================================================================
bool ObjCreator3d::addNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom)
{
  if (!mesh)
    return false;

  Node node;
  node.obj = new (tmpmem) MeshHolderObj(mesh);
  node.wtm = TMatrix::IDENT;
  node.flags = NODEFLG_RENDERABLE;
  node.mat = material;
  node.name = str_dup(name, strmem);

  dag::Span<StaticGeometryNode *> nodes = geom.getNodes();

  TMatrix matrix = TMatrix::IDENT;
  if (nodes.size())
    matrix = nodes[0]->wtm;

  geom.clear();
  geom.addNode(node);

  if (geom.nodes.size())
  {
    StaticGeometryNode *snode = geom.nodes[0];
    snode->flags = StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;
    snode->calcBoundBox();
    snode->calcBoundSphere();
  }

  dag::Span<StaticGeometryNode *> newNodes = geom.getNodes();

  if (newNodes.size())
    newNodes[0]->wtm = matrix;

  node.mat = NULL;

  return true;
}


//==============================================================================
bool ObjCreator3d::addNode(const char *name, Mesh *mesh, StaticGeometryMaterial *material, StaticGeometryContainer &geom)
{
  if (!mesh)
    return false;

  StaticGeometryMesh *sgMesh = new (midmem) StaticGeometryMesh(midmem);

  if (!sgMesh)
    return false;

  sgMesh->mesh = *mesh;
  sgMesh->mats.push_back(material);

  StaticGeometryNode *node = new (midmem) StaticGeometryNode;

  if (!node)
  {
    sgMesh->delRef();
    return false;
  }

  node->mesh = sgMesh;
  node->name = name;
  node->flags = StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;

  node->calcBoundBox();
  node->calcBoundSphere();

  dag::Span<StaticGeometryNode *> nodes = geom.getNodes();

  TMatrix matrix = TMatrix::IDENT;
  if (nodes.size())
    matrix = nodes[0]->wtm;

  node->wtm = matrix;

  geom.clear();
  geom.addNode(node);

  return true;
}


//==============================================================================
bool ObjCreator3d::generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generatePlane(cell_size, geom, &mat);
}


//==============================================================================
bool ObjCreator3d::generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, MaterialDataList *material)
{
  Mesh *meshPlane = generatePlaneMesh(cell_size);

  if (!meshPlane)
    return false;

  if (addNode(String(32, "Plane_%u", planeIdx++), meshPlane, material, geom))
    return true;
  else
  {
    del_it(meshPlane);
    return false;
  }
}


//==============================================================================
bool ObjCreator3d::generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material)
{
  Mesh *meshPlane = generatePlaneMesh(cell_size);

  if (!meshPlane)
    return false;

  if (addNode(String(32, "Plane_%u", planeIdx++), meshPlane, material, geom))
    return true;
  else
  {
    del_it(meshPlane);
    return false;
  }
}


//==============================================================================
bool ObjCreator3d::generateSphere(int segments, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateSphere(segments, geom, &mat);
}


//==============================================================================
bool ObjCreator3d::generateSphere(int segments, StaticGeometryContainer &geom, MaterialDataList *material)
{
  Mesh *mesh = generateSphereMesh(segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Sphere_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


//==============================================================================
bool ObjCreator3d::generateSphere(int segments, StaticGeometryContainer &geom, StaticGeometryMaterial *material, Color4 col)
{
  Mesh *mesh = generateSphereMesh(segments, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "Sphere_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


//==============================================================================
bool ObjCreator3d::generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateBox(segments_count, geom, &mat);
}


//==============================================================================
bool ObjCreator3d::generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom, MaterialDataList *material)
{
  Mesh *mesh = generateBoxMesh(segments_count);

  if (!mesh)
    return false;

  if (addNode(String(32, "Box_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


//==============================================================================
bool ObjCreator3d::generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
  Color4 col)
{
  Mesh *mesh = generateBoxMesh(segments_count, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "Box_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

//==============================================================================

bool ObjCreator3d::generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateCylinder(sides, height_segments, cap_segments, geom, &mat);
}

bool ObjCreator3d::generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom,
  MaterialDataList *material)
{
  Mesh *mesh = generateCylinderMesh(sides, height_segments, cap_segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Cylinder_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

bool ObjCreator3d::generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom,
  StaticGeometryMaterial *material)
{
  Mesh *mesh = generateCylinderMesh(sides, height_segments, cap_segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Cylinder_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

//==============================================================================

bool ObjCreator3d::generateCapsule(int sides, int segments, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateCapsule(sides, segments, geom, &mat);
}

bool ObjCreator3d::generateCapsule(int sides, int segments, StaticGeometryContainer &geom, MaterialDataList *material)
{
  Mesh *mesh = generateCapsuleMesh(sides, segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Capsule_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

bool ObjCreator3d::generateCapsule(int sides, int segments, StaticGeometryContainer &geom, StaticGeometryMaterial *material)
{
  Mesh *mesh = generateCapsuleMesh(sides, segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Capsule_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

//==============================================================================


bool ObjCreator3d::generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generatePolyMesh(points, height_segments, geom, &mat);
}

bool ObjCreator3d::generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom,
  MaterialDataList *material)
{
  Mesh *mesh = generatePolyMesh(points, height_segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Polymesh_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}

bool ObjCreator3d::generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom,
  StaticGeometryMaterial *material)
{
  Mesh *mesh = generatePolyMesh(points, height_segments);

  if (!mesh)
    return false;

  if (addNode(String(32, "Polymesh_%u", boxIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


//=======================================================================================================
bool ObjCreator3d::generateLoftObject(const Tab<Point3> &points, const Tab<Point3 *> &sections, int points_count, real step,
  StaticGeometryContainer &geom, bool closed)
{
  // generate spline
  Tab<Point3> path_points(tmpmem);
  for (int i = 0; i < points.size(); i++)
  {
    Point3 dir;
    if (i == 0)
      dir = points[i + 1] - points[i + 2];
    else if (i == points.size() - 1)
      dir = points[i - 1] - points[i - 2];
    else
      dir = points[i + 1] - points[i - 1];
    dir.normalize();

    path_points.push_back(points[i] + dir);
    path_points.push_back(points[i]);
  }

  BezierSpline3d path;
  path.calculate(path_points.data(), path_points.size(), false);

  return generateLoftObject(path, sections, points_count, step, geom, closed);
}


//=======================================================================================================
bool ObjCreator3d::generateLoftObject(BezierSpline3d &path, const Tab<Point3 *> &sections, int points_count, real step,
  StaticGeometryContainer &geom, bool closed)
{
  Mesh *mesh = generateLoftMesh(path, sections, points_count, step, closed);

  if (!mesh)
    return false;

  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  static unsigned loftIdx = 0;
  if (addNode(String(32, "Loft_%u", loftIdx++), mesh, &mat, geom))
    return true;

  del_it(mesh);
  return false;
}
