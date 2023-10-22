//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class Point3;
class TMatrix;


class IFmodService
{
public:
  static constexpr unsigned HUID = 0x222857A0u; // IFmodService

  virtual void init(const char *snd_filepath) = 0;
  virtual void term() = 0;

  virtual void setupListener(const TMatrix &cur_tm) = 0;
  virtual void updateSounds(bool calc_occlusion) = 0;

  virtual void *createEvent(const char *sound_asset_name) = 0;
  virtual void setEventPos(void *event, const Point3 &pos) = 0;
  virtual void getEventMinMaxDist(void *event, float &min_d, float &max_d) = 0;
  virtual void destroyEvent(void *event) = 0;
};
