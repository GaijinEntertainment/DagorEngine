#ifndef ROUGHNESS_DETAIL_INCLUDED
#define ROUGHNESS_DETAIL_INCLUDED 1

//detail function for two symmetrical roughness, found in Mathematica.
//inspired by https://www.activision.com/cdn/research/siggraph_2018_opt.pdf
//it ALWAYS make things rougher (like it should be)
//Beware! it was used not original LUT table, but rather RationFit found by Chan in https://www.activision.com/cdn/research/siggraph_2018_opt.pdf
// so we add errors over errors!
// todo: calculate original source LUT and only then FindFit
//Full Mathematica code:
//PF1[a_, b_] := -0.53558 + 1.002204*(a + b) - 0.22391*(a^2 + b^2) + 13.32315*a*b
//QF1[a_, b_] := 1.0 + 8.259559*(a + b) + 9.896132*(a^2 + b^2 ) \[Minus] 22.015902*a*b
//PQFOriginal[a_, b_] :=PF1[a,b]/QF1[a,b]
//roughnessToGloss[x_] := (1./18)*Log2[2/Max[0.00001, x^4]-1]
//glossToRoughness[x_] :=  (2/(1 + 2^(18*x)))^0.25
//smoothnessToGloss[x_] := roughnessToGloss[(1-x)]
//glossToSmoothness[x_] := 1-glossToRoughness[x]
//PQRough[a_, b_] :=glossToRoughness[PQFOriginal[roughnessToGloss[a],roughnessToGloss[b]]]
//PQSmooth[a_, b_] :=glossToSmoothness[PQFOriginal[smoothnessToGloss[a],smoothnessToGloss[b]]]
//modelPQ = (p0 +p1*(a+b)+p2*(a^2+b^2) + p3*a*b)/(1 +q1*(a+b)+q2*(a^2+b^2) + q3*a*b)
//modelQP = (1 +q1*(a+b)+q2*(a^2+b^2) + q3*a*b)/(p0 +p1*(a+b)+p2*(a^2+b^2) + p3*a*b)
//tableDataRough = Flatten[Table[{x,y, PQRough[x,y]}, {x, 0,1, 0.03125}, {y,0,1,0.03125}], 1]
//tableDataSmooth = Flatten[Table[{x,y, PQSmooth[x,y]}, {x, 0,1, 0.03125}, {y,0,1,0.03125}], 1]
//fitRough = FindFit[tableDataRough,modelQP,{p0,p1,p2,p3,q1,q2,q3},{a,b},MaxIterations->10000]
//fitSmooth = FindFit[tableDataSmooth,modelPQ,{p0,p1,p2,p3,q1,q2,q3},{a,b},MaxIterations->10000]


//returns always more than 0, minunum is 0.0627493
//saturate(val-0.0627492) if you want to get 0 in 0.0
float roughnessDetail(float linearRoughness1, float linearRoughness2)
{
  //modelQP = (1 +q1*(a+b)+q2*(a^2+b^2) + q3*a*b)/(p0 +p1*(a+b)+p2*(a^2+b^2) + p3*a*b)
  //{p0->15.9364,p1->-10.2163,p2->13.5138,p3->-19.1339,q1->6.39745,q2->11.1071,q3->-31.9622}
  float apb = linearRoughness1+linearRoughness2,
      a2pb2 = linearRoughness1*linearRoughness1 + linearRoughness2*linearRoughness2,
         ab = linearRoughness1*linearRoughness2;
  return saturate((1 + 6.39745*apb + -11.1071*a2pb2 + -31.9622*ab) / (15.9364 + -10.2163*apb + 13.5138*a2pb2 + -19.1339*ab));
}

//returns always less than 1, maximum is 0.937251
//divide by 0.937251 or add 0.062749 if you want to get 1 (and saturate after that)
float invRoughnessDetail(float linearSmoothness1, float linearSmoothness2)//our smoothness is invRoughness
{
  //modelPQ = (p0 +p1*(a+b)+p2*(a^2+b^2) + p3*a*b)/(1 +q1*(a+b)+q2*(a^2+b^2) + q3*a*b)
  //{p0->-0.191062,p1->-0.302614,p2->0.708371,p3->3.77566,q1->0.683553,q2->3.97742,q3->-5.63151}
  float apb = linearSmoothness1+linearSmoothness2,
      a2pb2 = linearRoughness1*linearRoughness1 + linearRoughness2*linearRoughness2,
         ab = linearRoughness1*linearRoughness2;
  return saturate((-0.191062 + -0.302614*apb + 0.708371*a2pb2 + 3.77566*ab)/(1+0.683553*apb + 3.97742*a2pb2 + -5.63151*ab));
}
#endif