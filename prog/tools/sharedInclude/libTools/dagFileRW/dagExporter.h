// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <3d/dag_materialData.h>


class Node;
class MaterialData;
class DagSaver;
class String;

class DagExporter
{
public:
  static bool exportGeometry(const char *fileName, Node *rootNode, bool slot_tex = false, bool preserve_hier = false,
    bool save_normal = false);

  static bool isObjectPresent(Node *node, int objectId);

private:
  static void getMaterialsAndTextures(Node *node, PtrTab<MaterialData> &materials, Tab<TEXTUREID> &textures);

  static void saveMaterialsAndTextures(const PtrTab<MaterialData> &materials, const Tab<TEXTUREID> &textures, bool slot_tex = false);

  static void saveNodeMaterials(Node *node, const PtrTab<MaterialData> &materials);
  static void saveBigMeshNode(Node *node, const PtrTab<MaterialData> &materials);
  static void saveBigMeshNodeHier(Node *node, const PtrTab<MaterialData> &materials);
  static void saveBigMeshNodes(Node *root, const PtrTab<MaterialData> &mat, bool preserve_hier);

  static void addNullMat(MaterialData *curMaterial, PtrTab<MaterialData> &materials, Tab<TEXTUREID> &textures);
  static void addNodeNullMat(const MaterialData *curMaterial, const PtrTab<MaterialData> &materials, Tab<uint16_t> &nodeMaterials);

private:
  static bool saveNormals;
  static DagSaver dagSaver;
  static String location;
};
