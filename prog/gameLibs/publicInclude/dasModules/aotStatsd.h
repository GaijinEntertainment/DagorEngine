//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "statsd/statsd.h"

namespace bind_dascript
{
// get_statsd_counter<N>::statsd_counter is a statsd_counter wrapper, that takes N tags in a form:
// get_statsd_counter<N>::statsd_counter(metric, value, tagKey1, tagVal1, ..., tagKeyN, tagValN)
template <std::size_t NTags, typename... Tags>
struct get_statsd_counter : get_statsd_counter<NTags - 1, Tags..., const char *, const char *>
{};

template <typename... Tags>
struct get_statsd_counter<0, Tags...>
{
private:
  template <std::size_t... I>
  static void statsd_counter_impl(const char *metric, long value, std::index_sequence<I...>, std::tuple<Tags...> tags)
  {
    statsd::counter(metric, value, {{std::get<I * 2>(tags), std::get<I * 2 + 1>(tags)}...});
  }

public:
  static inline void statsd_counter(const char *metric, int32_t value, Tags... tags)
  {
    statsd_counter_impl(metric ? metric : "", static_cast<long>(value), std::make_index_sequence<sizeof...(Tags) / 2>{},
      std::make_tuple((tags ? tags : "")...));
  }
};

inline void statsd_profile_long(const char *metric, int time_ms, const char *tagKey, const char *tagValue)
{
  statsd::profile(metric ? metric : "", (long)time_ms, {tagKey ? tagKey : "", tagValue ? tagValue : ""});
}
} // namespace bind_dascript
