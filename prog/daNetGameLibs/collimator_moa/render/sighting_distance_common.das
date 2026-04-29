module sighting_distance_common shared

require math

def calc_collimator_moa_reticle_sighting_offset(collimatorLensWtm : float3x4;
                                                collimatorCameraNodeWpos : float3;
                                                gunShootWpos : float3;
                                                parallaxPlaneDistance : float;
                                                sightingDistance : float)
  let moaLensCenterWpos = collimatorLensWtm[3]
  let lensDown = -normalize(collimatorLensWtm[2])

  let camNodeToLensCenterDist = length(collimatorCameraNodeWpos - moaLensCenterWpos)
  let shootPosHeight = dot(gunShootWpos - moaLensCenterWpos, lensDown)

  let camNodeToParallaxPlaneDist = camNodeToLensCenterDist + parallaxPlaneDistance
  return -1.0 * shootPosHeight * camNodeToParallaxPlaneDist / (camNodeToLensCenterDist + sightingDistance)
