#ifndef FX_DEBUG_COLOR_HLSLI_INCLUDED
#define FX_DEBUG_COLOR_HLSLI_INCLUDED 1

float3 calc_debug_color(float2 delta_xy, float delta_scale, float border_scale, float center_scale)
{
  float2 dd = delta_xy.xy * 0.5 + 0.5;

  float border = 0;
  float tt = 0.05;
  if ( dd.x < tt || dd.x > ( 1 - tt ) )
    border = 1;

  if ( dd.y < tt || dd.y > ( 1 - tt ) )
    border = 1;

  float center = 0;
  if ( dot( delta_xy, delta_xy ) < 0.3 )
    center = 1;

  dd *= delta_scale;
  border *= border_scale;
  center *= center_scale;

  float3 color;
  color.r = ( center > 0 ? 1 : 0 );
  color.gb = ( center > 0 ? 0 : 1 ) * dd;
  color.rgb += border;
  color = saturate( color );
  return color;
}

float3 calc_debug_color(float2 delta_xy)
{
  return calc_debug_color(delta_xy, 0.1, 0.2, 0.3);
}

#endif
