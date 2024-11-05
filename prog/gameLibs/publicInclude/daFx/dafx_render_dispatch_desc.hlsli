//
// cpu->gpu render params
//
struct RenderDispatchDesc
{
  uint dataRenOffset;
  uint parentRenOffset;

  uint startAndLimit; // packed 16b + 16b
  uint dataRenStrideAndInstanceCount; // packed 16b + 16b
};

#define DAFX_RENDER_GROUP_SIZE 4096