include "shader_global.sh"

int gpu_objects_max_triangles;
buffer gpu_objects_counter;

shader gpu_objects_get_group_count_cs
{
  ENABLE_ASSERT(cs)
  (cs)
  {
    max_triangles@i1 = (gpu_objects_max_triangles);
    counter@buf = gpu_objects_counter hlsl { ByteAddressBuffer counter@buf; }
  }

  hlsl(cs) {
    #include "gpuObjects/gpu_objects_const.hlsli"

    RWByteAddressBuffer group_count : register(u0);

    [numthreads(1, 1, 1)]
    void get_group_count()
    {
      uint triangles_count = min(loadBuffer(counter, TRIANGLES_COUNT * 4), (uint)max_triangles);
      uint groupsCnt = (triangles_count + MAX_GATHERED_TRIANGLES_SQRT - 1) / MAX_GATHERED_TRIANGLES_SQRT;
      storeBuffer(group_count, 0 * 4, groupsCnt);
      storeBuffer(group_count, 1 * 4, 1);
      storeBuffer(group_count, 2 * 4, 1);

      storeBuffer(group_count, 3 * 4, max(groupsCnt, 2) - 1);
      storeBuffer(group_count, 4 * 4, 1);
      storeBuffer(group_count, 5 * 4, 1);
    }
  }
  compile("cs_5_0", "get_group_count");
}

float4 gpu_objects_rendinst_mesh_params;

int gpu_objects_sum_area_step;
interval gpu_objects_sum_area_step : step0 < 1, step1 < 2, step2;

shader gpu_objects_sum_area_cs, gpu_objects_mesh_sum_area_cs
{
  if (shader == gpu_objects_sum_area_cs)
  {
    (cs)
    {
      max_triangles@i1 = (gpu_objects_max_triangles);
      counter@buf = gpu_objects_counter hlsl { ByteAddressBuffer counter@buf; }
    }
  }
  if (shader == gpu_objects_mesh_sum_area_cs)
  {
    (cs)
    {
      mesh_params@f4 = gpu_objects_rendinst_mesh_params;
    }
  }
  ENABLE_ASSERT(cs)

  hlsl(cs) {
    #include "gpuObjects/gpu_objects_const.hlsli"

    ##if shader == gpu_objects_sum_area_cs
      RWStructuredBuffer<GeometryTriangle> triangles : register(u0);
      #define AREA(face_id) structuredBufferAt(triangles, face_id).areaDoubled
    ##elif shader == gpu_objects_mesh_sum_area_cs
      RWStructuredBuffer<float> areas: register(u0);
      #define AREA(face_id) structuredBufferAt(areas, face_id)
    ##endif

    groupshared float temp[MAX_GATHERED_TRIANGLES_SQRT];

    [numthreads(MAX_GATHERED_TRIANGLES_SQRT / 2, 1, 1)]
    void sum_area(uint thread_id : SV_GroupThreadID, uint dispatch_thread_id : SV_DispatchThreadID)
    {
      const uint n = MAX_GATHERED_TRIANGLES_SQRT;
##if shader == gpu_objects_sum_area_cs
      uint triangles_count = min(loadBuffer(counter, TRIANGLES_COUNT * 4), (uint)max_triangles);
##elif shader == gpu_objects_mesh_sum_area_cs
      uint triangles_count = mesh_params.y;
##endif

##if gpu_objects_sum_area_step == step0
      uint triangleId1 = dispatch_thread_id * 2;
      uint triangleId2 = triangleId1 + 1;
##elif gpu_objects_sum_area_step == step1
      uint triangleId1 = dispatch_thread_id * 2 * n - 1;
      uint triangleId2 = triangleId1 + n;
##endif

##if gpu_objects_sum_area_step < step2
      BRANCH if (triangleId1 < triangles_count)
        temp[thread_id * 2] = AREA(triangleId1);
      else
        temp[thread_id * 2] = 0;

      BRANCH if (triangleId2 < triangles_count)
        temp[thread_id * 2 + 1] = AREA(triangleId2);
      else
        temp[thread_id * 2 + 1] = 0;

      uint offset = 0;
      uint d;
      UNROLL
      for (d = n >> 1; d > 0; d >>= 1)
      {
        GroupMemoryBarrierWithGroupSync();
        if (thread_id < d)
        {
          uint ai = ((2 * thread_id + 1) << offset) - 1;
          uint bi = ((2 * thread_id + 2) << offset) - 1;
          temp[bi] += temp[ai];
        }
        offset += 1;
      }

      if (thread_id == 0)
        temp[n - 1] = 0;

      UNROLL
      for (d = 1; d < n; d *= 2)
      {
        offset -= 1;
        GroupMemoryBarrierWithGroupSync();
        if (thread_id < d)
        {
          uint ai = ((2 * thread_id + 1) << offset) - 1;
          uint bi = ((2 * thread_id + 2) << offset) - 1;
          float t = temp[ai];
          temp[ai] = temp[bi];
          temp[bi] += t;
        }
      }
      GroupMemoryBarrierWithGroupSync();

      if (triangleId1 < triangles_count)
        AREA(triangleId1) += temp[thread_id * 2];
      if (triangleId2 < triangles_count)
        AREA(triangleId2) += temp[thread_id * 2 + 1];
##endif

##if gpu_objects_sum_area_step == step2
      uint triangleId1 = dispatch_thread_id * 2 + n;
      if (triangleId1 >= triangles_count)
        return;
      uint triangleId2 = triangleId1 + 1;
      uint block = dispatch_thread_id / (n / 2);
      float x = AREA((block + 1) * n - 1);
      AREA(dispatch_thread_id * 2 + n) += x;
      if (thread_id == (MAX_GATHERED_TRIANGLES_SQRT / 2 - 1))
        x = 0;
      if (triangleId2 < triangles_count)
        AREA(dispatch_thread_id * 2 + 1 + n) += x;
##endif

    }
  }
  compile("cs_5_0", "sum_area");
}
