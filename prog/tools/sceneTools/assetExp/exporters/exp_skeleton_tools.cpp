#include "exp_skeleton_tools.h"

#include <stdio.h>

#include <math/dag_Point3.h>
#include <math/dag_mathUtils.h>

#include <ioSys/dag_dataBlock.h>

#include <generic/dag_staticTab.h>
#include <generic/dag_tab.h>

#include <memory/dag_framemem.h>

#include <osApiWrappers/dag_direct.h>

#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>

#include <libTools/dagFileRW/nodeTreeBuilder.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/normalizeNodeTm.h>
#include <libTools/dagFileRW/geomMeshHelper.h>
#include <libTools/util/iLogWriter.h>

#include "exp_tools.h"

static bool remove_dm_suffix(SimpleString &str)
{
  const int len = str.length();
  if (len >= 3 && strcmp(str.str() + len - 3, "_dm") == 0)
  {
    str = SimpleString(str.str(), len - 3);
    return true;
  }
  else
    return false;
}

static void remove_dm_suffix(Node *node)
{
  remove_dm_suffix(node->name);
  for (auto child : node->child)
    remove_dm_suffix(child);
}

static bool has_child_with_name(Node *root, const char *name)
{
  for (auto child : root->child)
    if (stricmp(child->name, name) == 0)
      return true;
  return false;
}

static void combine_with(Node *node, Node *other_root)
{
  if (!node || !other_root)
    return;

  for (auto child : node->child)
  {
    Node *otherNode = other_root->find_inode(child->name);
    if (otherNode && ((!otherNode->parent && !child->parent) || //-V522
                       dd_stricmp(otherNode->parent->name, child->parent->name) == 0))
    {
      for (int i = 0; i < otherNode->child.size(); i++)
        if (!has_child_with_name(child, otherNode->child[i]->name))
        {
          otherNode->child[i]->parent = child;
          child->child.push_back(otherNode->child[i]);
          erase_items(otherNode->child, i, 1);
          i--;
        }
    }

    combine_with(child, other_root);
  }
}

static void remove_dm_suffix_from_mirrored_nodes(Node *dm_node, Node *node)
{
  SimpleString strWithoutSuffix = dm_node->name;
  if (remove_dm_suffix(strWithoutSuffix) && node->find_node(strWithoutSuffix.str()) != nullptr)
    dm_node->name = strWithoutSuffix;
  for (auto dmChild : dm_node->child)
    remove_dm_suffix_from_mirrored_nodes(dmChild, node);
}

static void calc_bounding_bbox(const DagorAsset *asset, const AScene &dag_scene, const GeomMeshHelperDagObject &dag_obj,
  BBox3 &out_bbox)
{
  DataBlock dagNodeScriptBlk;
  Node *dagNode = nullptr;
  if (dag_scene.root)
  {
    dagNode = dag_scene.root->find_node(dag_obj.name);
    if (dagNode)
      dblk::load_text(dagNodeScriptBlk, make_span_const(dagNode->script), dblk::ReadFlag::ROBUST, asset->getTargetFilePath());
  }

  bool forceBoxCollision = asset->props.getBool("forceBoxCollision", false);

  bool treeCapsule = false;
  if (!dd_strnicmp(dag_obj.name, "_Clip", i_strlen("_Clip")))
    ;
  else if (forceBoxCollision)
    ;
  else if (dagNode)
  {
    const char *collision = dagNodeScriptBlk.getStr("collision", NULL);
    if (collision)
      if (!stricmp(collision, "tree_capsule"))
        treeCapsule = true;
  }

  // calculate node bounding box.
  out_bbox.setempty();
  if (treeCapsule)
  {
    float treeRad = dagNodeScriptBlk.getReal("tree_radius", 0.1);
    out_bbox[0].x = -treeRad;
    out_bbox[0].z = -treeRad;
    out_bbox[1].x = treeRad;
    out_bbox[1].z = treeRad;
  }
  else
  {
    const Tab<Point3> &verts = dag_obj.mesh.verts;
    for (unsigned int vertexNo = 0; vertexNo < verts.size(); vertexNo++)
      out_bbox += verts[vertexNo];
  }
}

