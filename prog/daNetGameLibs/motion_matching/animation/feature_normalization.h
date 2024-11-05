// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void calculate_normalization(dag::Vector<AnimationClip> &clips, dag::Vector<vec3f> &avg, dag::Vector<vec3f> &std, int features);

void normalize_feature(dag::Span<vec3f> features, const dag::Vector<vec3f> &avg, const dag::Vector<vec3f> &std);
void denormalize_feature(dag::Span<vec3f> features, const dag::Vector<vec3f> &avg, const dag::Vector<vec3f> &std);
