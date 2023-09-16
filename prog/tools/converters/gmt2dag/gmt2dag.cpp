// gmt2dag.cpp : Conversion of rFactor GMT files to DAG format
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 // Specifies that the minimum required platform is Windows 2000
#endif

#include <stdio.h>
#include <string.h>
#include <crtdbg.h>
#include <float.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "hash.h"

#include "dag.h"
#include <osApiWrappers/dag_direct.h>

#define CHECK(x) _ASSERTE(x)

#define SEEK(Position)                                                              \
  if (fseek(pFile, (Position), 0) != 0)                                             \
  {                                                                                 \
    printf("Error: seek operation failed in the input file \"%s\".\n", mpFilename); \
    return false;                                                                   \
  }

#define READ(Address, Bytes)                                              \
  if (fread((Address), (Bytes), 1, pFile) != 1)                           \
  {                                                                       \
    printf("Error: unable to read the input file \"%s\".\n", mpFilename); \
    return false;                                                         \
  }

#define OUT_OF_MEMORY_READ printf("Error: out of memory while reading file \"%s\".\n", mpFilename)

#define OUT_OF_MEMORY_WRITE printf("Error: out of memory while writing output file.\n")

#define WRITE_ERROR printf("Error: write to DAG operation failed.\n")


//
// GMT elements format
////////////////////////////////////////////////////////////////////////////////

// Note, we assume that the compiler will not add gaps or additional padding

class CGMTHeader
{

public:
  unsigned int mMaterialsOffset;
  unsigned int mFlags;
  float mfRadius;    // ?
  float mHull[8][3]; // ?
  unsigned int mZeroes[24];
  float mUnknown[2][3]; // ?, [0]==[1]?
  unsigned int mZero;   // always 0?
  float mRanges[4];     //  {10000.0f, 0.0f, 0.0f, 0.0f}
  float mMatrix[16];
  float mfUnknown3; // zero
  float mOffset[3];
  float mUnknown4[10]; // zeroes?

  int miUnknown1; // always 0?
  int miNObjects;
  unsigned int mObjectsOffset;     // offset to objects
  int miNHullBlocks;               // amount of hull blocks;
  unsigned int mHullOffset;        // offset to mesh hull
  int miMeshNormals;               // amount of mesh normals
  unsigned int mMeshNormalsOffset; // offset to mesh normals block
  int miB3;                        // 0?
  unsigned int mOffsetB3;
  char mName[64];
  unsigned int mMagic[4];
  unsigned int mPadding[64];
};


class CGMTObjectData
{

public:
  unsigned int mFlags;
  unsigned int mVerticesOffset;
  unsigned int mUnknown4; // zero?
  unsigned int mTextureCoordinates1Offset;
  unsigned int mTextureCoordinates2Offset;
  unsigned int mNormalsOffset;
  int miNVertices;
  unsigned int mUnknown9; // zero?
  int miNIndices;
  unsigned int mIndicesOffset;
  unsigned int mZeroes1[4];
  int miMaterial;
  unsigned int mZeroes2[4];
  int miUnknown10; // -1
  unsigned int mPadding[13];
};


class CGMTVertex
{

public:
  float mPosition[3];
  float mNormal[3];
  unsigned int mDiffuseColor;
  unsigned int mSpecularColor;
};


class CGMTObject
{

public:
  CGMTObject();
  ~CGMTObject();


  CGMTVertex *mpVertices;
  float *mpTextureCoordinates1;
  float *mpTextureCoordinates2;
  float *mpNormals;
  unsigned short *mpIndices;
};


class CGMTHullBlock
{

public:
  unsigned short mI[3];
  unsigned short mOrient; // direction: +x:0, -x:1, +y:2, -y:3, +z:4, -z:5
  unsigned int mMesh;     // mesh index (its vertices are used)
  int miNormal;           // triange normal index
  float mCenter[3];       // triangle center
  float mfSize;           // triangle bound size
};


class CGMTMeshNormal
{
  float mVector[3];
  unsigned int mType; // or just a trash?
};


class CGMTTextureInfo
{

public:
  CGMTTextureInfo();
  ~CGMTTextureInfo();


  char mName[64];
  unsigned int mFlags;
  int miN1;        // -1, 1, 2 or 3. don't see any diff. can be set to -1.
  int miAnimation; // 1 if none, 2 if event, 3 for tire...


  // animation data (if available)
  int miAnimationFrames;    // ?
  int *mpAnimationState;    // 0 or 1
  int miAnimationUnknown;   // always 6 for sequence, 7 for video
  float mfAnimationUnknown; // always 1.0f?


  // "common"
  int miActor;  // who is going to act
  int miAction; // what it does

  float mfApply;     // can be 1.0f
  int miTexStage[2]; // always {0,0} -> {1,0} -> {2,0} and so on.
  unsigned int mColorKey;
  int miN5;         // alpha-test reference value. seems like.
  float mfUnknown2; // unknown, in range -1.0f..0.0f
  unsigned int mPadding[16];
};


class CGMTShaderInfo
{

public:
  CGMTShaderInfo();
  ~CGMTShaderInfo();


  int miFirstTextureInfo;
  int miNTextureInfos;
  char mName[64];
  unsigned int mPadding[48];

  CGMTTextureInfo *mpTextureInfos;
};


class CGMTMaterial
{

public:
  char mName[64];
  unsigned int mFlags; // 0x68900003, 0x68910023   // flags?
  float mAmbient[4];   // white
  float mDiffuse[4];   // white
  float mSpecular[4];  // usually white
  float mEmissive[4];  // mostly black
  unsigned int mSrcBlend;
  unsigned int mDestBlend;
  float mfSpecular;

  CGMTShaderInfo mShaders[3]; // dx7, dx8, dx9
};


//
// GMT data class
////////////////////////////////////////////////////////////////////////////////

class CGMTData
{

public:
  // constructor and destructor
  CGMTData();
  ~CGMTData();

  // the read function
  bool ReadFromFile(FILE *pFile, char *pFilename);


  // pointer to the next GMT for multi-file conversion
  CGMTData *mpNextGMT;


private:
  // the GMT data
  CGMTHeader mHeader;

  CGMTObject *mpObjects;
  CGMTObjectData *mpObjectData;

  CGMTHullBlock *mpHull;

  CGMTMeshNormal *mpMeshNormals;

  int miNMaterials;
  CGMTMaterial *mpMaterials;

  char *mpFilename;


  // helper functions
  bool ReadObjects(FILE *pFile);
  bool ReadMeshHull(FILE *pFile);
  bool ReadMeshNormals(FILE *pFile);
  bool ReadMaterials(FILE *pFile);


  friend class CDAGExporter;
};


////////////////////////////////////////////////////////////////////////////////
// Constructors and destructors
////////////////////////////////////////////////////////////////////////////////

CGMTData::CGMTData()
{
  mpObjects = NULL;
  mpObjectData = NULL;
  mpHull = NULL;
  mpMeshNormals = NULL;
  mpMaterials = NULL;
  mpNextGMT = NULL;
  mpFilename = NULL;
}

CGMTData::~CGMTData()
{
  if (mpObjects)
  {
    delete[] mpObjects;
  }

  if (mpObjectData)
  {
    delete[] mpObjectData;
  }

  if (mpHull)
  {
    delete[] mpHull;
  }

  if (mpMeshNormals)
  {
    delete[] mpMeshNormals;
  }

  if (mpMaterials)
  {
    delete[] mpMaterials;
  }

  if (mpFilename)
  {
    delete[] mpFilename;
  }
}


CGMTObject::CGMTObject()
{
  mpVertices = NULL;
  mpTextureCoordinates1 = NULL;
  mpTextureCoordinates2 = NULL;
  mpNormals = NULL;
  mpIndices = NULL;
}

CGMTObject::~CGMTObject()
{
  if (mpVertices)
  {
    delete[] mpVertices;
  }

  if (mpTextureCoordinates1)
  {
    delete[] mpTextureCoordinates1;
  }

  if (mpTextureCoordinates2)
  {
    delete[] mpTextureCoordinates2;
  }

  if (mpNormals)
  {
    delete[] mpNormals;
  }

  if (mpIndices)
  {
    delete[] mpIndices;
  }
}


CGMTShaderInfo::CGMTShaderInfo() { mpTextureInfos = NULL; }

CGMTShaderInfo::~CGMTShaderInfo()
{
  if (mpTextureInfos)
  {
    delete[] mpTextureInfos;
  }
}


CGMTTextureInfo::CGMTTextureInfo() { mpAnimationState = NULL; }

