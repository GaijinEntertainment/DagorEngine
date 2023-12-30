#include <stdio.h>
#include <stdlib.h>


#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <generic/dag_tab.h>
#include <libTools/dagFileRW/dagFileExport.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <util/dag_bitArray.h>
#include <math/dag_mesh.h>
#include <convert.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_baseIo.h>
#include <debug/dag_debug.h>
///@todo unzip on the fly
///@todo remove degeneratives, make smooothing from normals.
///@todo fix bsp texdata
///@todo get base texture from vmt
typedef unsigned short ushort;

typedef unsigned char byte;
typedef struct
{
  unsigned p[3];
} Quaternion48;
typedef struct
{
  unsigned p[3];
} Vector48;
typedef Point3 Vector;
typedef Point4 Vector4D;
typedef Point2 Vector2D;
typedef TMatrix matrix3x4_t;
typedef Quat Quaternion;

typedef real vec_t;
template <class T>
class CUtlVector : public Tab<T>
{
public:
  CUtlVector() : Tab<T>(midmem_ptr()) {}
};
#define COMPILE_TIME_ASSERT
class RadianEuler
{
public:
  inline RadianEuler(void) {}
  inline RadianEuler(vec_t X, vec_t Y, vec_t Z)
  {
    x = X;
    y = Y;
    z = Z;
  }
  // Initialization
  inline void Init(vec_t ix = 0.0f, vec_t iy = 0.0f, vec_t iz = 0.0f)
  {
    x = ix;
    y = iy;
    z = iz;
  }

  bool IsValid() const;

  // array access...
  vec_t operator[](int i) const;
  vec_t &operator[](int i);

  vec_t x, y, z;
};
#define Assert G_VERIFY
#include "studio.h"
#include "optimize.h"

typedef Color4 LightColorType;
static String extension(tmpmem_ptr());

