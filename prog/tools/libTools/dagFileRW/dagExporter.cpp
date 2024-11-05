// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/sceneImpIface.h>
#include <math/dag_mesh.h>
#include <libTools/dagFileRW/dagFileExport.h>
#include <debug/dag_debug.h>

#include <libTools/dagFileRW/dagExporter.h>


DagSaver DagExporter::dagSaver;
String DagExporter::location;
bool DagExporter::saveNormals = false;

bool DagExporter::isObjectPresent(Node *node, int objectId)
{
  if (!node)
    return false;
  if (node->obj && node->obj->isSubOf(objectId))
    return true;

  for (int nodeId = node->child.size(); --nodeId >= 0;)
    if (isObjectPresent(node->child[nodeId], objectId))
      return true;
  return false;
}

void DagExporter::addNullMat(MaterialData *curMaterial, PtrTab<MaterialData> &materials, Tab<TEXTUREID> &textures)
{
  if (!curMaterial)
    return;

  // add material
  int materialId = materials.size();
  for (; --materialId >= 0;)
    if (curMaterial == materials[materialId])
      break;
  if (materialId < 0)
    materials.push_back(curMaterial);

  // add texture
  for (int matTextureId = 0; matTextureId < MAXMATTEXNUM; ++matTextureId)
  {
    const TEXTUREID curTexture = curMaterial->mtex[matTextureId];
    if (curTexture == BAD_TEXTUREID)
      continue;

    int textureId = textures.size();
    for (; --textureId >= 0;)
      if (curTexture == textures[textureId])
        break;
    if (textureId < 0)
      textures.push_back(curTexture);
  }
}


void DagExporter::addNodeNullMat(const MaterialData *curMaterial, const PtrTab<MaterialData> &materials, Tab<uint16_t> &nodeMaterials)
{
  if (!curMaterial)
    return;

  int materialId = materials.size();
  for (; --materialId >= 0;)
    if (curMaterial == materials[materialId])
      break;
  if (materialId < 0)
  {
    //==message to log - material not found!
    materialId = 0;
  }

  nodeMaterials.push_back(materialId);
}


void DagExporter::getMaterialsAndTextures(Node *node, PtrTab<MaterialData> &materials, Tab<TEXTUREID> &textures)
{
  if (!node)
    return;

  if (node->mat && node->mat->subMatCount())
  {
    for (int nulMaterialId = 0; nulMaterialId < node->mat->subMatCount(); ++nulMaterialId)
    {
      MaterialData *curMaterial = node->mat->getSubMat(nulMaterialId);
      addNullMat(curMaterial, materials, textures);
    }
  }

  for (int nodeId = node->child.size(); --nodeId >= 0;)
    getMaterialsAndTextures(node->child[nodeId], materials, textures);
}


void DagExporter::saveMaterialsAndTextures(const PtrTab<MaterialData> &materials, const Tab<TEXTUREID> &textures, bool slot_tex)
{
  // save textures
  Tab<String> textureNames(tmpmem);
  Tab<const char *> pTextureNames(tmpmem);

  textureNames.resize(textures.size());
  pTextureNames.resize(textures.size());

  if (slot_tex)
    for (int textureId = 0; textureId < textures.size(); ++textureId)
      pTextureNames[textureId] = ::get_managed_texture_name(textures[textureId]);
  else
    for (int textureId = 0; textureId < textures.size(); ++textureId)
    {
      String textureFileName(::get_managed_texture_name(textures[textureId]));
      textureNames[textureId] = make_path_relative(textureFileName, location);
      pTextureNames[textureId] = (char *)textureNames[textureId];
    }

  dagSaver.save_textures(pTextureNames.size(), pTextureNames.data());

  // save matrials
  for (int materialId = 0; materialId < materials.size(); ++materialId)
  {
    MaterialData *curMaterial = materials[materialId];
    DagExpMater material;
    material.name = curMaterial->matName;
    material.clsname = curMaterial->className;
    material.script = curMaterial->matScript;

    material.amb = color3(curMaterial->mat.amb);
    material.diff = color3(curMaterial->mat.diff);
    material.spec = color3(curMaterial->mat.spec);
    material.emis = color3(curMaterial->mat.emis);
    material.power = curMaterial->mat.power;
    material.flags = (curMaterial->flags & MATF_2SIDED) ? DAG_MF_2SIDED : 0;
    material.flags |= DAG_MF_16TEX;

    memset(material.texid, 0xFF, sizeof(material.texid));

    for (int matTextureId = 0; (matTextureId < MAXMATTEXNUM) && (matTextureId < DAGTEXNUM); ++matTextureId)
    {
      int textureId = DAGBADMATID;
      const TEXTUREID curTexture = curMaterial->mtex[matTextureId];

      if (curTexture != BAD_TEXTUREID)
      {
        for (textureId = textures.size(); --textureId >= 0;)
          if (curTexture == textures[textureId])
            break;

        if (textureId < 0)
        {
          textureId = DAGBADMATID;
          // TODO: log internal error texture not found
        }
      }
      material.texid[matTextureId] = textureId;
    }

    dagSaver.save_mater(material);
  }
}

