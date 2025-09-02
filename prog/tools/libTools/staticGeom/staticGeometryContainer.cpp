// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>

#include <generic/dag_smallTab.h>
#include <libTools/util/iLogWriter.h>

#include <util/dag_globDef.h>
#include <util/dag_bitArray.h>
#include <util/dag_oaHashNameMap.h>

#include <stdlib.h>


void StaticGeometryContainer::clear()
{
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i])
      delete (nodes[i]);
  clear_and_shrink(nodes);
}


void StaticGeometryContainer::applyNodesLighting()
{
  Tab<StaticGeometryMaterial *> createdMats(tmpmem);

  for (int ni = 0; ni < nodes.size(); ++ni)
  {
    StaticGeometryNode *node = nodes[ni];

    if (node && node->mesh && node->lighting != StaticGeometryNode::LIGHT_DEFAULT)
    {
      for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
      {
        StaticGeometryMaterial *mat = node->mesh->mats[mi];
        if (!mat)
          continue;
        StaticGeometryMaterial curMat(*mat);

        node->setMaterialLighting(curMat);

        StaticGeometryMaterial *newMat = NULL;

        for (int cmi = 0; cmi < createdMats.size(); ++cmi)
          if (*createdMats[cmi] == curMat)
          {
            newMat = createdMats[cmi];
            break;
          }

        if (!newMat)
        {
          newMat = new (tmpmem) StaticGeometryMaterial(curMat);
          G_ASSERT(newMat);

          createdMats.push_back(newMat);
        }

        node->mesh->mats[mi] = newMat;
      }
    }
  }
}


