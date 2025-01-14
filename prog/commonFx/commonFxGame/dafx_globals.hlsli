#ifndef DAFX_GLOBALS_HLSL
#define DAFX_GLOBALS_HLSL

struct GlobalData
{
  float dt; float dt_rcp;
  float water_level; uint un00;
  float4x4 globtm;
  float4x4 globtm_sim;
  float3 world_view_pos; uint gravity_zone_count;
  float3 view_dir_x; uint un03;
  float3 view_dir_y; uint un04;
  float3 view_dir_z; uint un05;
  float2 target_size; float2 target_size_rcp;
  float2 depth_size; float2 depth_size_rcp;
  float2 normals_size; float2 normals_size_rcp;
  float2 depth_tci_offset; float proj_wk; float proj_hk;
  float4 zn_zfar;
  float3 from_sun_direction; uint un08;
  float3 sun_color; uint un09;
  float3 sky_color; uint un11;
  float3 wind_speed; float znear_offset;
  float2 wind_dir; float wind_power; float wind_scroll;
  float2 depth_size_for_collision; float2 depth_size_rcp_for_collision;
  float3 camera_velocity; uint un12;
};

#ifndef __cplusplus
  GlobalData global_data_load()
  {
    GlobalData o;
    o.dt = asfloat( dafx_global_data[0].x );
    o.dt_rcp = asfloat( dafx_global_data[0].y );
    o.water_level = asfloat( dafx_global_data[0].z );

    o.globtm[0] = asfloat( dafx_global_data[1] );
    o.globtm[1] = asfloat( dafx_global_data[2] );
    o.globtm[2] = asfloat( dafx_global_data[3] );
    o.globtm[3] = asfloat( dafx_global_data[4] );

    o.globtm_sim[0] = asfloat( dafx_global_data[5] );
    o.globtm_sim[1] = asfloat( dafx_global_data[6] );
    o.globtm_sim[2] = asfloat( dafx_global_data[7] );
    o.globtm_sim[3] = asfloat( dafx_global_data[8] );

    o.world_view_pos = asfloat( dafx_global_data[9].xyz );
    o.gravity_zone_count = dafx_global_data[9].w;

    o.view_dir_x = asfloat( dafx_global_data[10].xyz );
    o.view_dir_y = asfloat( dafx_global_data[11].xyz );
    o.view_dir_z = asfloat( dafx_global_data[12].xyz );

    o.target_size = asfloat( dafx_global_data[13].xy );
    o.target_size_rcp = asfloat( dafx_global_data[13].zw );

    o.depth_size = asfloat( dafx_global_data[14].xy );
    o.depth_size_rcp = asfloat( dafx_global_data[14].zw );

    o.normals_size = asfloat( dafx_global_data[15].xy );
    o.normals_size_rcp = asfloat( dafx_global_data[15].zw );

    o.depth_tci_offset = dafx_global_data[16].xy;
    o.proj_wk = asfloat( dafx_global_data[16].z );
    o.proj_hk = asfloat( dafx_global_data[16].w );

    o.zn_zfar = asfloat( dafx_global_data[17].xyzw );

    o.from_sun_direction = asfloat( dafx_global_data[18].xyz );
    o.sun_color = asfloat( dafx_global_data[19].xyz );
    o.sky_color = asfloat( dafx_global_data[20].xyz );

    o.wind_speed = asfloat( dafx_global_data[21].xyz );
    o.znear_offset = asfloat( dafx_global_data[21].w );

    o.wind_dir = asfloat( dafx_global_data[22].xy );
    o.wind_power = asfloat( dafx_global_data[22].z );
    o.wind_scroll = asfloat( dafx_global_data[22].w );

    o.depth_size_for_collision = asfloat(dafx_global_data[23].xy);
    o.depth_size_rcp_for_collision = asfloat(dafx_global_data[23].zw);

    o.camera_velocity = asfloat( dafx_global_data[24].xyz );

    return o;
  }
#endif

#define gdata_gravity float3( 0, -9.8f, 0 )

#endif