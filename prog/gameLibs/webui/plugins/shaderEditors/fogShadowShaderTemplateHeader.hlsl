
/////////////////////////////////////////////////////////
//  Fog Shader
/////////////////////////////////////////////////////////
// [[mrt_outputs_decl]]

// [[shader_local_functions]]

RWTexture3D<float4>  volfog_shadow : register(u6); // register must match volfog_ss_accumulated_shadow_const_no

