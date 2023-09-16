include "shader_global.sh"
include "waveWorks.sh"

float4 water_texture_size = (512, 512, 1/512, 1/512);
float4 water_gradient_scales0 = (1,1,0,0);
float4 water_gradient_scales1 = (1,1,0,0);
float4 water_gradient_scales2 = (1,1,0,0);
float4 water_gradient_scales3 = (1,1,0,0);
float4 water_gradient_scales4 = (1,1,0,0);
int water_foam_pass = 0;
interval water_foam_pass:first<1, second;
float4 foam_dissipation = (0,0,0,0);
float4 foam_blur_extents0123 = (0,0,0,0);
float4 foam_blur_extents4567 = (0,0,0,0);

texture water_foam_texture0;
texture water_foam_texture1;
texture water_foam_texture2;
texture water_foam_texture3;
texture water_foam_texture4;