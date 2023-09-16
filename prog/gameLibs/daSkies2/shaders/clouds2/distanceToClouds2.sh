include "base_distance_to_clouds.sh"

macro DISTANCE_TO_CLOUDS2(code)
  BASE_DISTANCE_TO_CLOUDS(code)
  local float radius_to_camera = (skies_planet_radius+max(skies_world_view_pos.y, 10-min_ground_offset)/1000);
  (code) {
    radius_to_camera_info@f3 = (radius_to_camera, radius_to_camera*radius_to_camera,radius_to_camera*radius_to_camera-skies_planet_radius*skies_planet_radius,0);
  }

  hlsl(code) {
    float distance_to_planet(float3 v, float maxDist)
    {
      //fixme: doesn't take skies_world_view_pos.xz into account. should we?
      float Rh=radius_to_camera_info.x;
      float b=Rh*v.y;
      float c=radius_to_camera_info.z;//Rh*Rh-RH*RH
      float b24c=b*b-c;

      return b24c < 0 ? maxDist : (-b-sqrt(b24c))*1000;//km to meters
      //float3 P = world_view_pos;
      //P.y += radius;
      //return intersection_rayXsphere(P, v, radius);
    }
    void distance_to_clouds(float3 v, out float dist0, out float dist1)
    {
      base_distance_to_clouds(v.y, dist0, dist1, radius_to_camera_info.x, radius_to_camera_info.y);
    }
  }
endmacro

