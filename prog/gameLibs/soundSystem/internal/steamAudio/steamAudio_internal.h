// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>

class DataBlock;
class TMatrix;
struct FMOD_DSP_DESCRIPTION;

namespace FMOD
{
class DSP;
class System;
} // namespace FMOD

namespace sndsys::steam_audio
{
bool is_inited();
void init(const DataBlock &blk, FMOD::System *);
void shutdown();
void update_listener(const TMatrix &listener_tm);
void update();

// Sets the binaural direction parameters on an existing DSP instance.
void set_spatialized_direction(FMOD::DSP *dsp, const Point3 &source_pos);

// Fills out_desc with the FMOD_DSP_DESCRIPTION for the binaural spatializer plugin.
// Used by steamAudioSpatializer.cpp to create per-event DSP instances.
bool get_spatializer_description(FMOD_DSP_DESCRIPTION &out_desc);
} // namespace sndsys::steam_audio
