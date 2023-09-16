#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ############################################################################
// ##                                                                        ##
// ##  Q3BSP.CPP                                                             ##
// ##                                                                        ##
// ##  Class to load Quake3 BSP files, interpret them, organize them into    ##
// ##  a triangle mesh, and save them out in various ASCII file formats.     ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################

#include "q3bsp.h"
#include "q3shader.h"
#include "patch.h"

#include "fload.h"
#include "loadbmp.h"


Quake3BSP::Quake3BSP(const StringRef &fname, const StringRef &location, const StringRef &code, bool lm)
{
  mMesh = 0;

  mOk = false;
  mName = fname;
  mCodeName = code;

  StdString ifoo = fname;
  StdString str = fname;
  if (!strstr(fname, ".bsp"))
    str = ifoo + ".bsp";

  Fload data(str);

  void *mem = data.GetData();
  int len = data.GetLen();
  if (mem)
  {

    mOk = mHeader.SetHeader(mem);

    if (mOk)
    {
      ReadFaces(mem);
      ReadElements(mem);
      ReadVertices(mem);
      if (lm)
        ReadLightmaps(location, mem);
      ReadShaders(mem);
      BuildVertexBuffers();
    }
  }
}

Quake3BSP::~Quake3BSP(void) { delete mMesh; }


bool QuakeHeader::SetHeader(const void *mem) // returns true if valid quake header.
{
  const int *ids = (const int *)mem;
#define BSPHEADERID (*(int *)"IBSP")
#define BSPVERSION  46
  if (ids[0] != BSPHEADERID)
    return false;
  if (ids[1] != BSPVERSION)
    return false;
  mId = ids[0];
  mVersion = ids[1];
  const QuakeLump *lump = (const QuakeLump *)&ids[2];
  for (int i = 0; i < NUM_LUMPS; i++)
    mLumps[i] = *lump++;
  return true;
}

void Quake3BSP::ReadShaders(const void *mem)
{
  assert(mOk);
  int lsize;
  int lcount;

  const unsigned char *smem = (const unsigned char *)mHeader.LumpInfo(Q3_SHADERREFS, mem, lsize, lcount);

  mShaders.clear();
  mShaders.reserve(lcount);
  printf("shaders: %d\n", lcount);
  for (int i = 0; i < lcount; i++)
  {
    ShaderReference s(smem);
    mShaders.push_back(s);
    smem += lsize;
  }
}

const void *QuakeHeader::LumpInfo(QuakeLumps lump, const void *mem, int &lsize, int &lcount)
{

  switch (lump)
  {
    case Q3_SHADERREFS: lsize = 64 + sizeof(int) * 2; break;
    case Q3_PLANES:
      lsize = sizeof(float) * 4; // size of a plane equations.
      break;
    case Q3_NODES: lsize = sizeof(int) * 9; break;
    case Q3_LEAFS: lsize = sizeof(int) * 12; break;
    case Q3_FACES: lsize = sizeof(int) * 26; break;
    case Q3_VERTS: lsize = sizeof(int) * 11; break;
    case Q3_LFACES: lsize = sizeof(int); break;
    case Q3_ELEMS: lsize = sizeof(int); break;
    case Q3_MODELS: lsize = sizeof(int) * 10; break;
    case Q3_LIGHTMAPS: lsize = 1; break;
    case Q3_VISIBILITY: lsize = 1; break;
    default: assert(0); // unsupported query type
  }

  int flen = mLumps[lump].GetFileLength();

  assert(flen);
  assert((flen % lsize) == 0); // must be evenly divisible by lump size.
  lcount = flen / lsize;       // number of lump items.

  const char *foo = (const char *)mem;
  foo = &foo[mLumps[lump].GetFileOffset()];
  return (const void *)foo;
}

QuakeNode::QuakeNode(const int *node)
{
  mPlane = node[0];
  mLeftChild = node[1];
  mRightChild = node[2];

  mBound.r1.x = float(node[3]);
  mBound.r1.y = float(node[4]);
  mBound.r1.z = float(node[5]);
  mBound.r2.x = float(node[6]);
  mBound.r2.y = float(node[7]);
  mBound.r2.z = float(node[8]);
}

QuakeLeaf::QuakeLeaf(const int *node)
{
  mCluster = node[0];
  mArea = node[1];

  mBound.r1.x = float(node[2]);
  mBound.r1.y = float(node[3]);
  mBound.r1.z = float(node[4]);

  mBound.r2.x = float(node[5]);
  mBound.r2.y = float(node[6]);
  mBound.r2.z = float(node[7]);

  mFirstFace = node[8];
  mFaceCount = node[9];
  mFirstUnknown = node[10];
  mNumberUnknowns = node[11];
}

