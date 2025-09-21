@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@rem %DAGOR_CDK_DIR%\resDiffUtil-dev.exe d:/WarThunder-prod d:/dagor4/skyquake/gameOnline content/base/res patch
@rem %DAGOR_CDK_DIR%\resDiffUtil-dev.exe d:/dagor4/samples/skiesSample/game d:/dagor4/samples/skiesSample/game- . d:/dagor4/samples/skiesSample/game.patch -vfs:res/grp_hdr.vromfs.bin -resBlk:res/respacks.blk
@rem -verbose -dryRun
