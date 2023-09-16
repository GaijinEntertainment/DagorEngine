#include <3d/dag_render.h>

bool grs_draw_wire = false, grs_paused = false, grs_slow = false;

DagorCurView grs_cur_view;
DagorCurFog grs_cur_fog;

int dgs_tex_quality = 0;
int dgs_tex_anisotropy = 1;
