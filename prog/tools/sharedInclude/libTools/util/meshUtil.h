//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Mesh;
class ILogWriter;
class Bitarray;

bool validate_mesh(const char *node_name, const Mesh &mesh, ILogWriter &log);

void split_real_two_sided(Mesh &m, Bitarray &is_material_real_two_sided_array, int side_channel_id);
