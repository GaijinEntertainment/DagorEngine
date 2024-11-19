// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void calculate_normalization(
  dag::Vector<AnimationClip> &clips, dag::Vector<vec4f> &avg, dag::Vector<vec4f> &std, int features, int normalization_groups);

void normalize_feature(dag::Span<vec4f> dst_features, const vec4f *src_features, const vec4f *avg, const vec4f *std);
inline void normalize_feature(dag::Span<vec4f> features, const vec4f *avg, const vec4f *std)
{
  normalize_feature(features, features.data(), avg, std);
}
void denormalize_feature(dag::Span<vec4f> features, const vec4f *avg, const vec4f *std);
