float clouds_ms_contribution = 0.7;//on sunset 0.7 is better, in noon 0.5-0.6 is better
float clouds_ms_attenuation = 0.3;//in sunset 0.25 is better, in noon 0.45 is better

macro CLOUDS_MULTIPLE_SCATTERING_BASE()
  //3 octaves
  local float contributionNorm = 1./(1+clouds_ms_contribution+clouds_ms_contribution*clouds_ms_contribution);//+pow(clouds_ms_contribution,3))
  //all 4 octaves
  local float4 ms_sigma = (1, clouds_ms_attenuation, clouds_ms_attenuation*clouds_ms_attenuation, pow(clouds_ms_attenuation,3))*global_clouds_sigma*1.44269504089;//1.44269504089 - is log2(e)

  //premultiplied by contribution
  local float4 clouds_ms_contribution4_base = (1,clouds_ms_contribution, clouds_ms_contribution*clouds_ms_contribution, pow(clouds_ms_contribution, 3))*contributionNorm;
  hlsl {
    //number of octaves used in shader = 3
    #define NUMBER_OF_MS_OCTAVES 3//should match contributionNorm and clouds_ms_attenuation_param
  }
endmacro
