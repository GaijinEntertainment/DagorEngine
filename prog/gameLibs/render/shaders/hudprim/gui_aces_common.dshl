float4 blue_to_alpha_mul = (0, 0, 0, 1);
texture tex;
texture mask_tex;
float4 mask_matrix_line_0 = (1, 0, 0, 0);
float4 mask_matrix_line_1 = (0, 1, 0, 0);

block(object) gui_aces_object
{
  (vs) {
    mask_matrix_line_0@f3 = mask_matrix_line_0;
    mask_matrix_line_1@f3 = mask_matrix_line_1;
  }
  (ps) {
    tex@smp2d = tex;
    mask_tex@smp2d = mask_tex;
    blue_to_alpha_mul@f4 = blue_to_alpha_mul;
  }
}