static Point2 get_collision_box_bounding_planes(const DagorAsset *asset, const AScene &dag_scene,
  const GeomMeshHelperDagObject &dag_obj, const Point3 &axis)
{
  const Point3 axisLocal = inverse(dag_obj.wtm) % axis;
  BBox3 modelBBox;
  calc_bounding_bbox(asset, dag_scene, dag_obj, modelBBox);
  return Point2(axis * dag_obj.wtm.getcol(3) - get_box_bounding_plane_dist(modelBBox, -axisLocal),
    axis * dag_obj.wtm.getcol(3) + get_box_bounding_plane_dist(modelBBox, axisLocal));
}

inline static float get_node_dist(const DagorAsset *asset, const AScene &dag_scene, const GeomMeshHelperDagObject &dag_obj,
  const Point3 &axis, float side)
{
  const Point2 nodeRange = get_collision_box_bounding_planes(asset, dag_scene, dag_obj, axis);
  const float width = nodeRange.y - nodeRange.x;
  const float middle = 0.5f * (nodeRange.x + nodeRange.y);
  return middle + 0.5f * width * side;
}

static const GeomMeshHelperDagObject *find_dag_obj_by_name(dag::ConstSpan<GeomMeshHelperDagObject> dag_objects_list, const char *name)
{
  for (int i = 0; i < dag_objects_list.size(); ++i)
    if (strcmp(dag_objects_list[i].name, name) == 0)
      return &dag_objects_list[i];
  return nullptr;
}

struct CuttingPlane
{
  Point3 axis;
  bool positive;
  float dist;
};

static void load_cutting_planes_from_blk(const DataBlock &blk, const DagorAsset *asset, const AScene &dag_scene,
  dag::ConstSpan<GeomMeshHelperDagObject> dag_obj_list, const GeomMeshHelperDagObject *default_dag_obj,
  StaticTab<CuttingPlane, 128> &out_cutting_planes)
{
  int axisIndex = 1;
  while (true)
  {
    if (out_cutting_planes.size() >= out_cutting_planes.capacity())
      return;
    int axisParamNum = blk.findParam(String(16, "axis%d", axisIndex).str());
    if (axisParamNum >= 0)
    {
      const Point3 axis = blk.getPoint3(axisParamNum);
      const bool positive = blk.getBool(String(16, "positive%d", axisIndex).str(), true);
      const float side = blk.getReal(String(16, "side%d", axisIndex).str(), -1.0f);
      float nodeDistMax = side > 0.0f ? -VERY_BIG_NUMBER : VERY_BIG_NUMBER;
      bool axisDmIsGiven = false;
      bool axisDmIsFound = false;
      int axisDmParamNum = -1;
      while (true)
      {
        axisDmParamNum = blk.findParam(String(16, "dm%d", axisIndex).str(), axisDmParamNum);
        if (axisDmParamNum >= 0)
        {
          axisDmIsGiven = true;
          const char *axisDmPartName = blk.getStr(axisDmParamNum);
          if (strchr(axisDmPartName, '%') != NULL)
          {
            int index = 1;
            while (true)
            {
              const String axisDmPartFullName(64, axisDmPartName, index);
              if (const GeomMeshHelperDagObject *axisDmPartDagObj = find_dag_obj_by_name(dag_obj_list, axisDmPartFullName.str()))
              {
                const float nodeDist = get_node_dist(asset, dag_scene, *axisDmPartDagObj, axis, side);
                nodeDistMax = side > 0.0f ? max(nodeDistMax, nodeDist) : min(nodeDistMax, nodeDist);
                axisDmIsFound = true;
                ++index;
              }
              else
                break;
            }
          }
          else
          {
            if (const GeomMeshHelperDagObject *axisDmPartDagObj = find_dag_obj_by_name(dag_obj_list, axisDmPartName))
            {
              const float nodeDist = get_node_dist(asset, dag_scene, *axisDmPartDagObj, axis, side);
              nodeDistMax = side > 0.0f ? max(nodeDistMax, nodeDist) : min(nodeDistMax, nodeDist);
              axisDmIsFound = true;
            }
          }
        }
        else
          break;
      }

      if (!axisDmIsGiven)
      {
        nodeDistMax = get_node_dist(asset, dag_scene, *default_dag_obj, axis, side);
        axisDmIsFound = true;
      }
      ++axisIndex;

      if (axisDmIsFound)
      {
        CuttingPlane &cuttingPlane = out_cutting_planes.push_back();
        cuttingPlane.axis = axis;
        cuttingPlane.positive = positive;
        cuttingPlane.dist = nodeDistMax;
      }
    }
    else
      break;
  }
}

