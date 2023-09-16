// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGORED_STATICGEOM_H
#define _GAIJIN_DAGORED_STATICGEOM_H
#pragma once


#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>

#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagFileExport.h>

// forward declarations for external classes
class IGenSave;
class IGenLoad;
class MaterialData;
class Node;
class ILogWriter;


class StaticGeometryContainer
{
public:
  //  PtrTab<StaticGeometryTexture> textures;
  Tab<StaticGeometryNode *> nodes;


  StaticGeometryContainer() : /*textures(tmpmem),*/ nodes(tmpmem), loaded(false) {}

  ~StaticGeometryContainer() { clear(); }

  inline bool isLoaded() const { return loaded; }

  void clear();

  void addNode(StaticGeometryNode *node)
  {
    if (!node)
      return;
    nodes.push_back(node);
  }

  void addNode(Node &node)
  {
    DagLoadData dag;
    loadDagNode(node, dag);
  }

  bool loadFromDag(const char *filename, ILogWriter *log, bool use_not_found_tex = false,
    int load_flags = LASF_MATNAMES | LASF_NULLMATS);

  bool importDag(const char *source_dag_name, const char *dest_dag_name);
  void exportDag(const char *path, bool make_tex_path_local = true) const;
  virtual void writeMaterials(DagSaver *sv, bool make_tex_path_local = true) const;
  virtual void writeNodes(DagSaver *sv) const;

  bool optimize(bool faces = true, bool materials = true); // return true if optimized

  inline dag::Span<StaticGeometryNode *> getNodes() { return dag::Span<StaticGeometryNode *>(nodes.data(), nodes.size()); }

  // change all nodes's materials to match their lighting
  // use it seldom, because function recreate all nodes materials
  void applyNodesLighting();

  void markNonTopLodNodes(ILogWriter *log, const char *filename);

protected:
  bool loaded;

  class DagLoadData
  {
  public:
    struct Mat
    {
      MaterialData *dagMat;
      Ptr<StaticGeometryMaterial> sgMat;

      Mat() : dagMat(NULL) {}
    };

    Tab<Mat> mats;

    DagLoadData() : mats(tmpmem), logWriter(NULL), useNotFoundTex(false) {}
    DagLoadData(const char *base) : mats(tmpmem), logWriter(NULL), useNotFoundTex(false)
    {
      basePath = base;
      ::location_from_path(basePath);
    }

    inline void setUseNotFoundTex(bool use) { useNotFoundTex = use; }
    inline bool getUseNotFoundTex() const { return useNotFoundTex; }

    inline ILogWriter *getLogWriter() const { return logWriter; }
    inline void setLogWriter(ILogWriter *log) { logWriter = log; }

    StaticGeometryMaterial *getMaterial(MaterialData *m);

  private:
    String basePath;
    bool useNotFoundTex;
    ILogWriter *logWriter;
  };

  void loadDagNode(class Node &node, DagLoadData &dag);

  void getBlkScript(const char *script, String &blk_script, String &non_blk_script);
};


#endif