CGMTTextureInfo::~CGMTTextureInfo()
{
  if (mpAnimationState)
  {
    delete[] mpAnimationState;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Read GMT from file
////////////////////////////////////////////////////////////////////////////////

bool CGMTData::ReadFromFile(FILE *pFile, char *pFilename)
{

  mpFilename = new char[strlen(pFilename) + 1];

  if (!mpFilename)
  {
    printf("Error: out of memory while reading file \"%s\".\n", pFilename);
    return false;
  }


  strcpy(mpFilename, pFilename);


  // read header

  READ(&mHeader, 740);


  unsigned int Values[] = {0xD8D2CFC8, 0xC3E6D8CE, 0xBED8DED6, 0x90BFA7BB};


  // Note, currently we only process files that are recognized as "rFactor"

  if (!memcmp(&mHeader.mfRadius, Values, sizeof(Values)) || mHeader.mFlags == 0x1 ||
      mHeader.mMaterialsOffset == 0x65636152 && mHeader.mFlags == 0x00544D47)
  {
    printf("Error: \"%s\" is not an rFactor input file.\n", mpFilename);
    return false;
  }


  char Zeroes[sizeof(mHeader.mZeroes)];
  memset(Zeroes, 0, sizeof(Zeroes));

  if (
    memcmp(Zeroes, mHeader.mZeroes, sizeof(mHeader.mZeroes)) != 0 || memcmp(Zeroes, mHeader.mUnknown4, sizeof(mHeader.mUnknown4)) != 0)
  {
    printf("Warning: header of the input file \"%s\" contains unexpected non-zero values.\n", mpFilename);
  }


  // verify file's matrix

  int i;

  for (i = 0; i < 16; i++)
  {
    if (!_finite(mHeader.mMatrix[i]))
    {
      break;
    }
  }

  if (i < 16)
  {
    printf("Warning: the matrix of the input file \"%s\" contains invalid value.\n", mpFilename);
  }


  if (!_finite(mHeader.mOffset[0]) || !_finite(mHeader.mOffset[1]) || !_finite(mHeader.mOffset[2]))
  {
    printf("Warning: the offset in the input file \"%s\" contains invalid value.\n", mpFilename);
  }


  for (i = 0; i < 16; i++)
  {
    if (mHeader.mMatrix[i] != 0.f)
    {
      break;
    }
  }


  if (i == 16)
  {
    // file's matrix is all zeroed
    mHeader.mMatrix[0] = 1.f;
    mHeader.mMatrix[5] = 1.f;
    mHeader.mMatrix[10] = 1.f;
    mHeader.mMatrix[15] = 1.f;
  }


  if (mHeader.mMatrix[3] != 0.f || mHeader.mMatrix[7] != 0.f || mHeader.mMatrix[11] != 0.f || mHeader.mMatrix[15] != 1.f)
  {
    printf("Warning: the \"edge values\" of the matrix of "
           "input file \"%s\" are not 0, 0, 0, 1.\n",
      mpFilename);
  }


  if (mHeader.mMatrix[12] != mHeader.mOffset[0] || mHeader.mMatrix[13] != mHeader.mOffset[1] ||
      mHeader.mMatrix[14] != mHeader.mOffset[2])
  {
    printf("Warning: offset component of the matrix of the "
           "input file \"%s\" is not equal to offset vector.\n",
      mpFilename);
  }


  if (mHeader.mMatrix[0] != 1.f || mHeader.mMatrix[1] != 0.f || mHeader.mMatrix[2] != 0.f || mHeader.mMatrix[4] != 0.f ||
      mHeader.mMatrix[5] != 1.f || mHeader.mMatrix[6] != 0.f || mHeader.mMatrix[8] != 0.f || mHeader.mMatrix[9] != 0.f ||
      mHeader.mMatrix[10] != 1.f)
  {
    printf("Warning: the 3x3 component of the matrix of input file \"%s\" is not identity.\n", mpFilename);
  }


  // read objects

  if (!ReadObjects(pFile))
  {
    return false;
  }


  if (!ReadMeshHull(pFile))
  {
    return false;
  }


  if (!ReadMeshNormals(pFile))
  {
    return false;
  }


  if (!ReadMaterials(pFile))
  {
    return false;
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Read objects from GMT file
////////////////////////////////////////////////////////////////////////////////

bool CGMTData::ReadObjects(FILE *pFile)
{
  SEEK(mHeader.mObjectsOffset + 4);


  if (mHeader.miNObjects < 0)
  {
    printf("Error: negative number of objects in the input file \"%s\".\n", mpFilename);
    return false;
  }


  if (mHeader.miNObjects == 0)
  {
    printf("Warning: zero objects in the input file \"%s\".\n", mpFilename);
    return true;
  }


  mpObjectData = new CGMTObjectData[mHeader.miNObjects];

  if (!mpObjectData)
  {
    OUT_OF_MEMORY_READ;
    return false;
  }


  READ(mpObjectData, mHeader.miNObjects * 132);


  mpObjects = new CGMTObject[mHeader.miNObjects];

  if (!mpObjects)
  {
    OUT_OF_MEMORY_READ;
    return false;
  }


  char Zeroes[sizeof(mpObjectData[0].mPadding)];
  memset(Zeroes, 0, sizeof(Zeroes));


  for (int i = 0; i < mHeader.miNObjects; i++)
  {
    CGMTObjectData &D = mpObjectData[i];


    // verify object data

    if (D.mUnknown4 != 0 || D.mUnknown9 != 0 || memcmp(D.mZeroes1, Zeroes, sizeof(D.mZeroes1)) != 0 ||
        memcmp(D.mZeroes2, Zeroes, sizeof(D.mZeroes2)) != 0 || memcmp(D.mPadding, Zeroes, sizeof(D.mPadding)) != 0)
    {
      printf("Error: unexpected non-zero data in "
             "an object in the input file \"%s\".\n",
        mpFilename);

      return false;
    }


    CGMTObject &O = mpObjects[i];


    // read vertices

    if (D.miNVertices <= 0)
    {
      printf("Error: incorrect number of vertices in an object "
             "in the input file \"%s\".\n",
        mpFilename);
      return false;
    }


    O.mpVertices = new CGMTVertex[D.miNVertices];

    if (!O.mpVertices)
    {
      OUT_OF_MEMORY_READ;
      return false;
    }


    SEEK(D.mVerticesOffset + 4);

    READ(O.mpVertices, D.miNVertices * 32);


    // texture coordinates 1 (4 pairs at each vertex)

    O.mpTextureCoordinates1 = new float[D.miNVertices * 2 * 4];

    if (!O.mpTextureCoordinates1)
    {
      OUT_OF_MEMORY_READ;
      return false;
    }

    SEEK(D.mTextureCoordinates1Offset + 4);

    READ(O.mpTextureCoordinates1, D.miNVertices * 32);


    // texture coordinates 2 (4 pairs at each vertex)

    if (D.mFlags & 0x20)
    {
      O.mpTextureCoordinates2 = new float[D.miNVertices * 2 * 4];

      if (!O.mpTextureCoordinates2)
      {
        OUT_OF_MEMORY_READ;
        return false;
      }

      SEEK(D.mTextureCoordinates2Offset + 4);

      READ(O.mpTextureCoordinates2, D.miNVertices * 32);
    }


    // normals (1 pair of axes at each vertex)

    if (D.mFlags & 0x04)
    {
      O.mpNormals = new float[D.miNVertices * 3 * 2];

      if (!O.mpNormals)
      {
        OUT_OF_MEMORY_READ;
        return false;
      }

      SEEK(D.mNormalsOffset + 4);

      READ(O.mpNormals, D.miNVertices * 24);
    }


    // indices

    if (D.miNIndices <= 0)
    {
      printf("Error: incorrect number of indices in an object "
             "in the input file \"%s\".\n",
        mpFilename);
      return false;
    }

    O.mpIndices = new unsigned short[D.miNIndices];

    if (!O.mpIndices)
    {
      OUT_OF_MEMORY_READ;
      return false;
    }

    SEEK(D.mIndicesOffset + 4);

    READ(O.mpIndices, D.miNIndices * 2);
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Read mesh hull from GMT file
////////////////////////////////////////////////////////////////////////////////

bool CGMTData::ReadMeshHull(FILE *pFile)
{
  SEEK(mHeader.mHullOffset + 4);


  if (mHeader.miNHullBlocks < 0)
  {
    printf("Error: negative number of hull blocks in the header "
           "of input file \"%s\".\n",
      mpFilename);
    return false;
  }


  if (mHeader.miNHullBlocks == 0)
  {
    printf("Warning: zero hull blocks in the header of input file \"%s\".\n", mpFilename);
    return true;
  }


  mpHull = new CGMTHullBlock[mHeader.miNHullBlocks];

  if (!mpHull)
  {
    OUT_OF_MEMORY_READ;
    return false;
  }


  READ(mpHull, mHeader.miNHullBlocks * 32);


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Read mesh normals from GMT file
////////////////////////////////////////////////////////////////////////////////

bool CGMTData::ReadMeshNormals(FILE *pFile)
{
  SEEK(mHeader.mMeshNormalsOffset + 4);


  if (mHeader.miMeshNormals < 0)
  {
    printf("Error: negative number of mesh normals in the header "
           "of input file \"%s\".\n",
      mpFilename);
    return false;
  }


  if (mHeader.miMeshNormals == 0)
  {
    printf("Warning: zero mesh normals in the header of input file \"%s\".\n", mpFilename);
    return true;
  }


  mpMeshNormals = new CGMTMeshNormal[mHeader.miMeshNormals];

  if (!mpMeshNormals)
  {
    OUT_OF_MEMORY_READ;
    return false;
  }


  READ(mpMeshNormals, mHeader.miMeshNormals * 16);


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Read materials from GMT file
////////////////////////////////////////////////////////////////////////////////

bool CGMTData::ReadMaterials(FILE *pFile)
{
  SEEK(mHeader.mMaterialsOffset + 4);


  READ(&miNMaterials, 4);

  if (miNMaterials > 65535)
  {
    printf("Error: too many materials in the input file \"%s\".\n", mpFilename);
    return false;
  }


  if (miNMaterials < 0)
  {
    printf("Error: negative number of materials in the input file \"%s\".\n", mpFilename);
    return false;
  }


  if (miNMaterials == 0)
  {
    printf("Warning: zero materials in the input file \"%s\".\n", mpFilename);
    return true;
  }


  mpMaterials = new CGMTMaterial[miNMaterials];

  if (!mpMaterials)
  {
    OUT_OF_MEMORY_READ;
    return false;
  }


  for (int i = 0; i < miNMaterials; i++)
  {
    CGMTMaterial &M = mpMaterials[i];


    READ(&M, 144);


    READ(&M.mShaders[0], 264);
    READ(&M.mShaders[1], 264);
    READ(&M.mShaders[2], 264);


    char Buffer[36];
    READ(Buffer, 36);


    int iNReadLayers = 0;

    for (int iShader = 0; iShader < 3; iShader++)
    {
      CGMTShaderInfo &S = M.mShaders[iShader];


      if (S.miFirstTextureInfo != iNReadLayers)
      {
        printf("Warning: format mismatch in file \"%s\", material %i: "
               "first texture block %i while %i expected.\n",
          mpFilename, i, S.miFirstTextureInfo, iNReadLayers);

        S.miNTextureInfos -= iNReadLayers - S.miFirstTextureInfo;
      }


      if (S.miNTextureInfos < 0)
      {
        printf("Error: negative number of texture infos in "
               "the input file \"%s\".\n",
          mpFilename);
        return false;
      }


      if (S.miNTextureInfos == 0)
      {
        continue;
      }


      S.mpTextureInfos = new CGMTTextureInfo[S.miNTextureInfos];

      if (!S.mpTextureInfos)
      {
        OUT_OF_MEMORY_READ;
        return false;
      }


      for (int t = 0; t < S.miNTextureInfos; t++)
      {
        CGMTTextureInfo &T = S.mpTextureInfos[t];

        READ(&T, 76);

        iNReadLayers++;


        if (T.mFlags & 0x00040000)
        {
          READ(&T.miAnimationUnknown, 4);
          READ(&T.mfAnimationUnknown, 4);
        }


        if (T.miAnimation != 1)
        {
          READ(&T.miAnimationFrames, 4);


          if (T.miAnimationFrames < 0)
          {
            printf("Error: negative number of animation frames in "
                   "the input file \"%s\".\n",
              mpFilename);
            return false;
          }
          else if (T.miAnimationFrames == 0)
          {
            printf("Warning: zero animation frames in a texture info "
                   "in the input file \"%s\".\n",
              mpFilename);
          }
          else
          {
            T.mpAnimationState = new int[T.miAnimationFrames];

            if (!T.mpAnimationState)
            {
              OUT_OF_MEMORY_READ;
              return false;
            }

            READ(T.mpAnimationState, T.miAnimationFrames * 4);

            READ(&T.miAnimationUnknown, 4);
            READ(&T.mfAnimationUnknown, 4);
          }
        }


        // read the rest:
        READ(&T.miActor, 96);
      }
    }
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Helper functions for color format conversion
////////////////////////////////////////////////////////////////////////////////

unsigned char ColorFloatToChar(float fColor, char *pFilename)
{
  if (!_finite(fColor))
  {
    printf("Warning: invalid color value in the input file \"%s\".\n", pFilename);
    return 0;
  }

  if (fColor < 0.f || fColor > 1.f)
  {
    printf("Warning: color value outside of [0, 1] range in "
           "the input file \"%s\".\n",
      pFilename);
  }


  int iRes = int(fColor * 255.f + 0.5);

  if (iRes < 0)
  {
    iRes = 0;
  }

  if (iRes > 255)
  {
    iRes = 255;
  }


  return (unsigned char)iRes;
}


void ColorFloatRGBToCharBGR(unsigned char Dest[3], float Src[3], char *pFilename)
{
  Dest[0] = ColorFloatToChar(Src[2], pFilename);
  Dest[1] = ColorFloatToChar(Src[1], pFilename);
  Dest[2] = ColorFloatToChar(Src[0], pFilename);
}


void Color32BitARGBOrBGRToFloatRGB(float Dest[3], unsigned int Src)
{
  if (Src & 0xFF000000)
  {
    Dest[0] = ((Src >> 16) & 0xFF) / 255.f;
    Dest[1] = ((Src >> 8) & 0xFF) / 255.f;
    Dest[2] = (Src & 0xFF) / 255.f;
  }
  else
  {
    Dest[0] = (Src & 0xFF) / 255.f;
    Dest[1] = ((Src >> 8) & 0xFF) / 255.f;
    Dest[2] = ((Src >> 16) & 0xFF) / 255.f;
  }
}


//
// DAG Export
////////////////////////////////////////////////////////////////////////////////

class CDAGExporter
{

public:
  CDAGExporter(CGMTData *pGMTData);
  ~CDAGExporter();

  bool Export(char *pFilename, bool bWriteObjects, bool bWriteHull, int iUseShader);

private:
  CGMTData *mpGMTData;

  float *mpPoints;

  CDAGBigFace *mpFaces;

  int miNChannels;
  CDAGBigMapChannel *mpChannels;

  CHash mTextureNames;

  int *mpStartMaterialIndices;


  bool ExportTextureNames(CDAGWriter &File, int iUseShader);

  bool ExportMaterials(CDAGWriter &File, int iUseShader);

  bool ExportObjects(CDAGWriter &File, CGMTData *pGMT, int iGMTIndex);

  bool ExportMeshHull(CDAGWriter &File, CGMTData *pGMT, int iGMTIndex);
};


class CTextureName : public CHashElement
{

public:
  char *mpName;
  int miIndex;
};


class CTextureCoordinates : public CHashElement
{

public:
  CTextureCoordinates(float fU, float fV);

  int mValues[2];
  int miIndex;
};


class CPoint : public CHashElement
{

public:
  CPoint(float fX, float fY, float fZ);

  int mValues[3];
  int miIndex;
};


////////////////////////////////////////////////////////////////////////////////
// Hash functions
////////////////////////////////////////////////////////////////////////////////

int TextureNameGetHashCode(CHashElement *pElement)
{
  char *p = ((CTextureName *)pElement)->mpName;


  int iRes = 0;
  int iPos = 0;

  while (p[iPos])
  {
    if (!(iPos & 3))
    {
      iRes = _rotr(iRes, 1);
    }

    iRes ^= tolower(p[iPos]) << ((iPos & 3) * 8);

    iPos++;
  }

  return iRes;
}


bool TextureNameIsSame(CHashElement *pA, CHashElement *pB)
{
  char *a = ((CTextureName *)pA)->mpName;
  char *b = ((CTextureName *)pB)->mpName;

  for (;;)
  {
    if (*a == 0)
    {
      return *b == 0;
    }
    else if (*b == 0)
    {
      return false;
    }

    if (tolower(*a) != tolower(*b))
    {
      return false;
    }

    a++;
    b++;
  }
}


void TextureNameDelete(CHashElement *p)
{
  delete[] ((CTextureName *)p)->mpName;
  delete (CTextureName *)p;
}


CTextureCoordinates::CTextureCoordinates(float fU, float fV)
{
  mValues[0] = *(int *)&fU;
  mValues[1] = *(int *)&fV;

  if (mValues[0] == 0x80000000)
  {
    mValues[0] = 0;
  }

  if (mValues[1] == 0x80000000)
  {
    mValues[1] = 0;
  }
}


int TextureCoordinatesGetHashCode(CHashElement *pElement)
{
  int *pValues = ((CTextureCoordinates *)pElement)->mValues;

  return pValues[0] ^ _rotr(pValues[1], 16);
}


bool TextureCoordinatesIsSame(CHashElement *pA, CHashElement *pB)
{
  return ((CTextureCoordinates *)pA)->mValues[0] == ((CTextureCoordinates *)pB)->mValues[0] &&
         ((CTextureCoordinates *)pA)->mValues[1] == ((CTextureCoordinates *)pB)->mValues[1];
}


void TextureCoordinatesDelete(CHashElement *p) { delete (CTextureCoordinates *)p; }


CPoint::CPoint(float fX, float fY, float fZ)
{
  mValues[0] = *(int *)&fX;
  mValues[1] = *(int *)&fY;
  mValues[2] = *(int *)&fZ;

  if (mValues[0] == 0x80000000)
  {
    mValues[0] = 0;
  }

  if (mValues[1] == 0x80000000)
  {
    mValues[1] = 0;
  }

  if (mValues[2] == 0x80000000)
  {
    mValues[2] = 0;
  }
}


int PointGetHashCode(CHashElement *pElement)
{
  int *pValues = ((CPoint *)pElement)->mValues;

  return pValues[0] ^ _rotr(pValues[1], 10) ^ _rotr(pValues[2], 20);
}


bool PointIsSame(CHashElement *pA, CHashElement *pB)
{
  return ((CPoint *)pA)->mValues[0] == ((CPoint *)pB)->mValues[0] && ((CPoint *)pA)->mValues[1] == ((CPoint *)pB)->mValues[1] &&
         ((CPoint *)pA)->mValues[2] == ((CPoint *)pB)->mValues[2];
}


void PointDelete(CHashElement *p) { delete (CPoint *)p; }


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
////////////////////////////////////////////////////////////////////////////////

CDAGExporter::CDAGExporter(CGMTData *pGMTData) : mTextureNames(TextureNameGetHashCode, TextureNameIsSame, TextureNameDelete)
{
  mpGMTData = pGMTData;

  mpPoints = NULL;

  mpFaces = NULL;

  miNChannels = 0;
  mpChannels = NULL;

  mpStartMaterialIndices = NULL;
}


CDAGExporter::~CDAGExporter()
{
  if (mpPoints)
  {
    delete[] mpPoints;
  }


  if (mpFaces)
  {
    delete[] mpFaces;
  }


  if (mpChannels)
  {
    CHECK(miNChannels > 0);

    for (int i = 0; i < miNChannels; i++)
    {
      if (mpChannels[i].mpTVertices)
      {
        delete[] mpChannels[i].mpTVertices;
      }

      if (mpChannels[i].mpTFaces)
      {
        delete[] mpChannels[i].mpTFaces;
      }
    }

    delete[] mpChannels;
  }


  if (mpStartMaterialIndices)
  {
    delete[] mpStartMaterialIndices;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Export to DAG format
////////////////////////////////////////////////////////////////////////////////

bool CDAGExporter::Export(char *pFilename, bool bWriteObjects, bool bWriteHull, int iUseShader)
{
  CHECK(pFilename);
  CHECK(0 <= iUseShader && iUseShader < 3);


  CDAGWriter File;


  if (!File.IsValid())
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  if (!File.Start(pFilename))
  {
    WRITE_ERROR;
    return false;
  }


  if (!ExportTextureNames(File, iUseShader))
  {
    return false;
  }


  if (!ExportMaterials(File, iUseShader))
  {
    return false;
  }


  // save objects


  if (!File.StartNodes())
  {
    WRITE_ERROR;
    return false;
  }


  int iGMTIndex = 0;

  for (CGMTData *pGMT = mpGMTData; pGMT; pGMT = pGMT->mpNextGMT)
  {
    if (bWriteObjects)
    {
      if (!ExportObjects(File, pGMT, iGMTIndex))
      {
        return false;
      }
    }


    if (bWriteHull)
    {
      if (!ExportMeshHull(File, pGMT, iGMTIndex))
      {
        return false;
      }
    }


    iGMTIndex++;
  }


  if (!File.StopNodes())
  {
    WRITE_ERROR;
    return false;
  }


  if (!File.Stop())
  {
    WRITE_ERROR;
    return false;
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Gather texture names and export them
////////////////////////////////////////////////////////////////////////////////

bool CDAGExporter::ExportTextureNames(CDAGWriter &File, int iUseShader)
{

  // Note, we are using one (the last, "dx9", by default) shader in GMT material to get texture names to export to DAG materials


  // count the maximum number of texture names

  int iMaxTextures = 1;


  for (CGMTData *pGMT = mpGMTData; pGMT; pGMT = pGMT->mpNextGMT)
  {
    CGMTData &G = *pGMT;

    for (int i = 0; i < G.miNMaterials; i++)
    {
      CHECK(G.mpMaterials[i].mShaders[iUseShader].miNTextureInfos >= 0);

      iMaxTextures += G.mpMaterials[i].mShaders[iUseShader].miNTextureInfos;
    }
  }


  // gather names


  if (!mTextureNames.IsValid())
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  int iNTextureNames = 0;

  char **pNamesArray = new char *[iMaxTextures];

  if (!pNamesArray)
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  for (CGMTData *pGMT = mpGMTData; pGMT; pGMT = pGMT->mpNextGMT)
  {
    CGMTData &G = *pGMT;


    for (int i = 0; i < G.miNMaterials; i++)
    {
      for (int t = 0; t < G.mpMaterials[i].mShaders[iUseShader].miNTextureInfos; t++)
      {
        char *pName = G.mpMaterials[i].mShaders[iUseShader].mpTextureInfos[t].mName;

        int iLength;

        if (memchr(pName, 0, 64) == NULL)
        {
          printf("Warning: a texture name in the input file \"%s\" is "
                 "not zero-terminated.\n",
            G.mpFilename);
          iLength = 64;
        }
        else
        {
          iLength = strlen(pName);
        }


        char s[65];


        if (iLength > 0)
        {
          memcpy(s, pName, iLength);
        }

        s[iLength] = 0;


        // Note, we gather texture names from the input file with no case-sensitivity


        CTextureName smp;
        smp.mpName = s;


        if (mTextureNames.Find(&smp))
        {
          continue;
        }


        CTextureName *pNew = new CTextureName;

        if (!pNew)
        {
          delete[] pNamesArray;
          OUT_OF_MEMORY_WRITE;
          return false;
        }


        pNew->mpName = new char[iLength + 1];

        if (!pNew->mpName)
        {
          delete pNew;
          delete[] pNamesArray;
          OUT_OF_MEMORY_WRITE;
          return false;
        }


        strcpy(pNew->mpName, s);

        pNew->miIndex = iNTextureNames;


        if (!mTextureNames.Add(pNew))
        {
          delete[] pNew->mpName;
          delete pNew;
          delete[] pNamesArray;
          OUT_OF_MEMORY_WRITE;
          return false;
        }


        pNamesArray[iNTextureNames] = pNew->mpName;


        iNTextureNames++;
      }
    }
  }


  if (iNTextureNames > 65535)
  {
    printf("Error: too many textures used in the input files.\n");

    delete[] pNamesArray;
    return false;
  }


  if (!File.SaveTextures(iNTextureNames, (const char **)pNamesArray))
  {
    delete[] pNamesArray;
    WRITE_ERROR;
    return false;
  }


  delete[] pNamesArray;


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Export materials
////////////////////////////////////////////////////////////////////////////////

// Note, we do not unite potentially identical materials occuring in the input files

bool CDAGExporter::ExportMaterials(CDAGWriter &File, int iUseShader)
{


  // count the number of GMTs

  int iNGMTs = 0;

  for (CGMTData *pGMT = mpGMTData; pGMT; pGMT = pGMT->mpNextGMT)
  {
    iNGMTs++;
  }


  // prepare array of start material indices of the GMTs

  CHECK(!mpStartMaterialIndices);
  CHECK(iNGMTs > 0);

  mpStartMaterialIndices = new int[iNGMTs];

  if (!mpStartMaterialIndices)
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  // write materials

  int iCurrentGMT = 0;

  int iMaterialsWritten = 0;


  for (CGMTData *pGMT = mpGMTData; pGMT; pGMT = pGMT->mpNextGMT)
  {
    CGMTData &G = *pGMT;


    CHECK(0 <= iCurrentGMT && iCurrentGMT < iNGMTs);

    mpStartMaterialIndices[iCurrentGMT] = iMaterialsWritten;


    for (int i = 0; i < G.mHeader.miNObjects; i++)
    {
      if (G.mpObjectData[i].miMaterial < 0 || G.mpObjectData[i].miMaterial >= G.miNMaterials)
      {
        printf("Error: an object in the input file \"%s\" has incorrect "
               "material index.\n",
          G.mpFilename);
        return false;
      }


      CGMTMaterial &InMat = G.mpMaterials[G.mpObjectData[i].miMaterial];


      CDAGMaterial Material;


      // Note, we do not export alpha channel in material colors, and the float range 0..1 is converted to byte value.

      ColorFloatRGBToCharBGR(Material.mAmbientColor, InMat.mAmbient, G.mpFilename);
      ColorFloatRGBToCharBGR(Material.mDiffuseColor, InMat.mDiffuse, G.mpFilename);
      ColorFloatRGBToCharBGR(Material.mSpecularColor, InMat.mSpecular, G.mpFilename);
      ColorFloatRGBToCharBGR(Material.mEmissiveColor, InMat.mEmissive, G.mpFilename);


      if (!_finite(InMat.mfSpecular))
      {
        printf("Warning: invalid specular power value in "
               "a material in the input file \"%s\".\n",
          G.mpFilename);
      }

      Material.mfPower = InMat.mfSpecular;


      Material.mFlags = 0;


      // Note, the "texture parameters" are set to zero

      memset(Material.mTextureParameters, 0, sizeof(Material.mTextureParameters));


      if (memchr(InMat.mName, 0, 64) == NULL)
      {
        printf("Warning: a material name in the input file \"%s\" is "
               "not zero-terminated.\n",
          G.mpFilename);
      }


      char Name[65];

      memcpy(Name, InMat.mName, 64);

      Name[64] = 0;

      Material.mpName = Name;


      Material.mpClassName = "simple";

      Material.mpScript = "";


      // setup texture IDs

      // Note, we export no more than 8 texture IDs per material

      int iNTextures = InMat.mShaders[iUseShader].miNTextureInfos;

      if (iNTextures > 8)
      {
        printf("Warning: too many textures in a material in "
               "the input file \"%s\".\n",
          G.mpFilename);

        iNTextures = 8;
      }


      for (int t = 0; t < iNTextures; t++)
      {
        char *pName = InMat.mShaders[iUseShader].mpTextureInfos[t].mName;

        int iLength;

        if (memchr(pName, 0, 64) == NULL)
        {
          iLength = 64;
        }
        else
        {
          iLength = strlen(pName);
        }


        char s[65];

        if (iLength > 0)
        {
          memcpy(s, pName, iLength);
        }

        s[iLength] = 0;


        CTextureName smp;
        smp.mpName = s;


        CTextureName *pTextureName = (CTextureName *)mTextureNames.Find(&smp);
        CHECK(pTextureName);


        Material.mTextureIDs[t] = (unsigned short)pTextureName->miIndex;
      }


      for (int t = iNTextures; t < 8; t++)
      {
        Material.mTextureIDs[t] = 0xFFFF;
      }


      if (!File.SaveMaterial(Material))
      {
        WRITE_ERROR;
        return false;
      }


      iMaterialsWritten++;
    }


    iCurrentGMT++;
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Export objects
////////////////////////////////////////////////////////////////////////////////

bool CDAGExporter::ExportObjects(CDAGWriter &File, CGMTData *pGMT, int iGMTIndex)
{
  CGMTData &G = *pGMT;


  char GMTName[65];

  memcpy(GMTName, G.mHeader.mName, 64);

  GMTName[64] = 0;


  for (int iObject = 0; iObject < G.mHeader.miNObjects; iObject++)
  {
    char Name[200];
    sprintf(Name, "%s_object%i", GMTName, iObject);


    CDAGMatrix Matrix;

    Matrix.m[0][0] = G.mHeader.mMatrix[0];
    Matrix.m[0][1] = G.mHeader.mMatrix[1];
    Matrix.m[0][2] = G.mHeader.mMatrix[2];

    Matrix.m[1][0] = G.mHeader.mMatrix[4];
    Matrix.m[1][1] = G.mHeader.mMatrix[5];
    Matrix.m[1][2] = G.mHeader.mMatrix[6];

    Matrix.m[2][0] = G.mHeader.mMatrix[8];
    Matrix.m[2][1] = G.mHeader.mMatrix[9];
    Matrix.m[2][2] = G.mHeader.mMatrix[10];

    Matrix.m[3][0] = G.mHeader.mMatrix[12];
    Matrix.m[3][1] = G.mHeader.mMatrix[13];
    Matrix.m[3][2] = G.mHeader.mMatrix[14];


    if (!File.StartNode(Name, Matrix))
    {
      WRITE_ERROR;
      return false;
    }


    CGMTObjectData &D = G.mpObjectData[iObject];

    CHECK(0 <= D.miMaterial && D.miMaterial <= 0xFFFF);

    CHECK(0 <= iGMTIndex);

    int iExportMaterialIndex = mpStartMaterialIndices[iGMTIndex] + D.miMaterial;


    if (!(0 <= iExportMaterialIndex && iExportMaterialIndex <= 0xFFFF))
    {
      printf("Error: material index in \"%s\" out of range.\n", G.mpFilename);

      return false;
    }


    unsigned short Material = (unsigned short)iExportMaterialIndex;

    if (!File.SaveNodeMaterials(1, &Material))
    {
      WRITE_ERROR;
      return false;
    }


    // prepare vertices


    CGMTObject &O = G.mpObjects[iObject];

    CHECK(D.miNVertices > 0);

    mpPoints = new float[D.miNVertices * 3];

    if (!mpPoints)
    {
      OUT_OF_MEMORY_WRITE;
      return false;
    }


    for (int i = 0; i < D.miNVertices; i++)
    {
      if (!_finite(O.mpVertices[i].mPosition[0]) || !_finite(O.mpVertices[i].mPosition[1]) || !_finite(O.mpVertices[i].mPosition[2]))
      {
        printf("Warning: invalid vertex coordinate in "
               "the input file \"%s\".\n",
          G.mpFilename);
      }

      memcpy(&mpPoints[i * 3], O.mpVertices[i].mPosition, 12);
    }


    // prepare faces

    // Note, we only support input files with triangle list format for geometry

    if (!(G.mHeader.mFlags & 0x20))
    {
      printf("Error: input file \"%s\" does not use triangle "
             "list format for geometry.\n",
        G.mpFilename);

      return false;
    }


    if ((D.miNIndices % 3) != 0)
    {
      printf("Error: incorrect number of indices in an "
             "object in the input file \"%s\".\n",
        G.mpFilename);

      return false;
    }


    int iNFaces = D.miNIndices / 3;

    mpFaces = new CDAGBigFace[iNFaces];

    if (!mpFaces)
    {
      OUT_OF_MEMORY_WRITE;
      return false;
    }


    for (int i = 0; i < iNFaces; i++)
    {
      if (int(O.mpIndices[i * 3]) >= D.miNVertices || int(O.mpIndices[i * 3 + 1]) >= D.miNVertices ||
          int(O.mpIndices[i * 3 + 2]) >= D.miNVertices)
      {
        printf("Error: incorrect index in an object in the "
               "input file \"%s\".\n",
          G.mpFilename);
        return false;
      }

      mpFaces[i].mVertices[0] = O.mpIndices[i * 3];
      mpFaces[i].mVertices[2] = O.mpIndices[i * 3 + 1];
      mpFaces[i].mVertices[1] = O.mpIndices[i * 3 + 2];

      mpFaces[i].mSMGR = 0;
      mpFaces[i].mMaterial = 0;
    }


    // prepare texture coordinate channels


    // Note, we do not export texture coordinate channels if they contain only zeroes.


    int iNChannelsToExport = 0;

    int iChannelsToExport[8];


    for (int iSet = 0; iSet < 2; iSet++)
    {
      float *pSet;


      if (iSet == 0)
      {
        pSet = O.mpTextureCoordinates1;
      }
      else
      {
        pSet = O.mpTextureCoordinates2;

        if (!pSet)
        {
          break;
        }
      }


      for (int i = 0; i < 4; i++)
      {
        int v;

        for (v = 0; v < D.miNVertices; v++)
        {
          CHECK(i * 2 + v * 8 + 1 < D.miNVertices * 8);


          float fT1 = pSet[i * 2 + v * 8];
          float fT2 = pSet[i * 2 + v * 8 + 1];


          if (!_finite(fT1) || !_finite(fT2))
          {
            printf("Warning: invalid texture coordinate value in "
                   "the input file \"%s\".\n",
              G.mpFilename);
          }


          if (fT1 != 0.f || fT2 != 0.f)
          {
            break;
          }
        }


        if (v < D.miNVertices)
        {
          CHECK(iNChannelsToExport < 8);

          iChannelsToExport[iNChannelsToExport] = i + iSet * 4;

          iNChannelsToExport++;
        }
      }
    }


    CHECK(0 <= iNChannelsToExport && iNChannelsToExport <= 8);


    miNChannels = iNChannelsToExport + 2;

    mpChannels = new CDAGBigMapChannel[miNChannels];

    if (!mpChannels)
    {
      OUT_OF_MEMORY_WRITE;
      return false;
    }


    for (int iChannel = 0; iChannel < miNChannels; iChannel++)
    {
      mpChannels[iChannel].mpTFaces = NULL;
      mpChannels[iChannel].mpTVertices = NULL;
    }


    for (int iChannel = 0; iChannel < iNChannelsToExport; iChannel++)
    {
      CDAGBigMapChannel &C = mpChannels[iChannel];

      C.mNTextureCoordinates = 2;
      C.mChannelID = unsigned char(iChannel + 1);


      // gather texture coordinate vertices for the object


      CHash Hash(TextureCoordinatesGetHashCode, TextureCoordinatesIsSame, TextureCoordinatesDelete);

      if (!Hash.IsValid())
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      C.mpTVertices = new float[2 * D.miNIndices];

      if (!C.mpTVertices)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }

      int iNTVertices = 0;


      float *pInTCoordinates;

      int iUseChannel = iChannelsToExport[iChannel];

      CHECK(0 <= iUseChannel && iUseChannel < 8);

      if (iUseChannel < 4)
      {
        pInTCoordinates = O.mpTextureCoordinates1 + iUseChannel * 2;
      }
      else
      {
        pInTCoordinates = O.mpTextureCoordinates2 + (iUseChannel - 4) * 2;
      }


      // Note, we can also iterate through entire texture coordinate array here (miNVertices texture vertices)

      for (int i = 0; i < D.miNIndices; i++)
      {
        CHECK(O.mpIndices[i] * 8 + 1 < D.miNVertices * 8);

        float t1 = pInTCoordinates[O.mpIndices[i] * 8];
        float t2 = pInTCoordinates[O.mpIndices[i] * 8 + 1];


        CTextureCoordinates smp(t1, t2);


        if (Hash.Find(&smp))
        {
          continue;
        }


        CTextureCoordinates *pNew = new CTextureCoordinates(t1, t2);

        if (!pNew)
        {
          OUT_OF_MEMORY_WRITE;
          return false;
        }


        pNew->miIndex = iNTVertices;

        if (!Hash.Add(pNew))
        {
          delete pNew;
          OUT_OF_MEMORY_WRITE;
          return false;
        }


        CHECK(iNTVertices * 2 + 1 < 2 * D.miNIndices);

        C.mpTVertices[iNTVertices * 2] = t1;
        C.mpTVertices[iNTVertices * 2 + 1] = 1.f - t2;

        iNTVertices++;
      }


      CHECK(iNTVertices <= D.miNIndices);


      C.miNTVertices = iNTVertices;


      C.mpTFaces = new CDAGBigTFace[iNFaces];

      if (!C.mpTFaces)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      for (int i = 0; i < iNFaces; i++)
      {
        for (int v = 0; v < 3; v++)
        {
          CHECK(O.mpIndices[(i * 3 + v)] * 8 + 1 < D.miNVertices * 8);

          float t1 = pInTCoordinates[O.mpIndices[(i * 3 + v)] * 8];
          float t2 = pInTCoordinates[O.mpIndices[(i * 3 + v)] * 8 + 1];


          CTextureCoordinates smp(t1, t2);


          CTextureCoordinates *pEx = (CTextureCoordinates *)Hash.Find(&smp);

          CHECK(pEx);


          if (v == 0)
          {
            C.mpTFaces[i].mT[0] = pEx->miIndex;
          }
          else if (v == 1)
          {
            C.mpTFaces[i].mT[2] = pEx->miIndex;
          }
          else
          {
            C.mpTFaces[i].mT[1] = pEx->miIndex;
          }
        }
      }
    }


    // Note, we export 2 additional channels, one with diffuse vertex colors (RGB), the other with vertex normals.

    // Note, the specular colors specified at vertices are not exported.


    // Note, we do not currently unite identical color and normal vertices


    // the diffuse color channel
    {
      CDAGBigMapChannel &C = mpChannels[miNChannels - 2];

      C.mNTextureCoordinates = 3;
      C.mChannelID = unsigned char(miNChannels - 1);


      C.mpTVertices = new float[3 * D.miNVertices];

      if (!C.mpTVertices)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }

      C.miNTVertices = D.miNVertices;


      for (int iVertex = 0; iVertex < D.miNVertices; iVertex++)
      {
        // Note, we assume different order of components of the input color depending on the value of its 4th (alpha) byte
        // Note, the alpha component of the diffuse vertex color is not exported
        Color32BitARGBOrBGRToFloatRGB(&C.mpTVertices[iVertex * 3], O.mpVertices[iVertex].mDiffuseColor);
      }


      C.mpTFaces = new CDAGBigTFace[iNFaces];

      if (!C.mpTFaces)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      for (int i = 0; i < iNFaces; i++)
      {
        CHECK(int(O.mpIndices[i * 3]) < D.miNVertices && int(O.mpIndices[i * 3 + 1]) < D.miNVertices &&
              int(O.mpIndices[i * 3 + 2]) < D.miNVertices);

        C.mpTFaces[i].mT[0] = O.mpIndices[i * 3];
        C.mpTFaces[i].mT[2] = O.mpIndices[i * 3 + 1];
        C.mpTFaces[i].mT[1] = O.mpIndices[i * 3 + 2];
      }
    }


    // the normals channel
    {
      CDAGBigMapChannel &C = mpChannels[miNChannels - 1];

      C.mNTextureCoordinates = 3;
      C.mChannelID = unsigned char(miNChannels);


      C.mpTVertices = new float[3 * D.miNVertices];

      if (!C.mpTVertices)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }

      C.miNTVertices = D.miNVertices;


      // Note, some components of normals in the example input files (both in vertices and in pair arrays) seem to be invalid. These
      // normals are currently changed to 1, 0, 0.

      /*if ( O.mpNormals )
      {
          for ( int iVertex = 0; iVertex < D.miNVertices; iVertex ++ )
          {
              if ( !_finite( O.mpNormals[ iVertex * 6 ] ) ||
                   !_finite( O.mpNormals[ iVertex * 6 + 1 ] ) ||
                   !_finite( O.mpNormals[ iVertex * 6 + 2 ] ) ||
                   !_finite( O.mpNormals[ iVertex * 6 + 3 ] ) ||
                   !_finite( O.mpNormals[ iVertex * 6 + 4 ] ) ||
                   !_finite( O.mpNormals[ iVertex * 6 + 5 ] ) )
              {
                  printf( "Warning: a normal component in the input file %i has invalid value.\n", iGMTIndex );
              }


              float x1 = O.mpNormals[ iVertex * 6 ],
                    y1 = O.mpNormals[ iVertex * 6 + 1 ],
                    z1 = O.mpNormals[ iVertex * 6 + 2 ],
                    x2 = O.mpNormals[ iVertex * 6 + 3 ],
                    y2 = O.mpNormals[ iVertex * 6 + 4 ],
                    z2 = O.mpNormals[ iVertex * 6 + 5 ];


              if ( fabsf( x1 * x1 + y1 * y1 + z1 * z1 - 1.f ) > 0.0001f )
              {
                  printf( "Warning: a normal in the input file %i is not unit-length.\n", iGMTIndex );
              }


              if ( fabsf( x2 * x2 + y2 * y2 + z2 * z2 - 1.f ) > 0.0001f )
              {
                  printf( "Warning: a normal in the input file %i is not unit-length.\n", iGMTIndex );
              }


              float x = y1 * z2 - z1 * y2;
              float y = z1 * x2 - x1 * z2;
              float z = x1 * y2 - y1 * x2;


              C.mpTVertices[ iVertex * 3 ] = x;
              C.mpTVertices[ iVertex * 3 + 1 ] = y;
              C.mpTVertices[ iVertex * 3 + 2 ] = z;
          }
      }
      else*/
      {
        for (int iVertex = 0; iVertex < D.miNVertices; iVertex++)
        {
          if (!_finite(O.mpVertices[iVertex].mNormal[0]) || !_finite(O.mpVertices[iVertex].mNormal[1]) ||
              !_finite(O.mpVertices[iVertex].mNormal[2]))
          {
            printf("Warning: a vertex normal component in the input "
                   "file \"%s\" has invalid value.\n",
              G.mpFilename);

            O.mpVertices[iVertex].mNormal[0] = 1.f;
            O.mpVertices[iVertex].mNormal[1] = 0.f;
            O.mpVertices[iVertex].mNormal[2] = 0.f;
          }


          if (fabsf(O.mpVertices[iVertex].mNormal[0] * O.mpVertices[iVertex].mNormal[0] +
                    O.mpVertices[iVertex].mNormal[1] * O.mpVertices[iVertex].mNormal[1] +
                    O.mpVertices[iVertex].mNormal[2] * O.mpVertices[iVertex].mNormal[2] - 1.f) > 0.0001f)
          {
            printf("Warning: a vertex normal in the input "
                   "file \"%s\" is not unit-length.\n",
              G.mpFilename);
          }


          memcpy(&C.mpTVertices[iVertex * 3], O.mpVertices[iVertex].mNormal, 12);
        }
      }


      C.mpTFaces = new CDAGBigTFace[iNFaces];

      if (!C.mpTFaces)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      for (int i = 0; i < iNFaces; i++)
      {
        CHECK(int(O.mpIndices[i * 3]) < D.miNVertices && int(O.mpIndices[i * 3 + 1]) < D.miNVertices &&
              int(O.mpIndices[i * 3 + 2]) < D.miNVertices);

        C.mpTFaces[i].mT[0] = O.mpIndices[i * 3];
        C.mpTFaces[i].mT[2] = O.mpIndices[i * 3 + 1];
        C.mpTFaces[i].mT[1] = O.mpIndices[i * 3 + 2];
      }
    }


    // export

    if (!File.SaveBigMesh(D.miNVertices, mpPoints, iNFaces, mpFaces, (unsigned char)miNChannels, mpChannels))
    {
      WRITE_ERROR;
      return false;
    }


    // clean up

    for (int i = 0; i < miNChannels; i++)
    {
      delete[] mpChannels[i].mpTVertices;
      delete[] mpChannels[i].mpTFaces;
    }

    delete[] mpChannels;

    mpChannels = NULL;


    delete[] mpFaces;

    mpFaces = NULL;


    delete[] mpPoints;

    mpPoints = NULL;


    if (!File.StopNode())
    {
      WRITE_ERROR;
      return false;
    }
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Export mesh hull
////////////////////////////////////////////////////////////////////////////////

bool CDAGExporter::ExportMeshHull(CDAGWriter &File, CGMTData *pGMT, int iGMTIndex)
{
  CGMTData &G = *pGMT;


  if (G.mHeader.miNHullBlocks == 0)
  {
    printf("Warning: there is no mesh hull in the input file \"%s\".\n", G.mpFilename);
    return true;
  }


  char GMTName[65];

  memcpy(GMTName, G.mHeader.mName, 64);

  GMTName[64] = 0;


  char Name[200];
  sprintf(Name, "%s_mesh_hull", GMTName);


  CDAGMatrix Matrix;

  Matrix.m[0][0] = 1.f;
  Matrix.m[0][1] = 0.f;
  Matrix.m[0][2] = 0.f;
  Matrix.m[1][0] = 0.f;
  Matrix.m[1][1] = 1.f;
  Matrix.m[1][2] = 0.f;
  Matrix.m[2][0] = 0.f;
  Matrix.m[2][1] = 0.f;
  Matrix.m[2][2] = 1.f;
  Matrix.m[3][0] = 0.f;
  Matrix.m[3][1] = 0.f;
  Matrix.m[3][2] = 0.f;


  if (!File.StartNode(Name, Matrix))
  {
    WRITE_ERROR;
    return false;
  }


  CHECK(0 <= iGMTIndex);

  int iExportMaterialIndex = mpStartMaterialIndices[iGMTIndex];


  if (!(0 <= iExportMaterialIndex && iExportMaterialIndex <= 0xFFFF))
  {
    printf("Error: material index in the input file \"%s\" out of range.\n", G.mpFilename);

    return false;
  }


  unsigned short Material = (unsigned short)iExportMaterialIndex;

  if (!File.SaveNodeMaterials(1, &Material))
  {
    WRITE_ERROR;
    return false;
  }


  // prepare vertices


  CHash Hash(PointGetHashCode, PointIsSame, PointDelete);

  if (!Hash.IsValid())
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  CHECK(G.mHeader.miNHullBlocks > 0);

  mpPoints = new float[G.mHeader.miNHullBlocks * 3 * 3];

  if (!mpPoints)
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }

  int iNPoints = 0;


  for (int iTr = 0; iTr < G.mHeader.miNHullBlocks; iTr++)
  {
    CGMTHullBlock &B = G.mpHull[iTr];

    if (int(B.mMesh) < 0 || int(B.mMesh) >= G.mHeader.miNObjects)
    {
      printf("Error: incorrect object index in a mesh hull "
             "block in the input file \"%s\".\n",
        G.mpFilename);
      return false;
    }


    CGMTObjectData &D = G.mpObjectData[B.mMesh];
    CGMTObject &O = G.mpObjects[B.mMesh];


    for (int iV = 0; iV < 3; iV++)
    {
      int iVertex = B.mI[iV];


      if (iVertex >= D.miNVertices)
      {
        printf("Error: incorrect index in a mesh hull "
               "block in the input file \"%s\".\n",
          G.mpFilename);
        return false;
      }


      CPoint smp(O.mpVertices[iVertex].mPosition[0], O.mpVertices[iVertex].mPosition[1], O.mpVertices[iVertex].mPosition[2]);


      if (Hash.Find(&smp))
      {
        continue;
      }


      CPoint *pNew =
        new CPoint(O.mpVertices[iVertex].mPosition[0], O.mpVertices[iVertex].mPosition[1], O.mpVertices[iVertex].mPosition[2]);

      if (!pNew)
      {
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      pNew->miIndex = iNPoints;


      if (!Hash.Add(pNew))
      {
        delete pNew;
        OUT_OF_MEMORY_WRITE;
        return false;
      }


      CHECK(iNPoints * 3 + 2 < G.mHeader.miNHullBlocks * 3 * 3);


      // transform point by GMT's matrix and write into array

      float x = O.mpVertices[iVertex].mPosition[0];
      float y = O.mpVertices[iVertex].mPosition[1];
      float z = O.mpVertices[iVertex].mPosition[2];


      mpPoints[iNPoints * 3] = x * G.mHeader.mMatrix[0] + y * G.mHeader.mMatrix[4] + z * G.mHeader.mMatrix[8] + G.mHeader.mMatrix[12];

      mpPoints[iNPoints * 3 + 1] =
        x * G.mHeader.mMatrix[1] + y * G.mHeader.mMatrix[5] + z * G.mHeader.mMatrix[9] + G.mHeader.mMatrix[13];

      mpPoints[iNPoints * 3 + 2] =
        x * G.mHeader.mMatrix[2] + y * G.mHeader.mMatrix[6] + z * G.mHeader.mMatrix[10] + G.mHeader.mMatrix[14];


      iNPoints++;
    }
  }


  // prepare faces

  mpFaces = new CDAGBigFace[G.mHeader.miNHullBlocks];

  if (!mpFaces)
  {
    OUT_OF_MEMORY_WRITE;
    return false;
  }


  for (int iTr = 0; iTr < G.mHeader.miNHullBlocks; iTr++)
  {
    CGMTHullBlock &B = G.mpHull[iTr];


    CGMTObject &O = G.mpObjects[B.mMesh];


    for (int iV = 0; iV < 3; iV++)
    {
      int iVertex = B.mI[iV];


      CPoint smp(O.mpVertices[iVertex].mPosition[0], O.mpVertices[iVertex].mPosition[1], O.mpVertices[iVertex].mPosition[2]);


      CPoint *pEx = (CPoint *)Hash.Find(&smp);

      CHECK(pEx);


      if (iV == 0)
      {
        mpFaces[iTr].mVertices[0] = pEx->miIndex;
      }
      else if (iV == 1)
      {
        mpFaces[iTr].mVertices[2] = pEx->miIndex;
      }
      else
      {
        mpFaces[iTr].mVertices[1] = pEx->miIndex;
      }
    }


    mpFaces[iTr].mSMGR = 0;
    mpFaces[iTr].mMaterial = 0;
  }


  // save mesh hull

  if (!File.SaveBigMesh(iNPoints, mpPoints, G.mHeader.miNHullBlocks, mpFaces, 0, NULL))
  {
    WRITE_ERROR;
    return false;
  }


  delete[] mpFaces;

  mpFaces = NULL;


  delete[] mpPoints;

  mpPoints = NULL;


  if (!File.StopNode())
  {
    WRITE_ERROR;
    return false;
  }


  return true;
}


//
// List of GMT files
////////////////////////////////////////////////////////////////////////////////

class CListOfGMTs
{

public:
  CListOfGMTs();
  ~CListOfGMTs();

  bool ReadOneFile(char *pFilename);


  CGMTData *mpStart;


private:
  CGMTData **mppNext;
};


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
////////////////////////////////////////////////////////////////////////////////

CListOfGMTs::CListOfGMTs()
{
  mpStart = NULL;
  mppNext = &mpStart;
}

CListOfGMTs::~CListOfGMTs()
{
  CGMTData *pGMT = mpStart;

  while (pGMT)
  {
    CGMTData *pNextGMT = pGMT->mpNextGMT;

    delete pGMT;

    pGMT = pNextGMT;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Read one GMT file
////////////////////////////////////////////////////////////////////////////////

bool CListOfGMTs::ReadOneFile(char *pFilename)
{
  CHECK(pFilename);


  CHECK(!*mppNext);


  *mppNext = new CGMTData;

  if (!*mppNext)
  {
    printf("Error: out of memory.\n");

    return false;
  }


  FILE *pFile = fopen(pFilename, "rb");

  if (!pFile)
  {
    printf("Error: cannot open input file \"%s\".\n", pFilename);

    return false;
  }


  bool bResult = (*mppNext)->ReadFromFile(pFile, pFilename);


  if (fclose(pFile) != 0)
  {
    printf("Warning: input file \"%s\" did not close correctly.\n", pFilename);
  }


  if (!bResult)
  {
    return false;
  }


  mppNext = &(*mppNext)->mpNextGMT;


  return true;
}


////////////////////////////////////////////////////////////////////////////////
// The main function
////////////////////////////////////////////////////////////////////////////////

int _cdecl main(int iNArgs, char *Args[])
{
  dd_get_fname(""); //== pull in directoryService.obj
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);


  // check arguments


  bool bHelp = false, bConvertObjects = true, bConvertHull = false;

  int iShader = 2;

  bool bCRead = false, bSRead = false;

  char *pOutFilename = NULL;


  if (iNArgs < 2)
  {
    bHelp = true;
  }
  else
  {
    for (int i = 1; i < iNArgs; i++)
    {
      if (_stricmp(Args[i], "-H") == 0 || _stricmp(Args[i], "/H") == 0 || _stricmp(Args[i], "-?") == 0 || _stricmp(Args[i], "/?") == 0)
      {
        bHelp = true;
      }
      else if (_stricmp(Args[i], "-Co") == 0 || _stricmp(Args[i], "/Co") == 0)
      {
        if (!bCRead)
        {
          bCRead = true;
          bConvertObjects = true;
          bConvertHull = false;
        }
        else
        {
          printf("Warning: duplicate -C option ignored.\n");
        }
      }
      else if (_stricmp(Args[i], "-Ch") == 0 || _stricmp(Args[i], "/Ch") == 0)
      {
        if (!bCRead)
        {
          bCRead = true;
          bConvertObjects = false;
          bConvertHull = true;
        }
        else
        {
          printf("Warning: duplicate -C option ignored.\n");
        }
      }
      else if (_stricmp(Args[i], "-Coh") == 0 || _stricmp(Args[i], "/Coh") == 0 || _stricmp(Args[i], "-Cho") == 0 ||
               _stricmp(Args[i], "/Cho") == 0)
      {
        if (!bCRead)
        {
          bCRead = true;
          bConvertObjects = true;
          bConvertHull = true;
        }
        else
        {
          printf("Warning: duplicate -C option ignored.\n");
        }
      }
      else if (_stricmp(Args[i], "-S0") == 0 || _stricmp(Args[i], "/S0") == 0)
      {
        if (!bSRead)
        {
          bSRead = true;
          iShader = 0;
        }
        else
        {
          printf("Warning: duplicate -S option ignored.\n");
        }
      }
      else if (_stricmp(Args[i], "-S1") == 0 || _stricmp(Args[i], "/S1") == 0)
      {
        if (!bSRead)
        {
          bSRead = true;
          iShader = 1;
        }
        else
        {
          printf("Warning: duplicate -S option ignored.\n");
        }
      }
      else if (_stricmp(Args[i], "-S2") == 0 || _stricmp(Args[i], "/S2") == 0)
      {
        if (!bSRead)
        {
          bSRead = true;
          iShader = 2;
        }
        else
        {
          printf("Warning: duplicate -S option ignored.\n");
        }
      }
      else if (!pOutFilename)
      {
        pOutFilename = Args[i];
      }
      else
      {
        printf("Warning: argument \"%s\" not recognized.\n", Args[i]);
      }
    }
  }


  if (bHelp)
  {
    printf("\n");
    printf("Usage\n");
    printf("  gmt2dag [options] output_file < column_list_of_input_files\n");
    printf("\n");
    printf("Options\n");
    printf("  -Co, -Ch, -Coh     Convert objects, mesh hull, or both. Default is -Co.\n");
    printf("  -S0, -S1, -S2      Which input shader to use for exporting texture names.\n");
    printf("                     Default is -S2.\n");
    printf("\n");


    if (iNArgs <= 2)
    {
      return 0;
    }
  }


  if (!pOutFilename)
  {
    printf("Error: no output file specified.\n");
    return 1;
  }


  // read the GMTs

  CListOfGMTs GMTs;


  char InFilename[2000];


  while (fgets(InFilename, 2000, stdin))
  {
    int iLength = strlen(InFilename);

    while (iLength > 0 && (InFilename[iLength - 1] == '\n' || InFilename[iLength - 1] == '\r'))
    {
      iLength--;
    }


    if (iLength == 0)
    {
      continue;
    }


    InFilename[iLength] = 0;


    // read the input file


    if (!GMTs.ReadOneFile(InFilename))
    {
      return 1;
    }
  }


  if (!feof(stdin))
  {
    printf("Warning: the reading of input stream did not complete correctly.\n");
  }


  if (!GMTs.mpStart)
  {
    printf("Error: no input files read.\n");
    return 1;
  }


  // write output

  const int iPathSize = _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 8000;

  char FullPath[iPathSize];


  if (_fullpath(FullPath, pOutFilename, iPathSize) != FullPath)
  {
    printf("Error: cannot get full path name for output file.\n");
    return 1;
  }


  CDAGExporter Exporter(GMTs.mpStart);

  if (!Exporter.Export(FullPath, bConvertObjects, bConvertHull, iShader))
  {
    printf("Error writing output file \"%s\".\n", FullPath);
    return 1;
  }


  return 0;


  /*
      _finddata_t FindData;
      intptr_t p = _findfirst( "*", &FindData );

      int iii = 0;

      if ( p != -1 )
      {
          do
          {
              if ( !( FindData.attrib & _A_SUBDIR ) )
              {
                  iii ++;




                  FILE *pIn = fopen( FindData.name, "rb" );

                  if ( pIn == NULL )
                  {
                      printf( "Error: cannot open the input file \"%s\".\n", FindData.name );
                      return 1;
                  }




                  CGMTData Data;


                  Data.ReadFromFile( pIn );


                  if ( fclose( pIn ) != 0 )
                  {
                      printf( "Warning: the input file did not close correctly.\n" );
                  }




                  Data.mpNextGMT = NULL;





                  const int iPathSize = _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 8000;

                  char FullPath[ iPathSize ];


                  _fullpath( FullPath,
                             FindData.name,
                             iPathSize );



                  char sDrive[ _MAX_DRIVE ],
                       sDir[ _MAX_DIR ],
                       sFName[ _MAX_FNAME ],
                       sExt[ _MAX_EXT ];

                  _splitpath( FullPath,
                              sDrive,
                              sDir,
                              sFName,
                              sExt );



                  FullPath[0] = 0;

                  strcat( FullPath, "e:" );
                  strcat( FullPath, "\\projects\\gmt2dag\\Output\\" );
                  strcat( FullPath, sFName );
                  strcat( FullPath, ".dag" );




                  CDAGExporter Exporter( &Data );

                  Exporter.Export( FullPath,
                                   true,
                                   false,
                                   2 );
              }
          }
          while ( _findnext( p, &FindData ) == 0 );
      }

      _findclose( p );








      return 0;
  */
}

// Note, remove debug information in this project and dag.dll
// Note, mention that < doesn't seem to work for rerouting streams
// Note the current matrix setup in the notes.