void show_warning(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "WARNING: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void show_warning(const String &s) { show_warning("%s", (const char *)s); }

static const char *lumpname[] = {"Entities", // 0
  "Planes",                                  // 1
  "TexData",                                 // 2
  "Vertexes",                                // 3
  "Visibility",                              // 4
  "Nodes",                                   // 5
  "TexInfo",                                 // 6
  "Faces",                                   // 7
  "Lighting",                                // 8
  "Occlusion",                               // 9
  "Leafs",                                   // 10
  "",                                        //
  "Edges",                                   // 12
  "SurfEdges",                               // 13
  "Models",                                  // 14
  "Worldlights",                             // 15
  "LeafFaces",                               // 16
  "LeafBrushes",                             // 17
  "Brushes",                                 // 18
  "BrushSides",                              // 19
  "Areas",                                   // 20
  "AreaPortals",                             // 21
  "Portals",                                 // 22
  "Clusters",                                // 23
  "PortalVerts",                             // 24
  "ClusterPortals",                          // 25
  "DispInfo",                                // 26
  "OriginalFaces",                           // 27
  "",
  "PhysCollide",                 // 29
  "VertNormals",                 // 30
  "VertNormalIndicies",          // 31
  "DispLightmapAlphas",          // 32
  "DispVerts",                   // 33
  "DispLightmapSamplePositions", // 34
  "GameLump",                    // 35
  "LeafWaterData",               // 36
  "Primatives",                  // 37
  "PrimVerts",                   // 38
  "PrimIndicies",                // 39
  "PakFile",                     // 40
  "ClipPortalVerts",             // 41
  "Cubemaps",                    // 42
  "TexDataStringData",           // 43
  "TexDataStringTable",          // 44
  "Overlays",                    // 45
  "LeafMinDistToWater",          // 46
  "FaceMacroTextureInfo",        // 47
  "DispTris",                    // 48
  "PhysCollideSurface",          // 49
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

static int lumpsize[] = {1, 20, 32, 12, 1, 32, 72, 56, 1, 1, // 0-9
  56, 0, 4, 4, 48, 86, 2, 2, 12, 8,                          // 10-19
  8, 12, 16, 8, 2, 2, 176, 56, 0, 1,                         // 20-29
  12, 2, 1, 1, 1, 1, 12, 10, 12, 2,                          // 30-39
  1, 12, 16, 1, 4, 352, 1, 2, 1, 1,                          // 40-49
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Tab<String> uniques(midmem_ptr());
static int uniqueID = 1;

static char *ensureUnique(const char *name)
{
  for (int i = 0; i < uniques.size(); ++i)
    if (strcmp(uniques[i], name) == 0)
    {
      uniques.push_back().printf(128, "%s%02d", name, uniqueID++);
      return uniques.back();
    }
  uniques.push_back() = name;
  return uniques.back();
}

static void unify(char *vtf)
{
  for (int i = 0; i < strlen(vtf); ++i)
    if (vtf[i] == '\\')
      vtf[i] = '/';
  strlwr(vtf);
}
static char *stristr(char *in, const char *pat)
{
  int patn = strlen(pat);
  for (int i = 0; i < strlen(in) - patn; ++i)
    if (strnicmp(in + i, pat, patn) == 0)
      return in + i;
  return NULL;
}

static bool readVMT(const String &root_textures, char *vmtName, String &vtf)
{
  if (!dd_file_exist(root_textures + String("materials/") + vmtName))
  {
    debug("no vmt %s", (char *)(root_textures + String("materials/") + vmtName));
    show_warning("no vmt %s", (char *)(root_textures + String("materials/") + vmtName));
    return false;
  }
  FullFileLoadCB cb((root_textures + String("materials/") + vmtName));
  if (!cb.fileHandle)
  {
    show_warning("cant read vmt %s", (char *)(root_textures + String("materials/") + vmtName));
    return false;
  }
  int len = df_length(cb.fileHandle);
  String data;
  data.resize(len + 1);
  cb.read(&data[0], len);
  data[len] = 0;
  char *basetexture = stristr(&data[0], "\"$baseTexture\"");
  if (basetexture)
  {
    basetexture += strlen("\"$baseTexture\"");
    char *token = strtok(basetexture, "\"");
    if (token)
      token = strtok(NULL, "\"");
    if (token)
    {
      vtf = (root_textures + String("materials/")) + token;
      vtf += extension;
      unify(&vtf[0]);
      debug("founded flname %s", &vtf[0]);
      return true;
    }
  }
  char *include = stristr(&data[0], "\"include\"");
  if (include)
  {
    include += strlen("\"include\"");
    char *token = strtok(include, "\"");
    if (token)
      token = strtok(NULL, "\"");
    if (token)
    {
      bool ret = false;
      char *materialsStripped = strstr(token, "materials");
      if (materialsStripped)
      {
        materialsStripped += strlen("materials");
        vtf = materialsStripped + 1;
        ret = readVMT(root_textures, materialsStripped + 1, vtf);
      }
      else
      {
        vtf = token;
        ret = readVMT(root_textures, token, vtf);
      }
      // ret = readVMT(root_textures, token, vtf);
      if (!ret)
      {
        vtf = root_textures + vtf + extension;
        unify(&vtf[0]);
      }
      return ret;
    }
  }
  return false;
}

class HL2Model
{
public:
  Tab<char> model, vvd, vtx;
  Tab<DagExpMater> materials;
  Tab<String> strtexnames;
  bool patched;
  struct Node
  {
    Tab<DagBigFace> face;
    Tab<DagBigTFace> tface;
    Tab<Point3> verts;
    Tab<Point2> tcoords;
    Tab<ushort> nodeMats;
    String name;
  };
  Tab<Node> nodes;
  HL2Model() : model(tmpmem_ptr()), vvd(tmpmem_ptr()), vtx(tmpmem_ptr()), patched(false) {}
  bool load(const char *bfn, const String &root_textures);
  void addTexturesMaterials(Tab<String> &tex, Tab<DagExpMater> &mats);
  bool save(const char *fn);
  bool saveNodes(DagSaver &sav, const TMatrix &wtm);
  bool gatherMesh(mstudiomesh_t *studioMesh, OptimizedModel::MeshHeader_t *mesh, Tab<DagBigFace> &face, Tab<DagBigTFace> &tface);
};
static HL2Model *cmodel = NULL;

bool HL2Model::gatherMesh(mstudiomesh_t *studioMesh, OptimizedModel::MeshHeader_t *mesh, Tab<DagBigFace> &face,
  Tab<DagBigTFace> &tface)
{
  const mstudio_meshvertexdata_t *vdata = studioMesh->GetVertexData();
  int faces = 0;
  for (int stripGr = 0; stripGr < mesh->numStripGroups; ++stripGr)
  {
    OptimizedModel::StripGroupHeader_t *stripGroup = mesh->pStripGroup(stripGr);
    for (int stripI = 0; stripI < stripGroup->numStrips; ++stripI)
    {
      OptimizedModel::StripHeader_t *strip = stripGroup->pStrip(stripI);

      OptimizedModel::Vertex_t *stripVertex = stripGroup->pVertex(strip->vertOffset);
      unsigned short *stripIndex = stripGroup->pIndex(strip->indexOffset);
      if (OptimizedModel::STRIP_IS_TRILIST == strip->flags)
      {
        int f = append_items(face, strip->numIndices / 3);
        append_items(tface, strip->numIndices / 3);
        for (int fi = 0; fi < strip->numIndices / 3; ++fi)
        {
          int v[3] = {stripIndex[fi * 3 + 0 + strip->indexOffset], stripIndex[fi * 3 + 1 + strip->indexOffset],
            stripIndex[fi * 3 + 2 + strip->indexOffset]};
          v[0] = stripVertex[v[0]].origMeshVertID;
          v[1] = stripVertex[v[1]].origMeshVertID;
          v[2] = stripVertex[v[2]].origMeshVertID;
          // debug("%d %d %d",v[0],v[1],v[2]);
          face[f + fi].v[0] = v[0];
          face[f + fi].v[1] = v[1];
          face[f + fi].v[2] = v[2];
          face[f + fi].mat = studioMesh->material;
          face[f + fi].smgr = 1; //==

          tface[f + fi].t[0] = v[0];
          tface[f + fi].t[1] = v[1];
          tface[f + fi].t[2] = v[2];
        }
        faces += strip->numIndices / 3;
        debug("list %d", strip->numIndices);
      }
      else if (OptimizedModel::STRIP_IS_TRISTRIP == strip->flags)
      {
        faces += strip->numIndices - 2;
        debug("strip %d", strip->numIndices);
        DAG_FATAL("not implemented");
      }
      else
      {
        show_warning("Unknown strip");
      }
    }
  }
  debug("faces =%d", faces);
  return true;
}

void HL2Model::addTexturesMaterials(Tab<String> &tex, Tab<DagExpMater> &mats)
{
  if (patched)
    return;
  Tab<int> texPatch(tmpmem);
  texPatch.resize(strtexnames.size());
  for (int t = 0; t < strtexnames.size(); ++t)
  {
    int j;
    for (j = 0; j < tex.size(); ++j)
    {
      if (stricmp(tex[j], strtexnames[t]) == 0)
      {
        texPatch[t] = j;
        break;
      }
    }
    if (j >= tex.size())
    {
      texPatch[t] = tex.size();
      tex.push_back(strtexnames[t]);
    }
  }
  for (int m = 0; m < materials.size(); ++m)
  {
    materials[m].texid[0] = texPatch[materials[m].texid[0]];
  }
  for (int n = 0; n < nodes.size(); ++n)
  {
    Node &node = nodes[n];
    for (int m = 0; m < node.nodeMats.size(); ++m)
    {
      node.nodeMats[m] += mats.size();
    }
  }
  append_items(mats, materials.size(), materials.data());
  patched = true;
}

bool HL2Model::saveNodes(DagSaver &sav, const TMatrix &wtm)
{
  for (int i = 0; i < nodes.size(); ++i)
  {
    Node &node = nodes[i];
    if (!sav.start_save_node(ensureUnique(node.name), wtm))
      throw 0;
    if (!sav.save_node_mats(node.nodeMats.size(), &node.nodeMats[0]))
      throw 0;
    DagBigMapChannel ch[1];
    ch[0].tverts = &node.tcoords[0].x;
    ch[0].tface = &node.tface[0];
    ch[0].numtv = node.tcoords.size();
    ch[0].num_tv_coords = 2;
    ch[0].channel_id = 1;
    if (node.face.size())
      if (!sav.save_dag_bigmesh(node.verts.size(), &node.verts[0], node.face.size(), &node.face[0],
            sizeof(ch) / sizeof(DagBigMapChannel), ch))
        throw 0;

    if (!sav.end_save_node())
      throw 0;
  }
  return true;
}

bool HL2Model::save(const char *fn)
{
  DagSaver sav;
  if (!sav.start_save_dag(String(fn) + ".dag"))
    return false;
  try
  {

    Tab<const char *> texnames(tmpmem);
    for (int t = 0; t < strtexnames.size(); ++t)
    {
      int tnm = append_items(texnames, 1);
      texnames[tnm] = strtexnames[t];
    }
    if (!sav.save_textures(texnames.size(), texnames.data()))
      throw 0;
    for (int ti = 0; ti < materials.size(); ti++)
    {
      sav.save_mater(materials[ti]);
    }

    if (!sav.start_save_nodes())
      throw 0;
    saveNodes(sav, TMatrix::IDENT);
    if (!sav.end_save_nodes())
      throw 0;
  }
  catch (int)
  {
    sav.end_save_dag();
    return false;
  }
  return sav.end_save_dag();
}

bool HL2Model::load(const char *_bfn, const String &root_textures)
{
  String bfn(_bfn);
  if (strstr(bfn, ".mdl"))
  {
    char *mdl = strstr(bfn, ".mdl");
    erase_items(bfn, mdl - bfn.data(), 4);
  }
  String fn = bfn + ".mdl";
  String vvdfn = bfn + ".vvd";
  String vtxfn = bfn + ".dx80.vtx";
  cmodel = this;
  {
    debug("reading mdl");
    FullFileLoadCB cb(fn);
    if (!cb.fileHandle)
      return false;
    int len = df_length(cb.fileHandle);

    model.resize(len);
    cb.read(&model[0], len);
  }

  {
    debug("reading vvd");
    FullFileLoadCB vvdcb(vvdfn);
    if (!vvdcb.fileHandle)
      return false;
    int vvdlen = df_length(vvdcb.fileHandle);

    debug("reading vvd=%d", vvdlen);
    vvd.resize(vvdlen);
    vvdcb.read(&vvd[0], vvdlen);
  }
  debug("reading vvd..done");

  {
    FullFileLoadCB vtxcb(vtxfn);
    if (!vtxcb.fileHandle)
      return false;
    int vtxlen = df_length(vtxcb.fileHandle);

    debug("reading vtx=%d", vtxlen);
    vtx.resize(vtxlen);
    vtxcb.read(&vtx[0], vtxlen);
  }

  vertexFileHeader_t *pVertexHdr = (vertexFileHeader_t *)&vvd[0];
  debug("%d len + %d", sizeof(mstudiovertex_t) * pVertexHdr->numLODVertexes[0], sizeof(Vector4D) * pVertexHdr->numLODVertexes[0]);
  if (pVertexHdr->numFixups != 0)
  {
    debug("Fixups not implemeted!");
  }
  debug("vdata = %d %d fixups=%d tang= %d", pVertexHdr->vertexDataStart, pVertexHdr->numLODVertexes[0], pVertexHdr->numFixups,
    pVertexHdr->tangentDataStart);

  /*for(int vi =0 ;vi<pVertexHdr->numLODVertexes[0]; ++vi){
   debug("%f %f %f", ((mstudiovertex_t*)(((byte *)pVertexHdr)+pVertexHdr->vertexDataStart) + vi)->m_vecPosition.x,
                     ((mstudiovertex_t*)(((byte *)pVertexHdr)+pVertexHdr->vertexDataStart) + vi)->m_vecPosition.y,
                     ((mstudiovertex_t*)(((byte *)pVertexHdr)+pVertexHdr->vertexDataStart) + vi)->m_vecPosition.z);
  }*/

  byte *pin;
  studiohdr_t *phdr;

  int version;

  pin = (byte *)&model[0];

  phdr = (studiohdr_t *)pin;
  version = phdr->version;
  debug("%d %d", phdr->length, phdr->version);
  debug("%d", phdr->flags);
  debug("bones %d %d", phdr->numbones, phdr->boneindex);
  debug("hit %d %d", phdr->numhitboxsets, phdr->hitboxsetindex);
  debug("texure %d %d", phdr->numtextures, phdr->textureindex);
  // debug("skin %d %d %d",phdr->numskinref,phdr->numskinfamilies,phdr->skinindex);
  debug("body %d %d", phdr->numbodyparts, phdr->bodypartindex);

  Studio_ConvertStudioHdrToNewVersion(phdr);

  version = phdr->version;
  if (version != STUDIO_VERSION)
  {
    memset(phdr, 0, sizeof(studiohdr_t));

    show_warning("%s has wrong version number (%i should be %i)\n", (char *)fn, version, STUDIO_VERSION);
    return false;
  }
  Studio_SetRootLOD(phdr, 0);


  // Make sure we don't have too many verts in each model (will kill decal vertex caching if so)
  // DagSaver sav;
  Tab<String> mats(tmpmem);
  // Tab<String> textures(tmpmem);
  // Tab<char*> texnames(tmpmem);
  Tab<ushort> nodeMats(tmpmem);
  mats.resize(phdr->numtextures);
  // texnames.resize(phdr->numtextures);
  // textures.resize(phdr->numtextures);
  nodeMats.resize(phdr->numtextures);

  Tab<int> matToTexnames(tmpmem);
  // Tab<char*> texnames(tmpmem);
  // Tab<String> strtexnames(tmpmem);
  matToTexnames.resize(phdr->numtextures);
  // Tab<unsigned short> allMats(tmpmem);
  // allMats.resize(td.size());


  for (int t = 0; t < phdr->numtextures; ++t)
  {
    mats[t] = phdr->pTexture(t)->pszName();
    String vtf = String(phdr->pTexture(t)->pszName()) + extension;

    for (int i = 0; i < phdr->numcdtextures; ++i)
    {
      String path(phdr->pCdtexture(i));
      path += mats[t];
      vtf = root_textures + path + extension;
      unify(&vtf[0]);
      if (readVMT(root_textures, path + ".vmt", vtf))
        break;
      if (dd_file_exist(root_textures + String("materials/") + path + ".vtf"))
        break;
    }
    int oi;
    for (oi = 0; oi < strtexnames.size(); ++oi)
    {
      if (stricmp(strtexnames[oi], vtf) == 0)
        break;
    }
    if (oi >= strtexnames.size())
    {
      oi = strtexnames.size();
      strtexnames.push_back(vtf);
      // int tnm = texnames.append(1);
      // texnames[tnm] = strtexnames[oi];
    }
    matToTexnames[t] = oi;
    nodeMats[t] = t;
  }
  // if(!sav.start_save_dag(String(fn)+".dag")) return false;
  try
  {

    // if(!sav.save_textures(texnames.size(), texnames)) throw 0;
    for (int ti = 0; ti < matToTexnames.size(); ti++)
    {
      DagExpMater mater;
      memset(&mater, 0, sizeof(mater));
      mater.name = str_dup(mats[ti], strmem);
      mater.clsname = "simple";
      mater.script = "lighting=vltmap";
      DagColor col;
      col.r = col.g = col.b = 255;
      mater.amb = mater.diff = mater.spec = col;
      mater.power = 0;
      mater.flags = 0;
      memset(mater.texid, 0xFF, sizeof(mater.texid));
      mater.texid[0] = matToTexnames[ti];
      materials.push_back(mater);
      // sav.save_mater(mater);
    }

    // if(!sav.start_save_nodes()) throw 0;

    int bodyPartID;
    debug("model = %s", phdr->name);
    debug("bodyparts = %d %d", phdr->bodypartindex, phdr->numbodyparts);
    OptimizedModel::FileHeader_t *vtxhdr = (OptimizedModel::FileHeader_t *)&vtx[0];
    if (vtxhdr->numBodyParts != phdr->numbodyparts)
    {
      DAG_FATAL("vtx_bodyparts(%d)!=model_bodypart(%d)", vtxhdr->numBodyParts, phdr->numbodyparts);
    }
    for (bodyPartID = 0; bodyPartID < phdr->numbodyparts; bodyPartID++)
    {

      mstudiobodyparts_t *pBodyPart = phdr->pBodypart(bodyPartID);
      debug("bodypart = %d %p", bodyPartID, pBodyPart);
      debug("bodypartinfo = %d %p", bodyPartID, pBodyPart);
      if ((char *)pBodyPart < &model[0] || (char *)pBodyPart > &model.back())
      {
        show_warning("odd bodypart %d", bodyPartID);
        return false;
      }
      debug("nm %d models %d base %d modind %d", pBodyPart->sznameindex, pBodyPart->nummodels, pBodyPart->base, pBodyPart->modelindex);
      if (pBodyPart->pszName() < &model.back())
        debug("bodypartname = %s", pBodyPart->pszName());

      OptimizedModel::BodyPartHeader_t *vtxBody = vtxhdr->pBodyPart(bodyPartID);
      if (vtxBody->numModels != pBodyPart->nummodels)
      {
        DAG_FATAL("%d, vtx_bodymodels(%d)!=model_bodymodels(%d)", bodyPartID, vtxBody->numModels, pBodyPart->nummodels);
      }
      int modelID;
      for (modelID = 0; modelID < pBodyPart->nummodels; modelID++)
      {
        debug("modelId = %d ", modelID);
        mstudiomodel_t *pModel = pBodyPart->pModel(modelID);
        //                      Assert( pModel->numvertices < MAXSTUDIOVERTS );
        debug("model = %p ", pModel);
        if ((char *)pModel > &model.back())
        {
          show_warning("odd model %d", modelID);
          return false;
        }
        OptimizedModel::ModelHeader_t *vtxModel = vtxBody->pModel(modelID);
        OptimizedModel::ModelLODHeader_t *vtxLodModel = vtxModel->pLOD(0); // high lod
        debug("model = %s meshes %d %d", pModel->name, pModel->nummeshes, pModel->meshindex);
        if (vtxLodModel->numMeshes != pModel->nummeshes)
        {
          DAG_FATAL("%d %d, vtx_bodymodelmeshes(%d)!=model_bodymodelmeshes(%d)", bodyPartID, modelID, vtxLodModel->numMeshes,
            pModel->nummeshes);
        }
        int n = append_items(nodes, 1);
        Node &node = nodes[n];
        // TMatrix wtm;
        // wtm.identity();
        node.name = pModel->name;
        // if(!sav.start_save_node(pModel->name,wtm)) throw 0;
        // if(!sav.save_node_mats(nodeMats.size(),&nodeMats[0])) throw 0;
        // node.verts.resize(pModel->numvertices);
        // node.tcoords.resize(pModel->numvertices);
        node.nodeMats = nodeMats;

        int meshID;
        for (meshID = 0; meshID < pModel->nummeshes; meshID++)
        {
          OptimizedModel::MeshHeader_t *vtxMesh = vtxLodModel->pMesh(meshID);
          mstudiomesh_t *pMesh = pModel->pMesh(meshID);
          gatherMesh(pMesh, vtxMesh, node.face, node.tface);

          // pMesh->numvertices  = pMesh->vertexdata.numLODVertexes[rootLOD];
        }

        /*if (pModel->nummeshes!=1) {
          DAG_FATAL("Not implemented meshes count");
        }*/
        BBox3 bbox;
        BBox2 tbox;
        for (meshID = 0; meshID < pModel->nummeshes; meshID++)
        {
          mstudiomesh_t *pMesh = pModel->pMesh(meshID);
          int vn = append_items(node.verts, pMesh->numvertices);
          int tn = append_items(node.tcoords, pMesh->numvertices);
          for (int vertID = 0; vertID < pMesh->numvertices; vertID++)
          {
            // debug("vert = " FMT_P3 "", pModel->GetVertexData()->Position(vertID)->x, pModel->GetVertexData()->Position(vertID)->y,
            // pModel->GetVertexData()->Position(vertID)->z);
            node.verts[vn + vertID] = *pMesh->GetVertexData()->Position(vertID) + pMesh->center;
            bbox += node.verts[vn + vertID];
            node.tcoords[tn + vertID] = *pMesh->GetVertexData()->Texcoord(vertID);
            tbox += node.tcoords[tn + vertID];
          }
        }
        Tab<int> degenerates(tmpmem);
        optimize_verts((DagBigFaceT *)&node.face[0], node.face.size(), node.verts, length(bbox.width()) * 0.0005, &degenerates);
        optimize_verts(&node.tface[0], node.tface.size(), node.tcoords, 0.0001);
        for (int di = 0; di < degenerates.size(); ++di)
        {
          erase_items(node.face, degenerates[di], 1);
          erase_items(node.tface, degenerates[di], 1);
        }
        if (pModel->numvertices >= MAXSTUDIOVERTS)
        {
          show_warning("%s exceeds MAXSTUDIOVERTS (%d >= %d)\n", phdr->name, pModel->numvertices, (int)MAXSTUDIOVERTS);
          return false;
        }

        //==if(!sav.save_node_mats(usedMaterials.size(),&usedMaterials[0])) throw 0;

        /*DagBigMapChannel ch[1];
        ch[0].tverts=&node.tcoords[0].x;
        ch[0].tface=&node.tface[0];
        ch[0].numtv=tcoords.size();
        ch[0].num_tv_coords=2;
        ch[0].channel_id=1;

        int c = 1;*/

        // flipFaces(&node.face[0], node.face.size());
        // flipFaces(&node.tface[0], node.tface.size());
        invertV(&node.tcoords[0], node.tcoords.size());
        /*
        if (node.face.size())
          if(!sav.save_dag_bigmesh(verts.size(),&verts[0],face.size(),&face[0],c,ch))
            throw 0;

        if(!sav.end_save_node()) throw 0;*/
      }
    }
    // if(!sav.end_save_nodes()) throw 0;
  }
  catch (int)
  {
    // sav.end_save_dag();
    return false;
  }
  // return sav.end_save_dag();
  return true;
}

const mstudio_modelvertexdata_t *mstudiomodel_t::GetVertexData()
{
  vertexFileHeader_t *pVertexHdr = (vertexFileHeader_t *)&cmodel->vvd[0];
  vertexdata.pVertexData = (byte *)pVertexHdr + pVertexHdr->vertexDataStart;
  vertexdata.pTangentData = (byte *)pVertexHdr + pVertexHdr->tangentDataStart;

  return &vertexdata;
}

void inline SinCos(float radians, float *sine, float *cosine)
{
  _asm
  {
    fld             DWORD PTR [radians]
    fsincos

    mov edx, DWORD PTR [cosine]
    mov eax, DWORD PTR [sine]

    fstp DWORD PTR [edx]
    fstp DWORD PTR [eax]
  }
}

enum
{
  PITCH = 0, // up / down
  YAW,       // left / right
  ROLL       // fall over
};

TMatrix PYRTM(Point3 &vAngles)
{
  float sr, sp, sy, cr, cp, cy;

  SinCos(vAngles[YAW], &sy, &cy);
  SinCos(vAngles[PITCH], &sp, &cp);
  SinCos(vAngles[ROLL], &sr, &cr);
  TMatrix m;

  // matrix = (YAW * PITCH) * ROLL
  m.m[0][0] = cp * cy;
  m.m[0][1] = cp * sy;
  m.m[0][2] = -sp;
  m.m[1][0] = sr * sp * cy + cr * -sy;
  m.m[1][1] = sr * sp * sy + cr * cy;
  m.m[1][2] = sr * cp;
  m.m[2][0] = (cr * sp * cy + -sr * -sy);
  m.m[2][1] = (cr * sp * sy + -sr * cy);
  m.m[2][2] = cr * cp;

  return m;
}

struct BSP
{
  struct Lump
  {


    static const char *name(int lindex) { return lumpname[lindex]; }

    static int size(int lindex)
    {
      if (lumpsize[lindex] == 0)
      {
        return -1;
      }
      return lumpsize[lindex];
    }

    int ofs;
    int len;
    int vers;
    int fourCC;
  };

  struct Texture
  {
    String name;
    struct Texinfo *texinfo;
    struct Texdata *texdata;
  };


  struct Face
  {
    int pnum;
    char side;
    char onnode;
    int fstedge;
    short numedge;
    short texinfo;
    short dispinfo;
    int orig;
    int smooth;
    float area;

    bool written;   // for ofaces, if written to vmf already
    bool undersize; // for ofaces, if smaller than sum of faces
    bool mark;      // for ofaces, for unique face counting
  };

  struct Model
  {
    int fstface;
    int numface;
    int headnode; // the head node of the model's BSP tree
    int entity;   // index of entity which uses this model
    Point3 origin;
    int fstbrush;
    int numbrush;
  };

  struct Entity
  {
    String classname;
    String structname;
    String targetname;
    String parentname;
    int model;
    Point3 o;
    Tab<Entity *> link; // link to other entity in this key/value

    int con;   // if set, is the 1st kv pair that is a connection
    bool mark; // computation only marker
    int index; // this entity's position in the array
  };
  struct Pstatic
  {
    float ox, oy, oz;
    float ax, ay, az;
    int skin;
    short type;
    byte solid, flags;
    float fademin, fademax;
  };

  struct Texinfo
  {
    float tv[2][4];
    float ltv[2][4];
    int flags;
    int texdata;
  };

  struct Texdata
  {
    int nameid;
    String name;
    int width, height;
  };

  struct Edge
  {
    unsigned short v0, v1;
  };

  struct GLump
  {
    int id;
    short flags;
    short vers;
    int ofs;
    int len;
  };

  // header

  int ident;
  int version;
  Lump *lump[64];
  int maprev;

  // vertexes

  Tab<Point3> verts;
  Tab<Edge> edges;
  Tab<int> surfEdges;
  Tab<char> pack;

  // faces
  Tab<Face> face;

  // models
  Tab<Model> mod;

  // entities
  Tab<Entity> ent;

  // original faces;
  Tab<Face> oface;

  Tab<Texinfo> tx;

  Tab<Texdata> td;
  String allStrings;

  Tab<String> texname;  // the string array
  Tab<ushort> macroTex; // the string id array

  Tab<GLump> glumps;
  struct StaticModel
  {
    HL2Model *model;
    String name;
    StaticModel() : model(NULL) {}
    void load(const String &models_dir, const String &textures_root)
    {
      if (!model)
        model = new HL2Model;
      else
        return;
      if (!model->load(models_dir + name, textures_root))
      {
        debug("Can't load model %s", (const char *)(models_dir + name));
        delete model;
        model = NULL;
      }
    }
  };
  Tab<StaticModel> psname;
  Tab<Pstatic> ps;

  void loadmap(IBaseLoad &b)
  {

    // Load the BSP header
    show_warning("Header");
    loadheader(b);

    // VERTEXES 3
    show_warning("Vertexes ");
    loadverts(b);

    show_warning("Edges ");
    loadedges(b);

    show_warning("SurfEdges ");
    loadsurfedges(b);

    // FACES 7
    show_warning("Faces ");
    loadfaces(b);

    // MODELS 14
    show_warning("Models ");
    loadmodels(b);

    // TEXINFOS 6
    show_warning("Texinfo ");
    loadtexinfos(b);


    // TEXDATASTRINGDATA 43
    show_warning("Texdata string data");
    loadtdsd(b);

    // TEXDATASTRINGTABLE 44
    show_warning("Texdata string table ");
    loadtdst(b);

    // TEXDATA 2
    show_warning("Texdata ");
    loadtexdatas(b);

    // ENTITIES 0
    show_warning("Entities ");
    loadentities(b);

    // ENTITIES 0
    show_warning("MacroTex ");
    loadmacro(b);

    show_warning("Statics");
    loadstatics(b);

    loadpack(b);

    transmodels(); // no origin move

    /*//ORIGINAL FACES 27
    show_warning("Orig faces ");
    loadorigfaces(b);

    */
  }

  void loadheader(IBaseLoad &b)
  {
    // Read the BSP Header

    b.seekto(0);

    ident = b.readInt();
    int idbsp = ('P' << 24) + ('S' << 16) + ('B' << 8) + 'V';
    version = b.readInt();

    if (ident != idbsp || version != 19)
    {
      show_warning("Unknown map file ident or version!");
      exit(1);
    }

    for (int i = 0; i < 64; i++)
    {
      lump[i] = new Lump();

      lump[i]->ofs = b.readInt();
      lump[i]->len = b.readInt();
      lump[i]->vers = b.readInt();
      lump[i]->fourCC = b.readInt();
      if (lump[i]->len > 0)
      {
        debug("%s %d %d\n", Lump::name(i), lump[i]->ofs, lump[i]->len);
      }
    }

    maprev = b.readInt();
  }

  void loadverts(IBaseLoad &b) // VERTEXES 3
  {
    b.seekto(lump[3]->ofs);

    int vertN = lump[3]->len / Lump::size(3);
    verts.resize(vertN);
    b.readTabData(verts);
  }

  void loadpack(IBaseLoad &b) // VERTEXES 3
  {
    b.seekto(lump[40]->ofs);

    int packN = lump[40]->len / Lump::size(40);
    pack.resize(packN);
    b.readTabData(pack);
  }

  void loadmacro(IBaseLoad &b) // VERTEXES 3
  {
    b.seekto(lump[47]->ofs);

    int macroN = lump[47]->len / Lump::size(47);
    debug("macroN %d", macroN);
    macroTex.resize(macroN);
    b.readTabData(macroTex);
  }

  void loadedges(IBaseLoad &b)
  {
    // EDGES 12
    b.seekto(lump[12]->ofs);

    int edgeN = lump[12]->len / Lump::size(12);
    debug("edges %d", edgeN);
    edges.resize(edgeN * 2);

    for (int i = 0; i < edgeN; i++)
    {

      edges[i + edgeN].v1 = edges[i].v0 = b.readIntP<2>();
      edges[i + edgeN].v0 = edges[i].v1 = b.readIntP<2>();
      if (edges[i].v0 > verts.size() || edges[i].v1 > verts.size())
      {
        show_warning("Vertex indices too high");
        exit(1);
      }
    }
  }

  void loadsurfedges(IBaseLoad &b)
  {
    // EDGES 12
    b.seekto(lump[13]->ofs);

    int edgeN = lump[13]->len / Lump::size(13);
    debug("sedges %d", edgeN);
    surfEdges.resize(edgeN);

    for (int i = 0; i < edgeN; i++)
      surfEdges[i] = b.readInt();
  }

  void loadfaces(IBaseLoad &b)
  {
    Tab<Face> oface(tmpmem);
    // debug("oface");
    // loadfaces(b, 27, oface);
    debug("face");
    loadfaces(b, 7, face);
  }

  void loadfaces(IBaseLoad &b, int id, Tab<Face> &faces)
  {
    // FACES 7

    b.seekto(lump[id]->ofs);

    int faceN = lump[id]->len / Lump::size(id);
    debug("faces %d", faceN);

    faces.resize(faceN);

    for (int i = 0; i < faceN; i++)
    {
      faces[i].pnum = (int)0xFFFF & b.readIntP<2>(); // planenum
      faces[i].side = b.readIntP<1>();               // side
      faces[i].onnode = b.readIntP<1>();             // onnode
      faces[i].fstedge = b.readInt();                // 1st edge
      faces[i].numedge = b.readIntP<2>();            // num edges
      faces[i].texinfo = b.readIntP<2>();            // texinfo
      faces[i].dispinfo = b.readIntP<2>();           // dispinfo
      b.readIntP<2>();                               // surfvolume

      b.readInt(); // styles[4]

      b.readInt();                  // lightofs
      faces[i].area = b.readReal(); // area
      b.readInt();                  // LightmapTexture[4]
      b.readInt();
      b.readInt();
      b.readInt();
      faces[i].orig = b.readInt();   // origface
      b.readIntP<2>();               // numprimitives
      b.readIntP<2>();               // firstprimitive
      faces[i].smooth = b.readInt(); // smoothgroups
    }
  }

  void loadmodels(IBaseLoad &b)
  {
    // MODELS 14

    b.seekto(lump[14]->ofs);
    int modelN = lump[14]->len / Lump::size(14);
    debug("models %d", modelN);

    mod.resize(modelN);

    for (int i = 0; i < modelN; i++)
    {
      b.readReal();                   // mxl
      b.readReal();                   // myl
      b.readReal();                   // mzl
      b.readReal();                   // mxh
      b.readReal();                   // myh
      b.readReal();                   // mzh
      mod[i].origin.x = b.readReal(); // mox
      mod[i].origin.y = b.readReal(); // moy
      mod[i].origin.z = b.readReal(); // moz
      mod[i].headnode = b.readInt();  // headnode
      mod[i].fstface = b.readInt();
      mod[i].numface = b.readInt();
      mod[i].entity = -1;
    }
  }

  void loadtexinfos(IBaseLoad &b)
  {
    // TEXINFO 6
    b.seekto(lump[6]->ofs);
    int texinfoN = lump[6]->len / Lump::size(6);
    debug("texinfo = %d", texinfoN);

    tx.resize(texinfoN);

    for (int i = 0; i < texinfoN; i++)
    {
      int j;
      for (j = 0; j < 2; j++)
      {
        for (int k = 0; k < 4; k++)
          tx[i].tv[j][k] = b.readReal(); // tex vecs texelspwu [2][4]
      }

      for (j = 0; j < 2; j++)
        for (int k = 0; k < 4; k++)
          tx[i].ltv[j][k] = b.readReal(); // tex vecs texelspwu [2][4]

      tx[i].flags = b.readInt();   // flags
      tx[i].texdata = b.readInt(); // texdata
    }
  }

  void loadtexdatas(IBaseLoad &b)
  {
    // TEXDATA 2
    b.seekto(lump[2]->ofs);
    int texdataN = lump[2]->len / Lump::size(2);
    debug("texdata=%d", texdataN);
    td.resize(texdataN);
    int minid = texdataN;
    for (int i = 0; i < texdataN; i++)
    {
      b.readReal();
      b.readReal();
      b.readReal();               // reflectivity vector
      td[i].nameid = b.readInt(); // name string ID
      td[i].width = b.readInt();
      td[i].height = b.readInt(); // width, heigh
      if (td[i].nameid >= texname.size())
        DAG_FATAL("invalid id");
      td[i].name = texname[td[i].nameid];
      b.readInt();
      b.readInt(); // view width, heigh
    }
    debug("minid %d", minid);
  }

  void loadtdsd(IBaseLoad &b)
  {
    // TEXDATA STRING DATA 43
    b.seekto(lump[43]->ofs);
    int tdsdN = lump[43]->len / Lump::size(43);

    allStrings.resize(tdsdN + 1);
    b.read(&allStrings[0], tdsdN); // the byte array
    allStrings[tdsdN] = 0;
  }


  void loadtdst(IBaseLoad &b)
  {
    // TEXDATA STRING TABLE 44
    b.seekto(lump[44]->ofs);
    int tdstN = lump[44]->len / Lump::size(44);
    debug("texnames %d", tdstN);

    texname.resize(tdstN);
    // texenv = new int[tdsts];        // the texture envmaps

    for (int i = 0; i < tdstN; i++)
    {
      int ix = b.readInt(); // index into allStrings
      if (ix >= allStrings.size())
        exit(1);
      texname[i] = &allStrings[ix]; // construct string from subarray
      debug("%d %d %s", i, ix, (char *)texname[i]);
    }
  }

  void loadgamelumpheader(IBaseLoad &b)
  {
    // GAME LUMP 35

    b.seekto(lump[35]->ofs);

    int glumpN = b.readInt(); // GL header count

    glumps.resize(glumpN);

    for (int i = 0; i < glumpN; i++)
    {
      glumps[i].id = b.readInt();
      glumps[i].flags = b.readIntP<2>();
      glumps[i].vers = b.readIntP<2>();

      glumps[i].ofs = b.readInt();
      glumps[i].len = b.readInt();
    }
  }
  void loadstatics(IBaseLoad &b)
  {
    loadgamelumpheader(b);
    // STATIC PROPS (Gamelump 35)
    int spid = -1;
    int i;
    for (i = 0; i < glumps.size(); i++)
    {
      if (glumps[i].id == 1936749168) // 'sprp'
        spid = i;
    }
    if (spid < 0)
      return;

    b.seekto(glumps[spid].ofs); // offset of sprp data
    int pslen = glumps[spid].len;

    int psnames = b.readInt(); // number of entries in dictionary
    psname.resize(psnames);

    for (i = 0; i < psnames; i++)
    {
      char name[129];
      b.read(name, 128);
      name[128] = 0;
      psname[i].name = name;
    }

    int propleaves = b.readInt(); // number of leafprops - not important
    b.seekrel(propleaves * 2);

    int propstatics = b.readInt();

    ps.resize(propstatics);

    for (int i = 0; i < propstatics; i++)
    {
      ps[i].ox = b.readReal(); // origin vector
      ps[i].oy = b.readReal();
      ps[i].oz = b.readReal();

      ps[i].ax = DegToRad(b.readReal()); // angles pyr
      ps[i].ay = DegToRad(b.readReal());
      ps[i].az = DegToRad(b.readReal());

      ps[i].type = b.readIntP<2>(); // prop_static type

      b.readIntP<2>(); // first leaf
      b.readIntP<2>(); // leaf count
      ps[i].solid = b.readIntP<1>();
      ps[i].flags = b.readIntP<1>();
      ps[i].skin = b.readInt();
      ps[i].fademin = b.readReal();
      ps[i].fademax = b.readReal();

      b.readReal(); // lighting vec
      b.readReal();
      b.readReal();
      if (glumps[spid].vers == 5)
        b.readInt(); // pad if version 4
    }
  }

  void loadentities(IBaseLoad &b) // load entities
  {
    // ENTITIES 0

    // first run through to find number of entities
    b.seekto(lump[0]->ofs);

    int endent = lump[0]->ofs + lump[0]->len;
    int numents = 0;
    String line;

    // quick and dirty - count the "}"'s
    while (b.tell() < endent)
    {
      line = readline(b, endent);
      if (b.tell() > endent)
        break;
      if (line == "}")
        numents++;
    }
    debug("entities %d", numents);
    int entities = numents;
    b.seekto(lump[0]->ofs); // reset to beginning of entity lump
    ent.resize(numents);    // the entity array
    Entity nent;
    bool eoe = false, eoel = false;

    numents = 0;

    while (!eoel)
    {
      while (!eoe) // parse each entity
      {
        line = readline(b, endent);
        debug(line);
        // show_warning(line);
        if (b.tell() > endent) // end of entity lump
        {
          eoel = true;
          break;
        }
        if (line == "{") // start of entity
        {

          nent.targetname.printf(128, "undefined__%d", numents);
          nent.index = numents; // position in the array
          nent.model = -1;
          nent.o = Point3(0, 0, 0); // default origin
          nent.mark = false;        // marker for tree building
          // show_warning("SOE");
          continue;
        }
        if (line == "}") // end of entity
        {
          // show_warning("EOE");
          eoe = true;
          break;
        }

        String keyval[10];
        char *token = strtok(line, "\"");
        int vals = 0;
        while (token != NULL && vals < 10)
        {
          keyval[vals++] = token;
          token = strtok(NULL, "\"");
        }
        if (vals == 3)
        {
          String ckey(keyval[0]); // find key/value pair
          String cval(keyval[2]);
          // show_warning(ckey+" "+cval);

          // nent.key.add(ckey);             // store all key/values
          // nent.value.add(cval);
          // nent.link.add(null);

          // parse known entities
          // if(ckey==("no_decomp"))
          //   no_decomp = true;            // protected map
          if (ckey == ("classname"))
            nent.classname = cval;
          else if (ckey == ("targetname"))
            nent.targetname = cval;
          else if (ckey == "parentname")
            nent.parentname = cval;
          else if (ckey == "model")
          {
            debug("model %s", &cval[1]);
            if (cval[0] == '*')
              nent.model = atoi(&cval[1]);
            else
              nent.model = -1; // prop model
          }
          else if (ckey == "origin")
          {
            float x, y, z;
            sscanf(cval, "%f %f %f", &x, &y, &z);
            nent.o = Point3(x, y, z);
          }
          else if (ckey == "angles")
          {
            float x, y, z;
            sscanf(cval, "%f %f %f", &x, &y, &z);
            nent.o = Point3(x, y, z);
          }
          else if (nent.con < 0) // try to gauge start of connections block
          {                      // not found yet
            // connection
            /*
              String [] constr = cval.split(","); // split value at comma
              if(constr.length == 5)              // five fields, probably a connection
              {
                  if(constr[4].equals("1") || constr[4].equals("-1"))     // last field is 1 or -1
                  {                               // almost certainly a connection
                      nent.con = nent.key.size() - 1;     // this kv is start of connection block
                  }
              }
              */
          }
        }
      }
      if (!eoel)
      {
        if (nent.model >= 0)
          mod[nent.model].entity = numents; // register entity with model
        ent[numents] = nent;                // assign the loaded entity to the array

        eoe = false;
        // show_warning("Entity: "+numents);
        numents++;
      }
    }
    ent[0].model = 0;
    mod[ent[0].model].entity = 0;

    if (numents != entities) // check that quick count and loaded count of entities match
      show_warning("Error! Entity count mismatch");
  }

  void transmodels() {}

  String readline(IBaseLoad &b, int end)
  {
    String linebuff;
    char c;
    while ((c = (char)b.readIntP<1>()) != '\n' && b.tell() < end)
    {
      linebuff.push_back(c);
    }
    linebuff.push_back(0);
    return linebuff;
  }

  String readstr(IBaseLoad &b, int len) // read len bytes, returning string up to first null
  {
    String strbuff;
    char c;
    int pos = 0;
    while ((c = (char)b.readIntP<1>()) != 0 && ++pos < len)
      strbuff.push_back(c); // read until null terminator

    while (++pos < len)
      b.readIntP<1>(); // read rest of null bytes to len

    return strbuff;
  }

  Point2 uvFromXYZ(Point3 &v, float texelsPerWorldUnits[2][4], float subtractOffset[2], int wd, int ht)
  {
    Point2 uv;
    int sz[2] = {wd, ht};
    for (int iCoord = 0; iCoord < 2; iCoord++)
    {
      float &npDestCoord = uv[iCoord];

      double pDestCoord = 0;
      for (int iDot = 0; iDot < 3; iDot++)
        pDestCoord += v[iDot] * texelsPerWorldUnits[iCoord][iDot];

      pDestCoord += texelsPerWorldUnits[iCoord][3];
      pDestCoord -= subtractOffset[iCoord];
      pDestCoord /= sz[iCoord];
      npDestCoord = pDestCoord;
    }
    return uv;
  }
  static void adjwtm(TMatrix &m)
  {
    for (int i = 0; i < 4; ++i)
    {
      float a = m[i][1];
      m[i][1] = m[i][2];
      m[i][2] = a;
    }
  }

  bool save(const char *dagfn, const char *zipfn, const char *root_textures, const char *modelsDir)
  {
    String root_tex_dir(root_textures);
    String models_dir(modelsDir);
    FullFileSaveCB zipCB(zipfn);
    zipCB.writeTabData(pack);

    DagSaver sav;

    if (!sav.start_save_dag(dagfn))
      return false;
    try
    {
      Tab<int> matToTexnames(tmpmem);
      Tab<String> strtexnames(tmpmem);
      matToTexnames.resize(td.size());
      // Tab<unsigned short> allMats(tmpmem);
      // allMats.resize(td.size());
      for (int ti = 0; ti < td.size(); ti++)
      {
        String vtf;
        vtf = root_tex_dir + td[ti].name + extension;
        unify(&vtf[0]);
        readVMT(root_tex_dir, td[ti].name + ".vmt", vtf);

        int oi;
        for (oi = 0; oi < strtexnames.size(); ++oi)
        {
          if (stricmp(strtexnames[oi], vtf) == 0)
            break;
        }
        if (oi >= strtexnames.size())
        {
          oi = strtexnames.size();
          strtexnames.push_back(vtf);
        }
        matToTexnames[ti] = oi;
      }
      Tab<DagExpMater> materials(tmpmem);
      for (int ti = 0; ti < td.size(); ti++)
      {
        DagExpMater mater;
        memset(&mater, 0, sizeof(mater));
        mater.name = td[ti].name;
        mater.clsname = "simple";
        mater.script = "lighting=lightmap";
        DagColor col;
        col.r = col.g = col.b = 255;
        mater.amb = mater.diff = mater.spec = col;
        mater.power = 0;
        mater.flags = 0;
        memset(mater.texid, 0xFF, sizeof(mater.texid));
        mater.texid[0] = matToTexnames[ti];
        // allMats[ti] = ti;
        materials.push_back(mater);
      }
      int statics;
      int totalLoaded = 0;
      for (statics = 0; statics < ps.size(); ++statics)
      {
        psname[ps[statics].type].load(models_dir, root_tex_dir);
        if (psname[ps[statics].type].model)
        {
          psname[ps[statics].type].model->addTexturesMaterials(strtexnames, materials);
          totalLoaded++;
        }
      }

      Tab<const char *> texnames(tmpmem);
      for (int t = 0; t < strtexnames.size(); ++t)
      {
        int tnm = append_items(texnames, 1);
        texnames[tnm] = strtexnames[t];
      }
      if (!sav.save_textures(texnames.size(), &texnames[0]))
        throw 0;

      for (int mi = 0; mi < materials.size(); mi++)
        sav.save_mater(materials[mi]);
      if (!sav.start_save_nodes())
        throw 0;

      for (int i = 0; i < mod.size(); i++)
      {
        if (mod[i].entity < 0)
          continue;
        Entity &entity = ent[mod[i].entity];
        if (stricmp(entity.classname, "worldspawn") != 0)
          continue;
        totalLoaded++;
      }
      TMatrix rootwtm = TMatrix::IDENT;
      rootwtm[0][0] = rootwtm[1][1] = rootwtm[2][2] = 0.0254f;
      if (!sav.start_save_node("root", rootwtm, 0, totalLoaded))
        throw 0;

      for (statics = 0; statics < ps.size(); ++statics)
      {
        if (psname[ps[statics].type].model)
        {
          TMatrix wtm = TMatrix::IDENT;
          wtm = PYRTM(Point3(ps[statics].ax, ps[statics].ay, ps[statics].az));
          wtm.setcol(3, ps[statics].ox, ps[statics].oy, ps[statics].oz);
          adjwtm(wtm);
          psname[ps[statics].type].model->saveNodes(sav, wtm);
        }
      }

      for (int i = 0; i < mod.size(); i++)
      {
        debug("model entity %d ", mod[i].entity);
        if (mod[i].entity < 0)
          continue;

        Entity &entity = ent[mod[i].entity];
        if (stricmp(entity.classname, "worldspawn") != 0)
          continue;
        TMatrix wtm;
        wtm.identity();
        wtm.setcol(3, entity.o);
        debug("%s", ensureUnique((char *)entity.targetname));
        // debug("%s %f %f %f",(char*)entity.targetname, mod[i].origin.x, mod[i].origin.y, mod[i].origin.z);
        adjwtm(wtm);
        if (!sav.start_save_node(entity.targetname, wtm))
          throw 0;
        Tab<DagBigFace> fc(tmpmem);
        Tab<ushort> usedMaterials(tmpmem);
        fc.reserve(mod[i].numface);
        Tab<int> centVert(tmpmem);
        centVert.resize(mod[i].numface);
        mem_set_ff(centVert);
        for (int mfi = 0; mfi < mod[i].numface; ++mfi)
        {
          int fIndex = mod[i].fstface + mfi;
          if (face[fIndex].numedge <= 4)
            continue;
          int edge1 = face[fIndex].fstedge;
          int sEdge1 = surfEdges[edge1];
          if (sEdge1 < 0)
            sEdge1 = edges.size() / 2 - sEdge1;

          int edge2 = face[fIndex].fstedge + (face[fIndex].numedge / 2);
          int sEdge2 = surfEdges[edge2];
          if (sEdge2 < 0)
            sEdge2 = edges.size() / 2 - sEdge2;
          Point3 cvert = 0.5f * (verts[edges[sEdge1].v0] + verts[edges[sEdge2].v0]);
          centVert[mfi] = verts.size();
          verts.push_back(cvert);
        }

        Tab<int> vertUsed(tmpmem);
        Tab<int> vertsMap(tmpmem);

        vertsMap.resize(verts.size());
        mem_set_ff(vertsMap);
        Tab<DagBigTFace> tface(tmpmem);
        Tab<Point2> tVerts(tmpmem), ltVerts(tmpmem);
        BBox3 bbox;
        BBox2 tbox;
        Tab<Point3> faceNormals(tmpmem);
        Tab<unsigned> smgr(tmpmem);
        Tab<unsigned> usmgr(tmpmem);
        usmgr.resize(mod[i].numface);
        mem_set_0(usmgr);
        smgr.resize(mod[i].numface);
        mem_set_0(smgr);
        faceNormals.resize(mod[i].numface);
        Tab<Tab<int>> edgeNeighbors(tmpmem);
        edgeNeighbors.resize(surfEdges.size());
        int mfi;
        for (int mfi = 0; mfi < mod[i].numface; ++mfi)
        {
          int fIndex = mod[i].fstface + mfi;
          int edge1 = face[fIndex].fstedge;
          int sEdge1 = surfEdges[edge1];
          if (sEdge1 < 0)
            sEdge1 = edges.size() / 2 - sEdge1;

          int edge2 = face[fIndex].fstedge + 1;
          int sEdge2 = surfEdges[edge2];
          if (sEdge2 < 0)
            sEdge2 = edges.size() / 2 - sEdge2;
          faceNormals[mfi] =
            normalize((verts[edges[sEdge1].v0] - verts[edges[sEdge1].v1]) % (verts[edges[sEdge2].v1] - verts[edges[sEdge2].v0]));
          for (int ei = 0, edge = face[fIndex].fstedge; ei < face[fIndex].numedge; ++edge, ++ei)
          {
            int sEdge = surfEdges[edge];
            if (sEdge < 0)
            {
              edgeNeighbors[-sEdge].push_back(mfi);
            }
            else
              edgeNeighbors[sEdge].push_back(mfi);
          }
        }
        // smgr[0] = 1;
        for (mfi = mod[i].numface - 1; mfi >= 0; --mfi)
        {
          if (faceNormals[mfi][0] >= 0.5)
            smgr[mfi] |= 1;
          if (faceNormals[mfi][0] <= -0.5)
            smgr[mfi] |= 2;
          if (faceNormals[mfi][1] >= 0.5)
            smgr[mfi] |= 4;
          if (faceNormals[mfi][1] <= -0.5)
            smgr[mfi] |= 8;
          if (faceNormals[mfi][2] >= 0.5)
            smgr[mfi] |= 16;
          if (faceNormals[mfi][2] <= -0.5)
            smgr[mfi] |= 32;
        }
        printf("smoothing...\n");
        /*
        Tab<int> visit(tmpmem);
        Bitarray visited;
        visit.resize(mod[i].numface);
        visited.resize(mod[i].numface);
        visited.reset();

        for (mfi=mod[i].numface-1; mfi>=0; --mfi) {
          visit.push_back(mfi);
        }
        printf("smoothing...\n");
        int grps = 0;
        while(visit.size()) {
          mfi = visit.back();
          visited.set(mfi);
          visit.pop_back();
          if (!smgr[mfi])
            smgr[mfi] = 1<<(((grps++)%31)+1);
          int fIndex = mod[i].fstface + mfi;
          for (int ei = 0, edge = face[fIndex].fstedge; ei < face[fIndex].numedge; ++edge, ++ ei) {
            int sEdge = surfEdges[edge];
            if (sEdge<0)
              sEdge = -sEdge;
            for(int n=0; n<edgeNeighbors[sEdge].size(); ++n){
              int fN = edgeNeighbors[sEdge][n];
              if (mfi == fN)
                continue;
              if ((faceNormals[mfi]*faceNormals[fN])>0.5f) {
                if (!visited[fN])
                  for(int vi=0; vi<visit.size(); ++vi){
                    if (visit[vi] == fN) {
                      visit.erase(vi,1);
                      visit.push_back(fN);
                      break;
                    }
                  }
                if ((usmgr[fN]&smgr[mfi])||(usmgr[mfi]&smgr[fN]) && smgr[mfi]!=0xFFFFFFFF) {
                  unsigned sub = (~usmgr[fN])&smgr[mfi];
                  unsigned sub2 = (~usmgr[mfi])&smgr[fN];
                  if (usmgr[fN] != smgr[mfi] && (sub!=0 || sub2!=0)) {
                    if (sub)
                      smgr[fN] |= sub;
                    else
                      smgr[mfi] |= sub2;
                  } else {
                    for(int i=31; i>=0; --i)
                      if (!(smgr[mfi]&(((unsigned)1)<<i))) {
                        smgr[fN] |= (((unsigned)1)<<(i));
                        smgr[mfi] |= (((unsigned)1)<<(i));
                        break;
                      }
                  }
                } else
                  smgr[fN] |= smgr[mfi];
                //neighboors
              } else {
                smgr[fN] &= ~smgr[mfi];
                usmgr[fN] |= smgr[mfi];
                usmgr[mfi]|= smgr[fN];
              }
            }
          }
        }*/
        Tab<unsigned char> faceFlags(tmpmem);

        for (mfi = 0; mfi < mod[i].numface; ++mfi)
        {
          int fIndex = mod[i].fstface + mfi;
          DagBigFace nFace;
          // nFace.smgr = face[fIndex].smooth;
          nFace.smgr = smgr[mfi];
          if (!nFace.smgr)
            nFace.smgr = 1 << 31; // mfi%31+1;
          int macro = macroTex[fIndex];
          if (macro == 0xFFFF)
            macro = tx[face[fIndex].texinfo].texdata;
          else
            debug("macro! %d %d", tx[face[fIndex].texinfo].texdata, macro);
          int mat = macro;
          nFace.mat = mat;
          int mi;
          for (mi = 0; mi < usedMaterials.size(); ++mi)
            if (usedMaterials[mi] == mat)
            {
              nFace.mat = mi;
              break;
            }

          if (mi >= usedMaterials.size())
          {
            nFace.mat = usedMaterials.size();
            usedMaterials.push_back(mat);
          }

          float zero[2] = {0, 0};
          if (face[fIndex].numedge > 4)
          {
            int stTVert = tVerts.size();
            tVerts.push_back(uvFromXYZ(verts[centVert[mfi]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
            for (int ei = 0, edge = face[fIndex].fstedge; ei < face[fIndex].numedge; ++edge, ++ei)
            {
              int sEdge = surfEdges[edge];
              if (sEdge < 0)
                sEdge = edges.size() / 2 - sEdge;
              nFace.v[0] = centVert[mfi];
              nFace.v[1] = edges[sEdge].v0;
              nFace.v[2] = edges[sEdge].v1;
              DagBigTFace tFace;
              unsigned char flag = FACEFLG_EDGE1;
              tFace.t[0] = stTVert;
              tFace.t[1] = tVerts.size();
              tVerts.push_back(uvFromXYZ(verts[nFace.v[1]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
              tFace.t[2] = tVerts.size();
              tVerts.push_back(uvFromXYZ(verts[nFace.v[2]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
              tbox += tVerts.back();
              for (int v = 0; v < 3; ++v)
              {
                if (vertsMap[nFace.v[v]] == -1)
                {
                  bbox += verts[nFace.v[v]];
                  vertsMap[nFace.v[v]] = vertUsed.size();
                  vertUsed.push_back(nFace.v[v]);
                }
                nFace.v[v] = vertsMap[nFace.v[v]];
              }
              int fi = fc.size();
              fc.push_back(nFace);
              faceFlags.push_back(flag);
              tface.push_back(tFace);
            }
          }
          else
          {
            int sEdge = surfEdges[face[fIndex].fstedge];
            if (sEdge < 0)
              sEdge = edges.size() / 2 - sEdge;
            centVert[mfi] = edges[sEdge].v0;
            int stTVert = tVerts.size();
            tVerts.push_back(uvFromXYZ(verts[centVert[mfi]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
            for (int ei = 1, edge = face[fIndex].fstedge + 1; ei < face[fIndex].numedge - 1; ++edge, ++ei)
            {
              int sEdge = surfEdges[edge];
              if (sEdge < 0)
                sEdge = edges.size() / 2 - sEdge;
              nFace.v[0] = centVert[mfi];
              nFace.v[1] = edges[sEdge].v0;
              nFace.v[2] = edges[sEdge].v1;
              DagBigTFace tFace;
              unsigned char flag = FACEFLG_EDGE1;
              if (ei == 1)
                flag |= FACEFLG_EDGE0;
              if (ei == face[fIndex].numedge - 2)
                flag |= FACEFLG_EDGE2;
              tFace.t[0] = stTVert;
              tFace.t[1] = tVerts.size();
              tVerts.push_back(uvFromXYZ(verts[nFace.v[1]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
              tFace.t[2] = tVerts.size();
              tVerts.push_back(uvFromXYZ(verts[nFace.v[2]], tx[face[fIndex].texinfo].tv, zero, td[mat].width, td[mat].height));
              tbox += tVerts.back();
              for (int v = 0; v < 3; ++v)
              {
                if (vertsMap[nFace.v[v]] == -1)
                {
                  bbox += verts[nFace.v[v]];
                  vertsMap[nFace.v[v]] = vertUsed.size();
                  vertUsed.push_back(nFace.v[v]);
                }
                nFace.v[v] = vertsMap[nFace.v[v]];
              }
              int fi = fc.size();
              fc.push_back(nFace);
              faceFlags.push_back(flag);
              tface.push_back(tFace);
            }
          }
        }

        if (fc.size())
        {
          Tab<Point3> uVerts(tmpmem);
          uVerts.resize(vertUsed.size());
          // flipFaces(&fc[0], fc.size());
          // flipFaces(&tface[0], tface.size());
          invertV(&tVerts[0], tVerts.size());
          // invertV(&ltVerts[0], ltVerts.size());

          for (int i = 0; i < vertUsed.size(); ++i)
          {
            uVerts[i] = verts[vertUsed[i]];
          }
          // usedMaterials = allMats;
          if (!sav.save_node_mats(usedMaterials.size(), &usedMaterials[0]))
            throw 0;
          Tab<int> degenerates(tmpmem);
          optimize_verts((DagBigFaceT *)&fc[0], fc.size(), uVerts, length(bbox.width()) * 0.001, &degenerates);
          optimize_verts(&tface[0], tface.size(), tVerts, 0.0001);
          for (int di = 0; di < degenerates.size(); ++di)
          {
            erase_items(fc, degenerates[di], 1);
            erase_items(tface, degenerates[di], 1);
          }

          DagBigMapChannel ch[2];
          ch[0].tverts = &tVerts[0].x;
          ch[0].tface = &tface[0];
          ch[0].numtv = tVerts.size();
          ch[0].num_tv_coords = 2;
          ch[0].channel_id = 1;

          int c = 1;

          if (!sav.save_dag_bigmesh(uVerts.size(), uVerts.data(), fc.size(), fc.data(), c, ch, faceFlags.data()))
            throw 0;
        }
        if (!sav.end_save_node())
          throw 0;
      }
      if (!sav.end_save_node())
        throw 0;
      if (!sav.end_save_nodes())
        throw 0;
    }
    catch (int)
    {
      sav.end_save_dag();
      return false;
    }
    return sav.end_save_dag();
  }
  /*
  Texture gettex(short index, Vec origin, DecimalFormat fp)
  {
      Texture tex = new Texture();

      if(index<0)
          return tex;
      Texinfo ti = tx[index];


      tex.texinfo = ti;
      if(ti.texdata<0)
          return tex;
      tex.texdata = td[ti.texdata];

      tex.name = texname[tex.texdata.nameid];

      tex.envmap = texenv[tex.texdata.nameid];

      float [][] tvec = ti.tv;
      double uaxis[]= new double[3];
      double vaxis[]= new double[3];
      double shift0, shift1, tw0, tw1, dotp0, dotp1;

      tw0 = 1.0 / Math.sqrt(Math.pow(tvec[0][0],2) + Math.pow(tvec[0][1],2) + Math.pow(tvec[0][2],2) );
      tw1 = 1.0 / Math.sqrt(Math.pow(tvec[1][0],2) + Math.pow(tvec[1][1],2) + Math.pow(tvec[1][2],2) );

      for(int i=0;i<3;i++)
      {
          uaxis[i] = tvec[0][i] * tw0;
          vaxis[i] = tvec[1][i] * tw1;
      }

      Vec t0 = new Vec(tvec[0][0], tvec[0][1], tvec[0][2]);
      Vec t1 = new Vec(tvec[1][0], tvec[1][1], tvec[1][2]);

      dotp0 = origin.dot(t0);
      dotp1 = origin.dot(t1);

      shift0 = tvec[0][3] - dotp0;
      shift1 = tvec[1][3] - dotp1;

      tex.uaxis = "["+fp.format(uaxis[0])+" "+fp.format(uaxis[1])+" "+fp.format(uaxis[2])+" "+fp.format(shift0)+"] "+fp.format(tw0);
      tex.vaxis = "["+fp.format(vaxis[0])+" "+fp.format(vaxis[1])+" "+fp.format(vaxis[2])+" "+fp.format(shift1)+"] "+fp.format(tw1);

      return tex;
  }

  Vec getVec(ByteBuffer b)
  {
      Vec v = new Vec();
      v.x = b.getFloat();
      v.y = b.getFloat();
      v.z = b.getFloat();
      return v;
  }*/
} bsp;

//==========================================================================//

static HL2Model mod;
int DagorWinMain(bool debugmode)
{
  dd_add_base_path("");
  extension = ".tga";

  fprintf(stderr, "\n BSP Tool v0.01\n"
                  "Copyright (C) Gaijin Games KFT, 2023\n\n");

  String input[3];
  input[2] = "./";
  input[1] = "./";
  int inp = 0;

  for (int i = 1; i < __argc; ++i)
  {
    const char *s = __argv[i];

    if (*s == '-' || *s == '/')
    {
      ++s;
      // options
    }
    else
    {
      input[inp++] = s;
      if (inp >= 3)
        break;
    }
  }
  if (strstr(input[0], ".mdl"))
  {
    if (inp < 2) // usage
      exit(0);
    fprintf(stderr, "Loading mod from '%s' '%s'...\n", (const char *)input[0], (const char *)input[1]);
    if (mod.load(input[0], input[1]))
      mod.save(input[0]);
  }
  else
  {
    if (inp < 2) // usage
      exit(0);
    fprintf(stderr, "Loading bsp from '%s' materials=<%s>...\n", (const char *)input[0], (const char *)input[1]);
    bsp.loadmap(FullFileLoadCB(input[0]));
    bsp.save(input[0] + ".dag", input[0] + ".zip", input[1], input[2]);
  }
  fprintf(stderr, "\nDone.\n");

  return 0;
}
