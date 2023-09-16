include "clouds_alt_fraction.sh"
texture cloud_layers_altitudes_tex;

hlsl {
  #define MAX_CLOUDS_DIST ((((1U<<5)-1)*32 + (1U<<5))*256.)
  #define MAX_CLOUDS_DIST_KM (MAX_CLOUDS_DIST*0.001)
  #define INFINITE_TRACE_DIST (MAX_CLOUDS_DIST+1)
  #define TRACE_DIST MAX_CLOUDS_DIST
  #define closeSequenceStepSize 64.//todo: rename
  #define closeSequenceSteps 32
}

macro BASE_DISTANCE_TO_CLOUDS(code)
  //local float cloud_start_radius = (skies_planet_radius+clouds_start_altitude2);
  //local float cloud_end_radius = (cloud_start_radius+clouds_thickness2);
  (code) {
    cloud_layers_altitudes_tex@smp2d = cloud_layers_altitudes_tex;
    //if no info
    //clouds_layer_info_big@f4 = (cloud_start_radius-0.001,
    //                            cloud_end_radius+0.001,
    //                             -cloud_start_radius*cloud_start_radius,
    //                             -cloud_end_radius*cloud_end_radius);
  }
  hlsl(code) {
    #include <distance_to_clouds.hlsl>
    void base_distance_to_clouds(float mu, out float dist0, out float dist1, float radius_to_point, float radius_to_point_sq)
    {
      math_distance_to_clouds(mu, dist0, dist1, radius_to_point, radius_to_point_sq, cloud_layers_altitudes_tex[uint2(0, 0)]);
      //base_distance_to_clouds(v, dist0, dist1, radius_to_point, radius_to_point_sq, clouds_layer_info_big);
    }
  }
endmacro
