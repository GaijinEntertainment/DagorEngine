#ifndef _GAIJIN_SHADERS_SCENE_SPLIT_OBJECTS_H
#define _GAIJIN_SHADERS_SCENE_SPLIT_OBJECTS_H
#pragma once

#include <math/dag_math3d.h>
#include <libTools/util/iLogWriter.h>

#define LIGHTS_PER_OBJECT         4
#define MAX_LIGHT_MESH_DIFFERENCE 0.1
#define FRONT_FACED_THRESHOLD     0.2


Point4 get_plane_from_points(const Point3 &point0, const Point3 &point1, const Point3 &point2)
{
  // ax+by+cz+d=0
  Point3 normal = normalize((point1 - point0) % (point2 - point0));
  Point4 plane(normal.x, normal.y, normal.z, -normal * point0);
  return plane;
}


#define MAX_INTERPOLATED_CHANNELS 12

struct ClipVertex
{
  Point3 position;
  Point4 channels[MAX_INTERPOLATED_CHANNELS];
};


void clip_triangles_by_plane(Tab<ClipVertex> &vertices_list, unsigned int num_interpolated_channels, const Point4 &plane)
{
  G_ASSERT(num_interpolated_channels < MAX_INTERPOLATED_CHANNELS);
  G_ASSERT(vertices_list.size() % 3 == 0);

  int numVerticesInitial = vertices_list.size();
  for (unsigned int baseVertexNo = 0; baseVertexNo < numVerticesInitial; baseVertexNo += 3)
  {
    // Clip triangle by plane.

#define DOT_PRODUCT_4_3(v4, v3) ((v4)*Point4((v3).x, (v3).y, (v3).z, 1.f))

    bool isOnNegativeSideArray[3];
    int numNegativeVertices = 0;
    for (unsigned int vertexNo = 0; vertexNo < 3; vertexNo++)
    {
      isOnNegativeSideArray[vertexNo] = DOT_PRODUCT_4_3(plane, vertices_list[baseVertexNo + vertexNo].position) < 0.f;
      if (isOnNegativeSideArray[vertexNo])
        numNegativeVertices++;
    }

    if (numNegativeVertices == 0) // Not clipped.
      continue;

    if (numNegativeVertices == 3) // All the triangle clipped.
    {
      erase_items(vertices_list, baseVertexNo, 3);
      baseVertexNo -= 3;
      numVerticesInitial -= 3;
      continue;
    }


    // Clip edges.

    ClipVertex newVertices[4];
    unsigned int numNewVertices = 0;
    for (unsigned int vertexANo = 0; vertexANo < 3; vertexANo++)
    {
      unsigned int vertexBNo = (vertexANo > 0) ? vertexANo - 1 : 2;

      if (isOnNegativeSideArray[vertexANo])
      {
        if (!isOnNegativeSideArray[vertexBNo])
        {
          const Point3 &v1 = vertices_list[baseVertexNo + vertexBNo].position;
          const Point3 &v2 = vertices_list[baseVertexNo + vertexANo].position;
          float t = DOT_PRODUCT_4_3(plane, v1) / (plane.x * (v1.x - v2.x) + plane.y * (v1.y - v2.y) + plane.z * (v1.z - v2.z));

          newVertices[numNewVertices].position = v1 * (1.f - t) + v2 * t;

          for (unsigned int channelNo = 0; channelNo < num_interpolated_channels; channelNo++)
          {
            newVertices[numNewVertices].channels[channelNo] = vertices_list[baseVertexNo + vertexBNo].channels[channelNo] * (1.f - t) +
                                                              vertices_list[baseVertexNo + vertexANo].channels[channelNo] * t;
          }

          numNewVertices++;
        }
      }
      else
      {
        if (isOnNegativeSideArray[vertexBNo])
        {
          const Point3 &v1 = vertices_list[baseVertexNo + vertexANo].position;
          const Point3 &v2 = vertices_list[baseVertexNo + vertexBNo].position;
          float t = DOT_PRODUCT_4_3(plane, v1) / (plane.x * (v1.x - v2.x) + plane.y * (v1.y - v2.y) + plane.z * (v1.z - v2.z));

          newVertices[numNewVertices].position = v1 * (1.f - t) + v2 * t;

          for (unsigned int channelNo = 0; channelNo < num_interpolated_channels; channelNo++)
          {
            newVertices[numNewVertices].channels[channelNo] = vertices_list[baseVertexNo + vertexANo].channels[channelNo] * (1.f - t) +
                                                              vertices_list[baseVertexNo + vertexBNo].channels[channelNo] * t;
          }

          numNewVertices++;
        }

        newVertices[numNewVertices] = vertices_list[baseVertexNo + vertexANo];
        numNewVertices++;
      }
    }


    // Split polygon to tri-list.

    for (unsigned int vertexNo = 1; vertexNo < numNewVertices - 1; vertexNo++)
    {
      vertices_list.push_back(newVertices[0]);
      vertices_list.push_back(newVertices[vertexNo]);
      vertices_list.push_back(newVertices[vertexNo + 1]);
    }


    // Remove original triangle.

    erase_items(vertices_list, baseVertexNo, 3);
    baseVertexNo -= 3;
    numVerticesInitial -= 3;
  }
}


