float4 hero_matrixX = (1,0,0,0);
float4 hero_matrixY = (0,1,0,0);
float4 hero_matrixZ = (0,0,1,0);

float4 prev_hero_matrixX = (1,0,0,0);
float4 prev_hero_matrixY = (0,1,0,0);
float4 prev_hero_matrixZ = (0,0,1,0);

macro INIT_HERO_MATRIX(code)
  (code) {
    hero_matrixX@f4 = hero_matrixX;
    hero_matrixY@f4 = hero_matrixY;
    hero_matrixZ@f4 = hero_matrixZ;
    prev_hero_matrixX@f4 = prev_hero_matrixX;
    prev_hero_matrixY@f4 = prev_hero_matrixY;
    prev_hero_matrixZ@f4 = prev_hero_matrixZ;
  }
endmacro

macro USE_HERO_MATRIX(code)
  hlsl(code) {
    bool apply_hero_matrix(inout float3 eye_to_point)
    {
      float3 heroPos = float3(dot(float4(eye_to_point, 1), hero_matrixX),
                              dot(float4(eye_to_point, 1), hero_matrixY),
                              dot(float4(eye_to_point, 1), hero_matrixZ));
      if (all(abs(heroPos) < 1))
      {
        eye_to_point = float3(dot(float4(eye_to_point, 1), prev_hero_matrixX),
                              dot(float4(eye_to_point, 1), prev_hero_matrixY),
                              dot(float4(eye_to_point, 1), prev_hero_matrixZ));
        return true;
      }
      return false;
    }
  }
endmacro