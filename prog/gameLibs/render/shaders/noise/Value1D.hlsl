#ifndef NOISE_VALUE_1D
#define NOISE_VALUE_1D 1
//derived from noise_Value2D
float noise_value1D( float P )
{
    //	establish our grid cell and unit position
    float Pi = floor(P);
    float Pf = P - Pi;

    //	calculate the hash.
    const float DOMAIN = 71.0;
    float2 Pt = float2( Pi, Pi + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / DOMAIN )) * DOMAIN;	//	truncate the domain
    Pt += 26.0;								//	offset to interesting part of the noise
    Pt *= Pt;											//	calculate and return the hash
    float2 hash = frac( Pt.xy * ((161*161) * ( 1.0 / 951.135664 )) );//assuming gridcell.y is 0, we have same as 2d hash

    //	blend the results and return
    float blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    return lerp( hash.x, hash.y, blend );
}
#endif
