#ifndef ATMOSPHERE_TRANSITTANCE_HLSLI_INCLUDED
#define ATMOSPHERE_TRANSITTANCE_HLSLI_INCLUDED 1
/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
<p>They use the following utility functions to avoid NaNs due to floating point
values slightly outside their theoretical bounds:
*/
INLINE Number ClampCosine(Number mu) {
  return clamp(mu, Number(-1.0), Number(1.0));
}

INLINE Length ClampDistance(Length d) {
  return max(d, 0.0 * meter);
}

INLINE Length ClampRadius(IN(AtmosphereParameters) atmosphere_p, Length r) {
  return clamp(r, atmosphere_p.bottom_radius, atmosphere_p.top_radius);
}

INLINE Length SafeSqrt(Area a) {
  return sqrt(max(a, 0.0 * m2));
}

/*
<h3 id="transmittance">Transmittance</h3>

<p>As the light travels from a point $\bp$ to a point $\bq$ in the atmosphere_p,
it is partially absorbed and scattered out of its initial direction because of
the air molecules and the aerosol particles. Thus, the light arriving at $\bq$
is only a fraction of the light from $\bp$, and this fraction, which depends on
wavelength, is called the
<a href="https://en.wikipedia.org/wiki/Transmittance">transmittance</a>. The
following sections describe how we compute it, how we store it in a precomputed
texture, and how we read it back.

<h4 id="transmittance_computation">Computation</h4>

<p>For 3 aligned points $\bp$, $\bq$ and $\br$ inside the atmosphere_p, in this
order, the transmittance between $\bp$ and $\br$ is the product of the
transmittance between $\bp$ and $\bq$ and between $\bq$ and $\br$. In
particular, the transmittance between $\bp$ and $\bq$ is the transmittance
between $\bp$ and the nearest intersection $\bi$ of the half-line $[\bp,\bq)$
with the top or bottom atmosphere_p boundary, divided by the transmittance between
$\bq$ and $\bi$ (or 0 if the segment $[\bp,\bq]$ intersects the ground):

<svg width="340px" height="195px">
  <style type="text/css"><![CDATA[
    circle { fill: #000000; stroke: none; }
    path { fill: none; stroke: #000000; }
    text { font-size: 16px; font-style: normal; font-family: Sans; }
    .vector { font-weight: bold; }
  ]]></style>
  <path d="m 0,26 a 600,600 0 0 1 340,0"/>
  <path d="m 0,110 a 520,520 0 0 1 340,0"/>
  <path d="m 170,190 0,-30"/>
  <path d="m 170,140 0,-130"/>
  <path d="m 170,50 165,-33"/>
  <path d="m 155,150 10,-10 10,10 10,-10"/>
  <path d="m 155,160 10,-10 10,10 10,-10"/>
  <path d="m 95,50 30,0"/>
  <path d="m 95,190 30,0"/>
  <path d="m 105,50 0,140" style="stroke-dasharray:8,2;"/>
  <path d="m 100,55 5,-5 5,5"/>
  <path d="m 100,185 5,5 5,-5"/>
  <path d="m 170,25 a 25,25 0 0 1 25,20" style="stroke-dasharray:4,2;"/>
  <path d="m 170,190 70,0"/>
  <path d="m 235,185 5,5 -5,5"/>
  <path d="m 165,125 5,-5 5,5"/>
  <circle cx="170" cy="190" r="2.5"/>
  <circle cx="170" cy="50" r="2.5"/>
  <circle cx="320" cy="20" r="2.5"/>
  <circle cx="270" cy="30" r="2.5"/>
  <text x="155" y="45" class="vector">p</text>
  <text x="265" y="45" class="vector">q</text>
  <text x="320" y="15" class="vector">i</text>
  <text x="175" y="185" class="vector">o</text>
  <text x="90" y="125">r</text>
  <text x="185" y="25">?=cos(?)</text>
  <text x="240" y="185">x</text>
  <text x="155" y="120">z</text>
</svg>

<p>Also, the transmittance between $\bp$ and $\bq$ and between $\bq$ and $\bp$
are the same. Thus, to compute the transmittance between arbitrary points, it
is sufficient to know the transmittance between a point $\bp$ in the atmosphere_p,
and points $\bi$ on the top atmosphere_p boundary. This transmittance depends on
only two parameters, which can be taken as the radius $r=\Vert\bo\bp\Vert$ and
the cosine of the "view zenith angle",
$\mu=\bo\bp\cdot\bp\bi/\Vert\bo\bp\Vert\Vert\bp\bi\Vert$. To compute it, we
first need to compute the length $\Vert\bp\bi\Vert$, and we need to know when
the segment $[\bp,\bi]$ intersects the ground.

<h5>Distance to the top atmosphere_p boundary</h5>

<p>A point at distance $d$ from $\bp$ along $[\bp,\bi)$ has coordinates
$[d\sqrt{1-\mu^2}, r+d\mu]^\top$, whose squared norm is $d^2+2r\mu d+r^2$.
Thus, by definition of $\bi$, we have
$\Vert\bp\bi\Vert^2+2r\mu\Vert\bp\bi\Vert+r^2=r_{\mathrm{top}}^2$,
from which we deduce the length $\Vert\bp\bi\Vert$:
*/

