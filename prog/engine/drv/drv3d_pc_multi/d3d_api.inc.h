// Copyright (C) Gaijin Games KFT.  All rights reserved.

bool init_driver();
bool is_inited();

bool init_video(void *hinst, main_wnd_f *f, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

void release_driver();
bool fill_interface_table(D3dInterfaceTable &d3dit);
void prepare_for_destroy();
void window_destroyed(void *hwnd);

void *get_device();
const Driver3dDesc &get_driver_desc();

void reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib, int);
void get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk);
void get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk);

// void *get_native_surface(BaseTexture* tex);
