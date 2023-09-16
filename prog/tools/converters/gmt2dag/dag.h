// dag.h: interface for the DLL wrapper of the library for DAG export
//
////////////////////////////////////////////////////////////////////////////////

class DagSaver;


//
// Supporting classes
////////////////////////////////////////////////////////////////////////////////

// Note, we assume that the member alignment and padding will be correct.

class CDAGMaterial
{

public:
  unsigned char mAmbientColor[3], // B, G, R
    mDiffuseColor[3], mSpecularColor[3], mEmissiveColor[3];

  float mfPower;

  unsigned int mFlags;

  unsigned short mTextureIDs[8];

  float mTextureParameters[8];

  char *mpName, *mpClassName, *mpScript;
};


class CDAGMatrix
{

public:
  float m[4][3];
};


#pragma pack(push, 1)

class CDAGBigFace
{

public:
  unsigned int mVertices[3];
  unsigned int mSMGR;
  unsigned short mMaterial;
};

#pragma pack(pop)


class CDAGBigTFace
{

public:
  unsigned int mT[3];
};


class CDAGBigMapChannel
{

public:
  int miNTVertices;
  unsigned char mNTextureCoordinates;
  unsigned char mChannelID;
  float *mpTVertices;
  CDAGBigTFace *mpTFaces;
};


#pragma pack(push, 1)

class CDAGLight
{

public:
  float mfR, mfG, mfB;

  float mfRange, mfDRad;

  unsigned char mDecay;
};


class CDAGLight2
{

public:
  unsigned char mType;

  float mfHotspot,
    mfFalloff; // in degrees, if angle

  float mfMult;
};

#pragma pack(pop)


//
// Class for writing a DAG file
////////////////////////////////////////////////////////////////////////////////

class CDAGWriter
{

public:
  CDAGWriter();
  ~CDAGWriter();

  bool IsValid();


  bool Start(char *pFilename);

  bool Stop();


  bool SaveTextures(int iN, const char **pTexFn);


  bool SaveMaterial(CDAGMaterial &Material);


  bool StartNodes();

  bool StopNodes();

  bool StartNode(char *pName, CDAGMatrix &Matrix,
    int iFlag = 7, // renderable 1, cast shadow 2, receive shadow 4
    int iChildren = 0);

  bool StopNode();

  bool SaveNodeMaterials(int iN, unsigned short *pData);

  bool SaveNodeScript(char *pData);


  bool SaveBigMesh(int iNVertices, float *pVertices, int iNFaces, CDAGBigFace *pFaces, unsigned char NChannels,
    CDAGBigMapChannel *pTexCh, unsigned char *pFaceFlg = 0);


  bool SaveLight(CDAGLight &Light, CDAGLight2 &Light2);


  bool SaveSpline(void **pData, int iCount);


  bool StartNodeRaw(char *pName, int iFlag, int iChildren);

  bool SaveNodeTM(CDAGMatrix &TM);

  bool StartChildren();

  bool StopChildren();

  bool StopNodeRaw();


private:
  DagSaver *mpSaver;
};
