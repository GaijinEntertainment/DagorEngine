#ifndef L2_CACHE_FRIENDLY_DISPATCH
#define L2_CACHE_FRIENDLY_DISPATCH 1
//https://developer.nvidia.com/blog/optimizing-compute-shaders-for-l2-locality-using-thread-group-id-swizzling/

//Divide the 2d-Dispatch_Grid into tiles of dimension [N, Dispatch_Grid_Dim.y]
//“CTA” (Cooperative Thread Array) == Thread Group in DirectX terminology
void dispatch_cache_friendly_pattern(
  uint2 Dispatch_Grid_Dim,//uint2(320, 180); //Get this from the C++ side.
  uint Number_of_CTAs_to_launch_in_x_dim,////Launch 16 CTAs in x-dimension
  uint2 groupId,//known in HLSL as GroupID
  out uint2 SwizzledvGroupID//dispatchThreadId is (NumThreadX.x)*SwizzledvGroupID.x + groupThreadIndex.x, (NumThreadY)*SwizzledvThreadGroupID.y + groupThreadIndex.y
  ,out uint Swizzled_vThreadGroupIDFlattened
)
{
  // A perfect tile is one with dimensions = [Number_of_CTAs_to_launch_in_x_dim, Dispatch_Grid_Dim.y]
  uint Number_of_CTAs_in_a_perfect_tile = Number_of_CTAs_to_launch_in_x_dim * (Dispatch_Grid_Dim.y);
  //Possible number of perfect tiles
  uint Number_of_perfect_tiles = (Dispatch_Grid_Dim.x)/Number_of_CTAs_to_launch_in_x_dim;
  //Total number of CTAs present in the perfect tiles
  uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * Number_of_CTAs_to_launch_in_x_dim * Dispatch_Grid_Dim.y - 1;
  uint vThreadGroupIDFlattened = (Dispatch_Grid_Dim.x)*groupId.y + groupId.x;
  //Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
  uint Tile_ID_of_current_CTA = (vThreadGroupIDFlattened)/Number_of_CTAs_in_a_perfect_tile;
  uint Local_CTA_ID_within_current_tile = (vThreadGroupIDFlattened)%Number_of_CTAs_in_a_perfect_tile;
  uint Local_CTA_ID_y_within_current_tile;
  uint Local_CTA_ID_x_within_current_tile;
  if(Total_CTAs_in_all_perfect_tiles < vThreadGroupIDFlattened && 0)
  {
      //Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched. 
      uint X_dimension_of_last_tile = (Dispatch_Grid_Dim.x)%Number_of_CTAs_to_launch_in_x_dim;
      Local_CTA_ID_y_within_current_tile = (Local_CTA_ID_within_current_tile)/X_dimension_of_last_tile;
      Local_CTA_ID_x_within_current_tile = (Local_CTA_ID_within_current_tile)%X_dimension_of_last_tile;
  }
  else
  {
      Local_CTA_ID_y_within_current_tile = (Local_CTA_ID_within_current_tile)/Number_of_CTAs_to_launch_in_x_dim;
      Local_CTA_ID_x_within_current_tile = (Local_CTA_ID_within_current_tile)%Number_of_CTAs_to_launch_in_x_dim;
  }
  Swizzled_vThreadGroupIDFlattened = Tile_ID_of_current_CTA*Number_of_CTAs_to_launch_in_x_dim + Local_CTA_ID_y_within_current_tile*(Dispatch_Grid_Dim.x) + Local_CTA_ID_x_within_current_tile;
  SwizzledvGroupID.y = Swizzled_vThreadGroupIDFlattened/Dispatch_Grid_Dim.x;
  SwizzledvGroupID.x = Swizzled_vThreadGroupIDFlattened%Dispatch_Grid_Dim.x;
  //uint2 SwizzledvThreadID;
  //SwizzledvThreadID.x = (CTA_Dim.x)*SwizzledvThreadGroupID.x + groupThreadIndex.x;
  //SwizzledvThreadID.y = (CTA_Dim.y)*SwizzledvThreadGroupID.y + groupThreadIndex.y;
}

#endif