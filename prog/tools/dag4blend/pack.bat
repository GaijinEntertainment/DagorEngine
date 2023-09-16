@echo off
pushd .
cd ..
zip dag4blend\dag4blend.zip dag4blend\__init__.py dag4blend\dagorShaders.cfg dag4blend\fixed_dagorShaders.cfg ^
dag4blend\constants.py dag4blend\face.py dag4blend\mesh.py dag4blend\node.py  ^
dag4blend\material.py dag4blend\nodeMaterial.py dag4blend\dagMath.py ^
dag4blend\settings.py ^
dag4blend\read_config.py dag4blend\cmp\cmp_const.py dag4blend\cmp\cmp_export.py dag4blend\cmp\cmp_import.py dag4blend\cmp\cmp_panels.py ^
dag4blend\dagormat\build_node_tree.py dag4blend\dagormat\compare_dagormats.py dag4blend\dagormat\dagormat.py dag4blend\dagormat\rw_dagormat_text.py ^
dag4blend\exporter\export_panel.py dag4blend\exporter\exporter.py dag4blend\exporter\writer.py ^
dag4blend\helpers\basename.py dag4blend\helpers\multiline.py dag4blend\helpers\popup.py dag4blend\helpers\props.py dag4blend\helpers\texts.py ^
dag4blend\importer\import_panel.py dag4blend\importer\importer.py dag4blend\importer\reader.py ^
dag4blend\object_properties\object_properties.py dag4blend\smooth_groups\smooth_groups.py dag4blend\tools\tools_panel.py dag4blend\tools\wind_panel.py dag4blend\tools\bake_panel.py ^
dag4blend\colprops\colprops.py dag4blend\extras\dag_perlin.png dag4blend\extras\library.blend dag4blend\extras\remap.txt ^
dag4blend\object_properties\presets\* ^
pythonCommon\pyparsing.py pythonCommon\datablock.py ^
popd