#include <math/dag_math3d.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>

#include <libTools/dagFileRW/dagFileExport.h>
#include <libTools/dagFileRW/splineShape.h>


DagSaver::DagSaver() : nodes(tmpmem) { rootofs = nodenum = 0; }


DagSaver::~DagSaver() {}


void DagSaver::start_save_node_raw(const char *node_name, int flg, int children_count)
{
  cb.beginTaggedBlock(DAG_NODE);

  cb.beginTaggedBlock(DAG_NODE_DATA);
  DagNodeData data;
  data.id = 0;
  data.cnum = children_count;
  data.flg = flg;
  cb.write(&data, sizeof(DagNodeData));
  if (node_name)
    cb.write(node_name, (int)strlen(node_name));
  cb.endBlock();
}
void DagSaver::save_node_tm(TMatrix &tm)
{
  cb.beginTaggedBlock(DAG_NODE_TM);
  cb.write(&tm, sizeof(tm));
  cb.endBlock();
}
void DagSaver::start_save_children() { cb.beginTaggedBlock(DAG_NODE_CHILDREN); }
void DagSaver::end_save_children() { cb.endBlock(); }
void DagSaver::end_save_node_raw() { cb.endBlock(); }

bool DagSaver::start_save_node(const char *nm, const TMatrix &wtm, int flg, int childN)
{
  if (nodes.size())
  {
    if (nodes.back() > 0)
      nodes.back()--;
    else
      return false;
  }

  cb.beginBlock();
  cb.writeInt(DAG_NODE);

  cb.beginBlock();
  cb.writeInt(DAG_NODE_TM);

  cb.write(&wtm, sizeof(wtm));

  cb.endBlock();

  if (nm)
  {
    cb.beginBlock();
    cb.writeInt(DAG_NODE_DATA);

    DagNodeData data;
    data.id = 0;
    data.cnum = childN;
    data.flg = flg;
    cb.write(&data, sizeof(DagNodeData));

    cb.write(nm, (int)strlen(nm));

    cb.endBlock();
  }

  if (childN == 0)
    nodes.push_back(-1);
  else
    nodes.push_back(childN);

  if (childN)
  {
    cb.beginBlock();
    cb.writeInt(DAG_NODE_CHILDREN);
  }

  return true;
}


bool DagSaver::end_save_node()
{
  if (!nodes.size())
    return false;

  if (nodes.back() != -1)
  {
    // children
    cb.endBlock();
  }

  // if not all children were saved
  if (nodes.back() > 0)
  {
    nodes.pop_back();
    cb.endBlock();

    return false;
  }

  nodes.pop_back();
  cb.endBlock();

  nodenum++;

  return true;
}


bool DagSaver::save_textures(int n, const char **tex)
{
  cb.beginBlock();
  cb.writeInt(DAG_TEXTURES);

  cb.write(&n, 2);

  for (int i = 0; i < n; ++i)
  {
    int l = 0;
    if (tex[i])
      l = (int)strlen(tex[i]);
    if (l > 255)
      return false;

    cb.write(&l, 1);
    if (l)
      cb.write(tex[i], l);
  }

  cb.endBlock();
  return true;
}


bool DagSaver::save_mater(DagExpMater &m)
{
  cb.beginBlock();
  cb.writeInt(DAG_MATER);

  int l = (m.name ? (int)strlen(m.name) : 0);
  cb.write(&l, 1);
  if (l)
    cb.write(m.name, l);

  cb.write(&(DagMater &)m, sizeof(DagMater));

  l = (m.clsname ? (int)strlen(m.clsname) : 0);
  cb.write(&l, 1);
  if (l)
    cb.write(m.clsname, l);

  l = (m.script ? (int)strlen(m.script) : 0);
  if (l)
    cb.write(m.script, l);

  cb.endBlock();
  return true;
}


bool DagSaver::save_node_mats(int n, uint16_t *id)
{
  cb.beginBlock();
  cb.writeInt(DAG_NODE_MATER);

  cb.write(id, n * sizeof(uint16_t));

  cb.endBlock();
  return true;
}


bool DagSaver::save_node_script(const char *s)
{
  if (!s)
    return true;
  if (!*s)
    return true;

  cb.beginBlock();
  cb.writeInt(DAG_NODE_SCRIPT);

  cb.write(s, (int)strlen(s));

  cb.endBlock();
  return true;
}


bool DagSaver::saveMesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch,
  const DagBigMapChannel *texch)
{
  cb.beginBlock();
  cb.writeInt(DAG_OBJ_MESH);

  int n = 0;
  unsigned short val;

  cb.write(&(val = vertn), sizeof(val));
  cb.write(vert, vertn * sizeof(Point3));

  cb.write(&(val = facen), sizeof(val));

  for (int faceId = 0; faceId < facen; ++faceId)
  {
    for (int i = 0; i < 3; ++i)
      cb.write(&(val = face[faceId].v[i]), sizeof(val));

    cb.write(&face[faceId].smgr, sizeof(face[faceId].smgr));
    cb.write(&face[faceId].mat, sizeof(face[faceId].mat));
  }

  cb.write(&numch, 1);

  for (int channelId = 0; channelId < numch; ++channelId)
  {
    cb.write(&(val = texch[channelId].numtv), sizeof(val));
    cb.write(&texch[channelId].num_tv_coords, 1);
    cb.write(&texch[channelId].channel_id, 1);
    cb.write(texch[channelId].tverts, texch[channelId].numtv * texch[channelId].num_tv_coords * sizeof(float));

    for (int i = 0; i < facen; ++i)
      for (int j = 0; j < 3; ++j)
        cb.write(&(val = texch[channelId].tface[i].t[j]), sizeof(val));
  }

  cb.endBlock();
  return true;
}