static int sort_axis_no;
static unsigned int sort_vb_stride;
static unsigned char *sort_vb;

inline int cmp_vb_pos_float3(int l0, int l1, int l2, int r0, int r1, int r2)
{
  int ofs = sort_axis_no * 4;

  float leftCenter = (*(float *)&sort_vb[sort_vb_stride * l0 + ofs]) + (*(float *)&sort_vb[sort_vb_stride * l1 + ofs]) +
                     (*(float *)&sort_vb[sort_vb_stride * l2 + ofs]);

  float rightCenter = (*(float *)&sort_vb[sort_vb_stride * r0 + ofs]) + (*(float *)&sort_vb[sort_vb_stride * r1 + ofs]) +
                      (*(float *)&sort_vb[sort_vb_stride * r2 + ofs]);

  return leftCenter < rightCenter ? -1 : (leftCenter > rightCenter ? 1 : 0);
}

inline int cmp_vb_pos_half4(int l0, int l1, int l2, int r0, int r1, int r2)
{
  int ofs = sort_axis_no * 2;

  float leftCenter = half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * l0 + ofs]) +
                     half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * l1 + ofs]) +
                     half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * l2 + ofs]);

  float rightCenter = half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * r0 + ofs]) +
                      half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * r1 + ofs]) +
                      half_to_float(*(unsigned short *)&sort_vb[sort_vb_stride * r2 + ofs]);

  return leftCenter < rightCenter ? -1 : (leftCenter > rightCenter ? 1 : 0);
}

static int __cdecl sort_along_axis32(const void *left, const void *right)
{
  unsigned int *leftIndex = (unsigned int *)left;
  unsigned int *rightIndex = (unsigned int *)right;
  return cmp_vb_pos_float3(leftIndex[0], leftIndex[1], leftIndex[2], rightIndex[0], rightIndex[1], rightIndex[2]);
}

static int __cdecl sort_along_axis16(const void *left, const void *right)
{
  unsigned short int *leftIndex = (unsigned short int *)left;
  unsigned short int *rightIndex = (unsigned short int *)right;
  return cmp_vb_pos_float3(leftIndex[0], leftIndex[1], leftIndex[2], rightIndex[0], rightIndex[1], rightIndex[2]);
}

static int __cdecl sort_along_axis32_half4(const void *left, const void *right)
{
  unsigned int *leftIndex = (unsigned int *)left;
  unsigned int *rightIndex = (unsigned int *)right;
  return cmp_vb_pos_half4(leftIndex[0], leftIndex[1], leftIndex[2], rightIndex[0], rightIndex[1], rightIndex[2]);
}

static int __cdecl sort_along_axis16_half4(const void *left, const void *right)
{
  unsigned short *leftIndex = (unsigned short *)left;
  unsigned short *rightIndex = (unsigned short *)right;
  return cmp_vb_pos_half4(leftIndex[0], leftIndex[1], leftIndex[2], rightIndex[0], rightIndex[1], rightIndex[2]);
}


template <class ObjectType>
unsigned int get_triangle_num(ObjectType &object)
{
  G_ASSERT(object.meshData);

  unsigned int triangleNum = 0;
  for (unsigned int elemNo = 0; elemNo < object.meshData->elems.size(); elemNo++)
    triangleNum += object.meshData->elems[elemNo].numf;

  return triangleNum;
}

