uint add_screenspace_probes_list(uint i, bool onlyHistoryProbe)
{
  uint listAt;
  screenspace_probes_list.InterlockedAdd(onlyHistoryProbe ? 4 : 0, 1, listAt);
  storeBuffer(screenspace_probes_list, listAt*4 + (onlyHistoryProbe ? sp_getNumTotalProbes()*4 : 0) + 8, i);
  return listAt;
}
