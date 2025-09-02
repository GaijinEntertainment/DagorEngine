//
// cpu->gpu render params
//
struct RenderDispatchDesc
{
  uint dataRenOffset;
  uint parentRenOffset;

  uint startAndLimit; // packed 16b + 16b
  uint dataRenStrideAndInstanceCount; // packed 15b + 1b + 16b // SYS_CPU_RENDER_REQ flag is stored in the 16th bit
};

#define DAFX_RENDER_GROUP_SIZE 4096