extern int shader_channel_type_size(int t);

static int find_pos0_ch_type(const GlobalVertexDataSrc &vd, int &offset)
{
  offset = 0;
  for (unsigned int cn = 0; cn < vd.vDesc.size(); cn++)
  {
    const CompiledShaderChannelId &channel = vd.vDesc[cn];
    if (channel.vbu == SCUSAGE_POS && channel.vbui == 0)
      return channel.t;
    else
    {
      offset += shader_channel_type_size(channel.t);
    }
  }
  return -1;
}

template <class ObjectType>
void recalc_sphere_and_bbox(ObjectType &object, Tab<Point3> &tmp_points)
{
  tmp_points.clear();
  object.bbox.setempty();

  dag::ConstSpan<ShaderMeshData::RElem> elems = object.meshData->elems;

  for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
  {
    const ShaderMeshData::RElem &re = elems[elemNo];
    const GlobalVertexDataSrc &vd = *re.vertexData;
    int offset;
    int pos_type = find_pos0_ch_type(vd, offset);

    switch (pos_type)
    {
      case VSDT_FLOAT3:
      case VSDT_FLOAT4:
        for (unsigned int ti = 0; ti < re.numf; ti++)
        {
          unsigned int st_vi = re.si + 3 * ti;
          for (unsigned int fvi = 0; fvi < 3; fvi++)
          {
            unsigned int idx = vd.iData.empty() ? vd.iData32[st_vi + fvi] : vd.iData[st_vi + fvi];
            Point3 v = *(Point3 *)&vd.vData[vd.stride * idx + offset];

            tmp_points.push_back(v);
            object.bbox += v;
          }
        }
        break;

      case VSDT_HALF4:
        for (unsigned int ti = 0; ti < re.numf; ti++)
        {
          unsigned int st_vi = re.si + 3 * ti;
          for (unsigned int fvi = 0; fvi < 3; fvi++)
          {
            unsigned int idx = vd.iData.empty() ? vd.iData32[st_vi + fvi] : vd.iData[st_vi + fvi];
            unsigned short *half = (unsigned short *)&vd.vData[vd.stride * idx + offset];
            Point3 v(half_to_float(half[0]), half_to_float(half[1]), half_to_float(half[2]));

            tmp_points.push_back(v);
            object.bbox += v;
          }
        }
        break;

      default: DAG_FATAL("POS0 channel is of unsupported type 0x%08X", pos_type);
    }
  }
  object.bsph = ::mesh_fast_bounding_sphere(tmp_points.data(), tmp_points.size());
}


template <class ObjectType>
void split_objects_to_elements(Tab<ObjectType> &objectList, IGenericProgressIndicator &pbar)
{
  Tab<Point3> tmp_points(tmpmem);

  bool split;
  do
  {
    split = false;
    pbar.setDone(0);
    pbar.setTotal(objectList.size());

    for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
    {
      pbar.incDone();

      if (objectList[objectNo].topLodObjId >= 0)
        continue; // don't split LODs

      if (objectList[objectNo].meshData->elems.empty())
      {
        erase_items(objectList, objectNo, 1);
        objectNo--;
        continue;
      }

      unsigned int triangleNum = get_triangle_num(objectList[objectNo]);
      // debug("%d triangles.", triangleNum);
      //  if (triangleNum > trianglesPerObjectTargeted && objectList[objectNo].bsph.r > minRadiusToSplit)
      //  Always split objects to elems. Elems are rendered one by one in any case.
      {
        ShaderMeshData *mesh = objectList[objectNo].meshData;

        if (mesh->elems.size() > 1)
        {
          unsigned int fattestElemNo;
          unsigned int fattestElemTriangleNum = 0;
          for (unsigned int elemNo = 0; elemNo < mesh->elems.size(); elemNo++)
          {
            if (mesh->elems[elemNo].numf >= fattestElemTriangleNum)
            {
              fattestElemNo = elemNo;
              fattestElemTriangleNum = mesh->elems[elemNo].numf;
            }
          }

          split = true;
          int objectIndexAdded = append_items(objectList, 1);
          ObjectType &newObject = objectList[objectIndexAdded];
          newObject.copySavableParams(objectList[objectNo]);
          newObject.meshData = new ShaderMeshData;

          newObject.meshData->addElem(mesh->elems[fattestElemNo].getStage(), mesh->elems[fattestElemNo]);
          mesh->delElem(fattestElemNo);

          recalc_sphere_and_bbox(objectList[objectNo], tmp_points);
          recalc_sphere_and_bbox(newObject, tmp_points);
          inplace_min(objectList[objectNo].maxSubRad, objectList[objectNo].bsph.r);
          inplace_min(newObject.maxSubRad, newObject.bsph.r);
          // debug("split(mat) '%s' with %d triangles to %d and %d triangles.",
          //   objectList[objectNo].name.str(), triangleNum,
          //   get_triangle_num(objectList[objectNo]), get_triangle_num(newObject));
        }
      }
    }
  } while (split);

  for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
  {
    if (objectList[objectNo].topLodObjId >= 0)
      continue; // don't split LODs

    G_ASSERT(objectList[objectNo].meshData->elems.size() == 1);
  }

  debug("materials split to %d objects.", objectList.size());
}

