import sys
import typing


def bvh(filepath: str = "",
        check_existing: bool = True,
        filter_glob: str = "*.bvh",
        global_scale: float = 1.0,
        frame_start: int = 0,
        frame_end: int = 0,
        rotate_mode: typing.Union[int, str] = 'NATIVE',
        root_transform_only: bool = False):
    ''' Save a BVH motion capture file from an armature

    :param filepath: File Path, Filepath used for exporting the file
    :type filepath: str
    :param check_existing: Check Existing, Check and warn on overwriting existing files
    :type check_existing: bool
    :param filter_glob: filter_glob
    :type filter_glob: str
    :param global_scale: Scale, Scale the BVH by this value
    :type global_scale: float
    :param frame_start: Start Frame, Starting frame to export
    :type frame_start: int
    :param frame_end: End Frame, End frame to export
    :type frame_end: int
    :param rotate_mode: Rotation, Rotation conversion * NATIVE Euler (Native), Use the rotation order defined in the BVH file. * XYZ Euler (XYZ), Convert rotations to euler XYZ. * XZY Euler (XZY), Convert rotations to euler XZY. * YXZ Euler (YXZ), Convert rotations to euler YXZ. * YZX Euler (YZX), Convert rotations to euler YZX. * ZXY Euler (ZXY), Convert rotations to euler ZXY. * ZYX Euler (ZYX), Convert rotations to euler ZYX.
    :type rotate_mode: typing.Union[int, str]
    :param root_transform_only: Root Translation Only, Only write out translation channels for the root bone
    :type root_transform_only: bool
    '''

    pass
