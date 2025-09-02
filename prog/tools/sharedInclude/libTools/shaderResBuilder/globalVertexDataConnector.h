// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
 connect vertex datas
************************************************************************/
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <generic/dag_ptrTab.h>
#include <math/dag_Point4.h>

class GlobalVertexDataConnector
{
public:
  DAG_DECLARE_NEW(tmpmem)

  class UpdateInfoInterface
  {
  public:
    UpdateInfoInterface() {}
    virtual void beginConnect(int total_count) {}
    virtual void update(int processed_count, int total_count) {}
    virtual void endConnect(int before_count, int after_count) {}
  };

  GlobalVertexDataConnector() : meshes(tmpmem), vdataSrc(tmpmem), vdataDest(tmpmem), srcMat(tmpmem), srcCr(tmpmem) {}

  void clear()
  {
    meshes.clear();
    vdataSrc.clear();
    srcMat.clear();
    vdataDest.clear();
    srcCr.clear();
  }

  // add meshdata to connect it's vertex datas & update sv/si indices
  void addMeshData(ShaderMeshData *md, const Point3 &c, real r);

  // make connect. if update info is not NULL, call update periodically
  void connectData(bool allow_32_bit, UpdateInfoInterface *update_info, unsigned gdata_flg = 0);

  // get result list with vertex datas
  inline const PtrTab<GlobalVertexDataSrc> &getResult() const { return vdataDest; }

  static bool allowVertexMerge; // =false by default
  static bool allowBaseVertex;  // =false by default

private:
  friend class GatherVertexData;

  Tab<ShaderMeshData *> meshes;
  PtrTab<GlobalVertexDataSrc> vdataSrc;
  PtrTab<ShaderMaterial> srcMat;
  PtrTab<GlobalVertexDataSrc> vdataDest;
  Tab<Point4> srcCr;

  void addVData(GlobalVertexDataSrc *, ShaderMaterial *, const Point4 &);
};
DAG_DECLARE_RELOCATABLE(GlobalVertexDataConnector);