template <class Integral>
void shift_indices(dag::Span<Integral> data, int delta)
{
  for (int i = 0; i < data.size(); i++)
    data[i] += delta;
}

template <class ObjectType>
bool split_mesh(ShaderMeshData::RElem &ne, ShaderMeshData::RElem &oe, dag::ConstSpan<ObjectType> objects, Tab<uint8_t> &tmp_v1,
  Tab<uint8_t> &tmp_v2, Tab<int> &tmp_i)
{
  GlobalVertexDataSrc &vd = *oe.vertexData.get();
  int stride = vd.stride;
  int numf = oe.numf;
  int numv = oe.numv;
  int sv = oe.sv;
  int end_i;

  // debug("%p.split sv=%d numv=%d si=%d numf=%d (vd=%d %d)",
  //   &oe, sv, numv, oe.si, numf, vd.numv, vd.numf);
  G_ASSERT(sv < vd.numv);
  G_ASSERT(oe.si / 3 + numf <= vd.numf);

  ne = oe;
  oe.numf /= 2;

  ne.numf = numf - oe.numf;
  ne.si = oe.si + 3 * oe.numf;
  ne.vdOrderIndex++;
  vd.partCount++;

  tmp_v1.clear();
  tmp_v2.clear();

  tmp_i.clear();
  end_i = oe.si + oe.numf * 3;
  for (int i = oe.si; i < end_i; i++)
  {
    int idx = vd.iData.empty() ? vd.iData32[i] : vd.iData[i];
    if (idx >= tmp_i.size())
    {
      int c = tmp_i.size();
      tmp_i.resize(idx + 1);
      mem_set_ff(make_span(tmp_i).subspan(c));
    }
    if (tmp_i[idx] == -1)
      tmp_i[idx] = append_items(tmp_v1, stride, &vd.vData[idx * stride]) / stride + sv;

    if (vd.iData.empty())
      vd.iData32[i] = tmp_i[idx];
    else
      vd.iData[i] = tmp_i[idx];
  }
  oe.numv = tmp_v1.size() / stride;

  tmp_i.clear();
  ne.sv = sv = oe.sv + oe.numv;
  end_i = ne.si + ne.numf * 3;
  for (int i = ne.si; i < end_i; i++)
  {
    int idx = vd.iData.empty() ? vd.iData32[i] : vd.iData[i];
    if (idx >= tmp_i.size())
    {
      int c = tmp_i.size();
      tmp_i.resize(idx + 1);
      mem_set_ff(make_span(tmp_i).subspan(c));
    }
    if (tmp_i[idx] == -1)
      tmp_i[idx] = append_items(tmp_v2, stride, &vd.vData[idx * stride]) / stride + sv;

    if (vd.iData.empty())
      vd.iData32[i] = tmp_i[idx];
    else
      vd.iData[i] = tmp_i[idx];
  }
  ne.numv = tmp_v2.size() / stride;

  int delta = ne.numv + oe.numv - numv;
  if (vd.numv + delta >= 65534)
  {
    oe.numf = numf;
    oe.numv = numv;
    return false;
  }

  if (delta > 0)
    insert_items(vd.vData, (oe.sv + numv) * stride, delta * stride);
  else if (delta < 0)
    erase_items(vd.vData, (oe.sv + numv) * stride, -delta * stride);
  mem_copy_to(tmp_v1, &vd.vData[oe.sv * stride]);
  mem_copy_to(tmp_v2, &vd.vData[ne.sv * stride]);
  vd.numv += delta;

  // Fix vdOrderIndex of the rest elems.
  for (int oi = 0; oi < (int)objects.size() - 1; oi++)
  {
    dag::ConstSpan<ShaderMeshData::RElem> elems = objects[oi].meshData->elems;
    for (int elemNo = 0; elemNo < elems.size(); elemNo++)
      if (elems[elemNo].vertexData == ne.vertexData && elems[elemNo].vdOrderIndex >= ne.vdOrderIndex)
      {
        ShaderMeshData::RElem &e = const_cast<ShaderMeshData::RElem &>(elems[elemNo]);
        e.vdOrderIndex++;
        if (delta)
        {
          e.sv += delta;
          if (e.vertexData->iData.empty())
            shift_indices(make_span(e.vertexData->iData32).subspan(e.si, e.numf * 3), delta);
          else
            shift_indices(make_span(e.vertexData->iData).subspan(e.si, e.numf * 3), delta);
        }
      }
  }
  // debug("-> %p.(sv=%d numv=%d si=%d numf=%d) %p.(sv=%d numv=%d si=%d numf=%d)",
  //   &oe, oe.sv, oe.numv, oe.si, oe.numf, &ne, ne.sv, ne.numv, ne.si, ne.numf);

  return true;
}

