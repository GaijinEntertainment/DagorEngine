#ifndef THREAD_GROUP_SWIZZLE
#define THREAD_GROUP_SWIZZLE

// This function reorders thread groups execution so that go col by col of width col_width.
// This helps to maximies cache hit rate for texture sampling heavy compute dispatches
// especially if sampling is spatially stochastic.
// https://developer.nvidia.com/blog/optimizing-compute-shaders-for-l2-locality-using-thread-group-id-swizzling/
// dispatch_size is in groups (actual API call).
// col_width is in groupd.
// returns swizzled dispatch thread id
uint2 thread_group_swizzle_x(
  const uint2 dispatch_size,
  const uint2 thread_group_size,
  const uint  col_width,
  const uint2 group_thread_id,
  const uint2 group_id)
{
  const uint perfectColThreadGroupCount = col_width * dispatch_size.y;
  const uint perfectColCount = dispatch_size.x / col_width;
  const uint threadGroupsInPerfectCols = perfectColCount * perfectColThreadGroupCount;
  const uint flatGroupId = dispatch_size.x * group_id.y + group_id.x;

  const uint colId     = flatGroupId / perfectColThreadGroupCount;
  const uint localThreadGroupId = flatGroupId % perfectColThreadGroupCount;

  uint localY, localX;
  if (threadGroupsInPerfectCols <= flatGroupId)
  {
    const uint lastColWidth = dispatch_size.x - perfectColCount * col_width;
    localY = localThreadGroupId / lastColWidth;
    localX = localThreadGroupId % lastColWidth;
  }
  else
  {
    localY = localThreadGroupId / col_width;
    localX = localThreadGroupId % col_width;
  }

  const uint swizzledFlat = colId * col_width + localY * dispatch_size.x + localX;
  uint2 swizzledGroupId;
  swizzledGroupId.y = swizzledFlat / dispatch_size.x;
  swizzledGroupId.x = swizzledFlat % dispatch_size.x;

  return thread_group_size * swizzledGroupId + group_thread_id;
}
#endif // THREAD_GROUP_SWIZZLE
