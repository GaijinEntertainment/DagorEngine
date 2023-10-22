#include <util/dag_bitFlagsMask.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMathDecl.h>


namespace rendinst
{

enum class DrawCollisionsFlag : uint32_t
{
  RendInst = 1 << 0,
  RendInstExtra = 1 << 1,
  All = RendInst | RendInstExtra,

  Wireframe = 1 << 2,
  Opacity = 1 << 3,
  Invisible = 1 << 4,
  RendInstCanopy = 1 << 5,
  PhysOnly = 1 << 6,
  TraceOnly = 1 << 7,

  ALL_FLAGS = All | Wireframe | Opacity | Invisible | RendInstCanopy | PhysOnly | TraceOnly
};
using DrawCollisionsFlags = BitFlagsMask<DrawCollisionsFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(rendinst::DrawCollisionsFlag);

inline constexpr float DEFAULT_MAX_COLLISION_DISTANCE_SQUARED = 10000000;
inline constexpr float DEFAULT_MAX_LABEL_DIST_SQ = 20 * 20;

void drawDebugCollisions(DrawCollisionsFlags flags, mat44f_cref globtm, const Point3 &view_pos, bool reverse_depth,
  float max_coll_dist_sq = DEFAULT_MAX_COLLISION_DISTANCE_SQUARED, float max_label_dist_sq = DEFAULT_MAX_LABEL_DIST_SQ);

} // namespace rendinst

template <>
struct BitFlagsTraits<rendinst::DrawCollisionsFlag>
{
  static constexpr auto allFlags = rendinst::DrawCollisionsFlag::ALL_FLAGS;
};

constexpr BitFlagsMask<rendinst::DrawCollisionsFlag> operator~(rendinst::DrawCollisionsFlag bit) { return ~make_bitmask(bit); }
