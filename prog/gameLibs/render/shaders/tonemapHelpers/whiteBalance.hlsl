#ifndef WHITE_BALANCE_HLSL_INCLUDE
#define WHITE_BALANCE_HLSL_INCLUDE 1
/*
We used ACES for white balance
=============================================================================
    ACES: Academy Color Encoding System
    https://github.com/ampas/aces-dev/tree/v1.0

    License Terms for Academy Color Encoding System Components

    Academy Color Encoding System (ACES) software and tools are provided by the Academy under
    the following terms and conditions: A worldwide, royalty-free, non-exclusive right to copy, modify, create
    derivatives, and use, in source and binary forms, is hereby granted, subject to acceptance of this license.

    Copyright © 2013 Academy of Motion Picture Arts and Sciences (A.M.P.A.S.). Portions contributed by
    others as indicated. All rights reserved.

    Performance of any of the aforementioned acts indicates acceptance to be bound by the following
    terms and conditions:

     *  Copies of source code, in whole or in part, must retain the above copyright
        notice, this list of conditions and the Disclaimer of Warranty.
     *  Use in binary form must retain the above copyright notice, this list of
        conditions and the Disclaimer of Warranty in the documentation and/or other
        materials provided with the distribution.
     *  Nothing in this license shall be deemed to grant any rights to trademarks,
        copyrights, patents, trade secrets or any other intellectual property of
        A.M.P.A.S. or any contributors, except as expressly stated herein.
     *  Neither the name "A.M.P.A.S." nor the name of any other contributors to this
        software may be used to endorse or promote products derivative of or based on
        this software without express prior written permission of A.M.P.A.S. or the
        contributors, as appropriate.

    This license shall be construed pursuant to the laws of the State of California,
    and any disputes related thereto shall be subject to the jurisdiction of the courts therein.

    Disclaimer of Warranty: THIS SOFTWARE IS PROVIDED BY A.M.P.A.S. AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL A.M.P.A.S., OR ANY
    CONTRIBUTORS OR DISTRIBUTORS, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, RESITUTIONARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    WITHOUT LIMITING THE GENERALITY OF THE FOREGOING, THE ACADEMY SPECIFICALLY
    DISCLAIMS ANY REPRESENTATIONS OR WARRANTIES WHATSOEVER RELATED TO PATENT OR
    OTHER INTELLECTUAL PROPERTY RIGHTS IN THE ACADEMY COLOR ENCODING SYSTEM, OR
    APPLICATIONS THEREOF, HELD BY PARTIES OTHER THAN A.M.P.A.S.,WHETHER DISCLOSED
    OR UNDISCLOSED.
=============================================================================
*/


  // Accurate for [1000K, 15000K]
  //https://en.wikipedia.org/wiki/Planckian_locus
  //CIE_1960_color_space UCS color space UV
  float2 planckian_locus_UCS( float temp )//returns XY CIE 1931 color chromaticity
  {
    float tempSq = temp*temp;
    float u = (0.860117757f + 1.54118254e-4f * temp + 1.28641212e-7f * tempSq) / (1.0f + 8.42420235e-4f * temp + 7.08145163e-7f * tempSq);
    float v = (0.317398726f + 4.22806245e-5f * temp + 4.20481691e-8f * tempSq) / (1.0f - 2.89741816e-5f * temp + 1.61456053e-7f * tempSq);
    return float2(u, v);
  }
  float2 CIE_UCS_UV_2_CIE_XY(float2 uv)
  {
    //CIE_1960_color_space UCS color space UV
    //https://en.wikipedia.org/wiki/CIE_1960_color_space
    //CIE XYZ
    return (float2(3,  2)*uv)/(2*uv.x - 8*uv.y + 4);
  }
  float2 planckian_locus_chromaticity( float temp )//returns XY CIE 1931 color chromaticity
  {
    //https://en.wikipedia.org/wiki/Planckian_locus
    float2 uv = planckian_locus_UCS(temp);
    return CIE_UCS_UV_2_CIE_XY(uv);
  }

  // Accurate for 4000K < temp < 25000K
  // in: correlated color temperature
  // returns XT of CIE 1931 XYZ chromaticity
  float2 standard_illuminant_chromaticity( float temp )
  {
    // This makes 6500 == D65
    temp *= 1.4388 / 1.438;
    //https://en.wikipedia.org/wiki/Standard_illuminant
    float x = temp <= 7000 ?
              0.244063 + ( 0.09911e3 + ( 2.9678e6 - 4.6070e9 / temp ) / temp ) / temp :
              0.237040 + ( 0.24748e3 + ( 1.9018e6 - 2.0064e9 / temp ) / temp ) / temp;

    float y = -3.0 * x*x + 2.870 * x - 0.275;

    return float2(x,y);
  }

  // Find closest color temperature to chromaticity
  float correlated_color_temperature( float2 xy )
  {
    float n = (xy.x - 0.3320) / (0.1858 - xy.y);
    // https://en.wikipedia.org/wiki/Color_temperature##Correlated_color_temperature
    return ((-449.*n + 3525.)*n - 6823.3)*n + 5520.33;
  }

  float2 planckian_isothermal( float temp, float tint )
  {
    float2 uv = planckian_locus_UCS(temp);
    float2 uvd = normalize( uv );

    // Correlated color temperature is meaningful within +/- 0.05
    uv += float2(-uvd.y, uvd.x) * tint * 0.05;
    return CIE_UCS_UV_2_CIE_XY(uv);
  }

  //https://en.wikipedia.org/wiki/Chromatic_adaptation
  //ACES
  float3 xyY_to_XYZ( float3 xyY )
  {
    float3 XYZ;
    float inv_xyY = xyY.z/max(xyY.y, 1e-10);
    XYZ.x = xyY.x * inv_xyY;
    XYZ.y = xyY.z;
    XYZ.z = (1.0 - xyY.x - xyY.y) * inv_xyY;
    return XYZ;
  }
  float3x3 calculate_cat_matrix(float2 srcWhiteXY, float2 destWhiteXY)
  {
    float3x3 CONE_RESP_MAT_BRADFORD = {
      float3( 0.89510,  0.26640, -0.16140),
      float3(-0.75020,  1.71350,  0.03670),
      float3( 0.03890, -0.06850,  1.02960),
    };

    float3x3 CONE_RESP_MAT_BRADFORD_INV =
    {
       float3(0.9869929, -0.1470543,  0.1599627),
       float3(0.4323053,  0.5183603,  0.0492912),
       float3(-0.0085287,  0.0400428,  0.9684867)
    };
    float3 src_XYZ = xyY_to_XYZ( float3( srcWhiteXY, 1 ) );
    float3 dst_XYZ = xyY_to_XYZ( float3( destWhiteXY, 1 ) );

    float3 src_coneResp = mul( CONE_RESP_MAT_BRADFORD, src_XYZ );
    float3 dst_coneResp = mul( CONE_RESP_MAT_BRADFORD, dst_XYZ );
    float3 dst_src = dst_coneResp/src_coneResp;

    float3x3 vkMat =
    {
      float3( dst_src.x, 0.0, 0.0 ),
      float3( 0.0, dst_src.y, 0.0 ),
      float3( 0.0, 0.0, dst_src.z )
    };
    return mul( CONE_RESP_MAT_BRADFORD_INV, mul( vkMat, CONE_RESP_MAT_BRADFORD ) );
  }

  float3 applyWhiteBalance( float3 linearColor, float whiteTemp, float whiteTint )
  {
    float2 whiteDaylight = standard_illuminant_chromaticity(whiteTemp);
    float2 whitePlankian = planckian_locus_chromaticity(whiteTemp);

    float2 srcWhite = whiteTemp < 4000 ? whitePlankian : whiteDaylight;
    float2 D65White = float2( 0.31270,  0.32900 );

    // Offset along isotherm
    srcWhite += planckian_isothermal(whiteTemp, whiteTint) - whitePlankian;
    // REC 709
    //https://ru.wikipedia.org/wiki/SRGB
    float3x3 XYZ_2_sRGB_MAT =
    {
       float3(3.2409699419, -1.5373831776, -0.4986107603),
       float3(-0.9692436363,  1.8759675015,  0.0415550574),
       float3(0.0556300797, -0.2039769589,  1.0569715142)
    };

    float3x3 sRGB_2_XYZ_MAT =
    {
      float3(0.4124564, 0.3575761, 0.1804375),
      float3(0.2126729, 0.7151522, 0.0721750),
      float3(0.0193339, 0.1191920, 0.9503041),
    };

    float3x3 whiteBalanceMat = calculate_cat_matrix( D65White, srcWhite );
    whiteBalanceMat = mul( XYZ_2_sRGB_MAT, mul( whiteBalanceMat, sRGB_2_XYZ_MAT ) );

    return mul( whiteBalanceMat, linearColor );
  }
#endif