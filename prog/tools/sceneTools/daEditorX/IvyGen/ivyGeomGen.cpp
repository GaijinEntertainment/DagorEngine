#include "ivyObject.h"
#include <dllPluginCore/core.h>
#include <de3_interface.h>
#include <de3_assetService.h>
#include <assets/asset.h>
#include <oldEditor/de_interface.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <de3_lightService.h>
#include <debug/dag_debug.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>

#define MAX_OF(x, y) (x > y ? x : y)
#define MIN_OF(x, y) (x < y ? x : y)

class GenProgressCB : public IProgressCB
{
public:
  GenProgressCB(IvyObject *o) { obj = o; }

  virtual void onCancel() { obj->markFinishGeneration(); }

  IvyObject *obj;
};

static GenProgressCB *genCb = NULL;

static void getMaterialDataByName(const char *n, Ptr<MaterialData> &md)
{
  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (assetSrv)
    md = assetSrv->getMaterialData(n);

  if (n && n[0] && !md.get())
    DAEDITOR3.conError("can't find material %s", n);
}


static float vectorToPolar(const Point2 &vector)
{
  float phi = (fabs(vector.x) < 1e-3) ? 0.0f : atan(vector.y / vector.x);

  if (vector.x < 0.0f)
  {
    phi += PI;
  }
  else
  {
    if (vector.y < 0.0f)
    {
      phi += TWOPI;
    }
  }

  return phi;
}


static float getAngle(const Point3 &vector1, const Point3 &vector2)
{
  float length1 = vector1.length();
  float length2 = vector2.length();

  if ((fabs(length1) < 1e-3) || (fabs(length2) < 1e-3))
    return 0.0f;

  real acosval = (vector1 * vector2) / (length1 * length2);

  if (acosval >= 1)
    return 0;

  if (acosval <= -1)
    return PI;

  real ret = acos(acosval);
  return ret;
}


static Point3 getPoint3Randomized()
{
  return normalize(Point3(rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f, rand() / (float)RAND_MAX - 0.5f));
}


static Point2 Point2getEpsilon()
{
  // return Point2(std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::epsilon());
  return Point2(1, 1);
}


static Point3 rotateAroundAxis(const Point3 &vector, const Point3 &axisPosition, const Point3 &axis, const float &angle)
{
  // determining the sinus and cosinus of the rotation angle
  float cosTheta = cos(angle);
  float sinTheta = sin(angle);

  // Vector3 from the given axis point to the initial point
  Point3 direction = vector - axisPosition;

  // new vector which will hold the direction from the given axis point to the new rotated point
  Point3 newDirection;

  // x-component of the direction from the given axis point to the rotated point
  newDirection.x = (cosTheta + (1 - cosTheta) * axis.x * axis.x) * direction.x +
                   ((1 - cosTheta) * axis.x * axis.y - axis.z * sinTheta) * direction.y +
                   ((1 - cosTheta) * axis.x * axis.z + axis.y * sinTheta) * direction.z;

  // y-component of the direction from the given axis point to the rotated point
  newDirection.y = ((1 - cosTheta) * axis.x * axis.y + axis.z * sinTheta) * direction.x +
                   (cosTheta + (1 - cosTheta) * axis.y * axis.y) * direction.y +
                   ((1 - cosTheta) * axis.y * axis.z - axis.x * sinTheta) * direction.z;

  // z-component of the direction from the given axis point to the rotated point
  newDirection.z = ((1 - cosTheta) * axis.x * axis.z - axis.y * sinTheta) * direction.x +
                   ((1 - cosTheta) * axis.y * axis.z + axis.x * sinTheta) * direction.y +
                   (cosTheta + (1 - cosTheta) * axis.z * axis.z) * direction.z;

  // returning the result by addind the new direction vector to the given axis point
  return axisPosition + newDirection;
}

