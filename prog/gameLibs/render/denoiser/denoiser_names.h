// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace denoiser
{

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

#define NAME(name) const char *TextureNames::name = TOSTRING(name);
ENUM_PERSISTENT_NAMES
const char *TextureNames::motion_vectors = "motion_vectors";
const char *TextureNames::half_motion_vectors = "half_motion_vectors";
const char *TextureNames::half_normals = "half_normals";
const char *TextureNames::half_depth = "half_depth";
const char *TextureNames::reblur_history_confidence = "reblur_history_confidence";
#undef NAME

#define NAME(name) const char *AODenoiser::TextureNames::name = TOSTRING(name);
ENUM_AO_DENOISED_NAMES
#undef NAME

#define NAME(name) const char *GIDenoiser::TextureNames::name = TOSTRING(name);
ENUM_GI_DENOISED_NAMES
#undef NAME

#define NAME(name) const char *ReflectionDenoiser::TextureNames::name = TOSTRING(name);
ENUM_RTR_DENOISED_NAMES
const char *ReflectionDenoiser::TextureNames::rtr_validation = "rtr_validation";
#undef NAME

#define NAME(name) const char *ShadowDenoiser::TextureNames::name = TOSTRING(name);
ENUM_RTSM_DENOISED_NAMES
#undef NAME

} // namespace denoiser