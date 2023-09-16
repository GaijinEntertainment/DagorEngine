#pragma once


class DataBlock;
class ILogWriter;
class BBox3;


struct LodValidationSettings
{
  bool validate = false, warnOnly = false;
  bool checkLodsGoLesser = true;
  float faceDensPerSqPixThres = 5.0f;

public:
  bool loadValidationSettings(const DataBlock &blk);

  bool checkLodValid(unsigned lod_idx, unsigned faces, unsigned prev_lod_faces, float lod_range, const BBox3 &bbox,
    unsigned lods_count, ILogWriter &log) const;
};