static Ptr<MaterialData> loadIvyMaterial(const DataBlock &blk, const char *matname)
{
  Ptr<MaterialData> md;
  const DataBlock *cb = blk.getBlockByName(matname);
  if (cb)
  {
    md = new MaterialData;
    md->className = cb->getStr("className", "");
    md->matScript = cb->getStr("script", "");
    md->matName = matname;
    md->flags = MATF_2SIDED;
    md->mat.power = 0;
    md->mat.amb = D3dColor(1, 1, 1, 1);
    md->mat.diff = D3dColor(1, 1, 1, 1);

    const DataBlock *texCb = cb->getBlockByName("textures");
    if (texCb)
    {
      int texId = 0;
      int nid = texCb->getNameId("texName");
      for (int i = 0; i < texCb->paramCount(); i++)
        if (texCb->getParamNameId(i) == nid && texCb->getParamType(i) == DataBlock::TYPE_STRING)
        {
          const char *texFName = texCb->getStr(i);
          if (texFName && *texFName)
          {
            String fullFn = ::make_full_path(DAGORED2->getSdkDir(), texFName);

            TEXTUREID textureId = dagRender->addManagedTexture(fullFn);
            md->mtex[texId++] = textureId;
          }
        }
    }

    // real tileU = cb->getReal("tileU", 0);

    return md;
  }

  int id = blk.findParam(matname);
  if (id != -1 && blk.getParamType(id) == DataBlock::TYPE_STRING)
  {
    const char *mat = blk.getStr(id);
    if (mat && *mat)
      getMaterialDataByName(mat, md);

    if (!md)
      DAEDITOR3.conError("Can't load material %s", matname);

    return md;
  }

  DAEDITOR3.conError("Can't load material %s", matname);
  return NULL;
}


