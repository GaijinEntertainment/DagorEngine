#ifndef RENDINST_BEND_DESC_HLSLI
#define RENDINST_BEND_DESC_HLSLI


//
// cpu->gpu
//
#define MAX_CLUSTER 254
#define BOMBS_BLAST_ANIMATION_TIME 6.0f //curve animation max time
#define MAX_POWER_FOR_BLAST 4.0f
#define MAX_POWER_FOR_DYNAMIC_WIND 4.0f
#define MAX_INNER_RADIUS 500.0f
#define BOMBS_BLAST_AIR_SPEED 500.0f
#define DISK_RADIUS 150.0f// 150 pixel for 500m/s 0.3sec to go from start to end of disk
#define MAXIMUM_GRID_BOX_CASCADE 2 // we can create a maximum of 2 cascade
#define MAX_BOX_PER_GRID 256
#define MAX_CLUSTER_PER_BOX 4

#define BYTE_2 0xFFFF
#define BYTE_1 0xFF
#define SHIFT_2_BYTES 16u
#define SHIFT_1_BYTES 8u

// gpu

struct ClusterDescGpu //16 bytes * clusterAmount
{
  uint2 sphere; // 2byte per axes + r
  uint time; // 2byte time, 2bytes free
  uint power; //2 bytes power 2byte free
};

//const buffer are padded to 16 byte
struct ClusterCascadeDescGpu
{
  uint pos; // 2 byte per axes [x/z]
  uint gridSize;
  uint boxWidthNum;
  uint clusterPerBox;
};

struct ClusterCascadeDescUncompressed
{
  float2 pos;
  float gridSize;
  float boxWidthNum;
  float clusterPerBox;
};

struct ClusterDescUncompressed
{
  float4 sphere;
  float time;
  float power;
};

// animation function //
//https://graphtoy.com/?f1(x,t)=5&v1=true&f2(x,t)=x*(30-5*f1(x))&v2=true&f3(x,t)=sin(x*(2*x))*(0.55/(pow(f2(x),0.7*x)))*f1(x)&v3=true&f4(x,t)=smoothstep(-1,1,f3(x))*2.0-1.0&v4=true&f5(x,t)=&v5=false&f6(x,t)=&v6=false&grid=NaN&coords=0,0,6.773687160645327
////////////////////////
#endif