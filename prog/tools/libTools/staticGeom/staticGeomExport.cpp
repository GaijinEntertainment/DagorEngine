#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>

#include <libTools/util/strUtil.h>

#include <libTools/dagFileRW/dagFileExport.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>


void StaticGeometryContainer::exportDag(const char *path, bool make_tex_path_local) const
{
  DagSaver dag;
  dag.start_save_dag(path);
  Tab<StaticGeometryMaterial *> materials(tmpmem);
  Tab<StaticGeometryTexture *> textures(tmpmem);
  int j;

  for (j = 0; j < nodes.size(); ++j)
  {
    if (!nodes[j]->mesh.get())
      continue;
    for (int nodeMat = 0; nodeMat < nodes[j]->mesh->mats.size(); ++nodeMat)
    {
      StaticGeometryMaterial *material = nodes[j]->mesh->mats[nodeMat];

      for (int matId = 0; matId < materials.size(); ++matId)
        if (materials[matId] == material)
        {
          material = NULL;
          break;
        }

      if (!material)
        continue;

      materials.push_back(material);

      for (int textureIndex = 0; textureIndex < StaticGeometryMaterial::NUM_TEXTURES; ++textureIndex)
      {
        StaticGeometryTexture *texture = material->textures[textureIndex];
        if (!texture)
          continue;

        for (int texId = 0; texId < textures.size(); ++texId)
          if (textures[texId] == texture)
          {
            texture = NULL;
            break;
          }

        if (!texture)
          continue;

        if (make_tex_path_local)
          texture->fileName = ::make_path_relative(texture->fileName, path);

        textures.push_back(texture);
      }
    }
  }

  Tab<const char *> texCharNames(tmpmem);
  texCharNames.resize(textures.size());

  for (int tex = 0; tex < textures.size(); ++tex)
    texCharNames[tex] = textures[tex]->fileName;

  dag.save_textures(texCharNames.size(), &texCharNames[0]);

  for (int mat = 0; mat < materials.size(); ++mat)
  {
    StaticGeometryMaterial *material = materials[mat];
    DagExpMater mater;
    mater.name = material->name;
    mater.clsname = material->className;
    mater.script = material->scriptText;

    mater.amb = color3(material->amb);
    mater.diff = color3(material->diff);
    mater.spec = color3(material->spec);
    mater.emis = color3(material->emis);
    mater.power = material->power;
    mater.flags = (material->flags & MatFlags::FLG_2SIDED) ? DAG_MF_2SIDED : 0;
    mater.flags |= DAG_MF_16TEX;

    memset(mater.texid, 0xFF, sizeof(mater.texid));

    for (int t = 0; t < StaticGeometryMaterial::NUM_TEXTURES && t < DAGTEXNUM; ++t)
    {
      uint16_t texId = DAGBADMATID;

      if (material->textures[t])
      {
        for (texId = 0; texId < textures.size(); ++texId)
          if (textures[texId] == material->textures[t])
            break;

        if (texId >= textures.size())
        {
          texId = DAGBADMATID;
          // TODO: log internal error texture not found
        }
      }

      mater.texid[t] = texId;
    }

    dag.save_mater(mater);
  }

  dag.start_save_nodes();

  for (j = 0; j < nodes.size(); ++j)
  {
    MeshData &mesh = nodes[j]->mesh->mesh.getMeshData();
    unsigned flags = 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_RENDERABLE) ? DAG_NF_RENDERABLE : 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS) ? DAG_NF_CASTSHADOW : 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_COLLIDABLE) ? DAG_NF_RCVSHADOW : 0;

    if (dag.start_save_node(nodes[j]->name, nodes[j]->wtm, flags))
    {
      Tab<uint16_t> nodeMaterials(tmpmem);
      DagBigMapChannel mapch[NUMMESHTCH];
      int mapChannels = 0;
      Tab<DagBigFace> fc(tmpmem);

      if (!nodes[j]->mesh.get())
        goto no_mesh;

      for (int nodeMat = 0; nodeMat < nodes[j]->mesh->mats.size(); ++nodeMat)
      {
        StaticGeometryMaterial *material = nodes[j]->mesh->mats[nodeMat];
        uint16_t matId;
        for (matId = 0; matId < materials.size(); ++matId)
          if (materials[matId] == material)
          {
            material = NULL;
            break;
          }
        if (matId == materials.size())
        {
          //==message to log - material not found!
          matId = 0;
        }
        nodeMaterials.push_back(matId);
      }

      dag.save_node_mats(nodeMaterials.size(), nodeMaterials.data());


      for (int m = 0; m < NUMMESHTCH; ++m)
      {
        if (!(mesh.tvert[m].size()))
          continue;

        mapch[mapChannels].numtv = mesh.tvert[m].size();
        mapch[mapChannels].num_tv_coords = 2;
        mapch[mapChannels].channel_id = m + 1;
        mapch[mapChannels].tverts = &mesh.tvert[m][0][0];

        mapch[mapChannels].tface = (DagBigTFace *)mesh.tface[m].data();
        ++mapChannels;
      }


      fc.resize(mesh.face.size());
      for (int f = 0; f < fc.size(); ++f)
      {
        fc[f].v[0] = mesh.face[f].v[0];
        fc[f].v[1] = mesh.face[f].v[1];
        fc[f].v[2] = mesh.face[f].v[2];
        fc[f].smgr = mesh.face[f].smgr;
        fc[f].mat = mesh.face[f].mat;
      }

    no_mesh:
      nodes[j]->script.setBool("renderable", nodes[j]->flags & StaticGeometryNode::FLG_RENDERABLE);

      nodes[j]->script.setBool("collidable", nodes[j]->flags & StaticGeometryNode::FLG_COLLIDABLE);

      nodes[j]->script.setBool("cast_shadows", nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS);

      nodes[j]->script.setBool("cast_on_self", nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF);

      nodes[j]->script.setBool("fade", nodes[j]->flags & StaticGeometryNode::FLG_FADE);

      nodes[j]->script.setBool("fade_null", nodes[j]->flags & StaticGeometryNode::FLG_FADENULL);

      nodes[j]->script.setBool("occluder", nodes[j]->flags & StaticGeometryNode::FLG_OCCLUDER);

      nodes[j]->script.setBool("billboard", nodes[j]->flags & StaticGeometryNode::FLG_BILLBOARD_MESH);

      nodes[j]->script.setBool("automatic_visrange", nodes[j]->flags & StaticGeometryNode::FLG_AUTOMATIC_VISRANGE);

      nodes[j]->script.setBool("not_mix_lods", nodes[j]->flags & StaticGeometryNode::FLG_DO_NOT_MIX_LODS);

      if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS)
      {
        nodes[j]->script.setStr("normals", "world");
        nodes[j]->script.setPoint3("dir", nodes[j]->normalsDir);

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
      }
      else if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS)
      {
        nodes[j]->script.setStr("normals", "local");
        nodes[j]->script.setPoint3("dir", nodes[j]->normalsDir);

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
      }
      else if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS)
      {
        nodes[j]->script.setStr("normals", "spherical");

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
      }
      else
        nodes[j]->script.removeParam("normals");

      nodes[j]->script.setReal("visibility_range", nodes[j]->visRange > 0 ? nodes[j]->visRange : 0.0);
      nodes[j]->script.setReal("lod_range", nodes[j]->lodRange > 0 ? nodes[j]->lodRange : 0);

      switch (nodes[j]->vss)
      {
        case 1: nodes[j]->script.setStr("vss_use", "force def"); break;
        case 0: nodes[j]->script.setStr("vss_use", "disable vss"); break;
        case -1: nodes[j]->script.setStr("vss_use", "use as is"); break;
      }

      if (nodes[j]->topLodName.length())
        nodes[j]->script.setStr("top_lod", nodes[j]->topLodName);
      else
        nodes[j]->script.removeParam("top_lod");

      if (nodes[j]->unwrapScheme != -1)
        nodes[j]->script.setInt("unwrapScheme", nodes[j]->unwrapScheme);
      else
        nodes[j]->script.removeParam("unwrapScheme");

      if (nodes[j]->lodNearVisRange > 0)
        nodes[j]->script.setReal("lodNearVisRange", nodes[j]->lodNearVisRange);
      else
        nodes[j]->script.removeParam("lodNearVisRange");

      if (nodes[j]->lodFarVisRange > 0)
        nodes[j]->script.setReal("lodFarVisRange", nodes[j]->lodFarVisRange);
      else
        nodes[j]->script.removeParam("lodFarVisRange");

      nodes[j]->script.setStr("linked_resource", nodes[j]->linkedResource);

      nodes[j]->script.setInt("priority", nodes[j]->transSortPriority);

      nodes[j]->script.setStr("lighting", StaticGeometryNode::lightingToStr(nodes[j]->lighting));
      nodes[j]->script.setReal("lt_mul", nodes[j]->ltMul);
      nodes[j]->script.setInt("vlt_mul", nodes[j]->vltMul);

      DynamicMemGeneralSaveCB cb(tmpmem);

      if (nodes[j]->script.saveToTextStream(cb))
      {
        Tab<char> newScript(tmpmem);
        const int buffSize = cb.size();

        append_items(newScript, buffSize, (const char *)cb.data());

        const int nonBlkLen = nodes[j]->nonBlkScript.length();
        if (nonBlkLen)
        {
          append_items(newScript, 2, "\r\n");
          append_items(newScript, nonBlkLen, nodes[j]->nonBlkScript.str());
        }

        newScript.push_back(0);

        dag.save_node_script(newScript.data());
      }
      else
        dag.save_node_script("");

      if (nodes[j]->mesh.get())
      {
        for (int m = 0; m < NUMMESHTCH; ++m)
          for (int tv = 0; tv < mesh.tvert[m].size(); ++tv)
            mesh.tvert[m][tv].y = -mesh.tvert[m][tv].y;

        dag.save_dag_bigmesh(mesh.vert.size(), mesh.vert.data(), mesh.face.size(), &fc[0], mapChannels, mapch);

        for (int m = 0; m < NUMMESHTCH; ++m)
          for (int tv = 0; tv < mesh.tvert[m].size(); ++tv)
            mesh.tvert[m][tv].y = -mesh.tvert[m][tv].y;
      }

      dag.end_save_node();
    }
  }

  dag.end_save_nodes();
  dag.end_save_dag();

  // TODO: processing of geoCont
}