void IvyObject::generateGeom(CoolConsole &con)
{
  if (!canGrow)
  {
    canGrow = true;
    seed();
  }

  while (growed < needToGrow)
    grow();

  // evolve a gaussian filter over the adhesian vectors
  float gaussian[11] = {1.0f, 2.0f, 4.0f, 7.0f, 9.0f, 10.0f, 9.0f, 7.0f, 4.0f, 2.0f, 1.0f};

  int nodesCnt = 0;

  for (int i = 0; i < roots.size(); i++)
  {
    nodesCnt += roots[i]->nodes.size();
    IvyRoot *root = roots[i];
    for (int g = 0; g < 5; ++g)
    {
      int j;
      for (j = 0; j < root->nodes.size(); j++)
      {
        Point3 e = Point3(0, 0, 0);

        for (int k = -5; k <= 5; ++k)
        {
          Point3 tmpAdhesion;

          if (j + k < 0)
            tmpAdhesion = root->nodes[0]->adhesionVector;
          if (j + k >= root->nodes.size() - 1)
            tmpAdhesion = root->nodes[root->nodes.size() - 1]->adhesionVector;
          if (j + k >= 0 && j + k < root->nodes.size() - 1)
            tmpAdhesion = root->nodes[j + k]->adhesionVector;

          e += tmpAdhesion * gaussian[k + 5];
        }

        root->nodes[j]->smoothAdhesionVector = e / 56.0f;
      }

      for (j = 0; j < root->nodes.size(); j++)
        root->nodes[j]->adhesionVector = root->nodes[j]->smoothAdhesionVector;
    }
  }

  // parameters that depend on the scene object bounding sphere
  float local_ivyLeafSize = size * ivyLeafSize;
  float local_ivyBranchSize = size * ivyBranchSize;


  // reset existing geometry
  dagGeom->geomObjectClear(*ivyGeom);
  trisCount = 0;

  DagorAsset *a = DAEDITOR3.getAssetByName(assetName, DAEDITOR3.getAssetTypeId("ivyClass"));
  if (!a)
    return;

  genCb = new GenProgressCB(this);
  con.startProgress(genCb);
  con.setActionDesc(String(512, "generating %s geometry...", getName()));

  Ptr<MaterialData> smStem = loadIvyMaterial(a->props, "stemMaterial");
  Ptr<MaterialData> smLeaves = loadIvyMaterial(a->props, "leavesMaterial");
  Ptr<MaterialData> smOldLeaves = loadIvyMaterial(a->props, "oldLeavesMaterial");

  real stemTileU = 0;
  // if (smStem)
  //   stemTileU = smStem->mpar[0];

  if (!smStem && !smOldLeaves && !smLeaves)
  {
    con.endProgress();
    return;
  }

  int total = (smLeaves && smOldLeaves) ? roots.size() : 0;
  if (smStem)
    total *= 2;

  con.setTotal(total);

  MaterialDataList *mdl = new MaterialDataList;

  int stemId = smStem ? mdl->addSubMat(smStem) : -1;
  int leavesId = smLeaves ? mdl->addSubMat(smLeaves) : -1;
  int oldLeavesId = smOldLeaves ? mdl->addSubMat(smOldLeaves) : -1;

  Mesh *mesh = dagGeom->newMesh();
  int seed = startSeed;

  // branches
  if (smStem)
    for (int rootId = 0; rootId < roots.size(); rootId++)
    {
      IvyRoot *root = roots[rootId];
      // process only roots with more than one node
      if (root->nodes.size() == 1)
        continue;

      // branch diameter depends on number of parents
      float local_ivyBranchDiameter = 1.0f / (float)(root->parents + 1) + 1.0f;

      for (int nodeId = 0; nodeId < (int)root->nodes.size() - 1; nodeId++)
      {
        if (finishGeneration)
          goto end;

        IvyNode *node = root->nodes[nodeId];
        // weight depending on ratio of node length to total length
        float weight = node->length / root->nodes[root->nodes.size() - 1]->length;

        // create trihedral vertices
        Point3 up = Point3(0.0f, -1.0f, 0.0f);
        Point3 nodeVector = root->nodes[nodeId + 1]->pos - node->pos;
        Point3 basis = normalize(nodeVector);

        if (nodesAngleDelta > 1e-3 && nodeId > 0 && nodeId < root->nodes.size() - 2)
        {
          Point3 prevNodeVector = node->pos - root->nodes[nodeId - 1]->pos;

          real ang = getAngle(nodeVector, prevNodeVector);
          if (ang < nodesAngleDelta * DEG_TO_RAD)
            continue;
        }

        if (fabs(basis.y) > 0.99)
          basis = Point3(1, 0, 0);

        Point3 b0 = normalize(up % basis) * local_ivyBranchDiameter * local_ivyBranchSize * (1.3f - weight) + node->pos;
        Point3 b1 = rotateAroundAxis(b0, node->pos, basis, 2.09f);
        Point3 b2 = rotateAroundAxis(b0, node->pos, basis, 4.18f);

        if (genGeomType == GEOM_TYPE_PRISM)
        {
          if (trisCount + 6 >= maxTriangles)
            goto end;

          mesh->vert.push_back(b0);
          mesh->vert.push_back(b1);
          mesh->vert.push_back(b2);

          float texU;
          if (fabs(stemTileU) < 1e-3)
            texU = nodeId % 2 == 0 ? 1.0f : 0.0f;
          else
            texU = node->length / stemTileU;

          mesh->tvert[0].push_back(Point2(texU, 0.0f));
          mesh->tvert[0].push_back(Point2(texU, 0.3f));
          mesh->tvert[0].push_back(Point2(texU, 0.6f));

          if (nodeId == 0)
            continue;

          Face tmpFace;

          tmpFace.set(mesh->vert.size() - 4, mesh->vert.size() - 1, mesh->vert.size() - 5, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 5, mesh->vert.size() - 1, mesh->vert.size() - 2, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 5, mesh->vert.size() - 2, mesh->vert.size() - 6, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 6, mesh->vert.size() - 2, mesh->vert.size() - 3, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 6, mesh->vert.size() - 3, mesh->vert.size() - 1, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 6, mesh->vert.size() - 1, mesh->vert.size() - 4, stemId);
          mesh->face.push_back(tmpFace);

          trisCount += 6;

          TFace tmpTexFace;
          tmpTexFace.t[0] = mesh->vert.size() - 4;
          tmpTexFace.t[1] = mesh->vert.size() - 1;
          tmpTexFace.t[2] = mesh->vert.size() - 5;
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 5;
          tmpTexFace.t[1] = mesh->vert.size() - 1;
          tmpTexFace.t[2] = mesh->vert.size() - 2;
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 5;
          tmpTexFace.t[1] = mesh->vert.size() - 2;
          tmpTexFace.t[2] = mesh->vert.size() - 6;
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 6;
          tmpTexFace.t[1] = mesh->vert.size() - 2;
          tmpTexFace.t[2] = mesh->vert.size() - 3;
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 6;
          tmpTexFace.t[1] = mesh->vert.size() - 3;
          tmpTexFace.t[2] = mesh->vert.size() - 1;
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 6;
          tmpTexFace.t[1] = mesh->vert.size() - 1;
          tmpTexFace.t[2] = mesh->vert.size() - 4;
          mesh->tface[0].push_back(tmpTexFace);
        }
        else if (genGeomType == GEOM_TYPE_BILLBOARDS)
        {
          if (trisCount + 2 >= maxTriangles)
            goto end;

          Point3 start = node->pos, end = root->nodes[nodeId + 1]->pos;
          real width = local_ivyBranchDiameter * local_ivyBranchSize * (1.3f - weight);

          Point3 t1 = end - start;
          t1.normalize();

          Point3 pdir = t1 % node->faceNormal;
          pdir.normalize();

          mesh->vert.push_back(start - pdir * width);
          mesh->vert.push_back(start + pdir * width);

          if (fabs(stemTileU) < 1e-3)
            stemTileU = 1;

          real texU = node->length / stemTileU;

          mesh->tvert[0].push_back(Point2(texU, 0.0f));
          mesh->tvert[0].push_back(Point2(texU, 1.0f));

          if (nodeId == 0)
            continue;

          Face tmpFace;
          tmpFace.mat = stemId;

          tmpFace.set(mesh->vert.size() - 4, mesh->vert.size() - 3, mesh->vert.size() - 1, stemId);
          mesh->face.push_back(tmpFace);
          tmpFace.set(mesh->vert.size() - 1, mesh->vert.size() - 2, mesh->vert.size() - 4, stemId);
          mesh->face.push_back(tmpFace);

          trisCount += 2;

          TFace tmpTexFace;
          tmpTexFace.t[0] = mesh->vert.size() - 4; // 0
          tmpTexFace.t[1] = mesh->vert.size() - 3; // 1
          tmpTexFace.t[2] = mesh->vert.size() - 1; // 3
          mesh->tface[0].push_back(tmpTexFace);

          tmpTexFace.t[0] = mesh->vert.size() - 1; // 3
          tmpTexFace.t[1] = mesh->vert.size() - 2; // 2
          tmpTexFace.t[2] = mesh->vert.size() - 4; // 0
          mesh->tface[0].push_back(tmpTexFace);
        }
      }

      con.incDone(1);
    }

  // create leafs

  if (smLeaves && smOldLeaves)
    for (int rootIt = 0; rootIt < roots.size(); rootIt++)
    {
      IvyRoot *root = roots[rootIt];

      // simple multiplier, just to make it a more dense
      for (int i = 0; i < 10; ++i)
      {
        for (int nodeIt = 0; nodeIt < root->nodes.size(); nodeIt++)
        {
          if (finishGeneration)
            goto end;

          if (trisCount + 2 >= maxTriangles)
            goto end;

          IvyNode *node = root->nodes[nodeIt];

          // weight depending on ratio of node length to total length
          float weight = pow(node->length / root->nodes[root->nodes.size() - 1]->length, 0.7f);

          // test: the probability of leaves on the ground is increased
          float ttv = Point3(0.0f, 1.0f, 0.0f) * normalize(node->adhesionVector);
          float groundIvy = MAX_OF(0.0f, -ttv);
          ttv = 1.0f - node->length / root->nodes[root->nodes.size() - 1]->length;
          weight += groundIvy * ttv * ttv;

          // random influence
          float probability = _frnd(currentSeed);

          if (probability * weight > leafDensity)
          {
            // alignment weight depends on the adhesion "strength"
            float alignmentWeight = node->adhesionVector.length();

            // horizontal angle (+ an epsilon vector, otherwise there's a problem at 0� and 90�)
            float phi =
              vectorToPolar(normalize(Point2(node->adhesionVector.z, node->adhesionVector.x)) + Point2getEpsilon()) - PI * 0.5f;

            // vertical angle, trimmed by 0.5
            float theta = getAngle(node->adhesionVector, Point3(0.0f, -1.0f, 0.0f)) * 0.5f;

            // center of leaf quad
            Point3 center = node->pos + getRandomized() * local_ivyLeafSize;

            // size of leaf
            float sizeWeight = 1.5f - (cos(weight * 2.0f * PI) * 0.5f + 0.5f);


            // random influence
            phi += (rand() / (float)RAND_MAX - 0.5f) * (4.1f - alignmentWeight);
            theta += (rand() / (float)RAND_MAX - 0.5f) * (1.1f - alignmentWeight);

            Point3 tmpVertex;

            tmpVertex = center + Point3(-local_ivyLeafSize * sizeWeight, 0.0f, local_ivyLeafSize * sizeWeight);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 0.0f, 1.0f), theta);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 1.0f, 0.0f), phi);
            tmpVertex += getPoint3Randomized() * local_ivyLeafSize * sizeWeight * 0.5f;
            mesh->vert.push_back(tmpVertex);

            tmpVertex = center + Point3(local_ivyLeafSize * sizeWeight, 0.0f, local_ivyLeafSize * sizeWeight);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 0.0f, 1.0f), theta);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 1.0f, 0.0f), phi);
            tmpVertex += getPoint3Randomized() * local_ivyLeafSize * sizeWeight * 0.5f;
            mesh->vert.push_back(tmpVertex);

            tmpVertex = center + Point3(-local_ivyLeafSize * sizeWeight, 0.0f, -local_ivyLeafSize * sizeWeight);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 0.0f, 1.0f), theta);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 1.0f, 0.0f), phi);
            tmpVertex += getPoint3Randomized() * local_ivyLeafSize * sizeWeight * 0.5f;
            mesh->vert.push_back(tmpVertex);

            tmpVertex = center + Point3(local_ivyLeafSize * sizeWeight, 0.0f, -local_ivyLeafSize * sizeWeight);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 0.0f, 1.0f), theta);
            tmpVertex = rotateAroundAxis(tmpVertex, center, Point3(0.0f, 1.0f, 0.0f), phi);
            tmpVertex += getPoint3Randomized() * local_ivyLeafSize * sizeWeight * 0.5f;
            mesh->vert.push_back(tmpVertex);

            Face tmpFace;
            tmpFace.mat = leavesId;
            tmpFace.smgr = 1;

            float probability = rand() / (float)RAND_MAX;
            if (probability * weight > leafDensity)
              tmpFace.mat = oldLeavesId;

            tmpFace.v[0] = mesh->vert.size() - 2;
            tmpFace.v[1] = mesh->vert.size() - 4;
            tmpFace.v[2] = mesh->vert.size() - 3;
            mesh->face.push_back(tmpFace);

            tmpFace.v[0] = mesh->vert.size() - 3;
            tmpFace.v[1] = mesh->vert.size() - 1;
            tmpFace.v[2] = mesh->vert.size() - 2;
            mesh->face.push_back(tmpFace);

            trisCount += 2;


            // create texCoords
            mesh->tvert[0].push_back(Point2(0.0f, 1.0f));
            mesh->tvert[0].push_back(Point2(1.0f, 1.0f));
            mesh->tvert[0].push_back(Point2(0.0f, 0.0f));
            mesh->tvert[0].push_back(Point2(1.0f, 0.0f));

            TFace tmpTexFace;
            tmpTexFace.t[0] = mesh->vert.size() - 2;
            tmpTexFace.t[1] = mesh->vert.size() - 4;
            tmpTexFace.t[2] = mesh->vert.size() - 3;
            mesh->tface[0].push_back(tmpTexFace);

            tmpTexFace.t[0] = mesh->vert.size() - 3;
            tmpTexFace.t[1] = mesh->vert.size() - 1;
            tmpTexFace.t[2] = mesh->vert.size() - 2;
            mesh->tface[0].push_back(tmpTexFace);
          }
        }
      }

      con.incDone(1);
    }

