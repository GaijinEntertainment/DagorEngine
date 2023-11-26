int tree_foliage_alpha_to_coverage = 0;
interval tree_foliage_alpha_to_coverage : off<1, on;

int impostor_alpha_to_coverage = 0;
interval impostor_alpha_to_coverage : off<1, on;

macro USE_ALPHA_TO_COVERAGE_IMPL(shader_interval)
    if (shader_interval == on)
    {
      alpha_to_coverage = true;
    }

    if (shader_interval == off)
    {
      hlsl(ps) {
        #define APPLY_A2C_TO_GBUFFER(output, alpha)
        #define ALPHA_TO_FOLIAGE_ALPHA_MUL 1
      }
    }
    else
    {
      hlsl(ps) {
        #define APPLY_A2C_TO_GBUFFER(output, alpha) output.col0.a = alpha
        #define ALPHA_TO_FOLIAGE_ALPHA_MUL 25.0
      }
    }

    hlsl(ps) {
      #define A2C_IMPOSTOR_ALPHA_MUL 15

      void clipAlphaA2C(float alpha) {
        ##if shader_interval == off
          clip_alpha(alpha);
        ##else
          if (alpha <= 0)
            discard;
        ##endif
      }
    }
endmacro

macro USE_FOLIAGE_ALPHA_TO_COVERAGE()
  USE_ALPHA_TO_COVERAGE_IMPL(tree_foliage_alpha_to_coverage)
endmacro

macro USE_IMPOSTOR_ALPHA_TO_COVERAGE()
  USE_ALPHA_TO_COVERAGE_IMPL(impostor_alpha_to_coverage)
endmacro