jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_DX11/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_DX12/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_vulkan/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_null/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_pc_multi/jamfile
jam -sRoot=../../.. -sPlatformArch=x86 -f drv3d_use_d3di/jamfile

jam -sRoot=../../.. -f drv3d_ps4/jamfile
jam -sRoot=../../.. -sPlatform=ps4 -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatform=ps4 -f drv3d_null/jamfile

jam -sRoot=../../.. -f drv3d_ps5/jamfile
jam -sRoot=../../.. -sPlatform=ps5 -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatform=ps5 -f drv3d_null/jamfile

jam -sRoot=../../.. -sPlatform=xboxOne -f drv3d_DX11/jamfile
jam -sRoot=../../.. -sPlatform=xboxOne -sXboxSdk=gdk -f drv3d_DX12/jamfile
jam -sRoot=../../.. -sPlatform=xboxOne -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatform=xboxOne -f drv3d_null/jamfile

jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_DX11/jamfile
jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_vulkan/jamfile
jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_null/jamfile
jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_pc_multi/jamfile
jam -sRoot=../../.. -sPlatformArch=x86_64 -f drv3d_use_d3di/jamfile

jam -sRoot=../../.. -sPlatform=android -f drv3d_vulkan/jamfile
jam -sRoot=../../.. -sPlatform=android -f drv3d_null/jamfile
jam -sRoot=../../.. -sPlatform=android -f drv3d_stub/jamfile

jam -sRoot=../../.. -sPlatform=nswitch -f drv3d_vulkan/jamfile
jam -sRoot=../../.. -sPlatform=nswitch -f drv3d_null/jamfile
jam -sRoot=../../.. -sPlatform=nswitch -f drv3d_stub/jamfile

jam -sRoot=../../.. -sPlatform=scarlett -f drv3d_DX12/jamfile
jam -sRoot=../../.. -sPlatform=scarlett -f drv3d_stub/jamfile
jam -sRoot=../../.. -sPlatform=scarlett -f drv3d_null/jamfile
