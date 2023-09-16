#include <movie/fullScreenMovie.h>
#include "movieAspectRatio.h"
#include <debug/dag_debug.h>

float yuvframerender::forced_movie_inv_aspect_ratio = 0;
namespace yuvframerender
{
bool prewarm_done = false;
}

void force_movie_inv_aspect_ratio(float h_by_w_ratio)
{
  G_ASSERT(h_by_w_ratio >= 0);
  yuvframerender::forced_movie_inv_aspect_ratio = h_by_w_ratio;
  if (h_by_w_ratio > 0)
    debug("force_movie_inv_aspect_ratio(%.4f), forced aspect=%.3f", h_by_w_ratio, 1.0 / h_by_w_ratio);
  else
    debug("force_movie_inv_aspect_ratio(0), switched auto aspect");
}
