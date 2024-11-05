#ifdef __cplusplus
{
  const GlobalData * __restrict globalData = (GlobalData*)data.globalValues;
  const DispatchDesc * __restrict dispatches = (DispatchDesc*)data.dispatches;
  uint32_t * __restrict curBuf = (uint32_t*)data.buffer;
  int4 * __restrict cullingBuff = (int4*)data.culling;

  ComputeCallDesc cdesc;

  for ( int i = 0, ie = data.dispatchesCount ; i < ie ; ++i )
  {
    const DispatchDesc &ddesc = dispatches[i];
    uint headOffset = ddesc.headOffsetAndLodOfs & 0xffffff;
    const DataHead &head = *(DataHead*)(curBuf + headOffset / DAFX_ELEM_STRIDE);
    dafx_fill_cdesc( head, ddesc, cdesc );
    cdesc.count = ddesc.startAndCount >> 16;
    cdesc.start = ddesc.startAndCount & 0xffff;

#if DAFX_PRELOAD_PARENT_DATA
    DAFX_PRELOAD_PARENT_REN_STRUCT parentRenData;
    DAFX_PRELOAD_PARENT_SIM_STRUCT parentSimData;
    dafx_preload_parent_ren_data( curBuf, cdesc.parentRenOffset, parentRenData );
    dafx_preload_parent_sim_data( curBuf, cdesc.parentSimOffset, parentSimData );
#endif

    for ( ; cdesc.idx < cdesc.count ; ++cdesc.idx )
    {
      cdesc.gid = ( cdesc.idx + cdesc.start ) % head.elemLimit;
      cdesc.dataRenOffsetRelative = ( cdesc.gid * head.dataRenStride ) / DAFX_ELEM_STRIDE;
      cdesc.dataSimOffsetRelative = ( cdesc.gid * head.dataSimStride ) / DAFX_ELEM_STRIDE;

      cdesc.dataRenOffsetCurrent = cdesc.dataRenOffsetStart + cdesc.dataRenOffsetRelative;
      cdesc.dataSimOffsetCurrent = cdesc.dataSimOffsetStart + cdesc.dataSimOffsetRelative;

      update_culling_data( cdesc.cullingId, DAFX_SHADER_INC_CALL, cullingBuff );
    }
  }
}
#else
{
  uint bucketSize = get_immediate_dword_0();
  uint dispatchStart = get_immediate_dword_1();

  uint bucketId = t_id.x / bucketSize;
  uint localId = t_id.x % bucketSize;

  DispatchDesc ddesc = dafx_dispatch_descs[dispatchStart + bucketId];
  uint start = ddesc.startAndCount & 0xffff;
  uint count = ddesc.startAndCount >> 16;
  if ( localId >= count )
    return;

  DataHead head;
  uint headOffset = ddesc.headOffsetAndLodOfs & 0xffffff;
  dafx_fill_data_head( headOffset / DAFX_ELEM_STRIDE, head );

  ComputeCallDesc cdesc;
  dafx_fill_cdesc( head, ddesc, cdesc );

#if DAFX_PRELOAD_PARENT_DATA
    DAFX_PRELOAD_PARENT_REN_STRUCT parentRenData;
    DAFX_PRELOAD_PARENT_SIM_STRUCT parentSimData;
    dafx_preload_parent_ren_data( 0, cdesc.parentRenOffset, parentRenData );
    dafx_preload_parent_sim_data( 0, cdesc.parentSimOffset, parentSimData );
#endif

  cdesc.start = start;
  cdesc.count = count;
  cdesc.gid = ( localId + start ) % head.elemLimit;
  cdesc.idx = localId;

  cdesc.dataRenOffsetRelative = ( cdesc.gid * head.dataRenStride ) / DAFX_ELEM_STRIDE;
  cdesc.dataSimOffsetRelative = ( cdesc.gid * head.dataSimStride ) / DAFX_ELEM_STRIDE;

  cdesc.dataRenOffsetCurrent = cdesc.dataRenOffsetStart + cdesc.dataRenOffsetRelative;
  cdesc.dataSimOffsetCurrent = cdesc.dataSimOffsetStart + cdesc.dataSimOffsetRelative;

  GlobalData globalData = global_data_load();

  update_culling_data( cdesc.cullingId, DAFX_SHADER_INC_CALL );
}
#endif