void StaticGeometryContainer::writeMaterials(DagSaver *sv, bool make_tex_path_local) const
{
  if (!sv)
    return;

  Tab<StaticGeometryMaterial *> materials(tmpmem);
  Tab<StaticGeometryTexture *> textures(tmpmem);
  int j;

  for (j = 0; j < nodes.size(); ++j)
  {
    for (int nodeMat = 0; nodeMat < nodes[j]->mesh->mats.size(); ++nodeMat)
    {
      StaticGeometryMaterial *material = nodes[j]->mesh->mats[nodeMat];

      for (int matId = 0; matId < materials.size(); ++matId)
        if (materials[matId] == material)
        {
          material = NULL;
          break;
        }

      if (!material)
        continue;

      materials.push_back(material);

      for (int textureIndex = 0; textureIndex < StaticGeometryMaterial::NUM_TEXTURES; ++textureIndex)
      {
        StaticGeometryTexture *texture = material->textures[textureIndex];
        if (!texture)
          continue;

        for (int texId = 0; texId < textures.size(); ++texId)
          if (textures[texId] == texture)
          {
            texture = NULL;
            break;
          }

        if (!texture)
          continue;

        if (make_tex_path_local)
          texture->fileName = ::make_path_relative(texture->fileName, sv->name);

        textures.push_back(texture);
      }
    }
  }

  Tab<const char *> texCharNames(tmpmem);
  texCharNames.resize(textures.size());

  for (int tex = 0; tex < textures.size(); ++tex)
    texCharNames[tex] = textures[tex]->fileName;

  sv->save_textures(texCharNames.size(), &texCharNames[0]);

  for (int mat = 0; mat < materials.size(); ++mat)
  {
    StaticGeometryMaterial *material = materials[mat];
    DagExpMater mater;
    mater.name = material->name;
    mater.clsname = material->className;
    mater.script = material->scriptText;

    mater.amb = color3(material->amb);
    mater.diff = color3(material->diff);
    mater.spec = color3(material->spec);
    mater.emis = color3(material->emis);
    mater.power = material->power;
    mater.flags = (material->flags & MatFlags::FLG_2SIDED) ? DAG_MF_2SIDED : 0;
    mater.flags |= DAG_MF_16TEX;

    memset(mater.texid, 0xFF, sizeof(mater.texid));

    for (int t = 0; t < StaticGeometryMaterial::NUM_TEXTURES && t < DAGTEXNUM; ++t)
    {
      uint16_t texId = DAGBADMATID;

      if (material->textures[t])
      {
        for (texId = 0; texId < textures.size(); ++texId)
          if (textures[texId] == material->textures[t])
            break;

        if (texId >= textures.size())
        {
          texId = DAGBADMATID;
          // TODO: log internal error texture not found
        }
      }

      mater.texid[t] = texId;
    }

    sv->save_mater(mater);
  }
}

