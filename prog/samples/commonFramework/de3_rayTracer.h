#pragma once

/** \addtogroup de3Common
  @{
*/

class Point3;

/**
Callback interface for ray tracers.
*/

class IRayTracer
{
public:
  /**
  \brief Traces ray.

  \param[in] p point ray traces from.
  \param[in] norm_dir tracing ray direction.
  \param[in] trace_dist length of the tracing ray.
  \param[out] max_dist to save distance from p to contact point to.

  @see Point3
  */
  virtual bool rayTrace(const Point3 &p, const Point3 &norm_dir, float trace_dist, float &max_dist) = 0;
};
/** @} */
