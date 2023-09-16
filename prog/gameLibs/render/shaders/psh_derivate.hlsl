#ifndef NORMAL_DERIVATE
#define NORMAL_DERIVATE 1

//http://mmikkelsen3d.blogspot.ru/2011/07/derivative-maps.html
half3 perturb_normal_height( half3 vN, float3 p, float dBs, float dBt)
{
    // get edge vectors of the pixel triangle
    float3 vSigmaS = ddx( p );
    float3 vSigmaT = ddy( p );
 
    // solve the linear system
    float3 vR1 = cross( vSigmaT, vN );
    float3 vR2 = cross( vN, vSigmaS );
    
    float fDet = dot(vSigmaS, vR1);

    float3 vSurfGrad = sign(fDet)*(dBs*vR1 + dBt*vR2);
    return (half3)normalize(abs(fDet)*vN - vSurfGrad);
}
half3 perturb_normal_height2d_grad( float2 vSigmaS, float2 vSigmaT, float dBs, float dBt)
{
    // solve the linear system
    //todo: optimize manually
    float3 vR1 = cross( float3(vSigmaT.x, 0, vSigmaT.y), float3(0,1,0) );

    float3 vR2 = cross( float3(0,1,0), float3(vSigmaS.x, 0, vSigmaS.y));
    
    float fDet = dot(float3(vSigmaS.x,0,vSigmaS.y), vR1);

    float3 vSurfGrad = sign(fDet)*(dBs*vR1 + dBt*vR2);
    return (half3)normalize(abs(fDet)*float3(0,1,0) - vSurfGrad);
}
half3 perturb_normal_height2d( float2 p, float dBs, float dBt)
{
    // get edge vectors of the pixel triangle
    float2 vSigmaS = ddx( p );
    float2 vSigmaT = ddy( p );
 
    // solve the linear system
    //todo: optimize manually
    float3 vR1 = cross( float3(vSigmaT.x, 0, vSigmaT.y), float3(0,1,0) );

    float3 vR2 = cross( float3(0,1,0), float3(vSigmaS.x, 0, vSigmaS.y));
    
    float fDet = dot(float3(vSigmaS.x,0,vSigmaS.y), vR1);

    float3 vSurfGrad = sign(fDet)*(dBs*vR1 + dBt*vR2);
    return (half3)normalize(abs(fDet)*float3(0,1,0) - vSurfGrad);
}
half3 perturb_normal_height( half3 vN, float3 p, half height)
{
    float dBs = ddx(height);
    float dBt = ddy(height);
    return (half3)perturb_normal_height(vN, p, dBs, dBt);
}
half3 perturb_normal_height2d( float2 p, half height)
{
    float dBs = ddx(height);
    float dBt = ddy(height);
    return (half3)perturb_normal_height2d(p, dBs, dBt);
}
/*half3 perturb_normal_height( half3 vN, float3 p, float2 uv, sampler2D bumpTex, float htscale)
{
    float Hll = tex2D(bumpTex, uv).x*htscale;
    float dBs = (tex2D(bumpTex, uv+ddx(uv)).x*htscale - Hll);
    float dBt = (tex2D(bumpTex, uv+ddy(uv)).x*htscale - Hll);
    return perturb_normal_height(vN, p, dBs, dBt);
}*/
//POM:
//http://mmikkelsen3d.blogspot.ru/2012/02/parallaxpoc-mapping-and-no-tangent.html
/*
I thought I would do yet another follow up post regarding thoughts on Parallax/POM mapping without the use of conventional tangent space. A lot of the terms involved are terms we already have when doing bump mapping using either derivative or height maps.


// terms shared between bump and pom
float2 TexDx = ddx(In.texST);
float2 TexDy = ddy(In.texST);
float3 vSigmaX = ddx(surf_pos);
float3 vSigmaY = ddy(surf_pos);
float3 vN = surf_norm;     // normalized
float3 vR1 = cross(vSigmaY, vN);
float3 vR2 = cross(vN, vSigmaX);
float fDet = dot(vSigmaX, vR1);

// specific to Parallax/POM
float3 vV = vView;   // normalized view vector in same space as surf_pos and vN
float2 vProjVscr = (1/fDet) * float2( dot(vR1, vV), dot(vR2, vV) );
float2 vProjVtex = TexDx*vProjVscr.x + TexDy*vProjVscr.y;

The resulting 2D vector vProjVtex is the offset vector in normalized texture space which corresponds to moving along the surface of the object by the plane projected view vector which is exactly what we want for POM. The remaining work is done the usual way.

 The magnitude of vProjVtex (in normalized texture space) will correspond to the magnitude of the projected view vector at the surface. To obtain the third component of the transformed view vector the applied bump_scale must be taken into account. This is done using the following line of code:

float vProjVtexZ = dot(vN, vV) / bump_scale;

 If we consider T the texture coordinate and the surface gradient of T a 2x3 matrix then an alternative way to think of how we obtain vProjVtex is through the use of surface gradients. One per component of the texture coordinate since each of these represent a scalar field.

float2 vProjVtex = mul(SurfGrad(T), vView)


 The first row of SurfGrad(T) is equal to (1/fDet)*(TexDx.x*vR1 + TexDy.x*vR2) and similar for the second row but using the .y components of TexDx and TexDy. In practice it doesn't really simplify the code much unless we need to transform multiple vectors but it's a fun fact :)
*/
#endif
