uint get_sp_probes_count(bool onlyHistoryProbe)
{
  return loadBuffer(screenspace_probes_list, onlyHistoryProbe ? 4 : 0);
}
uint get_sp_probe_unsafe(uint i, bool onlyHistoryProbe)
{
  return loadBuffer(screenspace_probes_list, 8 + i*4 + (onlyHistoryProbe ? sp_getNumTotalProbes()*4 : 0));
}
