// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_gradient_value.h"
#include "graph_value_format.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include <math/dag_mathBase.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace
{
bool stop_is_finite(const GradientStop &s)
{
  return isfinite(s.t) && isfinite(s.r) && isfinite(s.g) && isfinite(s.b) && isfinite(s.a);
}

// Idempotent, so it only ever moves a value once.
void quantize_stop_colors(eastl::vector<GradientStop> &stops)
{
  auto quantize = [](float v) { return gradient_color_byte(v) / 255.f; };
  for (GradientStop &s : stops)
  {
    s.r = quantize(s.r);
    s.g = quantize(s.g);
    s.b = quantize(s.b);
    s.a = quantize(s.a);
  }
}
} // namespace

int parse_gradient(const char *stored, eastl::vector<GradientStop> &out_stops, bool &out_nearest)
{
  out_stops.clear();
  out_nearest = false;
  if (!stored)
  {
    return 0;
  }

  const char *p = stored;
  if (*p == 'N') // the consumer tests offset 0, before any space skipping
  {
    out_nearest = true;
    ++p;
  }

  eastl::vector<float> nums;
  while (*p)
  {
    while (isspace((unsigned char)*p) || *p == ',')
    {
      ++p;
    }
    if (!*p)
    {
      break;
    }
    char *next = nullptr;
    const double v = strtod(p, &next);
    // Junk: keep the prefix, as the consumer does.
    if (next == p)
    {
      break;
    }
    nums.push_back(static_cast<float>(v));
    p = next;
  }

  const int stopCount = static_cast<int>(nums.size()) / 5;
  out_stops.reserve(stopCount);
  for (int i = 0; i < stopCount; ++i)
  {
    GradientStop s;
    s.t = nums[i * 5 + 0];
    s.r = nums[i * 5 + 1];
    s.g = nums[i * 5 + 2];
    s.b = nums[i * 5 + 3];
    s.a = nums[i * 5 + 4];
    out_stops.push_back(s);
  }
  return static_cast<int>(nums.size()) % 5;
}

void sanitize_gradient(eastl::vector<GradientStop> &stops)
{
  stops.erase(eastl::remove_if(stops.begin(), stops.end(), [](const GradientStop &s) { return !stop_is_finite(s); }), stops.end());
  for (GradientStop &s : stops)
  {
    s.t = clamp(s.t, 0.f, 1.f);
    s.r = clamp(s.r, 0.f, 1.f);
    s.g = clamp(s.g, 0.f, 1.f);
    s.b = clamp(s.b, 0.f, 1.f);
    s.a = clamp(s.a, 0.f, 1.f);
  }
  eastl::stable_sort(stops.begin(), stops.end(), [](const GradientStop &l, const GradientStop &r) { return l.t < r.t; });
}

void normalize_gradient_for_editing(eastl::vector<GradientStop> &stops)
{
  sanitize_gradient(stops);

  if (stops.empty()) // the implicit ramp an empty value renders as
  {
    GradientStop black;
    black.a = 1.f;
    GradientStop white;
    white.t = 1.f;
    white.r = white.g = white.b = white.a = 1.f;
    stops.push_back(black);
    stops.push_back(white);
    return;
  }

  if (stops.size() == 1) // renders as one flat colour, and so does the pair
  {
    stops.push_back(stops[0]);
    stops[0].t = 0.f;
    stops[1].t = 1.f;
    return;
  }

  if (stops.front().t > 0.f)
  {
    GradientStop head = stops.front();
    head.t = 0.f;
    stops.insert(stops.begin(), head);
  }
  if (stops.back().t < 1.f)
  {
    GradientStop tail = stops.back();
    tail.t = 1.f;
    stops.push_back(tail);
  }

  // The control drops a key that is not strictly between its neighbours. One float step apart is
  // far below a texel of the 512 wide texture, so the hard edge equal positions encode survives.
  for (int i = 1; i < static_cast<int>(stops.size()); ++i)
  {
    if (stops[i].t <= stops[i - 1].t)
    {
      stops[i].t = nextafterf(stops[i - 1].t, 2.f);
    }
  }
  if (stops.back().t > 1.f) // a run of equal positions at the end pushed past the range
  {
    stops.back().t = 1.f;
    for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i)
    {
      if (stops[i].t >= stops[i + 1].t)
      {
        stops[i].t = nextafterf(stops[i + 1].t, -1.f);
      }
    }
  }
}

bool can_edit_gradient(const eastl::vector<GradientStop> &stops)
{
  if (stops.size() < 2 || stops.size() > GRADIENT_MAX_STOPS)
  {
    return false;
  }
  if (stops.front().t != 0.f || stops.back().t != 1.f)
  {
    return false;
  }
  for (int i = 0; i < static_cast<int>(stops.size()); ++i)
  {
    if (!stop_is_finite(stops[i]))
    {
      return false;
    }
    if (i > 0 && stops[i].t <= stops[i - 1].t)
    {
      return false;
    }
  }
  return true;
}

eastl::string format_gradient(const eastl::vector<GradientStop> &stops, bool nearest)
{
  eastl::string out;
  if (nearest)
  {
    out += 'N';
  }
  for (int i = 0; i < static_cast<int>(stops.size()); ++i)
  {
    if (i > 0)
    {
      out += ", ";
    }
    append_shortest(out, stops[i].t);
    // Colours come back from the control quantized to 8 bits, so 3 decimals is exact for them and
    // reproduces the text the JS editor wrote.
    out += ", ";
    append_rounded(out, stops[i].r, 3);
    out += ", ";
    append_rounded(out, stops[i].g, 3);
    out += ", ";
    append_rounded(out, stops[i].b, 3);
    out += ", ";
    append_rounded(out, stops[i].a, 3);
  }
  return out;
}

eastl::string canonical_gradient(const char *stored)
{
  eastl::vector<GradientStop> stops;
  bool nearest = false;
  parse_gradient(stored, stops, nearest);
  normalize_gradient_for_editing(stops);
  quantize_stop_colors(stops);
  return format_gradient(stops, nearest);
}
