#pragma once

#include <3d/dag_drv3dConsts.h>

unsigned long *parse_fsh_shader(const FSHTYPE *p, int n, int &sz);
unsigned long *parse_vpr_shader(const VPRTYPE *p, int n, int &sz);

void print_fsh_shader(const FSHTYPE *p, int n);
