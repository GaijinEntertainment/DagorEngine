float4 sun_light_color;//ground
float cloud_shadow_intensity = 1;
include "shader_global.sh"

float4 real_skies_sun_light_dir;
macro USE_SUBPASS_LOADS()
  //match VS & PS compiler modes, so there is no IO layout problems
  hlsl(vs) {
    ##if hardware.vulkan
      #pragma spir-v compiler dxc
    ##endif
  }
  hlsl(ps) {
    ##if hardware.vulkan
      #pragma spir-v compiler dxc
      #if SHADER_COMPILER_DXC
        #define SUBPASS_RESOURCE(name, reg, iatt_idx) [[vk::input_attachment_index(iatt_idx)]] SubpassInput name;
        #define SUBPASS_LOAD(tgt, tc) tgt.SubpassLoad()
      #else
        //if pragma somehow don't have needed effect
        #error "this shader can't be compiled with HLSLcc"
      #endif
    ##else
    //  #define SUBPASS_RESOURCE(name, reg, iatt_idx) \
    //    Texture2D name: register(t##reg); \
    //    SamplerState name##_samplerstate:register(s##reg)
    //  #define SUBPASS_READ(tgt, tc) tex2D(tgt, tc)
    ##endif
  }
endmacro

macro INIT_SUBPASS_LOAD_DEPTH_GBUFFER(reg, iatt_idx)
endmacro

macro USE_SUBPASS_LOAD_DEPTH_GBUFFER()
endmacro
