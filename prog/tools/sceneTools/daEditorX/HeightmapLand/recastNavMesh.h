#pragma once

enum class NavmeshExportType : uint8_t
{
  WATER = 0,
  SPLINES,
  HEIGHT_FROM_ABOVE,
  GEOMETRY,
  WATER_AND_GEOMETRY,
  INVALID,
  COUNT
};

NavmeshExportType navmesh_export_type_name_to_enum(const char *name);
const char *navmesh_export_type_to_string(NavmeshExportType type);
