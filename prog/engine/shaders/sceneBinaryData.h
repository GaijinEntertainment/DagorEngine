#pragma once

namespace renderscenebindump
{
struct SceneHdr
{
  int version, rev;
  int texNum, matNum, vdataNum, mvhdrSz;
  int ltmapNum, ltmapFormat;
  float lightmapScale, directionalLightmapScale;
  int objNum, fadeObjNum, ltobjNum;
  int objDataSize;
};

/* Data layout:
 * SceneHdr
 * texIdx[]
 * ltmap catalog
 * ShaderMatVdata::MatVdataHdr
 * vb/ib data (compressed)
 * obj[]
 * fadeObjInds
 * lightingObjectDataList
 */
} // namespace renderscenebindump