void DagExporter::saveNodeMaterials(Node *node, const PtrTab<MaterialData> &materials)
{
  Tab<uint16_t> nodeMaterials(tmpmem);

  if (node->mat && node->mat->subMatCount())
  {
    for (int nulMaterialId = 0; nulMaterialId < node->mat->subMatCount(); ++nulMaterialId)
    {
      const MaterialData *curMaterial = node->mat->getSubMat(nulMaterialId);
      addNodeNullMat(curMaterial, materials, nodeMaterials);
    }
  }

  dagSaver.save_node_mats(nodeMaterials.size(), nodeMaterials.data());
}


void DagExporter::saveBigMeshNodes(Node *rootNode, const PtrTab<MaterialData> &materials, bool preserve_hier)
{
  if (!preserve_hier)
  {
    dagSaver.start_save_nodes();
    saveBigMeshNode(rootNode, materials);
    dagSaver.end_save_nodes();
  }
  else
  {
    dagSaver.start_save_node_raw(NULL, 0, 1);
    dagSaver.start_save_children();
    saveBigMeshNodeHier(rootNode, materials);
    dagSaver.end_save_children();
    dagSaver.end_save_node_raw();
  }
}

static void writeMeshToDag(DagSaver &dagSaver, Mesh &mesh, bool saveNormals)
{
  DagBigMapChannel mapChannel[NUMMESHTCH];
  int mapChannelCount = 0;
  TVertTab tvert[NUMMESHTCH];
  memset(mapChannel, 0, sizeof(mapChannel));
  for (int mapChannelId = 0; mapChannelId < NUMMESHTCH; ++mapChannelId)
  {
    if (!mesh.getTFace(mapChannelId).size())
      continue;
    mapChannel[mapChannelCount].numtv = mesh.getTVert(mapChannelId).size();
    mapChannel[mapChannelCount].num_tv_coords = 2;
    mapChannel[mapChannelCount].channel_id = mapChannelCount + 1;
    tvert[mapChannelCount] = mesh.getTVert(mapChannelId);
    for (int tvertId = tvert[mapChannelCount].size(); --tvertId >= 0;)
      tvert[mapChannelCount][tvertId].y = -tvert[mapChannelCount][tvertId].y;
    mapChannel[mapChannelCount].tverts = &tvert[mapChannelCount][0].x;
    mapChannel[mapChannelCount].tface = (DagBigTFace *)&mesh.getTFace(mapChannelId)[0];
    ++mapChannelCount;
  }

  dag::ConstSpan<Face> face = mesh.getFace();
  Tab<DagBigFace> dagBigFace(tmpmem);
  dagBigFace.resize(face.size());

  for (int faceId = 0; faceId < face.size(); ++faceId)
  {
    dagBigFace[faceId].v[0] = face[faceId].v[0];
    dagBigFace[faceId].v[1] = face[faceId].v[1];
    dagBigFace[faceId].v[2] = face[faceId].v[2];
    dagBigFace[faceId].smgr = mesh.getFaceSMGR(faceId);
    dagBigFace[faceId].mat = mesh.getFaceMaterial(faceId);
  }

  if (!saveNormals)
  {
    dagSaver.save_dag_bigmesh(mesh.getVert().size(), &mesh.getVert()[0], dagBigFace.size(), &dagBigFace[0], mapChannelCount,
      mapChannel);
  }
  else
  {
    Tab<DagBigTFace> tFace(tmpmem);
    tFace.resize(face.size());
    for (int i = 0; i < face.size(); i++)
    {
      FaceNGr &f = mesh.facengr[i];
      for (int j = 0; j < 3; j++)
        tFace[i].t[j] = f[j];
    }

    dagSaver.save_dag_bigmesh(mesh.getVert().size(), &mesh.getVert()[0], dagBigFace.size(), &dagBigFace[0], mapChannelCount,
      mapChannel, NULL, &tFace[0], &mesh.getVertNorm()[0], mesh.getVertNorm().size());
  }
}

