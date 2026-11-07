// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>

// Codec for a gradient_editor value, as its only consumer TextureRegManager::fillGradientTexture
// defines it: optional leading 'N' (nearest), then floats, 5 per stop (t, r, g, b, a), in 0..1 and
// ascending by t. It clamps outside the stop range; an empty string is a black to white ramp.
struct GradientStop
{
  float t = 0.f;
  float r = 0.f, g = 0.f, b = 0.f, a = 1.f;
};

// The control holds colours as 8 bits. Both the panel and canonical_gradient round through this,
// or a value that was only opened would compare as changed.
inline unsigned char gradient_color_byte(float v) { return static_cast<unsigned char>(v * 255.f + 0.5f); }

// fillGradientTexture reads only the first 200 floats, so never write more.
inline constexpr int GRADIENT_MAX_STOPS = 40;

// Returns the count of trailing floats that make no whole stop; the consumer ignores them too.
int parse_gradient(const char *stored, eastl::vector<GradientStop> &out_stops, bool &out_nearest);

// Repairs what the control can hand back: a key dragged past its neighbour, or a nan position.
void sanitize_gradient(eastl::vector<GradientStop> &stops);

// Makes the stops representable by the PropPanel gradient control (at least 2 keys, strictly
// ascending, first at 0 and last at 1) without changing what fillGradientTexture renders.
void normalize_gradient_for_editing(eastl::vector<GradientStop> &stops);

// GradientControlStandalone::setValue's preconditions, checked rather than assumed: it rejects the
// value in silence and leaves its own default on screen, which the user can then commit.
bool can_edit_gradient(const eastl::vector<GradientStop> &stops);

eastl::string format_gradient(const eastl::vector<GradientStop> &stops, bool nearest);

// The stored value as this editor would write it back, for telling a real edit from a no-op.
eastl::string canonical_gradient(const char *stored);
