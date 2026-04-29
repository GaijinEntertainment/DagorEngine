//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct HeightmapMetricsQuality
{
  // this is calculated for view projection, not nesessarily current projection.
  // For CSM it is main camera one, so it matches.
  float distanceScale = 0;
  static constexpr int invalid_tess = -127;
  int8_t maxRelativeTexelTess = invalid_tess; // if maxRelativeTexelTess != invalid_tess, we won't tesselate more than said amount of
                                              // texels pow 2. 1 means at most over tesselation of 2, -3 - under tesselation of 0.125
  enum : uint8_t
  {
    FASTEST = 0,
    USE_MORPH = 1,
    EXACT_EDGES = 2,
    BEST = USE_MORPH | EXACT_EDGES
  } quality = BEST;
  float heightRelativeErr = 0; // if heightRelativeErr > 0, we will try to achieve said relative result

  // if displacementDist = 0 or displacementMetersRepeat =0,
  // then maximum tesselation would be naturally not more than 2 (one level above texel-to-quad), regardless of maxRelativeTexelTess

  float displacementMetersRepeat = 0.2; // now, this is a bit hard to measure.
                                        // We assume displacement is not completely random, like each pixel can be of any value
                                        // but it is not only more-or less monotonic,
                                        // but also have statistical "repeatition" - i.e. statistically minimums are separated by this
                                        // distance (it doesn't have to be actually tiled texture, just statiscally) the bigger this
                                        // number is, the farther away displacement matters, so it can be percieved as displacement
                                        // importance
  float displacementDist = 180;         // we assume, that within displacement_dist there is displacement of -maxDownwardDisplacement,
                                        // maxUpwardDisplacement
  float displacementFalloff = 0.1;      // % of displacement_dist, so displacement tesselation doesn't end abruptly.

  float maxAbsHeightErr = 0; // max_abs_height_err height_relative_err > 0, we will not allow error more than it. It should be rather
                             // big number!
  int8_t minRelativeTexelTess = invalid_tess; // if minRelativeTexelTess != invalid_tess, we won't tesselate LESS than said amount of
                                              // texels. for example, -2 means at least there would be 4 texels per quad
};