class FindNodesWithIndex : public Node::NodeEnumCB
{
  const char *name;
  Tab<Node *> &nodes;
  Tab<int> &indexes;
  virtual int node(Node &c)
  {
    int index = -1;
    if (sscanf(c.name.str(), name, &index) == 1 && strcmp(String(32, name, index).str(), c.name.str()) == 0)
      if (find_value_idx(indexes, index) < 0)
      {
        nodes.push_back(&c);
        indexes.push_back(index);
      }
    return 0;
  }

public:
  FindNodesWithIndex(const char *in_name, Tab<Node *> &in_nodes, Tab<int> &in_indexes) :
    name(in_name), nodes(in_nodes), indexes(in_indexes)
  {}
};

static void calc_tm(Node *node)
{
  if (node->parent)
    node->tm = inverse(node->parent->wtm) * node->wtm;
  else
    node->tm = node->wtm;
  for (int i = 0; i < node->child.size(); ++i)
    calc_tm(node->child[i]);
}

static void remove_suffix(char *buff, const char *str, const char *suffix)
{
  int suffixLen = i_strlen(suffix);
  if (suffixLen > 0)
  {
    if (const char *suffixPos = strstr(str, suffix))
    {
      if (buff == str)
        memmove(buff + (suffixPos - str), suffixPos + suffixLen, strlen(suffixPos + suffixLen) + 1);
      else
      {
        strncpy(buff, str, suffixPos - str);
        strcpy(buff + (suffixPos - str), suffixPos + suffixLen);
      }
      return;
    }
  }
  if (buff != str)
    strcpy(buff, str);
}

static void reset_hierarchy_auto_complete(Node *node)
{
  node->tmpval = 0;
  for (int i = 0; i < node->child.size(); ++i)
    reset_hierarchy_auto_complete(node->child[i]);
}