bool DagSaver::saveBigMesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch,
  const DagBigMapChannel *texch)
{
  cb.beginBlock();
  cb.writeInt(DAG_OBJ_BIGMESH);

  cb.write(&vertn, 4);
  cb.write(vert, vertn * sizeof(Point3));

  cb.write(&facen, 4);
  cb.write(face, facen * sizeof(DagBigFace));

  cb.write(&numch, 1);

  for (int i = 0; i < numch; ++i)
  {
    cb.write(&texch[i].numtv, 4);
    cb.write(&texch[i].num_tv_coords, 1);
    cb.write(&texch[i].channel_id, 1);
    cb.write(texch[i].tverts, texch[i].numtv * texch[i].num_tv_coords * sizeof(float));
    cb.write(texch[i].tface, facen * sizeof(DagBigTFace));
  }

  cb.endBlock();

  return true;
}


bool DagSaver::save_dag_bigmesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch,
  const DagBigMapChannel *texch, uint8_t *faceflg, const DagBigTFace *face_norm, const Point3 *normals, int num_normals)
{
  if (vertn < 0 || facen < 0)
    return false;

  cb.beginBlock();
  cb.writeInt(DAG_NODE_OBJ);

  if ((vertn < 0xFFFF) && (facen < 0xFFFF))
  {
    if (!saveMesh(vertn, vert, facen, face, numch, texch))
      return false;
  }
  else
  {
    if (!saveBigMesh(vertn, vert, facen, face, numch, texch))
      return false;
  }

  if (faceflg)
  {
    cb.beginBlock();
    cb.writeInt(DAG_OBJ_FACEFLG);

    cb.write(faceflg, facen * sizeof(*faceflg));

    cb.endBlock();
  }
  if (face_norm && num_normals > 0)
  {
    cb.beginBlock();
    cb.writeInt(DAG_OBJ_NORMALS);
    cb.writeInt(num_normals);
    cb.write(normals, num_normals * sizeof(Point3));
    cb.write(face_norm, facen * sizeof(DagBigTFace));
    cb.endBlock();
  }


  cb.endBlock();
  return true;
}


bool DagSaver::save_dag_light(DagLight &lt, DagLight2 &lt2)
{
  cb.beginBlock();
  cb.writeInt(DAG_NODE_OBJ);

  cb.beginBlock();
  cb.writeInt(DAG_OBJ_LIGHT);

  cb.write(&lt, sizeof(lt));
  cb.write(&lt2, sizeof(lt2));

  cb.endBlock();
  cb.endBlock();
  return true;
}

bool DagSaver::save_dag_spline(DagSpline **splines, int cnt)
{
  cb.beginBlock();
  cb.writeInt(DAG_NODE_OBJ);

  cb.beginBlock();
  cb.writeInt(DAG_OBJ_SPLINES);
  cb.writeInt(cnt);

  for (int i = 0; i < cnt; ++i)
  {
    if (!splines[i])
      continue;
    cb.writeIntP<1>(splines[i]->closed ? DAG_SPLINE_CLOSED : 0);
    cb.writeInt(splines[i]->knot.size());
    for (int ki = 0; ki < splines[i]->knot.size(); ++ki)
    {
      cb.writeIntP<1>(DAG_SKNOT_BEZIER);
      cb.write(&splines[i]->knot[ki].i, sizeof(Point3));
      cb.write(&splines[i]->knot[ki].p, sizeof(Point3));
      cb.write(&splines[i]->knot[ki].o, sizeof(Point3));
    }
  }
  cb.endBlock();
  cb.endBlock();
  return true;
}

bool DagSaver::start_save_nodes()
{
  cb.beginBlock();
  cb.writeInt(DAG_NODE);

  cb.beginBlock();
  cb.writeInt(DAG_NODE_DATA);

  DagNodeData data;
  data.id = 0;
  data.cnum = 0;
  data.flg = 0;

  rootofs = cb.tell();

  cb.write(&data, sizeof(DagNodeData));

  cb.endBlock();

  cb.beginBlock();
  cb.writeInt(DAG_NODE_CHILDREN);

  return true;
}


bool DagSaver::end_save_nodes()
{
  int cur = cb.tell();

  cb.seekto(rootofs);

  DagNodeData data;
  data.id = 0;
  data.cnum = nodenum;
  data.flg = 0;

  cb.write(&data, sizeof(DagNodeData));

  cb.seekto(cur);

  cb.endBlock();
  cb.endBlock();
  return true;
}


bool DagSaver::start_save_dag(const char *nm)
{
  if (cb.fileHandle)
    end_save_dag();

  name = nm;
  if (!cb.open(name, DF_CREATE | DF_WRITE))
    return false;

  cb.writeInt(DAG_ID);

  cb.beginBlock();
  cb.writeInt(DAG_ID);

  return true;
}


bool DagSaver::end_save_dag()
{
  cb.beginBlock();
  cb.writeInt(DAG_END);

  cb.endBlock();

  cb.endBlock();

  cb.close();
  return true;
}