INLINE Length DistanceToTopAtmosphereBoundary(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu) {
  assert(r <= atmosphere_p.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  Area discriminant = r * r * (mu * mu - 1.0) +
      atmosphere_p.top_radius * atmosphere_p.top_radius;
  return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

INLINE Length DistanceToSomeAtmosphereBoundary(Length r, Number mu, Length radius) {
  if (r > radius)
    return 0;
  assert(mu >= -1.0 && mu <= 1.0);
  Area discriminant = r * r * (mu * mu - 1.0) +
      radius * radius;
  return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

/*
<p>We will also need, in the other sections, the distance to the bottom
atmosphere_p boundary, which can be computed in a similar way (this code assumes
that $[\bp,\bi)$ intersects the ground):
*/

INLINE Length DistanceToBottomAtmosphereBoundary(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu) {
  assert(r >= atmosphere_p.bottom_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  Area discriminant = r * r * (mu * mu - 1.0) +
      atmosphere_p.bottom_radius * atmosphere_p.bottom_radius;
  return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

//return -1 if no hit
INLINE Length DistanceToBottomAtmosphereBoundarySigned(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu)
{
  assert(mu >= -1.0 && mu <= 1.0);
  Area discriminant = r * r * (mu * mu - 1.0) +
      atmosphere_p.bottom_radius * atmosphere_p.bottom_radius;
  return mu < 0 && discriminant >= 0 ? (-r * mu - sqrt(discriminant)) : (-1 * meter);
}

/*
<h5>Intersections with the ground</h5>

<p>The segment $[\bp,\bi]$ intersects the ground when
$d^2+2r\mu d+r^2=r_{\mathrm{bottom}}^2$ has a solution with $d \ge 0$. This
requires the discriminant $r^2(\mu^2-1)+r_{\mathrm{bottom}}^2$ to be positive,
from which we deduce the following function:
*/

INLINE bool RayIntersectsGround(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu) {
  assert(r >= atmosphere_p.bottom_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  return mu < 0.0 && r * r * (mu * mu - 1.0) +
      atmosphere_p.bottom_radius * atmosphere_p.bottom_radius >= 0.0 * m2;
}

/*
<h5>Transmittance to the top atmosphere_p boundary</h5>

<p>We can now compute the transmittance between $\bp$ and $\bi$. From its
definition and the
<a href="https://en.wikipedia.org/wiki/Beer-Lambert_law">Beer-Lambert law</a>,
this involves the integral of the number density of air molecules along the
segment $[\bp,\bi]$, as well as the integral of the number density of aerosols
and the integral of the number density of air molecules that absorb light
(e.g. ozone) - along the same segment. These 3 integrals have the same form and,
when the segment $[\bp,\bi]$ does not intersect the ground, they can be computed
numerically with the help of the following auxilliary function (using the <a
href="https://en.wikipedia.org/wiki/Trapezoidal_rule">trapezoidal rule</a>):
*/

INLINE Number GetMieDensity(IN(AtmosphereParameters) atmosphere_p, Length altitude) {
  //return saturate(exp(atmosphere_p.mie_density_altitude_exp_term * altitude));//todo: exp2
  return saturate(exp(atmosphere_p.mie_density_altitude_exp_term * altitude))
        +atmosphere_p.mie2_strength * saturate(exp(atmosphere_p.mie2_density_altitude_exp_term * max(0., altitude-atmosphere_p.mie2_altitude)));//todo: exp2
}

INLINE Number GetRayDensity(IN(AtmosphereParameters) atmosphere_p, Length altitude) {
  return saturate(exp(atmosphere_p.rayleigh_density_altitude_exp_term * altitude));//todo exp2
}

INLINE Number GetAbsDensity(IN(AtmosphereParameters) atmosphere_p, Length altitude) {
  return saturate(altitude < atmosphere_p.absorption_density_max_alt ?
    atmosphere_p.absorption_density_linear_term0.x * altitude + atmosphere_p.absorption_density_linear_term0.y:
    atmosphere_p.absorption_density_linear_term1.x * altitude + atmosphere_p.absorption_density_linear_term1.y);
}

struct MediumSampleRGB
{
  DimensionlessSpectrum scattering;
  DimensionlessSpectrum extinction;

  DimensionlessSpectrum scatteringMie;
  DimensionlessSpectrum scatteringRay;
};

INLINE MediumSampleRGB SampleMediumFull(IN(AtmosphereParameters) atmosphere_p, Length altitude, IN(Position) worldPos)//worldPos is for other volumetrics
{
  G_UNUSED(worldPos);
  Number densityMie = GetMieDensity(atmosphere_p, altitude);
  Number densityRay = GetRayDensity(atmosphere_p, altitude);
  Number densityAbs = GetAbsDensity(atmosphere_p, altitude);

  MediumSampleRGB s;

  s.scatteringMie = densityMie * atmosphere_p.mie_scattering;
  s.scatteringRay = densityRay * atmosphere_p.rayleigh_scattering;
  s.extinction = densityMie * atmosphere_p.mie_extinction + s.scatteringRay + densityAbs * atmosphere_p.absorption_extinction;
  DimensionlessSpectrum msScattering = DimensionlessSpectrum(0,0,0);
  #if CUSTOM_SKIES_FOG
  getSkiesCustomFog(s.scatteringMie, s.scatteringRay, msScattering, s.extinction, altitude, worldPos);//todo: add fixed phase fog
  #endif
  s.scattering = s.scatteringMie + s.scatteringRay + msScattering;
  return s;
}

INLINE void SampleMedium(IN(AtmosphereParameters) atmosphere_p, Length altitude, IN(Position) worldPos,
                    OUT(DimensionlessSpectrum) scattering, OUT(DimensionlessSpectrum) extinction)
{
  MediumSampleRGB medium = SampleMediumFull(atmosphere_p, altitude, worldPos);
  scattering = medium.scattering;
  extinction = medium.extinction;
}
/*
<p>With this function the transmittance between $\bp$ and $\bi$ is now easy to
compute (we continue to assume that the segment does not intersect the ground):
*/

INLINE DimensionlessSpectrum ComputeTransmittanceToTopAtmosphereBoundary(
    IN(AtmosphereParameters) atmosphere_p, Length r, Number mu, Length maxDist, int SAMPLE_COUNT,
    IN(Position) worldPos, IN(Direction) worldDir//only for custom fog
    )
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);

  Length dist = min(maxDist, DistanceToTopAtmosphereBoundary(atmosphere_p, r, mu));
  Length dx = dist / Number(SAMPLE_COUNT);
  // Integration loop.
  //generic loop, works for any medium
  DimensionlessSpectrum sampleScattering, sampleExtinction;
  SampleMedium(atmosphere_p, r - atmosphere_p.bottom_radius, worldPos, sampleScattering, sampleExtinction);
  DimensionlessSpectrum extinction = sampleExtinction*0.5;
  for (int i = 1; i < SAMPLE_COUNT; ++i)
  {
    Length d_i = Number(i) * dx;
    // Distance between the current sample point and the planet center.
    Length r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
    SampleMedium(atmosphere_p, r_i - atmosphere_p.bottom_radius, worldPos + worldDir*d_i, sampleScattering, sampleExtinction);
    extinction += sampleExtinction;
  }
  SampleMedium(atmosphere_p, sqrt(dist * dist + 2.0 * r * mu * dist + r * r) - atmosphere_p.bottom_radius, worldPos + worldDir*dist,
    sampleScattering, sampleExtinction);
  extinction += sampleExtinction*0.5;
  return exp(-dx*extinction);
}

/*
<h4 id="transmittance_precomputation">Precomputation</h4>

<p>The above function is quite costly to evaluate, and a lot of evaluations are
needed to compute single and multiple scattering. Fortunately this function
depends on only two parameters and is quite smooth, so we can precompute it in a
small 2D texture to optimize its evaluation.

<p>For this we need a mapping between the function parameters $(r,\mu)$ and the
texture coordinates $(u,v)$, and vice-versa, because these parameters do not
have the same units and range of values. And even if it was the case, storing a
function $f$ from the $[0,1]$ interval in a texture of size $n$ would sample the
function at $0.5/n$, $1.5/n$, ... $(n-0.5)/n$, because texture samples are at
the center of texels. Therefore, this texture would only give us extrapolated
function values at the domain boundaries ($0$ and $1$). To avoid this we need
to store $f(0)$ at the center of texel 0 and $f(1)$ at the center of texel
$n-1$. This can be done with the following mapping from values $x$ in $[0,1]$ to
texture coordinates $u$ in $[0.5/n,1-0.5/n]$ - and its inverse:
*/
#endif