end:
  con.endProgress();
  finishGeneration = false;
  delete genCb;
  genCb = NULL;

  StaticGeometryContainer *g = dagGeom->newStaticGeometryContainer();

  String node_name(128, "ivy mesh %s", getName());
  dagGeom->objCreator3dAddNode(node_name, mesh, mdl, *g);

  StaticGeometryContainer *geom = ivyGeom->getGeometryContainer();

  for (int i = 0; i < g->nodes.size(); ++i)
  {
    StaticGeometryNode *node = g->nodes[i];

    node->flags |= StaticGeometryNode::FLG_CASTSHADOWS | StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF |
                   StaticGeometryNode::FLG_COLLIDABLE | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;

    dagGeom->staticGeometryNodeCalcBoundBox(*node);
    dagGeom->staticGeometryNodeCalcBoundSphere(*node);

    node->name = (const char *)String(512, "%s%d", (char *)node_name, i);
    geom->addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
  }

  dagGeom->deleteStaticGeometryContainer(g);

  ivyGeom->setTm(TMatrix::IDENT);
  recalcLighting();

  ivyGeom->notChangeVertexColors(true);
  dagGeom->geomObjectRecompile(*ivyGeom);
  ivyGeom->notChangeVertexColors(false);
}


void IvyObject::recalcLighting()
{
  if (!ivyGeom)
    return;

  const StaticGeometryContainer &geom = *ivyGeom->getGeometryContainer();

  ISceneLightService *ltService = DAGORED2->queryEditorInterface<ISceneLightService>();

  if (!ltService)
    return;

  Color3 ltCol1, ltCol2, ambCol;
  Point3 ltDir1, ltDir2;

  ltService->getDirectLight(ltDir1, ltCol1, ltDir2, ltCol2, ambCol);

  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = geom.nodes[ni];

    if (node && node->mesh)
    {
      Mesh &mesh = node->mesh->mesh;
      mesh.calc_ngr();
      mesh.calc_vertnorms();

      mesh.cvert.resize(mesh.face.size() * 3);
      mesh.cface.resize(mesh.face.size());

      Point3 normal;

      for (int f = 0; f < mesh.face.size(); ++f)
      {
        for (unsigned v = 0; v < 3; ++v)
        {
          normal = -mesh.vertnorm[mesh.facengr[f][v]];

          const int vi = f * 3 + v;

          Color3 resColor = ambCol;
          real k = normal * ltDir1;
          if (k > 0)
            resColor += ltCol1 * k;
          k = normal * ltDir2;
          if (k > 0)
            resColor += ltCol2 * k;

          mesh.cvert[vi] = ::color4(resColor, 1);
          mesh.cface[f].t[v] = vi;
        }
      }
    }
  }
}