void DagExporter::saveBigMeshNode(Node *node, const PtrTab<MaterialData> &materials)
{
  if (!node)
    return;

  int flags = 0;
  if (node->flags & NODEFLG_RENDERABLE)
    flags |= IMP_NF_RENDERABLE;
  if (node->flags & NODEFLG_CASTSHADOW)
    flags |= IMP_NF_CASTSHADOW;
  if (node->flags & NODEFLG_RCVSHADOW)
    flags |= IMP_NF_RCVSHADOW;
  if (node->flags & NODEFLG_POINTCLOUD)
    flags |= IMP_NF_POINTCLOUD;

  if (!dagSaver.start_save_node(node->name, node->wtm, flags))
    goto process_children;

  if (node->script)
    dagSaver.save_node_script(node->script);

  if (node->obj && node->obj->isSubOf(OCID_MESHHOLDER))
  {
    const MeshHolderObj *curObject = static_cast<MeshHolderObj *>(node->obj);

    if (curObject->mesh)
    {
      saveNodeMaterials(node, materials);
      writeMeshToDag(dagSaver, *curObject->mesh, saveNormals);
    }
  }
  dagSaver.end_save_node();

process_children:
  for (int nodeId = node->child.size(); --nodeId >= 0;)
    saveBigMeshNode(node->child[nodeId], materials);
}

void DagExporter::saveBigMeshNodeHier(Node *node, const PtrTab<MaterialData> &materials)
{
  int flags = 0;
  if (node->flags & NODEFLG_RENDERABLE)
    flags |= IMP_NF_RENDERABLE;
  if (node->flags & NODEFLG_CASTSHADOW)
    flags |= IMP_NF_CASTSHADOW;
  if (node->flags & NODEFLG_RCVSHADOW)
    flags |= IMP_NF_RCVSHADOW;
  if (node->flags & NODEFLG_POINTCLOUD)
    flags |= IMP_NF_POINTCLOUD;

  dagSaver.start_save_node_raw(node->name, flags, node->child.size());

  if (node->script)
    dagSaver.save_node_script(node->script);

  if (node->obj && node->obj->isSubOf(OCID_MESHHOLDER))
  {
    const MeshHolderObj *curObject = static_cast<MeshHolderObj *>(node->obj);

    if (curObject->mesh)
    {
      saveNodeMaterials(node, materials);
      writeMeshToDag(dagSaver, *curObject->mesh, saveNormals);
    }
  }
  dagSaver.save_node_tm(node->tm);

  if (node->child.size())
  {
    dagSaver.start_save_children();
    for (int i = 0; i < node->child.size(); i++)
      saveBigMeshNodeHier(node->child[i], materials);
    dagSaver.end_save_children();
  }
  dagSaver.end_save_node_raw();
}

bool DagExporter::exportGeometry(const char *fileName, Node *rootNode, bool slot_tex, bool preserve_hier, bool save_normal)
{
  if (!fileName || !rootNode)
    return false;
  PtrTab<MaterialData> materials(tmpmem);
  Tab<TEXTUREID> textures(tmpmem);

  location = fileName;
  location_from_path(location);

  saveNormals = save_normal;

  getMaterialsAndTextures(rootNode, materials, textures);

  dagSaver.start_save_dag((char *)fileName);
  saveMaterialsAndTextures(materials, textures, slot_tex);
  saveBigMeshNodes(rootNode, materials, preserve_hier);
  dagSaver.end_save_dag();

  saveNormals = false;
  return true;
}
