#ifndef __MICRO_SHADOW_INCLUDED__
#define __MICRO_SHADOW_INCLUDED__

//from http://advances.realtimerendering.com/other/2016/naughty_dog/index.html
// http://advances.realtimerendering.com/other/2016/naughty_dog/NaughtyDog_TechArt_Final.pdf
half calc_micro_shadow(half NoL, half AO)
{
  return (half)saturate(abs(NoL) + half(2.0) * pow2(AO) - half(1.0));
}
#endif