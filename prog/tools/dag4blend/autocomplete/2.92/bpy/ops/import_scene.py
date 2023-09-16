import sys
import typing
import bpy.types


def fbx(filepath: str = "",
        directory: str = "",
        filter_glob: str = "*.fbx",
        files: typing.
        Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.
              List['bpy.types.OperatorFileListElement'],
              'bpy_prop_collection'] = None,
        ui_tab: typing.Union[int, str] = 'MAIN',
        use_manual_orientation: bool = False,
        global_scale: float = 1.0,
        bake_space_transform: bool = False,
        use_custom_normals: bool = True,
        use_image_search: bool = True,
        use_alpha_decals: bool = False,
        decal_offset: float = 0.0,
        use_anim: bool = True,
        anim_offset: float = 1.0,
        use_subsurf: bool = False,
        use_custom_props: bool = True,
        use_custom_props_enum_as_string: bool = True,
        ignore_leaf_bones: bool = False,
        force_connect_children: bool = False,
        automatic_bone_orientation: bool = False,
        primary_bone_axis: typing.Union[int, str] = 'Y',
        secondary_bone_axis: typing.Union[int, str] = 'X',
        use_prepost_rot: bool = True,
        axis_forward: typing.Union[int, str] = '-Z',
        axis_up: typing.Union[int, str] = 'Y'):
    ''' Load a FBX file

    :param filepath: File Path, Filepath used for importing the file
    :type filepath: str
    :param directory: directory
    :type directory: str
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param files: File Path
    :type files: typing.Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.List['bpy.types.OperatorFileListElement'], 'bpy_prop_collection']
    :param ui_tab: ui_tab, Import options categories * MAIN Main, Main basic settings. * ARMATURE Armatures, Armature-related settings.
    :type ui_tab: typing.Union[int, str]
    :param use_manual_orientation: Manual Orientation, Specify orientation and scale, instead of using embedded data in FBX file
    :type use_manual_orientation: bool
    :param global_scale: Scale
    :type global_scale: float
    :param bake_space_transform: Apply Transform, Bake space transform into object data, avoids getting unwanted rotations to objects when target space is not aligned with Blender's space (WARNING! experimental option, use at own risks, known broken with armatures/animations)
    :type bake_space_transform: bool
    :param use_custom_normals: Custom Normals, Import custom normals, if available (otherwise Blender will recompute them)
    :type use_custom_normals: bool
    :param use_image_search: Image Search, Search subdirs for any associated images (WARNING: may be slow)
    :type use_image_search: bool
    :param use_alpha_decals: Alpha Decals, Treat materials with alpha as decals (no shadow casting)
    :type use_alpha_decals: bool
    :param decal_offset: Decal Offset, Displace geometry of alpha meshes
    :type decal_offset: float
    :param use_anim: Import Animation, Import FBX animation
    :type use_anim: bool
    :param anim_offset: Animation Offset, Offset to apply to animation during import, in frames
    :type anim_offset: float
    :param use_subsurf: Subdivision Data, Import FBX subdivision information as subdivision surface modifiers
    :type use_subsurf: bool
    :param use_custom_props: Custom Properties, Import user properties as custom properties
    :type use_custom_props: bool
    :param use_custom_props_enum_as_string: Import Enums As Strings, Store enumeration values as strings
    :type use_custom_props_enum_as_string: bool
    :param ignore_leaf_bones: Ignore Leaf Bones, Ignore the last bone at the end of each chain (used to mark the length of the previous bone)
    :type ignore_leaf_bones: bool
    :param force_connect_children: Force Connect Children, Force connection of children bones to their parent, even if their computed head/tail positions do not match (can be useful with pure-joints-type armatures)
    :type force_connect_children: bool
    :param automatic_bone_orientation: Automatic Bone Orientation, Try to align the major bone axis with the bone children
    :type automatic_bone_orientation: bool
    :param primary_bone_axis: Primary Bone Axis
    :type primary_bone_axis: typing.Union[int, str]
    :param secondary_bone_axis: Secondary Bone Axis
    :type secondary_bone_axis: typing.Union[int, str]
    :param use_prepost_rot: Use Pre/Post Rotation, Use pre/post rotation from FBX transform (you may have to disable that in some cases)
    :type use_prepost_rot: bool
    :param axis_forward: Forward
    :type axis_forward: typing.Union[int, str]
    :param axis_up: Up
    :type axis_up: typing.Union[int, str]
    '''

    pass


