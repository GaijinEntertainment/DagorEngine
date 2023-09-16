import sys
import typing
import bpy_types


class BoneConstraintPanel(bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_rna = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class ConstraintButtonsPanel(bpy_types.Panel, bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class ConstraintButtonsSubPanel(bpy_types.Panel, bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class ObjectConstraintPanel(bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_rna = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_constraints(BoneConstraintPanel, bpy_types.Panel,
                          bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bActionConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bArmatureConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                  bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bCameraSolverConstraint(ConstraintButtonsPanel,
                                      BoneConstraintPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bChildOfConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                 bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bClampToConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                 bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bDampTrackConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bDistLimitConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bFollowPathConstraint(ConstraintButtonsPanel,
                                    BoneConstraintPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bFollowTrackConstraint(ConstraintButtonsPanel,
                                     BoneConstraintPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bKinematicConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bLocLimitConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                  bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bLocateLikeConstraint(ConstraintButtonsPanel,
                                    BoneConstraintPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bLockTrackConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bMinMaxConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bObjectSolverConstraint(ConstraintButtonsPanel,
                                      BoneConstraintPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bPivotConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                               bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bPythonConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bRotLimitConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                  bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bRotateLikeConstraint(ConstraintButtonsPanel,
                                    BoneConstraintPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSameVolumeConstraint(ConstraintButtonsPanel,
                                    BoneConstraintPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bShrinkwrapConstraint(ConstraintButtonsPanel,
                                    BoneConstraintPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSizeLikeConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                  bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSizeLimitConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSplineIKConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                  bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bStretchToConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTrackToConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                 bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTransLikeConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTransformCacheConstraint(ConstraintButtonsPanel,
                                        BoneConstraintPanel, bpy_types.Panel,
                                        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTransformConstraint(ConstraintButtonsPanel, BoneConstraintPanel,
                                   bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bActionConstraint_action(ConstraintButtonsSubPanel,
                                       BoneConstraintPanel, bpy_types.Panel,
                                       bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bActionConstraint_target(ConstraintButtonsSubPanel,
                                       BoneConstraintPanel, bpy_types.Panel,
                                       bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bArmatureConstraint_bones(ConstraintButtonsSubPanel,
                                        BoneConstraintPanel, bpy_types.Panel,
                                        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSplineIKConstraint_chain_scaling(
        ConstraintButtonsSubPanel, BoneConstraintPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bSplineIKConstraint_fitting(ConstraintButtonsSubPanel,
                                          BoneConstraintPanel, bpy_types.Panel,
                                          bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTransformConstraint_from(ConstraintButtonsSubPanel,
                                        BoneConstraintPanel, bpy_types.Panel,
                                        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class BONE_PT_bTransformConstraint_to(ConstraintButtonsSubPanel,
                                      BoneConstraintPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bActionConstraint(ObjectConstraintPanel,
                                  ConstraintButtonsPanel, bpy_types.Panel,
                                  bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bActionConstraint_action(
        ObjectConstraintPanel, ConstraintButtonsSubPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bActionConstraint_target(
        ObjectConstraintPanel, ConstraintButtonsSubPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bArmatureConstraint(ObjectConstraintPanel,
                                    ConstraintButtonsPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bArmatureConstraint_bones(
        ObjectConstraintPanel, ConstraintButtonsSubPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bCameraSolverConstraint(ObjectConstraintPanel,
                                        ConstraintButtonsPanel,
                                        bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bChildOfConstraint(ObjectConstraintPanel,
                                   ConstraintButtonsPanel, bpy_types.Panel,
                                   bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bClampToConstraint(ObjectConstraintPanel,
                                   ConstraintButtonsPanel, bpy_types.Panel,
                                   bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bDampTrackConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bDistLimitConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bFollowPathConstraint(ObjectConstraintPanel,
                                      ConstraintButtonsPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bFollowTrackConstraint(ObjectConstraintPanel,
                                       ConstraintButtonsPanel, bpy_types.Panel,
                                       bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bKinematicConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bLocLimitConstraint(ObjectConstraintPanel,
                                    ConstraintButtonsPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bLocateLikeConstraint(ObjectConstraintPanel,
                                      ConstraintButtonsPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bLockTrackConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bMinMaxConstraint(ObjectConstraintPanel,
                                  ConstraintButtonsPanel, bpy_types.Panel,
                                  bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bObjectSolverConstraint(ObjectConstraintPanel,
                                        ConstraintButtonsPanel,
                                        bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bPivotConstraint(ObjectConstraintPanel, ConstraintButtonsPanel,
                                 bpy_types.Panel, bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bPythonConstraint(ObjectConstraintPanel,
                                  ConstraintButtonsPanel, bpy_types.Panel,
                                  bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bRotLimitConstraint(ObjectConstraintPanel,
                                    ConstraintButtonsPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bRotateLikeConstraint(ObjectConstraintPanel,
                                      ConstraintButtonsPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bSameVolumeConstraint(ObjectConstraintPanel,
                                      ConstraintButtonsPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bShrinkwrapConstraint(ObjectConstraintPanel,
                                      ConstraintButtonsPanel, bpy_types.Panel,
                                      bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bSizeLikeConstraint(ObjectConstraintPanel,
                                    ConstraintButtonsPanel, bpy_types.Panel,
                                    bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bSizeLimitConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bStretchToConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTrackToConstraint(ObjectConstraintPanel,
                                   ConstraintButtonsPanel, bpy_types.Panel,
                                   bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTransLikeConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTransformCacheConstraint(
        ObjectConstraintPanel, ConstraintButtonsPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTransformConstraint(ObjectConstraintPanel,
                                     ConstraintButtonsPanel, bpy_types.Panel,
                                     bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action(self, context):
        ''' 

        '''
        pass

    def draw_armature(self, context):
        ''' 

        '''
        pass

    def draw_camera_solver(self, context):
        ''' 

        '''
        pass

    def draw_childof(self, context):
        ''' 

        '''
        pass

    def draw_clamp_to(self, context):
        ''' 

        '''
        pass

    def draw_damp_track(self, context):
        ''' 

        '''
        pass

    def draw_dist_limit(self, context):
        ''' 

        '''
        pass

    def draw_follow_path(self, context):
        ''' 

        '''
        pass

    def draw_follow_track(self, context):
        ''' 

        '''
        pass

    def draw_header(self, context):
        ''' 

        '''
        pass

    def draw_influence(self, layout, con):
        ''' 

        '''
        pass

    def draw_kinematic(self, context):
        ''' 

        '''
        pass

    def draw_loc_limit(self, context):
        ''' 

        '''
        pass

    def draw_locate_like(self, context):
        ''' 

        '''
        pass

    def draw_lock_track(self, context):
        ''' 

        '''
        pass

    def draw_min_max(self, context):
        ''' 

        '''
        pass

    def draw_object_solver(self, context):
        ''' 

        '''
        pass

    def draw_pivot(self, context):
        ''' 

        '''
        pass

    def draw_python_constraint(self, context):
        ''' 

        '''
        pass

    def draw_rot_limit(self, context):
        ''' 

        '''
        pass

    def draw_rotate_like(self, context):
        ''' 

        '''
        pass

    def draw_same_volume(self, context):
        ''' 

        '''
        pass

    def draw_shrinkwrap(self, context):
        ''' 

        '''
        pass

    def draw_size_like(self, context):
        ''' 

        '''
        pass

    def draw_size_limit(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik(self, context):
        ''' 

        '''
        pass

    def draw_stretch_to(self, context):
        ''' 

        '''
        pass

    def draw_trackto(self, context):
        ''' 

        '''
        pass

    def draw_trans_like(self, context):
        ''' 

        '''
        pass

    def draw_transform(self, context):
        ''' 

        '''
        pass

    def draw_transform_cache(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def space_template(self, layout, con, target, owner, separator):
        ''' 

        '''
        pass

    def target_template(self, layout, con, subtargets):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTransformConstraint_destination(
        ObjectConstraintPanel, ConstraintButtonsSubPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_bTransformConstraint_source(
        ObjectConstraintPanel, ConstraintButtonsSubPanel, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_parent_id = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_action_action(self, context):
        ''' 

        '''
        pass

    def draw_action_target(self, context):
        ''' 

        '''
        pass

    def draw_armature_bones(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_chain_scaling(self, context):
        ''' 

        '''
        pass

    def draw_spline_ik_fitting(self, context):
        ''' 

        '''
        pass

    def draw_transform_from(self, context):
        ''' 

        '''
        pass

    def draw_transform_to(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def get_constraint(self, context):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class OBJECT_PT_constraints(ObjectConstraintPanel, bpy_types.Panel,
                            bpy_types._GenericUI):
    bl_context = None
    ''' '''

    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def poll(self, context):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass
