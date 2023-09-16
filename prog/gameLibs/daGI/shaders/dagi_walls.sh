int current_walls_count;
float4 wallsGridLT = (-10000,-10000,-10000,1);
float4 wallsGridCellSize = (1,1,1,1);
buffer wallsList;
buffer wallsGridList;
buffer PlanesCbuffer;

macro USE_WALLS_BASE(code)
  (code) {
    current_walls_count@i1 = (current_walls_count);
    wallsGridLT@f4 = wallsGridLT;
    wallsGridLT_inv@f3 = (-wallsGridLT.x/wallsGridCellSize.x, -wallsGridLT.y/wallsGridCellSize.y, -wallsGridLT.z/wallsGridCellSize.x, 0.);
    wallsGridCellSize@f2 = (wallsGridCellSize.x, wallsGridCellSize.y, 0., 0.);
    wallsGridCellSize_inv@f2 = (1.0/wallsGridCellSize.x, 1.0/wallsGridCellSize.y, 0., 0.);
    wallsList@buf = wallsList hlsl {StructuredBuffer<uint> wallsList@buf;};
    PlanesCbuffer@cbuf = PlanesCbuffer hlsl {
      cbuffer PlanesCbuffer@cbuf
      {
        float4 planeList[4096];
      };
    }
  }
endmacro


macro USE_WALLS(code)
  USE_WALLS_BASE(code)
  (code) {
    wallsGridList@buf = wallsGridList hlsl {
      #include <gi_walls.hlsli>
      StructuredBuffer<WallGridType> wallsGridList@buf;
    }
  }

  hlsl(code)
  {
    #if 0
    float raycast_walls_nocull(float3 wpos, float3 wdir)
    {
      float finalRange = 1e6;
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float2 range = float2(-1e6,1e6);
        for (uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24); pi < pie; ++pi)
        {
          float4 plane = planeList[pi];
          float denom = dot(wdir, plane.xyz);
          float dist = dot(wpos, plane.xyz) + plane.w;
          float t = -dist/denom;
          if (denom<0)
            range.x = max(range.x, t);
          else
            range.y = min(range.y, t);
        }
        if (range.x > range.y || range.y<0)
          continue;//discard;
        //if (range.x<0)//inside, intersection is in range.y
        finalRange = min(finalRange, (range.x<0) ? range.y : range.x);
      }
      return finalRange;
    }

    float raycast_walls_from_outside(float3 wpos, float3 wdir)
    {
      float finalRange = 1e6;
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float2 range = float2(-1e6,1e6);
        for (uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24); pi < pie; ++pi)
        {
          float4 plane = planeList[pi];
          float denom = dot(wdir, plane.xyz);
          if (abs(denom)<1e-6f)
            continue;
          float dist = dot(wpos, plane.xyz) + plane.w;
          float t = -dist/denom;
          if (denom<0)
            range.x = max(range.x, t);
          else
            range.y = min(range.y, t);
        }
        if (range.x < range.y && range.y>=0 && range.x > 0)
          finalRange = min(finalRange, range.x);
      }
      return finalRange;
    }

    float raycast_walls_from_inside(float3 wpos, float3 wdir)
    {
      float finalRange = -1e6;
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float2 range = float2(-1e6,1e6);
        for (uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24); pi < pie; ++pi)
        {
          float4 plane = planeList[pi];
          float denom = dot(wdir, plane.xyz);
          float dist = dot(wpos, plane.xyz) + plane.w;
          float t = -dist/denom;
          if (denom<0)
            range.x = max(range.x, t);
          else
            range.y = min(range.y, t);
        }
        if (range.x < range.y && range.y>=0 && range.x < 0)
          finalRange = max(finalRange, range.y);
      }
      return finalRange;
    }

    float raycast_walls_from_inside_shrinked(float3 wpos, float3 wdir, float dist_shrink)
    {
      float finalRange = -1e6;
      //for (uint wi = 1, wie = wallsList[0]+1; wi < wie; ++wi)
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float2 range = float2(-1e6,1e6);
        #define PLANE(ii)\
        {\
          float4 plane = planeList[ii];\
          float denom = dot(wdir, plane.xyz);\
          float dist = dot(wpos, plane.xyz) + plane.w + dist_shrink;\
          float t = -dist/denom;\
          if (denom<0)\
            range.x = max(range.x, t);\
          else\
            range.y = min(range.y, t);\
        }
        uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24);
        //for (;pi<pie;pi+=6)
        {PLANE(pi+0);PLANE(pi+1);PLANE(pi+2);PLANE(pi+3);PLANE(pi+4);PLANE(pi+5);}
        //for (uint pi = convex&((1<<24)-1), pie = pi + (convex>>24); pi < pie; ++pi)
        //  PLANE(pi);
        if (range.x < range.y && range.y>=0 && range.x < 0)
          finalRange = max(finalRange, range.y);
        #undef PLANE
      }
      return finalRange;
    }
    float2 raycast_walls(float3 wpos, float3 wdir)//from inside, from outside
    {
      float2 finalRange = float2(-1e6, 1e6);
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float2 range = float2(-1e6,1e6);
        for (uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24); pi < pie; ++pi)
        {
          float4 plane = planeList[pi];
          float denom = dot(wdir, plane.xyz);
          float dist = dot(wpos, plane.xyz) + plane.w;
          float t = -dist/denom;
          if (denom<0)
            range.x = max(range.x, t);
          else
            range.y = min(range.y, t);
        }
        FLATTEN
        if (range.x <= range.y && range.y>=0)
        {
          if (range.x < 0)
            finalRange.x = max(finalRange.x, range.y);
          else
            finalRange.y = min(finalRange.y, range.x);
        }
      }
      return finalRange;
    }
    #endif
    bool ssgi_pre_filter_worldPos(inout float3 worldPos, float maxEffect)
    {
      float3 cellSizeInv = float3(wallsGridCellSize_inv.x, wallsGridCellSize_inv.y, wallsGridCellSize_inv.x);
      int3 gridPos = worldPos * cellSizeInv + wallsGridLT_inv.xyz;
      uint3 gridPosI = uint3(gridPos);
      if (any(gridPosI >= uint3(WALLS_GRID_XZ, WALLS_GRID_Y, WALLS_GRID_XZ)))
        return false;
      //uint2 wallConvex = wallsGridList[gridPosI.y + gridPosI.x*WALLS_GRID_Y + gridPosI.z*WALLS_GRID_Y*WALLS_GRID_XZ];
      WallGridType wallConvex = structuredBufferAt(wallsGridList, gridPosI.x + gridPosI.y*WALLS_GRID_XZ + gridPosI.z*WALLS_GRID_Y*WALLS_GRID_XZ);
      #define MIN_DIST -0.04
      bool ret = false;
      UNROLL
      for (uint wi = 0; wi < MAX_WALLS_CELL_COUNT; ++wi)
      {
        uint convex = wallConvex[wi];
        BRANCH
        if (!convex)
          return ret;
        uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24);
        //uint pi = (wallConvex.y&0xFFFF);
        float3 convexWorldOffset = 0;
        float maxDist = -1e6;
        float totalW = 1;
        #define PLANE(ii)\
        {\
          float4 plane = planeList[ii];\
          float worldDist = dot(worldPos, plane.xyz) + plane.w;\
          float w = ((saturate(worldDist*(1./MIN_DIST)))); totalW*=w;\
          convexWorldOffset -= plane.xyz*(saturate(maxEffect+worldDist));\
          maxDist = max(maxDist, worldDist);\
        }
        //for (uint group = 0, ge = current_walls_count/2; group < ge; ++group, pi+=6)
        //todo: support other convexes (not boxes) with more planes
        //do
        {
          PLANE(pi+0); PLANE(pi+1); PLANE(pi+2); PLANE(pi+3); PLANE(pi+4); PLANE(pi+5);
          pi+=6;
        };// while(pi<pie && convexBestPlanes.planes[0].w<0);
        #undef PLANE

        //if (maxDist<-maxEffect)//we are completely inside some other convex, no need to filter
        //  return;
        if (maxDist < 0)//we are inside convex
        {
          worldPos += convexWorldOffset*totalW;
          ret = true;
        }
        #undef MIN_DIST
      }
      return ret;
    }
    void ssgi_pre_filter_worldPos_all(inout float3 worldPos, float maxEffect)
    {
      #define MIN_DIST -0.04
      for (uint wi = 0, pi = 0; wi < current_walls_count; ++wi)
      {
        uint convex = structuredBufferAt(wallsList, wi);
        float3 convexWorldOffset = 0;
        float maxDist = -1e6;
        float totalW = 1;
        #define PLANE(ii)\
        {\
          float4 plane = planeList[ii];\
          float worldDist = dot(worldPos, plane.xyz) + plane.w;\
          float w = ((saturate(worldDist*(1./MIN_DIST)))); totalW*=w;\
          convexWorldOffset -= plane.xyz*(saturate(maxEffect+worldDist));\
          maxDist = max(maxDist, worldDist);\
        }
        //uint pi = wi*6-6;
        //pi = convex&((1<<24)-1);
        //uint pi = (wi-1)*6;
        //for (uint group = 0, ge = current_walls_count/2; group < ge; ++group, pi+=6)
        //todo: support other convexes (not boxes) with more planes
        {
          PLANE(pi+0); PLANE(pi+1); PLANE(pi+2); PLANE(pi+3); PLANE(pi+4); PLANE(pi+5);
          pi+=6;
        }
        #undef PLANE

        if (maxDist<-maxEffect)//we are completely inside some other convex, no need to filter
          return;
        //resultWorldOffset += maxDist < 0 ? convexWorldOffset : 0;
        if (maxDist < 0)
          worldPos += convexWorldOffset*totalW;
        #undef MIN_DIST
      }
    }
  }
