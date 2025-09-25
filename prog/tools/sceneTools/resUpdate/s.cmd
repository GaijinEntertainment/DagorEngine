@rem DEP_DOMAIN skyquake/prog
@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\resUpd-dev.exe -dryRun C:\WarThunder   C:\dagor2\skyquake\gameOnline.clean\res\grp_hdr.vromfs.bin res
@rem C:\dagor2\skyquake\gameOnline.clean\content\pkg_dev\res\grp_hdr.vromfs.bin content/pkg_dev/res