bool StaticGeometryContainer::optimize(bool faces, bool materials)
{
  // remove double faces
  bool wasOpt = false;
  if (faces)
  {
    Tab<Tabint> faceMap(tmpmem);

    for (int ni = 0; ni < nodes.size(); ++ni)
    {
      const StaticGeometryNode *node = nodes[ni];

      if (node && node->mesh)
      {
        Mesh &nodeMesh = node->mesh->mesh;
        SmallTab<Face, TmpmemAlloc> mfaces;
        mfaces = nodeMesh.getFace();

        // set vertex indexes in faces in ascending order
        for (int fi = 0; fi < mfaces.size(); ++fi)
        {
          Face &face = mfaces[fi];

          int m = ::min(::min(face.v[0], face.v[1]), face.v[2]);
          if (m == face.v[1])
          {
            m = face.v[0];
            face.v[0] = face.v[1];
            face.v[1] = face.v[2];
            face.v[2] = m;
          }
          else if (m == face.v[2])
          {
            face.v[2] = face.v[1];
            face.v[1] = face.v[0];
            face.v[0] = m;
          }
        }

        faceMap.clear();

        if (nodeMesh.get_vert2facemap(faceMap))
        {
          Bitarray erase(tmpmem);

          erase.resize(mfaces.size());

          for (int vi = 0; vi < faceMap.size(); ++vi)
          {
            const Tab<int> &faces = faceMap[vi];

            for (int fi1 = 0; fi1 < (int)faces.size() - 1; ++fi1)
            {
              if (erase.get(faces[fi1]))
                continue;
              for (int fi2 = fi1 + 1; fi2 < faces.size(); ++fi2)
              {
                if (erase.get(faces[fi2]))
                  continue;

                const Face &f1 = mfaces[faces[fi1]];
                const Face &f2 = mfaces[faces[fi2]];

                if (f1.v[0] == f2.v[0] && f1.mat == f2.mat)
                {
                  // idential faces
                  if (f1.v[1] == f2.v[1] && f1.v[2] == f2.v[2])
                  {
                    erase.set(faces[fi2]);
                  } // can checl backfaces
                }
              }
            }
          }

          for (int fi = mfaces.size() - 1; fi >= 0; --fi)
          {
            if (erase[fi])
            {
              wasOpt = true;
              nodeMesh.removeFacesFast(fi, 1);
            }
          }

          nodeMesh.calc_facenorms();
        }
      }
    }
  }

  // remove unused node materials
  bool nodeMaterials = materials & faces;
  if (nodeMaterials)
  {
    Tab<int> nodeMaterialsMap(tmpmem);

    for (int ni = 0; ni < nodes.size(); ++ni)
    {
      const StaticGeometryNode *node = nodes[ni];

      if (node && node->mesh && node->mesh->mats.size())
      {
        nodeMaterialsMap.resize(node->mesh->mats.size());
        for (int mi = 0; mi < nodeMaterialsMap.size(); ++mi)
          nodeMaterialsMap[mi] = -1;
        Mesh &mesh = node->mesh->mesh;

        // find used materials
        for (int fi = 0; fi < mesh.getFace().size(); ++fi)
        {
          int mat = mesh.getFaceMaterial(fi);
          if (mat >= nodeMaterialsMap.size())
            mat = nodeMaterialsMap.size() - 1;
          nodeMaterialsMap[mat] = mat;
        }

        int materialsErased = 0;
        for (int mi = 0; mi < nodeMaterialsMap.size(); ++mi)
        {
          if (nodeMaterialsMap[mi] < 0)
          {
            materialsErased++;
          }
          else
          {
            nodeMaterialsMap[mi] -= materialsErased;
          }
        }
        for (int mi = nodeMaterialsMap.size() - 1; mi >= 0; --mi)
        {
          if (nodeMaterialsMap[mi] < 0)
          {
            erase_items(node->mesh->mats, mi, 1);
            wasOpt = true;
          }
        }

        for (int fi = 0; fi < mesh.getFace().size(); ++fi)
        {
          int mat = mesh.getFaceMaterial(fi);
          if (mat >= nodeMaterialsMap.size())
            mat = nodeMaterialsMap.size() - 1;
          G_ASSERT(nodeMaterialsMap[mat] >= 0);
          mesh.setFaceMaterial(fi, nodeMaterialsMap[mat]);
        }
      }
    }
  }

  // remove unused and double materials
  if (materials)
  {
    PtrTab<StaticGeometryMaterial> materials(tmpmem);

    for (int ni = 0; ni < nodes.size(); ++ni)
    {
      const StaticGeometryNode *node = nodes[ni];

      if (node && node->mesh)
      {
        Mesh &mesh = node->mesh->mesh;
        PtrTab<StaticGeometryMaterial> &mats = node->mesh->mats;

        // register used materials
        Bitarray usedMats;
        usedMats.resize(mats.size());

        for (int fi = 0; fi < mesh.getFace().size(); ++fi)
        {
          int mat = mesh.getFaceMaterial(fi);
          if (mat >= mats.size())
            mat = mats.size() - 1;
          usedMats.set(mat);
        }

        // delete unused materials and find double materials
        for (int mi = 0; mi < mats.size(); ++mi)
        {
          if (!usedMats[mi])
          {
            wasOpt = true;
            mats[mi] = NULL;
          }
          else
          {
            bool found = false;
            for (int i = 0; i < materials.size(); ++i)
            {
              if (mats[mi]->equal(*materials[i]))
              {
                mats[mi] = materials[i];
                found = true;
                break;
              }
            }

            if (!found)
              materials.push_back(mats[mi]);
          }
        }
      }
    }
  }

  return wasOpt;
}


void StaticGeometryContainer::markNonTopLodNodes(ILogWriter *log, const char *filename)
{
  static FastNameMap foundTopLodes, missingTopLodes;

  for (int i = nodes.size() - 1; i >= 0; i--)
  {
    const char *toplod = nodes[i]->topLodName.length() ? (const char *)nodes[i]->topLodName : NULL;

    nodes[i]->flags &= ~StaticGeometryNode::FLG_NON_TOP_LOD;

    if (toplod && strcmp(toplod, nodes[i]->name) != 0)
    {
      if (foundTopLodes.getNameId(toplod) != -1)
        nodes[i]->flags |= StaticGeometryNode::FLG_NON_TOP_LOD;
      else if (missingTopLodes.getNameId(toplod) != -1)
        ; // do nothing, topLodName reference is invalid
      else
      {
        bool found = false;
        for (int j = nodes.size() - 1; j >= 0; j--)
          if (strcmp(toplod, nodes[j]->name) == 0)
          {
            found = true;
            break;
          }

        if (found)
          foundTopLodes.addNameId(toplod);
        else
        {
          missingTopLodes.addNameId(toplod);
          if (log)
            log->addMessage(ILogWriter::ERROR, "Incorrect topLodName <%s> in node <%s> in DAG:\n  <%s>", toplod, nodes[i]->name.str(),
              filename);
        }
      }
    }
  }
  foundTopLodes.reset();
  missingTopLodes.reset();
}
