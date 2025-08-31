#if NBS_SHADER
  #define camera_in_camera_prev_minus_vp_ellipse_center (-(camera_in_camera_prev_vp_ellipse_center.xy))
  #define camera_in_camera_prev_vp_ellipse_x_axis       (camera_in_camera_prev_vp_ellipse_xy_axes.xy)
  #define camera_in_camera_prev_vp_ellipse_y_axis       (camera_in_camera_prev_vp_ellipse_xy_axes.zw)
  #define camera_in_camera_minus_vp_ellipse_center      (-(camera_in_camera_vp_ellipse_center.xy))
  #define camera_in_camera_vp_ellipse_x_axis            (camera_in_camera_vp_ellipse_xy_axes.xy)
  #define camera_in_camera_vp_ellipse_y_axis            (camera_in_camera_vp_ellipse_xy_axes.zw)
  #define camera_in_camera_vp_u_remapping               (camera_in_camera_vp_uv_remapping.xy)
  #define camera_in_camera_vp_v_remapping               (camera_in_camera_vp_uv_remapping.zw)
#endif

struct CameraView
{
  bool isMain;
};

CameraView camera_in_camera_main_view()
{
  CameraView v;
  v.isMain = true;
  return v;
}

bool is_same_camera_view(CameraView l, CameraView r)
{
  return l.isMain == r.isMain;
}

#if USE_CAMERA_IN_CAMERA

bool camera_has_sub_view()
{
  return camera_in_camera_active != 0.0;
}

CameraView get_camera_view_internal(float2 uv, float2 minus_vp_ellipse_center, float2 vp_ellipse_x_axis, float2 vp_ellipse_y_axis)
{
  float2 dr = float2(
    uv.x + minus_vp_ellipse_center.x,
    uv.y * camera_in_camera_vp_y_scale + minus_vp_ellipse_center.y
  );

  float2 xy = float2(
    dot(dr, vp_ellipse_x_axis),
    dot(dr, vp_ellipse_y_axis));

  CameraView view;
  view.isMain =  lengthSq(xy) > 1.0;
  return view;
}

CameraView get_camera_view_internal(float2 uv)
{
  return get_camera_view_internal(uv,
    camera_in_camera_minus_vp_ellipse_center,
    camera_in_camera_vp_ellipse_x_axis,
    camera_in_camera_vp_ellipse_y_axis);
}

CameraView get_prev_frame_camera_view_internal(float2 uv)
{
  return get_camera_view_internal(uv,
    camera_in_camera_prev_minus_vp_ellipse_center,
    camera_in_camera_prev_vp_ellipse_x_axis,
    camera_in_camera_prev_vp_ellipse_y_axis);
}

bool is_main_camera_view(float2 uv)
{
  BRANCH
  if (camera_in_camera_active != 0.0)
    return is_same_camera_view(get_camera_view_internal(uv), camera_in_camera_main_view());
  else
    return true;
}

bool is_main_camera_view_postfx()
{
  return camera_in_camera_postfx_lens_view == 0.0;
}

CameraView get_camera_view(float2 uv)
{
  BRANCH
  if (camera_in_camera_active != 0.0)
    return get_camera_view_internal(uv);
  else
    return camera_in_camera_main_view();
}

CameraView get_prev_frame_camera_view(float2 uv)
{
  BRANCH
  if (camera_in_camera_active != 0.0)
    return get_prev_frame_camera_view_internal(uv);
  else
    return camera_in_camera_main_view();
}

//use it with stencil test enabled only
CameraView get_camera_view_postfx()
{
  CameraView view;
  view.isMain = camera_in_camera_active == 0.0 || camera_in_camera_postfx_lens_view == 0.0;
  return view;
}

float2 remap_to_main_view_uv(float2 uv)
{
  float x = uv.x * camera_in_camera_vp_u_remapping.x + camera_in_camera_vp_u_remapping.y;
  float y = uv.y * camera_in_camera_vp_v_remapping.x + camera_in_camera_vp_v_remapping.y;

  return float2(x,y);
}

float2 check_and_remap_to_main_view_uv(float2 uv)
{
  BRANCH
  if (camera_in_camera_active != 0.0)
  {
    return get_camera_view_internal(uv).isMain ? uv : remap_to_main_view_uv(uv);
  }
  else
    return uv;
}

bool is_wrong_camera_view_area_postfx(float2 uv, out CameraView view)
{
  const bool isMainViewRender = camera_in_camera_postfx_lens_view == 0.0;
  view = get_camera_view_internal(uv);
  return isMainViewRender != view.isMain;
}