template <class ObjectType>
void split_objects_by_triangles(Tab<ObjectType> &objectList, unsigned int trianglesPerObjectTargeted, float minRadiusToSplit,
  IGenericProgressIndicator &pbar)
{
  Tab<Point3> tmp_points(tmpmem);
  Tab<uint8_t> tmp_v1(tmpmem), tmp_v2(tmpmem);
  Tab<int> tmp_i(tmpmem);
  int overhead = 0;

  // for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
  //   objectList[objectNo].isSplitFinished = false;

  bool shouldContinueSplit;
  do
  {
    pbar.setDone(0);
    pbar.setTotal(objectList.size());

    shouldContinueSplit = false;
    for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
    {
      pbar.incDone();

      if (objectList[objectNo].topLodObjId >= 0)
        continue; // don't split LODs

      // if (objectList[objectNo].isSplitFinished)
      //   continue;

      ShaderMeshData::RElem &element = objectList[objectNo].meshData->elems[0];

      bool shouldSplitForTrianglesAndRadius =
        element.numf > trianglesPerObjectTargeted && objectList[objectNo].bsph.r > minRadiusToSplit;

      if (shouldSplitForTrianglesAndRadius)
      {
        // Create new object.

        ObjectType &newObject = objectList.push_back();
        ;
        newObject.copySavableParams(objectList[objectNo]);
        newObject.meshData = new ShaderMeshData;


        // Create new elem.

        ShaderMeshData::RElem *initialElem = &objectList[objectNo].meshData->elems[0];
        ShaderMeshData::RElem *newElem = newObject.meshData->addElem(initialElem->getStage());
        newElem->flags = initialElem->getStage();


        // Get raw primary axis.
        GlobalVertexDataSrc &vd = *initialElem->vertexData.get();
        int offset;
        int pos_type = find_pos0_ch_type(vd, offset);
        int stride = vd.stride;

        BBox3 bbox;
        switch (pos_type)
        {
          case VSDT_FLOAT3:
          case VSDT_FLOAT4:
            for (unsigned int triangleNo = 0; triangleNo < initialElem->numf; triangleNo++)
            {
              unsigned int indexStart = initialElem->si + 3 * triangleNo;
              unsigned int index0 = vd.iData.empty() ? vd.iData32[indexStart] : vd.iData[indexStart];
              Point3 *corner0 = (Point3 *)&vd.vData[stride * index0 + offset];
              G_ASSERT(index0 < vd.numv);
              bbox += *corner0;
            }
            break;

          case VSDT_HALF4:
            for (unsigned int triangleNo = 0; triangleNo < initialElem->numf; triangleNo++)
            {
              unsigned int indexStart = initialElem->si + 3 * triangleNo;
              unsigned int index0 = vd.iData.empty() ? vd.iData32[indexStart] : vd.iData[indexStart];
              unsigned short *half = (unsigned short *)&vd.vData[stride * index0 + offset];
              G_ASSERT(index0 < vd.numv);
              bbox += Point3(half_to_float(half[0]), half_to_float(half[1]), half_to_float(half[2]));
            }
            break;

          default: DAG_FATAL("POS0 channel is of unsupported type 0x%08X", pos_type);
        }


        // Sort triangles along the axis.
        sort_vb = &vd.vData[offset];
        sort_vb_stride = stride;
        if (bbox.width().x > bbox.width().y && bbox.width().x > bbox.width().z)
          sort_axis_no = 0;
        else if (bbox.width().y > bbox.width().z)
          sort_axis_no = 1;
        else
          sort_axis_no = 2;


        switch (pos_type)
        {
          case VSDT_FLOAT3:
          case VSDT_FLOAT4:
            if (vd.iData.empty())
              qsort((void *)&vd.iData32[initialElem->si], initialElem->numf, 3 * sizeof(unsigned int), &sort_along_axis32);
            else
              qsort((void *)&vd.iData[initialElem->si], initialElem->numf, 3 * sizeof(unsigned short int), &sort_along_axis16);
            break;

          case VSDT_HALF4:
            if (vd.iData.empty())
              qsort((void *)&vd.iData32[initialElem->si], initialElem->numf, 3 * sizeof(unsigned int), &sort_along_axis32_half4);
            else
              qsort((void *)&vd.iData[initialElem->si], initialElem->numf, 3 * sizeof(unsigned short int), &sort_along_axis16_half4);
            break;

          default: DAG_FATAL("POS0 channel is of unsupported type 0x%08X", pos_type);
        }

        // Split.
        unsigned int totalFaceNum = initialElem->numf;
        unsigned int totalVertNum = initialElem->numv;
        if (!split_mesh(*newElem, *initialElem, make_span_const(objectList), tmp_v1, tmp_v2, tmp_i))
        {
          objectList.pop_back();
          debug("cannot split mesh '%s' - tool large vbuf", objectList[objectNo].name.str());
          continue;
        }

        // recalc bounds
        recalc_sphere_and_bbox(objectList[objectNo], tmp_points);
        recalc_sphere_and_bbox(newObject, tmp_points);
        inplace_min(objectList[objectNo].maxSubRad, objectList[objectNo].bsph.r);
        inplace_min(newObject.maxSubRad, newObject.bsph.r);

        // debug("split(tri) elem of '%s' with %d triangles to %d and %d triangles (+ %d verts)",
        //   objectList[objectNo].name.str(), totalFaceNum, initialElem->numf, newElem->numf,
        //   initialElem->numv + newElem->numv - totalVertNum);
        overhead += (initialElem->numv + newElem->numv - totalVertNum) * stride;

        shouldContinueSplit = true;
      }
    }
    debug("pass done, shouldContinueSplit=%d", shouldContinueSplit);
  } while (shouldContinueSplit);
  debug("total split overhead: %d bytes", overhead);
}


