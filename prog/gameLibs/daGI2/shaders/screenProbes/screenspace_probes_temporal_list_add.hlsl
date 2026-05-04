uint add_screenspace_probes_list(uint i, bool onlyHistoryProbe)
{
  uint listAt;
  #if WAVE_INTRINSICS
    uint histPrefix = WavePrefixCountBits(onlyHistoryProbe);
    uint selPrefix = WavePrefixCountBits(!onlyHistoryProbe);
    uint histCount = WaveActiveCountBits(onlyHistoryProbe);
    uint selCount = WaveActiveCountBits(!onlyHistoryProbe);

    uint histBaseAt = 0;
    uint selBaseAt = 0;
    if (onlyHistoryProbe && histPrefix == 0)
      screenspace_probes_list.InterlockedAdd(4, histCount, histBaseAt);
    if (!onlyHistoryProbe && selPrefix == 0)
      screenspace_probes_list.InterlockedAdd(0, selCount, selBaseAt);

    histBaseAt = WaveAllBitOr(histBaseAt);
    selBaseAt = WaveAllBitOr(selBaseAt);
    listAt = onlyHistoryProbe ? (histBaseAt + histPrefix) : (selBaseAt + selPrefix);
  #else
    uint counterByteAt = onlyHistoryProbe ? 4 : 0;
    screenspace_probes_list.InterlockedAdd(counterByteAt, 1, listAt);
  #endif
  storeBuffer(screenspace_probes_list, listAt*4 + (onlyHistoryProbe ? sp_getNumTotalProbes()*4 : 0) + 8, i);
  return listAt;
}
