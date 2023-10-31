let PI = ::PI
let cos = ::cos
let makeColor = ::makeColor
let mulColors = ::mulColors
let mulColors2x = ::mulColors2x
let sampleImage = ::sampleImage
let blendColors = ::blendColors

let degToRad = @(a) a*PI/180

::getParamsList <- function()
{
  local list=
  [
    {name= "fullGrassAngle", type="real", value=30},
    {name="startGrassAngle", type="real", value=45},
    {name="maxGrassHeight", type="real", value=50},
    {name="grassFadeDist", type="real", value=20},

    {name="sandCurvature", type="real", value=1},

    {name= "fullSnowAngle", type="real", value=30},
    {name="startSnowAngle", type="real", value=45},
    {name="minSnowHeight", type="real", value=150},
    {name="snowFadeDist", type="real", value=50},

    {name="grassTexture", type="image", value="grass"},
    {name="grassGrad", type="image", value="grassGrad"},
    {name="dissolveMap", type="image", value="noise1"},

    {name="dirtTexture", type="image", value="dirtMap"},
    {name="dirtGradient", type="image", value="dirtGrad"},

    {name="sandTexture", type="image", value="sand"},

    {name="snowTexture", type="image", value="snow"},
  ];
  return list;
}


::getGenerator <- function()
{
  local maxGrassHeight = ::maxGrassHeight
  local dirtTexture = ::dirtTexture
  local dirtGradient = ::dirtGradient
  local sandTexture = ::sandTexture
  local grassTexture = ::grassTexture
  local dissolveMap = ::dissolveMap
  local grassGrad = ::grassGrad
  local snowTexture = ::snowTexture

  local  fullGrassCos=cos(degToRad(::fullGrassAngle));
  local startGrassCos=cos(degToRad(::startGrassAngle));
  local minGrassHeight=maxGrassHeight-::grassFadeDist;

  local sandCurv=::sandCurvature/100.0;

  local  fullSnowCos=cos(degToRad( fullSnowAngle));
  local startSnowCos=cos(degToRad(startSnowAngle));
  local maxSnowHeight=minSnowHeight+snowFadeDist;

  local sampleImageAt = ::sampleImageAt
  local sampleMask = ::sampleMask
  local sampleMask1 = ::sampleMask1
  local sampleMask8 = ::sampleMask8
  local getHeight = ::getHeight
  local getNormalY = ::getNormalY
  local getCurvature = ::getCurvature
  local blendColors = ::blendColors
  local dissolveOver = ::dissolveOver
  local calcBlendK = ::calcBlendK
  local smoothStep = ::smoothStep
  local noiseStrength = ::noiseStrength

  return function()
  {
    local normalY=getNormalY();
    local height=getHeight();
    local curvature=getCurvature();

    // base 'dirt' / 'stone'
    local res=mulColors2x(sampleImage(dirtTexture), sampleImage(dirtGradient));

    // add 'sand' by curvature
    res=blendColors(res, sampleImage(sandTexture),
      calcBlendK(0.0, sandCurv, curvature) );

    // calc coordinate for grass color gradient (by angle)
    local grassG=calcBlendK(startGrassCos, 1.0, normalY);

    // add grass by angle and fade by height
    res=dissolveOver(res,
      mulColors2x(sampleImage(grassTexture), sampleImageAt(grassGrad, grassG, grassG)),
      smoothStep( calcBlendK(startGrassCos, fullGrassCos, normalY) )
      * smoothStep( calcBlendK(maxGrassHeight, minGrassHeight, height) ),
      sampleMask(dissolveMap));

    // add snow by angle and fade by height
    res=dissolveOver(res, sampleImage(snowTexture),
      smoothStep( calcBlendK(startSnowCos, fullSnowCos, normalY) )
      * smoothStep( calcBlendK(minSnowHeight, maxSnowHeight, height) ),
      sampleMask(dissolveMap));

    return res;
  }
}
