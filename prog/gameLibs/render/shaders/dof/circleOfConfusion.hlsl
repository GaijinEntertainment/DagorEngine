#ifndef CIRCLE_OF_CONFUSION
#define CIRCLE_OF_CONFUSION 1

//focalLength = 0.5f * sensorHeight / tan(0.5f * fov);

//float A = appertureSize;
//CoCScale = (A * focalLength * focalPlane * (zFar - zNear)) / ((focalPlane - focalLength) * zNear * zFar)
//CoCBias = (A * focalLength * (zNear - focalPlane)) / (focalPlane - focalLength) * zNear)
//return CoCScale*rawDepth+CoCBias;

//float sensorHeight = 0.024;
//coc = ((linearDepth-focalPlane)/linearDepth) * A * (focalLength/(focalPlane-focalLength))/sensorHeight;
//coc = (1-focalPlane/linearDepth) * A * (focalLength/(focalPlane-focalLength))/sensorHeight;

//reference:
//A = (0.5)*focalLength/fStops;
//float coeff = focalLength * focalLength / (_fNumber * (focalPlane - focalLength) * sensorHeight * 2);
//float coeff = focalLength * focalLength / (_fNumber * (max(focalLength+0.001, focalPlane) - focalLength) * sensorHeight * 2);
//float coc = (1 - focalPlane / linearDepth)  * coeff; //(negative - near, positive - far)
//float cocInfinite = (focalLength * focalLength)/(_fNumber *sensorHeight * 2)*(1/linearDepth)
//coc = 0: linearDepth = 1/focalPlane;
//coc = -4: linearDepth = focalPlane/(4/coeff+1)


float ComputeFarCircleOfConfusion(float rawDepth, float4 dof_focus_params)
{
  float vCocFar = saturate(rawDepth * dof_focus_params.z + dof_focus_params.w);
  //farCoC = min(farCoC, 4);//remove
  return vCocFar;
}

float4 ComputeFarCircleOfConfusion4(float4 rawDepth, float4 dof_focus_params)
{
  return saturate(rawDepth * dof_focus_params.zzzz + dof_focus_params.wwww);
}

float ComputeNearCircleOfConfusion(float rawDepth, float4 dof_focus_params)
{
  float vCocNear = saturate(rawDepth * dof_focus_params.x + dof_focus_params.y);
  //nearCoC = min(nearCoC, 4);//remove
  return vCocNear;
}


void ComputeCircleOfConfusion(in float rawDepth, out float nearCoC, out float farCoC, float4 dof_focus_params)
{
  nearCoC = ComputeNearCircleOfConfusion(rawDepth, dof_focus_params);
  farCoC = ComputeFarCircleOfConfusion(rawDepth, dof_focus_params);
}

void Compute4CircleOfConfusion(in float4 rawDepths, out float4 nearCoC, out float4 farCoC, float4 dof_focus_params)
{
  farCoC = saturate(rawDepths * dof_focus_params.z + dof_focus_params.wwww);
  nearCoC = saturate(rawDepths * dof_focus_params.x + dof_focus_params.yyyy);
  //farCoC = min(farCoC, 4);
  //nearCoC = min(nearCoC, 4);
}
#endif