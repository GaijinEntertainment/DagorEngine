// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_carray.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

constexpr int TRAJECTORY_DIMENSION = 2;
constexpr int NODE_DIMENSION = 2;
using FrameFeaturesData = dag::Vector<vec4f>;
/*
  FrameFeaturesData contains next structure for each frame of animation clip
  For performance we will use vec math operation, so store these features in vec4f
  To get easy access to data use below wrappers based on spans
   N - number of trajectory points
   M - number of featured nodes
{
  //trajectory
  Point2 rootPositions         [N];
  Point2 rootDirections        [N];
  //nodes
  Point3 nodePositions         [M];
  Point3 nodeVelocities        [M];
};
*/

struct FrameFeature
{
  dag::Span<Point2> rootPositions;
  dag::Span<Point2> rootDirections;
  dag::Span<Point3> nodePositions;
  dag::Span<Point3> nodeVelocities;
};

struct ConstFrameFeature
{
  dag::ConstSpan<Point2> rootPositions;
  dag::ConstSpan<Point2> rootDirections;
  dag::ConstSpan<Point3> nodePositions;
  dag::ConstSpan<Point3> nodeVelocities;
};

struct TrajectoryFeature
{
  dag::Span<Point2> rootPositions;
  dag::Span<Point2> rootDirections;
};

struct ConstTrajectoryFeature
{
  dag::ConstSpan<Point2> rootPositions;
  dag::ConstSpan<Point2> rootDirections;
};
struct NodeFeature
{
  dag::Span<Point3> nodePositions;
  dag::Span<Point3> nodeVelocities;
};

struct ConstNodeFeature
{
  dag::ConstSpan<Point3> nodePositions;
  dag::ConstSpan<Point3> nodeVelocities;
};

struct FeaturesSizes
{
  int featuresSizeInVec4f;   // size of all features in vec4f
  int trajectorySizeInVec4f; // size only of trajectory features
};

inline FeaturesSizes caclulate_features_sizes(int node_count, int trajectory_point_count)
{
  int nodeFeaturesCount = node_count * NODE_DIMENSION * 3;                         // 3 from Point3
  int trajectoryFeaturesCount = trajectory_point_count * TRAJECTORY_DIMENSION * 2; // 2 from Point2
  G_ASSERT(trajectoryFeaturesCount % 4 == 0 && "trajectory should be aligned by vec4f for safe usage");

  FeaturesSizes result;
  result.trajectorySizeInVec4f = trajectoryFeaturesCount / 4;
  result.featuresSizeInVec4f = (nodeFeaturesCount + trajectoryFeaturesCount + 3) / 4; // convert float count to vec4f count
  return result;
}

inline int get_features_sizes(int node_count, int trajectory_point_count)
{
  return caclulate_features_sizes(node_count, trajectory_point_count).featuresSizeInVec4f;
}


struct FrameFeatures
{
  FrameFeaturesData data;
  int nodeCount = 0;
  int trajectoryPointCount = 0;
  int featuresSizeInVec4f = 0;
  int trajectorySizeInVec4f = 0;

  void init(int frames, int node_count, int trajectory_point_count)
  {
    trajectoryPointCount = trajectory_point_count;
    nodeCount = node_count;
    FeaturesSizes sizes = caclulate_features_sizes(node_count, trajectory_point_count);

    featuresSizeInVec4f = sizes.featuresSizeInVec4f;
    trajectorySizeInVec4f = sizes.trajectorySizeInVec4f;
    data.resize(frames * featuresSizeInVec4f, v_zero());
  }

  void validateFrameOOB(int frame) const
  {
    G_UNUSED(frame); // compiles in release
    G_ASSERTF(frame >= 0 && frame * featuresSizeInVec4f < data.size(), "frame=%d featuresSizeInVec4f=%d data.size=%d", frame,
      featuresSizeInVec4f, data.size());
  }

  dag::Span<vec4f> get_trajectory_features(int frame)
  {
    validateFrameOOB(frame);
    return make_span(data.data() + frame * featuresSizeInVec4f, trajectorySizeInVec4f);
  }
  dag::Span<vec4f> get_node_features_raw(int frame)
  {
    validateFrameOOB(frame);
    return make_span(data.data() + frame * featuresSizeInVec4f + trajectorySizeInVec4f, featuresSizeInVec4f - trajectorySizeInVec4f);
  }
  dag::Span<Point2> get_root_positions(int frame)
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<Point2 *>(data.data() + offset), trajectoryPointCount);
  }
  dag::Span<Point2> get_root_directions(int frame)
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<Point2 *>(data.data() + offset) + trajectoryPointCount, trajectoryPointCount);
  }
  dag::Span<Point3> get_node_positions(int frame)
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<Point3 *>(data.data() + offset + trajectorySizeInVec4f), nodeCount);
  }
  dag::Span<Point3> get_node_velocities(int frame)
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<Point3 *>(data.data() + offset + trajectorySizeInVec4f) + nodeCount, nodeCount);
  }

  dag::ConstSpan<Point2> get_root_positions(int frame) const
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<const Point2 *>(data.data() + offset), trajectoryPointCount);
  }
  dag::ConstSpan<Point2> get_root_directions(int frame) const
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<const Point2 *>(data.data() + offset) + trajectoryPointCount, trajectoryPointCount);
  }
  dag::ConstSpan<Point3> get_node_positions(int frame) const
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<const Point3 *>(data.data() + offset + trajectorySizeInVec4f), nodeCount);
  }
  dag::ConstSpan<Point3> get_node_velocities(int frame) const
  {
    validateFrameOOB(frame);
    int offset = frame * featuresSizeInVec4f;
    return make_span(reinterpret_cast<const Point3 *>(data.data() + offset + trajectorySizeInVec4f) + nodeCount, nodeCount);
  }

  FrameFeature get_feature(int frame)
  {
    return FrameFeature{get_root_positions(frame), get_root_directions(frame), get_node_positions(frame), get_node_velocities(frame)};
  }
  ConstFrameFeature get_feature(int frame) const
  {
    return ConstFrameFeature{
      get_root_positions(frame), get_root_directions(frame), get_node_positions(frame), get_node_velocities(frame)};
  }
  TrajectoryFeature get_trajectory_feature(int frame)
  {
    return TrajectoryFeature{get_root_positions(frame), get_root_directions(frame)};
  }
  ConstTrajectoryFeature get_trajectory_feature(int frame) const
  {
    return ConstTrajectoryFeature{get_root_positions(frame), get_root_directions(frame)};
  }
  NodeFeature get_node_feature(int frame) { return NodeFeature{get_node_positions(frame), get_node_velocities(frame)}; }
  ConstNodeFeature get_node_feature(int frame) const
  {
    return ConstNodeFeature{get_node_positions(frame), get_node_velocities(frame)};
  }
};

struct FeatureWeights
{
  // featureWeights should be equal to concatenation of next 4 vectors. not binded to das.
  dag::Vector<vec4f> featureWeights; // vectorized weights

  // we use it in das for more readability in das.
  // manage internal state with init_feature_weights and commit_feature_weights.
  dag::Vector<float> rootPositions;
  dag::Vector<float> rootDirections;
  dag::Vector<float> nodePositions;
  dag::Vector<float> nodeVelocities;
};
