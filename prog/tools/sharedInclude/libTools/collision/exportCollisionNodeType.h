// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum ExportCollisionNodeType
{
  UNKNOWN_TYPE = -1,
  MESH,
  BOX,
  SPHERE,
  KDOP,
  CONVEX_COMPUTER,
  CONVEX_VHACD,
  NODE_TYPES_COUNT,
};

ExportCollisionNodeType get_export_type_by_name(const char *type_name);
const char *get_export_type_name(ExportCollisionNodeType type);