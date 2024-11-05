// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagFileFormat.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>


struct DagBigMapChannel
{
  int numtv;
  unsigned char num_tv_coords;
  unsigned char channel_id;
  float *tverts;
  DagBigTFace *tface;
};


struct DagExpMater : public DagMater
{
  const char *name, *clsname, *script;
};


class Point3;

// Dag file exporter.
// Throws IGenSave::SaveException on write errors, so watch for it.
class DagSpline;
class DagSaver
{
public:
  String name;

  DagSaver();
  virtual ~DagSaver();

  virtual bool start_save_dag(const char *name);
  virtual bool end_save_dag();

  virtual bool save_textures(int n, const char **texfn);
  virtual bool save_mater(DagExpMater &);

  virtual bool start_save_nodes();
  virtual bool end_save_nodes();

  virtual bool start_save_node(const char *name, const TMatrix &wtm,
    int flg = DAG_NF_RENDERABLE | DAG_NF_CASTSHADOW | DAG_NF_RCVSHADOW, int children = 0);

  virtual bool end_save_node();

  virtual bool save_node_mats(int n, uint16_t *);
  virtual bool save_node_script(const char *);

  virtual bool save_dag_bigmesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch,
    const DagBigMapChannel *texch, unsigned char *faceflg = NULL, const DagBigTFace *face_norms = NULL, const Point3 *norms = NULL,
    int num_normals = 0);

  virtual bool save_dag_light(DagLight &, DagLight2 &);
  virtual bool save_dag_spline(DagSpline **, int cnt);

  virtual void start_save_node_raw(const char *name, int flg, int children_count);
  virtual void save_node_tm(TMatrix &tm);
  virtual void start_save_children();
  virtual void end_save_children();
  virtual void end_save_node_raw();

private:
  FullFileSaveCB cb;

  int rootofs, nodenum;
  Tab<int> nodes;

  bool saveMesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch, const DagBigMapChannel *texch);
  bool saveBigMesh(int vertn, const Point3 *vert, int facen, const DagBigFace *face, unsigned char numch,
    const DagBigMapChannel *texch);
};
