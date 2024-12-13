# dabuild
jam -s Root=../.. -f libTools/daKernel/jamfile
jam -s Root=../.. -f sceneTools/assetExp/jamfile
jam -s Root=../.. -f converters/ddsxCvt2/jamfile

# daImpostorBaker
jam -s Root=../.. -f sceneTools/impostorBaker/jamfile

# DDSx plugins
jam -s Root=../.. -f sceneTools/assetExp/ddsxConv/jamfile
jam -s Root=../.. -s iOS_exp=yes -f sceneTools/assetExp/ddsxConv/jamfile
jam -s Root=../.. -s Tegra_exp=yes -f sceneTools/assetExp/ddsxConv/jamfile

# shader compilers
jam -s Root=../.. -f ShaderCompiler2/jamfile

# common minimal gui shaders for tools
pushd sceneTools/guiShaders_commonData
./compile_gui_shaders_spirv.sh
popd

# utils
jam -s Root=../.. -f sceneTools/vromfsPacker/jamfile
jam -s Root=../.. -f sceneTools/csvUtil/jamfile
jam -s Root=../.. -f converters/ddsxCvt/jamfile
jam -s Root=../.. -f converters/ddsx2dds/jamfile
jam -s Root=../.. -f converters/ddsConverter/jamfile
jam -s Root=../.. -f FontGenerator/jamfile
jam -s Root=../.. -f converters/GuiTex/jamfile
jam -s Root=../.. -f sceneTools/utils/jamfile-binBlk

jam -s Root=../.. -f sceneTools/dumpGrp/jamfile
jam -s Root=../.. -f sceneTools/dbldUtil/jamfile
jam -s Root=../.. -f sceneTools/resDiff/jamfile
jam -s Root=../.. -f sceneTools/resUpdate/jamfile
jam -s Root=../.. -f sceneTools/resClean/jamfile

jam -s Root=../.. -sConfig=dev -f consoleSq/jamfile

# Blender plugin
pushd dag4blend
python3 __build_pack.py FINAL
popd
