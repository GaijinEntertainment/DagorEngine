float linear_to_log(float linear_luminance)
{
  float GAIN = 128.f;
  float SCALE =3.399738f;
  //max at ~20
  linear_luminance = clamp(linear_luminance, 0.001, 20);
  return log2(linear_luminance*GAIN+1)*(0.3010299956639812/SCALE);//mad, log2, mul
}
float lut_log_to_linear(float log_space_luminance)
{
  float GAIN = 128.f;
  float SCALE =3.399738f;
  //max at ~20
  return exp2(log_space_luminance*(3.321928094887362*SCALE))*(1/GAIN) - 1/GAIN;//mul, mad, exp2
}