void StaticGeometryContainer::writeNodes(DagSaver *sv) const
{
  if (!sv)
    return;

  Tab<StaticGeometryMaterial *> materials(tmpmem);
  int j;
  for (j = 0; j < nodes.size(); ++j)
  {
    for (int nodeMat = 0; nodeMat < nodes[j]->mesh->mats.size(); ++nodeMat)
    {
      StaticGeometryMaterial *material = nodes[j]->mesh->mats[nodeMat];

      for (int matId = 0; matId < materials.size(); ++matId)
        if (materials[matId] == material)
        {
          material = NULL;
          break;
        }

      if (!material)
        continue;

      materials.push_back(material);
    }
  }

  for (j = 0; j < nodes.size(); ++j)
  {
    MeshData &mesh = nodes[j]->mesh->mesh.getMeshData();
    unsigned flags = 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_RENDERABLE) ? DAG_NF_RENDERABLE : 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS) ? DAG_NF_CASTSHADOW : 0;
    flags |= (nodes[j]->flags & StaticGeometryNode::FLG_COLLIDABLE) ? DAG_NF_RCVSHADOW : 0;

    if (sv->start_save_node(nodes[j]->name, nodes[j]->wtm, flags))
    {
      Tab<uint16_t> nodeMaterials(tmpmem);

      for (int nodeMat = 0; nodeMat < nodes[j]->mesh->mats.size(); ++nodeMat)
      {
        StaticGeometryMaterial *material = nodes[j]->mesh->mats[nodeMat];
        uint16_t matId;
        for (matId = 0; matId < materials.size(); ++matId)
          if (materials[matId] == material)
          {
            material = NULL;
            break;
          }
        if (matId == materials.size())
        {
          //==message to log - material not found!
          matId = 0;
        }
        nodeMaterials.push_back(matId);
      }

      sv->save_node_mats(nodeMaterials.size(), nodeMaterials.data());

      DagBigMapChannel mapch[NUMMESHTCH];
      int mapChannels = 0;

      for (int m = 0; m < NUMMESHTCH; ++m)
      {
        if (!(mesh.tvert[m].size()))
          continue;

        mapch[mapChannels].numtv = mesh.tvert[m].size();
        mapch[mapChannels].num_tv_coords = 2;
        mapch[mapChannels].channel_id = m + 1;
        mapch[mapChannels].tverts = &mesh.tvert[m][0][0];

        mapch[mapChannels].tface = (DagBigTFace *)mesh.tface[m].data();
        ++mapChannels;
      }

      Tab<DagBigFace> fc(tmpmem);
      fc.resize(mesh.face.size());

      for (int f = 0; f < fc.size(); ++f)
      {
        fc[f].v[0] = mesh.face[f].v[0];
        fc[f].v[1] = mesh.face[f].v[1];
        fc[f].v[2] = mesh.face[f].v[2];
        fc[f].smgr = mesh.face[f].smgr;
        fc[f].mat = mesh.face[f].mat;
      }

      nodes[j]->script.setBool("renderable", nodes[j]->flags & StaticGeometryNode::FLG_RENDERABLE);

      nodes[j]->script.setBool("collidable", nodes[j]->flags & StaticGeometryNode::FLG_COLLIDABLE);

      nodes[j]->script.setBool("cast_shadows", nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS);

      nodes[j]->script.setBool("cast_on_self", nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF);

      nodes[j]->script.setBool("fade", nodes[j]->flags & StaticGeometryNode::FLG_FADE);

      nodes[j]->script.setBool("fade_null", nodes[j]->flags & StaticGeometryNode::FLG_FADENULL);

      nodes[j]->script.setBool("occluder", nodes[j]->flags & StaticGeometryNode::FLG_OCCLUDER);

      nodes[j]->script.setBool("billboard", nodes[j]->flags & StaticGeometryNode::FLG_BILLBOARD_MESH);

      nodes[j]->script.setBool("automatic_visrange", nodes[j]->flags & StaticGeometryNode::FLG_AUTOMATIC_VISRANGE);

      nodes[j]->script.setBool("not_mix_lods", nodes[j]->flags & StaticGeometryNode::FLG_DO_NOT_MIX_LODS);

      if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS)
      {
        nodes[j]->script.setStr("normals", "world");
        nodes[j]->script.setPoint3("dir", nodes[j]->normalsDir);

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
      }
      else if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS)
      {
        nodes[j]->script.setStr("normals", "local");
        nodes[j]->script.setPoint3("dir", nodes[j]->normalsDir);

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
      }
      else if (nodes[j]->flags & StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS)
      {
        nodes[j]->script.setStr("normals", "spherical");

        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        nodes[j]->flags &= ~StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
      }
      else
        nodes[j]->script.removeParam("normals");

      nodes[j]->script.setReal("visibility_range", nodes[j]->visRange > 0 ? nodes[j]->visRange : 0.0);
      nodes[j]->script.setReal("lod_range", nodes[j]->lodRange > 0 ? nodes[j]->lodRange : 0);

      switch (nodes[j]->vss)
      {
        case 1: nodes[j]->script.setStr("vss_use", "force def"); break;
        case 0: nodes[j]->script.setStr("vss_use", "disable vss"); break;
        case -1: nodes[j]->script.setStr("vss_use", "use as is"); break;
      }

      if (nodes[j]->topLodName.length())
        nodes[j]->script.setStr("top_lod", nodes[j]->topLodName);
      else
        nodes[j]->script.removeParam("top_lod");

      if (nodes[j]->unwrapScheme != -1)
        nodes[j]->script.setInt("unwrapScheme", nodes[j]->unwrapScheme);
      else
        nodes[j]->script.removeParam("unwrapScheme");

      if (nodes[j]->lodNearVisRange > 0)
        nodes[j]->script.setReal("lodNearVisRange", nodes[j]->lodNearVisRange);
      else
        nodes[j]->script.removeParam("lodNearVisRange");

      if (nodes[j]->lodFarVisRange > 0)
        nodes[j]->script.setReal("lodFarVisRange", nodes[j]->lodFarVisRange);
      else
        nodes[j]->script.removeParam("lodFarVisRange");

      nodes[j]->script.setStr("linked_resource", nodes[j]->linkedResource);

      nodes[j]->script.setInt("priority", nodes[j]->transSortPriority);

      nodes[j]->script.setStr("lighting", StaticGeometryNode::lightingToStr(nodes[j]->lighting));
      nodes[j]->script.setReal("lt_mul", nodes[j]->ltMul);
      nodes[j]->script.setInt("vlt_mul", nodes[j]->vltMul);

      DynamicMemGeneralSaveCB cb(tmpmem);

      if (nodes[j]->script.saveToTextStream(cb))
      {
        Tab<char> newScript(tmpmem);
        const int buffSize = cb.size();

        append_items(newScript, buffSize, (const char *)cb.data());

        const int nonBlkLen = nodes[j]->nonBlkScript.length();
        if (nonBlkLen)
        {
          append_items(newScript, 2, "\r\n");
          append_items(newScript, nonBlkLen, nodes[j]->nonBlkScript.str());
        }

        newScript.push_back(0);

        sv->save_node_script(newScript.data());
      }
      else
        sv->save_node_script("");

      for (int m = 0; m < NUMMESHTCH; ++m)
        for (int tv = 0; tv < mesh.tvert[m].size(); ++tv)
          mesh.tvert[m][tv].y = -mesh.tvert[m][tv].y;

      sv->save_dag_bigmesh(mesh.vert.size(), mesh.vert.data(), mesh.face.size(), &fc[0], mapChannels, mapch);

      for (int m = 0; m < NUMMESHTCH; ++m)
        for (int tv = 0; tv < mesh.tvert[m].size(); ++tv)
          mesh.tvert[m][tv].y = -mesh.tvert[m][tv].y;

      sv->end_save_node();
    }
  }
}