endmacro

macro CALC_CLOSEST_PLANES()
  USE_WALLS(cs)
  ENABLE_ASSERT(cs)
  hlsl(cs) {
    #define SELECT_BEST(bestPlanes, plane)\
    {\
      FLATTEN\
      if (bestPlanes[0].w<plane.w)\
      {\
        bestPlanes[2] = bestPlanes[1];\
        bestPlanes[1] = bestPlanes[0];\
        bestPlanes[0] = plane;\
      }\
      else if (bestPlanes[1].w<plane.w)\
      {\
        bestPlanes[2] = bestPlanes[1];\
        bestPlanes[1] = plane;\
      }\
      else if (bestPlanes[2].w<plane.w)\
      {\
        bestPlanes[2] = plane;\
      }\
    }
    //todo: check for co-planar planes, replace them first
    #define SELECT_BEST_RESULT(bestPlanes, plane) SELECT_BEST(bestPlanes, plane)
    #define PLANE(ii)\
    {\
      float4 plane = planeList[ii];plane.w=plane.w+dot(wpos, plane.xyz);\
      SELECT_BEST(convexBestPlanes.planes, plane)\
    }

    void select_closest_walls_from_inside_all(float3 wpos, out AmbientVoxelsPlanes resultPlanes, float shrink = 0.05)
    {
      resultPlanes.planes[0] = resultPlanes.planes[1] = resultPlanes.planes[2] = -1e6;
      for (uint wi = 0; wi < current_walls_count; ++wi)
      {
        AmbientVoxelsPlanes convexBestPlanes;// = (AmbientVoxelsPlanes)(-1e-6);
        convexBestPlanes.planes[0] = convexBestPlanes.planes[1] = convexBestPlanes.planes[2] = -1e6;
        uint convex = structuredBufferAt(wallsList, wi);
        uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24);
        do
        {
          PLANE(pi+0);PLANE(pi+1);PLANE(pi+2);PLANE(pi+3);PLANE(pi+4);PLANE(pi+5);
          pi+=6;
        } while(pi<pie && convexBestPlanes.planes[0].w<0);
        if (convexBestPlanes.planes[0].w<0)
        {
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[0])
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[1])
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[2])
        }
      }
      resultPlanes.planes[0] = resultPlanes.planes[0].w > -1e6 ? float4(resultPlanes.planes[0].xyz, -1/(min(resultPlanes.planes[0].w + shrink, -0.001))) : float4(0,0,0,0);
      resultPlanes.planes[1] = resultPlanes.planes[1].w > -1e6 ? float4(resultPlanes.planes[1].xyz, -1/(min(resultPlanes.planes[1].w + shrink, -0.001))) : float4(0,0,0,0);
      resultPlanes.planes[2] = resultPlanes.planes[2].w > -1e6 ? float4(resultPlanes.planes[2].xyz, -1/(min(resultPlanes.planes[2].w + shrink, -0.001))) : float4(0,0,0,0);
    }

    void select_closest_walls_from_inside_grid(float3 wpos, out AmbientVoxelsPlanes resultPlanes, float shrink = 0.05)
    {
      float3 cellSizeInv = float3(wallsGridCellSize_inv.x, wallsGridCellSize_inv.y, wallsGridCellSize_inv.x);
      resultPlanes.planes[0] = resultPlanes.planes[1] = resultPlanes.planes[2] = -1e6;
      int3 gridPos = wpos * cellSizeInv + wallsGridLT_inv.xyz;
      if (any(gridPos < 0 || gridPos >= int3(WALLS_GRID_XZ, WALLS_GRID_Y, WALLS_GRID_XZ)))
        return;
      uint3 gridPosI = uint3(gridPos);
      WallGridType wallConvex = wallsGridList[gridPosI.x + gridPosI.y*WALLS_GRID_XZ + gridPosI.z*WALLS_GRID_Y*WALLS_GRID_XZ];
      UNROLL
      for (uint wi = 0; wi < MAX_WALLS_CELL_COUNT; ++wi)
      {
        uint convex = wallConvex[wi];
        BRANCH
        if (!convex)
          break;
        AmbientVoxelsPlanes convexBestPlanes;// = (AmbientVoxelsPlanes)(-1e-6);
        convexBestPlanes.planes[0] = convexBestPlanes.planes[1] = convexBestPlanes.planes[2] = -1e6;
        uint pi = convex&((1U<<24)-1), pie = pi + (convex>>24);
        do
        {
          PLANE(pi+0);PLANE(pi+1);PLANE(pi+2);PLANE(pi+3);PLANE(pi+4);PLANE(pi+5);
          pi+=6;
        } while(pi<pie && convexBestPlanes.planes[0].w<0);
        if (convexBestPlanes.planes[0].w<0)
        {
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[0])
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[1])
          SELECT_BEST_RESULT(resultPlanes.planes, convexBestPlanes.planes[2])
        }
      }
      resultPlanes.planes[0] = resultPlanes.planes[0].w > -1e6 ? float4(resultPlanes.planes[0].xyz, -1/(min(resultPlanes.planes[0].w + shrink, -0.001))) : float4(0,0,0,0);
      resultPlanes.planes[1] = resultPlanes.planes[1].w > -1e6 ? float4(resultPlanes.planes[1].xyz, -1/(min(resultPlanes.planes[1].w + shrink, -0.001))) : float4(0,0,0,0);
      resultPlanes.planes[2] = resultPlanes.planes[2].w > -1e6 ? float4(resultPlanes.planes[2].xyz, -1/(min(resultPlanes.planes[2].w + shrink, -0.001))) : float4(0,0,0,0);
    }
    #undef PLANE
    #undef SELECT_BEST
  }
endmacro

macro USE_CLOSEST_PLANES()
hlsl(cs) {
  float calc_max_start_dist(float3 wdir, AmbientVoxelsPlanes planes)
  {
    float maxStartDist = -1e6;
    #define PLANE(ii)\
    {\
      float4 plane = planes.planes[ii];\
      float denom = dot(wdir, plane.xyz);\
      /*float dist = dot(worldPos, plane.xyz) + plane.w;float t = -dist/denom; replaced due to replacing plane.w with 1/(shrinked_dist)*/\
      float t = plane.w*denom;\
      FLATTEN\
      if (denom>0)\
        maxStartDist = max(maxStartDist, t);\
    }
    PLANE(0);PLANE(1);PLANE(2);
    #undef PLANE
    return (maxStartDist>0) ? rcp(maxStartDist) : 1e6;
  }
}
endmacro