QuakeFace::QuakeFace(const int *face)
{
  mFrameNo = 0;

  const float *fface = (const float *)face;

  mShader = face[0];
  mUnknown = face[1];

  mType = (FaceType)face[2];

  mFirstVertice = face[3];
  mVcount = face[4];
  mFirstElement = face[5];
  mEcount = face[6];
  mLightmap = face[7];
  mOffsetX = face[8];
  mOffsetY = face[9];
  mSizeX = face[10];
  mSizeY = face[11];

  mOrig.x = fface[12];
  mOrig.y = fface[13];
  mOrig.z = fface[14];

  mBound.r1.x = fface[15];
  mBound.r1.y = fface[16];
  mBound.r1.z = fface[17];

  mBound.r2.x = fface[18];
  mBound.r2.y = fface[19];
  mBound.r2.z = fface[20];

  mNormal.x = fface[21];
  mNormal.y = fface[22];
  mNormal.z = fface[23];

  mControlX = face[24];
  mControlY = face[25];

  static int faceno = 0;

  if (mType == FACETYPE_TRISURF)
  {
    printf("Face: %d\n", faceno++);
    printf("Shader: %d\n", mShader);
    printf("Unknown: %d\n", mUnknown);
    printf("Type: %d\n", mType);
    printf("FirstVertice: %d\n", mFirstVertice);
    printf("Vcount: %d\n", mVcount);
    printf("FirstElement: %d\n", mFirstElement);
    printf("Ecount: %d\n", mEcount);
    printf("Lightmap: %d\n", mLightmap);
    printf("OffsetX: %d\n", mOffsetX);
    printf("OffsetY: %d\n", mOffsetY);
    printf("SizeX: %d\n", mSizeX);
    printf("SizeY: %d\n", mSizeY);
    printf("ControlX: %d\n", mControlX);
    printf("ControlY: %d\n", mControlY);
  }
}

QuakeFace::~QuakeFace(void) {}

QuakeVertex::QuakeVertex(const int *vert)
{
  const float *fvert = (const float *)vert;

  mPos.x = fvert[0];
  mPos.y = fvert[1];
  mPos.z = fvert[2];

  mTexel1.x = fvert[3];
  mTexel1.y = fvert[4];

  mTexel2.x = fvert[5];
  mTexel2.y = fvert[6];

  mNormal.x = fvert[7];
  mNormal.y = fvert[8];
  mNormal.z = fvert[9];

  mColor = (unsigned int)vert[10];
}

void Quake3BSP::ReadFaces(const void *mem)
{
  assert(mOk);
  int lsize;
  int lcount;
  const int *faces = (const int *)mHeader.LumpInfo(Q3_FACES, mem, lsize, lcount);
  assert(lsize == sizeof(int) * 26);

  mFaces.clear();
  mFaces.reserve(lcount);

  for (int i = 0; i < lcount; i++)
  {
    QuakeFace f(faces);

    mFaces.push_back(f);

    faces += 26;
  }
}

/*void Quake3BSP::ReadModels(const void *mem)
{
  assert( mOk );
  int lsize;
  int lcount;
  const char *faces = (const int *) mHeader.LumpInfo(Q3_MODELS,mem,lsize,lcount);

  for (int i=0; i<lcount; i++)
  {
    //*b.readReal(); //mxl
    b.readReal(); //myl
    b.readReal(); //mzl
    b.readReal(); //mxh
    b.readReal(); //myh
    b.readReal(); //mzh
    mod[i].origin.x=b.readReal(); //mox
    mod[i].origin.y=b.readReal(); //moy
    mod[i].origin.z=b.readReal(); //moz
    mod[i].headnode = b.readInt(); //headnode
    mod[i].fstface = b.readInt();
    mod[i].numface = b.readInt();
    mod[i].entity = -1;
    //

    faces+=lsize;
  }
}*/

void Quake3BSP::ReadVertices(const void *mem)
{
  assert(mOk);
  int lsize;
  int lcount;
  const int *vertices = (const int *)mHeader.LumpInfo(Q3_VERTS, mem, lsize, lcount);
  assert(lsize == sizeof(int) * 11);

  mVertices.clear();
  mVertices.reserve(lcount);

  mBound.InitMinMax();

  for (int i = 0; i < lcount; i++)
  {
    QuakeVertex v(vertices);

/// 1.0f/45.0f
#define RECIP (0.0254)

    v.mPos.x *= RECIP;
    v.mPos.y *= -RECIP;
    v.mPos.z *= RECIP;

    mBound.MinMax(v.mPos);
    mVertices.push_back(v);
    vertices += 11;
  }
}

void Quake3BSP::ReadLightmaps(const char *location, const void *mem)
{
  assert(mOk);

  int lsize;
  int lcount;
  const unsigned char *maps = (const unsigned char *)mHeader.LumpInfo(Q3_LIGHTMAPS, mem, lsize, lcount);
  unsigned char *lmaps = new unsigned char[lcount];
  memcpy(lmaps, maps, lcount);

  for (int j = 0; j < lcount; j++)
  {
    unsigned int c = lmaps[j];
    lmaps[j] = (unsigned char)c;
  }

#define LMWID  128
#define LMHIT  128
#define LMSIZE (LMWID * LMHIT * 3)

  int texcount = lcount / LMSIZE;

  Bmp bmp;

  unsigned char *map = lmaps;

  for (int i = 0; i < texcount; i++)
  {
    char scratch[256];
    sprintf(scratch, "%slm%s%02d", (const char *)location, (const char *)mCodeName, i);
    StdString sname = scratch;
    StdString bname = sname + ".bmp";
    bmp.SaveBMP(bname.c_str(), map, LMWID, LMHIT, 3);
    map += LMSIZE;
  }


  delete lmaps;
}

