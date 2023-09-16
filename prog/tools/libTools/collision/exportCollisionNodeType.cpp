#include <libTools/collision/exportCollisionNodeType.h>
#include <util/dag_lookup.h>
#include <debug/dag_debug.h>

static constexpr int node_types_count = ExportCollisionNodeType::NODE_TYPES_COUNT;
static const char *node_types[node_types_count] = {"mesh", "box", "sphere", "kdop", "convexComputer", "convexVhacd"};

ExportCollisionNodeType get_export_type_by_name(const char *type_name)
{
  if (!type_name)
  {
    debug("Warring, colliiosn export type have wrong name");
    return ExportCollisionNodeType::UNKNOWN_TYPE;
  }

  return static_cast<ExportCollisionNodeType>(lup(type_name, node_types, node_types_count));
}

const char *get_export_type_name(ExportCollisionNodeType type)
{
  if (type == ExportCollisionNodeType::UNKNOWN_TYPE)
    return "<unknown>";
  return node_types[type];
}