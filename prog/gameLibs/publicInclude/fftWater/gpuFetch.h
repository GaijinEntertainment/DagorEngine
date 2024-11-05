//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point3;

#include <generic/dag_span.h>

namespace fft_water
{
class GpuOnePointQuery;
class GpuFetchQuery;
bool init_gpu_fetch();
void close_gpu_fetch();
GpuOnePointQuery *create_one_point_query();
GpuFetchQuery *create_gpu_fetch_query(int max_number);
void destroy_query(GpuFetchQuery *&q);
void destroy_query(GpuOnePointQuery *&q);
void before_reset(GpuOnePointQuery *q);

void update_query(GpuOnePointQuery *q, const Point3 &pos, float height); // fire new query. Not checking if last one is done
void update_query(GpuFetchQuery *q, const Point3 *pos, int count);       // fire new query. Not checking if last one is done

bool update_query_status(GpuOnePointQuery *q); // return true if can fire new one without stall
bool update_query_status(GpuFetchQuery *q);    // return true if can fire new one without stall

// if is_last != NULL, then it is out parameter, saying if it is valid result from last call (if false, then last call hasn't been
// finished or never started)
dag::ConstSpan<float> get_results(GpuFetchQuery *q, bool *is_last = NULL);
float get_result(GpuOnePointQuery *q, bool *is_last = NULL);
}; // namespace fft_water