void Quake3BSP::ReadElements(const void *mem)
{
  assert(mOk);
  int lsize;
  int lcount;
  const int *elements = (const int *)mHeader.LumpInfo(Q3_ELEMS, mem, lsize, lcount);
  assert(lsize == sizeof(int));

  mElements.clear();
  mElements.reserve(lcount);

  for (int i = 0; i < lcount; i++)
  {
    unsigned short ic = (unsigned short)(*elements);
    mElements.push_back(ic);
    elements++;
  }
}

void QuakeVertex::Get(LightMapVertex &vtx) const
{
  vtx.mPos.x = mPos.x;
  vtx.mPos.y = mPos.y;
  vtx.mPos.z = mPos.z;


  vtx.mTexel1.x = mTexel1.x;
  vtx.mTexel1.y = mTexel1.y;

  vtx.mTexel2.x = mTexel2.x;
  vtx.mTexel2.y = mTexel2.y;
}

void Quake3BSP::BuildVertexBuffers(void)
{
  mMesh = 0;
  mMesh = new VertexMesh;

  QuakeFaceVector::iterator i;
  for (i = mFaces.begin(); i != mFaces.end(); ++i)
  {
    (*i).Build(mElements, mVertices, mShaders, mName, mCodeName, *mMesh);
  }
}

void QuakeFace::Build(const UShortVector &elements, const QuakeVertexVector &vertices, ShaderReferenceVector &shaders,
  const StringRef &sourcename, const StringRef &code, VertexMesh &mesh)
{
  assert(mVcount < 1024);
  assert(mShader < shaders.size());
  StringRef mat;


  static StringRef oname = StringDict::gStringDict().Get("outside");
  static StringRef inside = StringDict::gStringDict().Get("inside");

  StringRef name = inside;


  int type = 0;

  LightMapVertex verts[1024];

  for (int i = 0; i < mVcount; i++)
  {
    vertices[i + mFirstVertice].Get(verts[i]);
  }

  char scratch[256];
  char texname[256];

  shaders[mShader].GetTextureName(texname);

  if (strcmp("toxicskytim_ctf1", texname) == 0)
    return;
  if (mLightmap < 0)
    return;

  StringRef basetexture = StringDict::gStringDict().Get(texname);

  QuakeShader *shader = QuakeShaderFactory::gQuakeShaderFactory().Locate(basetexture);

  if (shader)
  {
    shader->GetBaseTexture(basetexture);
    printf("Shader uses texture: %s\n", (const char *)basetexture);
  }
  sprintf(scratch, "%s+lm%s%02d", (const char *)basetexture, (const char *)code, mLightmap);

  mat = StringDict::gStringDict().Get(scratch);

  char lmapspace[256];
  sprintf(lmapspace, "lm%s%02d", (const char *)code, mLightmap);
  StringRef lmap = SGET(lmapspace);

  switch (mType)
  {
    case FACETYPE_NORMAL:
    case FACETYPE_TRISURF:
      if (1)
      {
        assert((mEcount % 3) == 0);
        const unsigned short *idx = &elements[mFirstElement];
        int tcount = mEcount / 3;
        for (int j = 0; j < tcount; j++)
        {
          int i1 = idx[0];
          int i2 = idx[1];
          int i3 = idx[2];

          mesh.AddTri(mat, verts[i1], verts[i2], verts[i3]);

          idx += 3;
        }
      }
      break;
    case FACETYPE_MESH:
      if (1)
      {
        PatchSurface surface(verts, mVcount, mControlX, mControlY);
        const LightMapVertex *vlist = surface.GetVerts();
        const unsigned short *indices = surface.GetIndices();
        int tcount = surface.GetIndiceCount() / 3;

        for (int j = 0; j < tcount; j++)
        {

          int i1 = *indices++;
          int i2 = *indices++;
          int i3 = *indices++;

          mesh.AddTri(mat, vlist[i1], vlist[i2], vlist[i3]);
        }
      }
      break;
    case FACETYPE_FLARE: break;
    default:
      //      assert( 0 );
      break;
  }
}

void ShaderReference::GetTextureName(char *tname)
{
  strcpy(tname, "null");
  int len = strlen(mName);
  if (!len)
    return;
  const char *foo = &mName[0];
  // while ( *foo && (*foo != '/' && *foo != '\\') ) foo--;
  // if ( !*foo ) return;
  // foo++;
  if (!*foo)
    return;
  char *dest = tname;
  while (*foo)
  {
    *dest++ = *foo++;
  }
  *dest = 0;
}
