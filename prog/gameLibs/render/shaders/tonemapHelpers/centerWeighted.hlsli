float2 getCenterWeightedTc(int index, int samples_count)
{
  float E2 = (float)(index) / samples_count;
  float E1 = frac(index * 0.6180339887498949);
  float2 epiTc = float2( E1, E2 );
  //float2 epiTc = fibonacci_sphere( DTid, adaptation_cw_samples_count.z );
  //float2 epiTc = (DTid+0.5)/adaptation_samples_count.xy;
  float ang = epiTc.x*PI*2;
  float2 dir = float2(sin(ang), cos(ang));
  //float rad = epiTc.y/max(abs(dir.x), abs(dir.y));//this is too much center weighted
  float centerWeighted = 0.5;//centerWeighted == 0 - not center weighted
  float rad = lerp(sqrt(epiTc.y), epiTc.y, centerWeighted)/max(abs(dir.x), abs(dir.y));
  return saturate(0.5 + (0.5*rad)*dir);
}