def gltf(filepath: str = "",
         filter_glob: str = "*.glb;*.gltf",
         files: typing.
         Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.
               List['bpy.types.OperatorFileListElement'],
               'bpy_prop_collection'] = None,
         loglevel: int = 0,
         import_pack_images: bool = True,
         merge_vertices: bool = False,
         import_shading: typing.Union[int, str] = 'NORMALS',
         bone_heuristic: typing.Union[int, str] = 'TEMPERANCE',
         guess_original_bind_pose: bool = True):
    ''' Load a glTF 2.0 file

    :param filepath: File Path, Filepath used for importing the file
    :type filepath: str
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param files: File Path
    :type files: typing.Union[typing.Dict[str, 'bpy.types.OperatorFileListElement'], typing.List['bpy.types.OperatorFileListElement'], 'bpy_prop_collection']
    :param loglevel: Log Level, Log Level
    :type loglevel: int
    :param import_pack_images: Pack Images, Pack all images into .blend file
    :type import_pack_images: bool
    :param merge_vertices: Merge Vertices, The glTF format requires discontinuous normals, UVs, and other vertex attributes to be stored as separate vertices, as required for rendering on typical graphics hardware. This option attempts to combine co-located vertices where possible. Currently cannot combine verts with different normals
    :type merge_vertices: bool
    :param import_shading: Shading, How normals are computed during import
    :type import_shading: typing.Union[int, str]
    :param bone_heuristic: Bone Dir, Heuristic for placing bones. Tries to make bones pretty * BLENDER Blender (best for re-importing), Good for re-importing glTFs exported from Blender. Bone tips are placed on their local +Y axis (in glTF space). * TEMPERANCE Temperance (average), Decent all-around strategy. A bone with one child has its tip placed on the local axis closest to its child. * FORTUNE Fortune (may look better, less accurate), Might look better than Temperance, but also might have errors. A bone with one child has its tip placed at its child's root. Non-uniform scalings may get messed up though, so beware.
    :type bone_heuristic: typing.Union[int, str]
    :param guess_original_bind_pose: Guess Original Bind Pose, Try to guess the original bind pose for skinned meshes from the inverse bind matrices. When off, use default/rest pose as bind pose
    :type guess_original_bind_pose: bool
    '''

    pass


def obj(filepath: str = "",
        filter_glob: str = "*.obj;*.mtl",
        use_edges: bool = True,
        use_smooth_groups: bool = True,
        use_split_objects: bool = True,
        use_split_groups: bool = False,
        use_groups_as_vgroups: bool = False,
        use_image_search: bool = True,
        split_mode: typing.Union[int, str] = 'ON',
        global_clamp_size: float = 0.0,
        axis_forward: typing.Union[int, str] = '-Z',
        axis_up: typing.Union[int, str] = 'Y'):
    ''' Load a Wavefront OBJ File

    :param filepath: File Path, Filepath used for importing the file
    :type filepath: str
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param use_edges: Lines, Import lines and faces with 2 verts as edge
    :type use_edges: bool
    :param use_smooth_groups: Smooth Groups, Surround smooth groups by sharp edges
    :type use_smooth_groups: bool
    :param use_split_objects: Object, Import OBJ Objects into Blender Objects
    :type use_split_objects: bool
    :param use_split_groups: Group, Import OBJ Groups into Blender Objects
    :type use_split_groups: bool
    :param use_groups_as_vgroups: Poly Groups, Import OBJ groups as vertex groups
    :type use_groups_as_vgroups: bool
    :param use_image_search: Image Search, Search subdirs for any associated images (Warning, may be slow)
    :type use_image_search: bool
    :param split_mode: Split * ON Split, Split geometry, omits unused verts. * OFF Keep Vert Order, Keep vertex order from file.
    :type split_mode: typing.Union[int, str]
    :param global_clamp_size: Clamp Size, Clamp bounds under this value (zero to disable)
    :type global_clamp_size: float
    :param axis_forward: Forward
    :type axis_forward: typing.Union[int, str]
    :param axis_up: Up
    :type axis_up: typing.Union[int, str]
    '''

    pass


def x3d(filepath: str = "",
        filter_glob: str = "*.x3d;*.wrl",
        axis_forward: typing.Union[int, str] = 'Z',
        axis_up: typing.Union[int, str] = 'Y'):
    ''' Import an X3D or VRML2 file

    :param filepath: File Path, Filepath used for importing the file
    :type filepath: str
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param axis_forward: Forward
    :type axis_forward: typing.Union[int, str]
    :param axis_up: Up
    :type axis_up: typing.Union[int, str]
    '''

    pass