static bool add_dependencies(const DataBlock &blk, const char *suffix_to_remove, Node *node, DagorAssetMgr &asset_mgr, ILogWriter &log)
{
  if (node == nullptr)
    return false;

  const char *collisionNameRaw = blk.getStr("collisionName", nullptr);
  if (collisionNameRaw == nullptr)
  {
    log.addMessage(log.ERROR, "collisionName is not specified");
    return false;
  }
  char collisionName[128];
  remove_suffix(collisionName, collisionNameRaw, suffix_to_remove);
  remove_suffix(collisionName, collisionName, blk.getStr("collisionSuffixToRemove", ""));
  DagorAsset *dmCollisionAsset = asset_mgr.findAsset(collisionName, asset_mgr.getAssetTypeId("collision"));
  if (dmCollisionAsset == nullptr)
  {
    log.addMessage(log.NOTE, "cannot read collision from %s", collisionName);
    return false;
  }

  String filePath(dmCollisionAsset->getTargetFilePath());
  Tab<GeomMeshHelperDagObject> dagObjectsList(tmpmem);
  AScene dagScene;
  if (stricmp(::dd_get_fname_ext(filePath), ".dag") == 0)
  {
    if (!import_geom_mesh_helpers_dag(filePath, dagObjectsList))
    {
      log.addMessage(log.ERROR, "%s: cannot read geometry from %s", dmCollisionAsset->getName(), filePath);
      return false;
    }
    load_ascene(filePath, dagScene, LASF_NOMATS);
  }
  else
    read_data_from_blk(dagObjectsList, dmCollisionAsset->props);

  bool res = false;

  reset_hierarchy_auto_complete(node);

  const int dpNameId = blk.getNameId("dp");
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *dpBlk = blk.getBlock(i);
    if (dpBlk->getBlockNameId() == dpNameId)
      if (const char *parentDmPartName = dpBlk->getStr("dm", NULL))
      {
        const bool overwriteLinks = dpBlk->getBool("overwriteLinks", false);
        if (const GeomMeshHelperDagObject *parentDmPartDagObj = find_dag_obj_by_name(dagObjectsList, parentDmPartName))
        {
          Node *parentNode = node->find_node(parentDmPartName);
          if (parentNode == nullptr)
          {
            SimpleString parentDmPartNameWithoutSuffix(parentDmPartName);
            remove_dm_suffix(parentDmPartNameWithoutSuffix);
            parentNode = node->find_node(parentDmPartNameWithoutSuffix.str());

            const bool separateDm = dpBlk->getBool("separateDm", false);
            const bool standAloneDm = dpBlk->getBool("standAloneDm", false);

            if (parentNode != nullptr && !standAloneDm)
            {
              if (separateDm)
              {
                if (Node *parentParentNode = parentNode->parent)
                {
                  Node *dmParentNode = new Node();
                  dmParentNode->name = parentDmPartName;
                  dmParentNode->wtm = parentDmPartDagObj->wtm;
                  dmParentNode->parent = parentParentNode;
                  dmParentNode->tm = inverse(dmParentNode->parent->wtm) * dmParentNode->wtm;
                  dmParentNode->child.push_back(parentNode);

                  erase_item_by_value(parentParentNode->child, parentNode);
                  parentParentNode->child.push_back(dmParentNode);

                  parentNode->parent = dmParentNode;
                  parentNode->tm = inverse(parentNode->parent->wtm) * parentNode->wtm;
                  parentNode = dmParentNode;

                  res = true;
                }
              }
            }
            else
            {
              Node *dmParentNode = new Node();
              dmParentNode->name = parentDmPartName;
              dmParentNode->parent = node;
              dmParentNode->wtm = parentDmPartDagObj->wtm;
              dmParentNode->tm = inverse(dmParentNode->parent->wtm) * dmParentNode->wtm;
              node->child.push_back(dmParentNode);
              parentNode = dmParentNode;

              res = true;
            }
          }

          {
            // planes to check
            StaticTab<CuttingPlane, 128> cuttingPlanes;
            load_cutting_planes_from_blk(*dpBlk, dmCollisionAsset, dagScene, dagObjectsList, parentDmPartDagObj, cuttingPlanes);

            // all possible dependent dm parts
            Tab<const GeomMeshHelperDagObject *> dmPartsDagObjs(framemem_ptr());
            // all dependent anim nodes, anim nodes are not checked against cutting planes
            Tab<Node *> dpAnimNodes(framemem_ptr());
            // collect
            for (int j = 0; j < dpBlk->paramCount(); ++j)
              if (dpBlk->getParamNameId(j) == dpNameId)
              {
                const char *dpName = dpBlk->getStr(j);
                if (strchr(dpName, '%') != nullptr)
                {
                  // dm parts
                  Tab<int> partIndexes(framemem_ptr());
                  for (int j = 0; j < dagObjectsList.size(); ++j)
                  {
                    const GeomMeshHelperDagObject &dagObj = dagObjectsList[j];
                    const char *nodeName = dagObj.name.str();
                    int index = 0;
                    if (sscanf(nodeName, dpName, &index) == 1 && strcmp(String(32, dpName, index).str(), nodeName) == 0)
                      if (find_value_idx(partIndexes, index) < 0)
                      {
                        dmPartsDagObjs.push_back(&dagObj);
                        partIndexes.push_back(index);
                      }
                  }
                  if (cuttingPlanes.empty()) // no cutting planes - add anim nodes
                  {
                    FindNodesWithIndex findNodesWithIndex(dpName, dpAnimNodes, partIndexes); // anim parts, _dm
                    node->enum_nodes(findNodesWithIndex);

                    SimpleString dpNameWithoutSuffix(dpName);
                    remove_dm_suffix(dpNameWithoutSuffix);
                    FindNodesWithIndex findNodesWithIndexWithoutSuffix(dpNameWithoutSuffix.str(), dpAnimNodes, partIndexes); // anim
                                                                                                                             // parts
                    node->enum_nodes(findNodesWithIndexWithoutSuffix);
                  }
                }
                else
                {
                  if (const GeomMeshHelperDagObject *dmPartDagObj = find_dag_obj_by_name(dagObjectsList, dpName)) // dm part
                    dmPartsDagObjs.push_back(dmPartDagObj);
                  else if (cuttingPlanes.empty()) // no cutting planes - add anim node
                  {
                    if (Node *animNode = node->find_node(dpName)) // anim node, _dm
                      dpAnimNodes.push_back(animNode);
                    else
                    {
                      SimpleString dpNameWithoutSuffix(dpName);
                      remove_dm_suffix(dpNameWithoutSuffix);
                      if (Node *animNode = node->find_node(dpNameWithoutSuffix.str())) // anim node
                        dpAnimNodes.push_back(animNode);
                    }
                  }
                }
              }

            // check and connect dependent dm parts from blk
            Tab<const GeomMeshHelperDagObject *> dpPartsDagObjs(framemem_ptr());
            for (int j = 0; j < dmPartsDagObjs.size(); ++j)
            {
              const GeomMeshHelperDagObject *dagObj = dmPartsDagObjs[j];
              bool dpIsValid = true;
              if (!cuttingPlanes.empty())
              {
                BBox3 bbox;
                calc_bounding_bbox(dmCollisionAsset, dagScene, *dagObj, bbox);
                for (int k = 0; k < cuttingPlanes.size(); ++k)
                {
                  bool dpAxisIsValid;
                  if (!cuttingPlanes[k].positive)
                  {
                    dpAxisIsValid = true;
                    for (int l = 0; l < 8; ++l)
                    {
                      const Point3 bboxVertex = dagObj->wtm * bbox.point(l);
                      const float proj = bboxVertex * cuttingPlanes[k].axis;
                      if (proj > cuttingPlanes[k].dist)
                      {
                        dpAxisIsValid = false;
                        break;
                      }
                    }
                  }
                  else
                  {
                    dpAxisIsValid = false;
                    for (int l = 0; l < 8; ++l)
                    {
                      const Point3 bboxVertex = dagObj->wtm * bbox.point(l);
                      const float proj = bboxVertex * cuttingPlanes[k].axis;
                      if (proj > cuttingPlanes[k].dist)
                      {
                        dpAxisIsValid = true;
                        break;
                      }
                    }
                  }
                  if (!dpAxisIsValid)
                  {
                    dpIsValid = false;
                    break;
                  }
                }
              }
              if (dpIsValid)
                dpPartsDagObjs.push_back(dagObj);
            }

            // link dm nodes
            for (int j = 0; j < dpPartsDagObjs.size(); ++j)
            {
              const GeomMeshHelperDagObject *dpPartDagObj = dpPartsDagObjs[j];
              Node *childNode = node->find_node(dpPartDagObj->name.str());
              if (childNode == nullptr)
              {
                SimpleString dpPartDagObjNameWithoutSuffix(dpPartDagObj->name);
                remove_dm_suffix(dpPartDagObjNameWithoutSuffix);
                childNode = node->find_node(dpPartDagObjNameWithoutSuffix.str());
              }
              if (childNode != nullptr)
              {
                if (childNode == parentNode)
                  continue;
                else if (childNode->parent == nullptr)
                  continue;
                else if (childNode->parent == parentNode)
                  continue;
                else if (childNode->parent->parent != nullptr && (childNode->tmpval == 0 || !overwriteLinks))
                  continue;
                else if (parentNode->parent == childNode)
                  continue;
                erase_item_by_value(childNode->parent->child, childNode);
              }
              else
              {
                Node *dmChildNode = new Node();
                dmChildNode->name = dpPartDagObj->name;
                dmChildNode->wtm = dpPartDagObj->wtm;
                childNode = dmChildNode;
              }
              childNode->tmpval = 1;
              childNode->parent = parentNode;
              childNode->tm = inverse(childNode->parent->wtm) * childNode->wtm;
              childNode->parent->child.push_back(childNode);

              res = true;
            }

            // link anim nodes
            for (int j = 0; j < dpAnimNodes.size(); ++j)
            {
              Node *childNode = dpAnimNodes[j];
              if (childNode->parent == nullptr)
                continue;
              if (childNode->parent == parentNode)
                continue;
              else if (childNode->parent->parent != nullptr && (childNode->tmpval == 0 || !overwriteLinks))
                continue;
              else if (parentNode->parent == childNode)
                continue;
              childNode->tmpval = 1;
              erase_item_by_value(childNode->parent->child, childNode);
              childNode->parent = parentNode;
              childNode->tm = inverse(childNode->parent->wtm) * childNode->wtm;
              childNode->parent->child.push_back(childNode);

              res = true;
            }
          }
        }
      }
  }
  return res;
}