#define DISCARD_IF_INVALID_VIEW_AREA_INTERNAL(uv, discard_op)             \
BRANCH                                                                    \
if (camera_in_camera_active != 0.0)                                       \
{                                                                         \
  const bool isMainViewRender = camera_in_camera_postfx_lens_view == 0.0; \
  CameraView view = get_camera_view_internal(uv);                         \
  if (isMainViewRender != view.isMain)                                    \
    discard_op;                                                           \
}

#define DISCARD_IF_INVALID_VIEW_AREA_PS(uv) DISCARD_IF_INVALID_VIEW_AREA_INTERNAL(uv, discard)
#define DISCARD_IF_INVALID_VIEW_AREA_CS(uv) DISCARD_IF_INVALID_VIEW_AREA_INTERNAL(uv, return)

#if WAVE_INTRINSICS
  #define IS_WHOLE_GROUP_IN_WRONG_VIEW_AREA(lds_var, uv, out_view_type, out_res) \
  {                                                                              \
    bool isWrongArea = is_wrong_camera_view_area_postfx(uv, out_view_type);      \
    out_res = WaveActiveAllTrue (isWrongArea);                                   \
  }
#else
  #define IS_WHOLE_GROUP_IN_WRONG_VIEW_AREA(lds_var, uv, out_view_type, out_res) \
  {                                                                              \
    bool isWrongArea = is_wrong_camera_view_area_postfx(uv, out_view_type);      \
    InterlockedAdd(lds_var, isWrongArea ? 0 : 1);                                \
    GroupMemoryBarrierWithGroupSync();                                           \
    out_res = lds_var == 0;                                                      \
  }
#endif

#define DISCARD_GROUP_IN_INVALID_VIEW_AREA(lds_var, uv, out_view_type)          \
BRANCH                                                                          \
if (camera_has_sub_view())                                                      \
{                                                                               \
  bool discardGroup;                                                            \
  IS_WHOLE_GROUP_IN_WRONG_VIEW_AREA(lds_var, uv, out_view_type, discardGroup);  \
  if (discardGroup)                                                             \
    return;                                                                     \
}

bool is_wrong_camera_view_area_postfx(float2 uv)
{
  CameraView stub;
  return is_wrong_camera_view_area_postfx(uv, stub);
}

bool invalidate_mvec_to_invalid_view_area(float2 uv, inout float2 motion_vec)
{
  BRANCH
  if (camera_has_sub_view())
  {
    CameraView curView = get_camera_view_postfx();
    CameraView prevFrameView = get_prev_frame_camera_view(uv + motion_vec);

    if (!is_same_camera_view(curView, prevFrameView))
    {
      const float2 outOfScreenMotionVec = float2(-3, -3);
      motion_vec = outOfScreenMotionVec;
      return true;
    }
  }

  return false;
}

void move_uv_to_offscreen_if_not_eq_views(inout float2 uv, CameraView v1, CameraView v2)
{
  uv = is_same_camera_view(v1,v2) ? uv : float2(2,2);
}

//ends USE_CAMERA_IN_CAMERA
#else

#define DISCARD_IF_INVALID_VIEW_AREA_PS(uv) {}
#define DISCARD_IF_INVALID_VIEW_AREA_CS(uv) {}
#define DISCARD_GROUP_IN_INVALID_VIEW_AREA(lds_var, uv, out_view_type) \
{                                                                      \
  out_view_type = camera_in_camera_main_view();                        \
}

bool camera_has_sub_view() { return false; }
bool is_main_camera_view(float2 uv) { return true; }
bool is_main_camera_view_postfx() { return true; }
CameraView get_camera_view(float2 uv) { return camera_in_camera_main_view(); }
CameraView get_camera_view_postfx() { return camera_in_camera_main_view(); }
CameraView get_prev_frame_camera_view(float2 uv) { return camera_in_camera_main_view(); }
float2 remap_to_main_view_uv(float2 uv) { return uv; }
float2 check_and_remap_to_main_view_uv(float2 uv) { return uv; }
bool is_wrong_camera_view_area_postfx(float2 uv, out CameraView view) { view = camera_in_camera_main_view(); return false;}
bool is_wrong_camera_view_area_postfx(float2 uv) { return false;}
bool invalidate_mvec_to_invalid_view_area(float2 uv, inout float2 motion_vec) { return false; }
void move_uv_to_offscreen_if_not_eq_views(inout float2 uv, CameraView v1, CameraView v2) {}
#endif
