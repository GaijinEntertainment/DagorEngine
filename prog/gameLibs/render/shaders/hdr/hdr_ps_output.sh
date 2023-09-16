int hdr_ps_output_mode = 0;
interval hdr_ps_output_mode : sdr_only < 1, hdr10_and_sdr < 2, hdr10_only < 3, hdr_only;

macro HDR_PS_OUTPUT(name, type)
  hlsl(ps) {
    struct name
    {
    ##if (hdr_ps_output_mode == hdr10_and_sdr)
      type hdr_result : SV_Target0;
      type sdr_result : SV_Target1;
    ##elif (hdr_ps_output_mode == hdr10_only) || (hdr_ps_output_mode == hdr_only)
      type hdr_result : SV_Target0;
    ##else
      type sdr_result : SV_Target0;
    ##endif
    };
  }
endmacro