template <class ObjectType>
void split_objects(Tab<ObjectType> &objectList, unsigned int trianglesPerObjectTargeted, float minRadiusToSplit,
  IGenericProgressIndicator &pbar, ILogWriter &log)
{
  debug("split_objects started with %d objects.", objectList.size());

  unsigned int initialTrianglesNum = 0;
  for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
  {
    ObjectType &object = objectList[objectNo];

    for (unsigned int elemNo = 0; elemNo < object.meshData->elems.size(); elemNo++)
      initialTrianglesNum += object.meshData->elems[elemNo].numf;
  }


  // Split by elements.

  split_objects_to_elements(objectList, pbar);


  // Split by triangles.

  split_objects_by_triangles(objectList, trianglesPerObjectTargeted, minRadiusToSplit, pbar);


  // Verify.

  unsigned int finalTrianglesNum = 0;
  for (unsigned int objectNo = 0; objectNo < objectList.size(); objectNo++)
  {
    ObjectType &object = objectList[objectNo];

    for (unsigned int elemNo = 0; elemNo < object.meshData->elems.size(); elemNo++)
      finalTrianglesNum += object.meshData->elems[elemNo].numf;
  }

  debug("split_objects finished with %d objects, %d triangles.", objectList.size(), finalTrianglesNum);
}


#endif //_GAIJIN_SHADERS_SCENE_SPLIT_OBJECTS_H
