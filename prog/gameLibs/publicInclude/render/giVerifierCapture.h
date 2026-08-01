//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>

struct Driver3dPerspective;

namespace gi_verify
{
// Write a giVerifier replay capture to `dir` (created if missing). Produces a
// capture.blk plus its side files: the canonical gbuffer planes decoded from
// the currently-bound gbuffer through the gi_verify_capture shader, the
// environment as a lat-long EXR, and the daGI2 albedo voxel scene. The
// gbuffer, envi_probe_specular, daGI2 albedo bindings and sun_light_color are
// read from their standard shader vars (common to every game); the caller
// supplies the view, resolution, sun direction and the collision/level paths
// that the replay reads the source geometry from. source_dump is required -
// it is the geometry the replay traces, so an empty one fails the capture. dir_to_sun need not be
// unit length (it is normalized internally), but it must be non-zero whenever
// the game has a sun - the capture fails rather than record a direction the
// replay would light nothing with; width/height must be positive.
//
// The game must compile gameLibs/render/shaders/gi_verify_capture.dshl into
// its shader list - it holds both gi_verify_capture and gi_verify_env_latlong.
// Without it the capture fails loudly rather than writing garbage planes.
//
// Must be called with the game's gbuffer bound (i.e. after the deferred pass,
// before it is consumed/overwritten). Returns true only for a COMPLETE
// capture: capture.blk written and every component that was ATTEMPTED having
// succeeded. A component the game does not have is not a failure - passing no
// EnvWriter skips the environment, and a game with no daGI2 albedo scene
// bound captures without one; both record valid:b=no in the blk and the
// replay falls back to its own material where the capture cannot answer. Each component also carries its own "valid" flag in the blk
// and logs on failure, so a caller can tell which part failed; a caller that only checks the bool can still trust it to mean the
// capture is replayable.
//
// The files (including capture.blk) are written in place, not atomically: a
// crash mid-write can leave a partial capture. This is a developer debug
// capture written to a fresh directory, so that is acceptable; a consumer
// that archives captures should treat a folder as valid only once complete.

// The environment plane is handed to the caller to encode rather than written
// here: gameLibs/render is linked by every game, and an image encoder is not a
// dependency they should all inherit for a developer capture (save_exr pulls
// in tinyexr). `rgba16f` is tightly packed, w*h texels of 4 halves. Pass
// nullptr to skip the environment plane; the blk then records env{valid:b=no}.
using EnvWriter = bool (*)(const char *path, const uint16_t *rgba16f, int w, int h);

bool save_capture(const char *dir, const TMatrix &view_itm, const Driver3dPerspective &persp, int width, int height,
  const Point3 &dir_to_sun, const char *source_dump, const char *level_bin, EnvWriter env_writer);
} // namespace gi_verify
