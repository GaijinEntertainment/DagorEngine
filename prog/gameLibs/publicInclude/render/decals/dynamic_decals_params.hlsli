// Uncomment this to tweak decal params and unlock shader vars and console (needs shader/exe compilation)
// also needs uncommenting in dynamic_decals_inc.dshl

// #define DYNAMIC_DECALS_TWEAKING

#define dyn_decals_rimScale  1.0
#define dyn_decals_colorDistMul  0.4
#define dyn_decals_noiseUvMul  20.0f
#define dyn_decals_noiseScale  0.2f
#define dyn_decals_params        float4(dyn_decals_rimScale, dyn_decals_colorDistMul, dyn_decals_noiseUvMul, dyn_decals_noiseScale)

#define dyn_decals_rimDistMul   3.0f
#define dyn_decals_rimDistMul2 10.0f
#define dyn_decals_params2       float4(dyn_decals_rimDistMul, dyn_decals_rimDistMul2, 0, 0)

#define dyn_decals_cutting_color float4(0.4, 0.4, 0.4, 1.0)
#define dyn_decals_burn_color    float4(0.01, 0.01, 0.01, 0.8)
#define dyn_decals_wood_color    float4(0.9, 0.71, 0.38, 1.0)
#define dyn_decals_fabric_color  float4(0.9, 0.8, 0.5, 1.0)

#define dyn_decals_steel_hole_noise       float4(0.8, 0.7, 0.3, 1.0)
#define dyn_decals_wood_hole_noise        float4(0.8, 0.7, 0.3, 1.0)
#define dyn_decals_fabrics_hole_noise     float4(1.0, 1.0, 0.65, 1.5)

#define dyn_decals_steel_wreackage_noise  float4(1.0, 1.0, 0.0, 0.0)
#define dyn_decals_wood_wreackage_noise   float4(2.0, 1.0, 0.3, 0.2)
#define dyn_decals_fabric_wreackage_noise float4(1.0, 1.0, 0.0, 0.0)

#define dyn_decals_holes_burn_mark_noise  float4(4, 0.3, 0.01, 3)

#define dyn_decals_bullet_mark_noise float4(0.2, 2.0, 3.0, 1.0)

//mult/min/max
#define dyn_decals_diff_mark_params        float4(1.0, -1.0, 1.0, 0.0)
#define dyn_decals_burn_mark_params        float4(0.2, 0.01, 1.0, 0.0)
#define dyn_decals_bullet_diff_mark_params float4(1.0, 0.00, 0.1, 0.0)

#define dyn_decals_smoothness  0.3
#define dyn_decals_metallness  0.6

#define dyn_decals_material float2(dyn_decals_smoothness, dyn_decals_metallness)
