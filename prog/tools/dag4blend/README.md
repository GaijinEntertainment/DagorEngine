# 1. Installation
 - Edit/Preferences/Add-ons: Install. Search for bl_dag_exporter.zip, press OK.
# 2. Export to .dag
 - File/Export/Dagor Engine (.dag)
 - Messages are shown on the console during export: Window/Toggle System Console
 - Create collections LOD00, LOD01, etc to export the meshes as *.lodnn.dag as separate lod meshes.
 - If no LODnn collection is created, every mesh exported as a single .dag file
 - Log messages can be visible by clicking Window/Toggle System Console
 # 3. .dag Material
 - Dagor related materials can be set up in the Material Properties tab under .dag Material panel
 - Textures have to be provided in the texture slots, format must be .tga
 - Shader class have to be seleceted for the appropriate shader to be used for the material
 - There are shader properties can be adjusted for the selected shader under it
 - There are different texture slots have to be filled for different shader classes
# 4. .dag Object Properties
 - Object properties can be set in the Dagor tab at the side of 3D View as Dagor Object Properties panel