bool load_skeleton(DagorAsset &a, const char *suffix_to_remove, int flags, ILogWriter &log, AScene &scene)
{
  if (const char *skeletonNameRaw = a.props.getStr("skeletonName", NULL))
  {
    char skeletonName[128];
    remove_suffix(skeletonName, skeletonNameRaw, suffix_to_remove);
    if (DagorAsset *dmTreeAsset = a.getMgr().findAsset(skeletonName, a.getMgr().getAssetTypeId("skeleton")))
    {
      if (!load_ascene(dmTreeAsset->getTargetFilePath(), scene, flags, false))
        log.addMessage(ILogWriter::WARNING, "cannot load skeleton: %s", skeletonName);
      else if (scene.root != nullptr)
      {
        scene.root->calc_wtm();
        return true;
      }
    }
  }
  return false;
}

bool combine_skeleton(DagorAsset &a, int flags, ILogWriter &log, Node *root)
{
  if (root == nullptr)
    return false;
  if (const char *combineSkeletonName = a.props.getStr("combineWithSkeleton", NULL))
  {
    if (DagorAsset *dmTreeAsset = a.getMgr().findAsset(combineSkeletonName, a.getMgr().getAssetTypeId("skeleton")))
    {
      AScene combineWithScene;
      if (!load_ascene(dmTreeAsset->getTargetFilePath(), combineWithScene, flags, false))
        log.addMessage(ILogWriter::WARNING, "cannot load skeleton: %s", combineSkeletonName);

      remove_dm_suffix(combineWithScene.root);
      combine_with(root, combineWithScene.root);
      return true;
    }
    else
      log.addMessage(ILogWriter::WARNING, "can't get skeleton asset <%s>", combineSkeletonName);
  }
  return false;
}

