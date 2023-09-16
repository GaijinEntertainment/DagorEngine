#ifndef SSGI_MOVE_CASCADE
#define SSGI_MOVE_CASCADE 1

#ifdef GI_VOLMAP_HMAP
bool getMovedWorldPos(int3 coord, uint cascadeId, int3 move, out float3 worldPos)
{
  int dim = volmap_xz_dim(cascadeId);
  int3 coordOfs = int3((dim.xx+(coord.xy + (ssgi_ambient_volmap_crd_ofs_xz(cascadeId)+move.xy)%dim)%dim)%dim, coord.z);
  coordOfs.xy -= move.xy;
  coordOfs.z = coord.z - move.z;
  float3 cofs = (coordOfs).xzy*volmap[cascadeId].voxelSize.xyx;
  worldPos = cofs + ssgi_ambient_volmap_crd_to_world1_xyz(cascadeId);
  float y0 = worldPos.y;
  // calculate the current height of the ambient cube when aligned to heightmap
  float y1 = get_height_for_volmap(worldPos);
  y1 += (coord).z * volmap[cascadeId].voxelSize.y;
  y1 = floor(y1 / volmap[cascadeId].voxelSize.y) * volmap[cascadeId].voxelSize.y;
  y1 += 0.5 * volmap[cascadeId].voxelSize.y;

  worldPos.y = max( y0, y1);
  int2 oldCoord = (dim.xx + (coord.xy + ssgi_ambient_volmap_crd_ofs_xz(cascadeId))%dim)%dim;
  return (all(oldCoord.xy - coordOfs.xy == 0) && coordOfs.z == coord.z)&& (coordOfs.z < int(volmap_y_dim(cascadeId)) && coordOfs.z >= 0);
}
#else
bool getMovedWorldPos(int3 coord, uint cascadeId, int3 move, out float3 worldPos)
{
  int dim = volmap_xz_dim(cascadeId);
  int3 coordOfs = int3((dim.xx+(coord.xy + (ssgi_ambient_volmap_crd_ofs_xz(cascadeId)+move.xy)%dim)%dim)%dim, coord.z);
  coordOfs.xy -= move.xy;
  coordOfs.z = coord.z - move.z;
  worldPos = ambientOffsettedCoordToWorldPos(coordOfs, cascadeId);
  int2 oldCoord = (dim.xx + (coord.xy + ssgi_ambient_volmap_crd_ofs_xz(cascadeId))%dim)%dim;
  return all(oldCoord.xy - coordOfs.xy==0) && (coordOfs.z < int(volmap_y_dim(cascadeId)) && coordOfs.z >= 0);
}
#endif
#endif