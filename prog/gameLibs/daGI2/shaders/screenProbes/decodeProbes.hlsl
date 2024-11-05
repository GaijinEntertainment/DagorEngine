uint getAdditionalScreenProbesCountRaw(UseByteAddressBuffer pos_buffer, uint screen_tile_index)
{
  return loadBuffer(pos_buffer, screen_tile_index*4 + sp_additionalProbesByteAt());
}

bool getAdditionalScreenProbesCount(UseByteAddressBuffer pos_buffer, uint screen_tile_index, out uint count, out uint at)
{
  uint countAt = getAdditionalScreenProbesCountRaw(pos_buffer, screen_tile_index);
  count = countAt>>24;
  at = countAt&((1<<24)-1);
  return count;
}
uint sp_loadEncodedProbe(UseByteAddressBuffer pos_buffer, uint probe_index) {return loadBuffer(pos_buffer, probe_index*4);}
uint sp_loadEncodedProbeNormalCoord(UseByteAddressBuffer pos_buffer, uint probe_index, uint total_probes) {return loadBuffer(pos_buffer, (probe_index + total_probes)*4);}

DecodedProbe getScreenProbeUnsafe(UseByteAddressBuffer pos_buffer, uint probe_index)
{
  return sp_decodeProbeInfo(sp_loadEncodedProbe(pos_buffer, probe_index));
}