bool auto_complete_skeleton(DagorAsset &a, const char *suffix_to_remove, int flags, ILogWriter &log, Node *root)
{
  if (root == nullptr)
    return false;
  bool res = false;
  if (const DataBlock *autoCompleteBlk = a.props.getBlockByName("autoComplete"))
  {
    if (const char *dmSkeletonNameRaw = autoCompleteBlk->getStr("dmSkeletonName", NULL))
    {
      char dmSkeletonName[128];
      remove_suffix(dmSkeletonName, dmSkeletonNameRaw, suffix_to_remove);
      remove_suffix(dmSkeletonName, dmSkeletonName, autoCompleteBlk->getStr("dmSkeletonSuffixToRemove", ""));
      if (DagorAsset *dmTreeAsset = a.getMgr().findAsset(dmSkeletonName, a.getMgr().getAssetTypeId("skeleton")))
      {
        AScene combineWithDmScene;
        if (!load_ascene(dmTreeAsset->getTargetFilePath(), combineWithDmScene, flags, false))
          log.addMessage(ILogWriter::WARNING, "cannot load skeleton: %s", dmSkeletonName);

        combineWithDmScene.root->calc_wtm();
        remove_dm_suffix_from_mirrored_nodes(combineWithDmScene.root, root);
        combine_with(root, combineWithDmScene.root);
        res = true;
      }
      else
        log.addMessage(ILogWriter::WARNING, "can't get skeleton asset <%s>", dmSkeletonName);
    }
    if (add_dependencies(*autoCompleteBlk, suffix_to_remove, root, a.getMgr(), log))
    {
      debug("skeleton \"%s\" has been auto-completed", a.getName());
      res = true;
    }

    calc_tm(root);
  }

  return res;
}