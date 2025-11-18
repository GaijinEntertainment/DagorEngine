import bpy
from bpy.types  import Operator, Panel
from bpy.props  import IntProperty, StringProperty
from bpy.utils  import register_class, unregister_class
from os.path    import dirname, isfile, isdir

from ..helpers.getters   import get_local_props, get_preferences
from ..ui.draw_elements  import draw_custom_header

def is_dirpath_valid(dirpath):
    if dirpath == "":
        return False
    return isdir(dirpath)

def is_filepath_valid(filepath):
    if not filepath.endswith('.dag'):
        return False
    return isfile(filepath)

def get_includes():
    props = get_local_props()
    keys = props.importer.includes.keys()
    values = [props.importer.includes[key] for key in keys]
    values = [value for value in values if value != ""]
    if values.__len__() == 0:
        return "*"
    return ";".join(values)

def get_excludes():
    props = get_local_props()
    keys = props.importer.excludes.keys()
    values = [props.importer.excludes[key] for key in keys]
    values = [value for value in values if value != ""]
    return ";".join(values)

def get_includes_re():
    props = get_local_props()
    keys = props.importer.includes_re.keys()
    values = [props.importer.includes_re[key] for key in keys]
    values = [value for value in values if value != ""]
    if values.__len__() ==0:
        return ".*"
    if values.__len__() == 1:
        return values[0]
    includes_re = "|".join(values)
    return f"({includes_re})"

def get_excludes_re():
    props = get_local_props()
    keys = props.importer.excludes_re.keys()
    values = [props.importer.excludes_re[key] for key in keys]
    values = [value for value in values if value != ""]
    if values.__len__() ==0:
        return "^$"
    if values.__len__() == 1:
        return values[0]
    excludes_re = "|".join(values)
    return f"({excludes_re})"

#classes

classes = []

class DAGOR_OT_Edit_Importer_Filter(Operator):
    bl_idname = 'dt.edit_importer_filter'
    bl_label = "Test"
    bl_description = "Adds or removes importer filter depending on context"

    index:  IntProperty(default = -1)
    value:  StringProperty(default = "")

    @classmethod
    def poll(self, context):
        if getattr(context, "prop_owner", None) is None:
            return False
        return True

    def execute(self, context):
        prop_owner = context.prop_owner
        props_amount = prop_owner.keys().__len__()
        if self.index < 0:
            name = str(props_amount)
            prop_owner[name] = self.value
            return {'FINISHED'}
        for index in range(self.index, props_amount-1):
            prop_owner[str(index)] = prop_owner[str(index+1)]
        del prop_owner[str(props_amount-1)]
        return {'FINISHED'}
classes.append(DAGOR_OT_Edit_Importer_Filter)

class DAGOR_PT_Import(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Batch import"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}

    def draw_header(self,context):
        layout = self.layout
        props = get_local_props()
        props_import = props.importer
        layout.prop(props_import, 'show_help', text = "", toggle = True, icon = 'QUESTION')
        return

    def draw_help(self, context, layout):
        layout = self.layout
        box = layout.box()
        props = get_local_props()
        mode = props.importer.mode
        draw_custom_header(box, "Help", props.importer, 'help_maximized', icon = 'QUESTION')
        if not props.importer.help_maximized:
            return
        if mode == 'SIMPLE':
            self.draw_help_simple(context, box)
        elif mode == 'WILDCARD':
            self.draw_help_wildcard(context, box)
        else:  # mode == 'WILDCARD'
            self.draw_help_regex(context, box)
        return

    def draw_help_simple(self, context, layout):
        col = layout.column(align = True)
        col.label(text = 'Import "*.dag"')
        col.label(text = 'and related variations')

#table
        col.separator()
        split = col.split(factor = 0.2, align = True)
        left = split.column(align = True)
        left.alignment = 'CENTER'
        right = split.column(align = True)
        box = left.box()
        box.label(text = "LODs")
        box = left.box()
        box.label(text = "DPs")
        box = left.box()
        box.label(text = "DMGs")
        box = left.box()
        box.label(text = "Destr")
        box = right.box()
        box.label(text = "*.lodNN.dag")
        box = right.box()
        box.label(text = "*_dp_NN.lodNN.dag")
        box = right.box()
        box.label(text = "*_dmg.lodNN.dag")
        box = right.box()
        box.label(text = "*_destr.lod00.dag")
        col.separator()
        return

    def draw_help_wildcard(self, context, layout):
        col = layout.column(align = True)
        col.label(text = 'Uses "fnmatch" library')
        col.label(text = 'to check asset names')
#table
        col.separator()
        split = col.split(factor = 0.2, align = True)
        left = split.column(align = True)
        left.alignment = 'CENTER'
        right = split.column(align = True)
        box = left.box()
        box.label(text = "*")
        box = left.box()
        box.label(text = "?")
        box = left.box()
        box.label(text = "[seq]")
        box = left.box()
        box.label(text = "[!seq]")
        box = right.box()
        box.label(text = "everything")
        box = right.box()
        box.label(text = "any single character")
        box = right.box()
        box.label(text = "any character in seq")
        box = right.box()
        box.label(text = "any character not in seq")
        col.separator()

        button = col.operator('wm.url_open', text = "Documentation")
        button.url = "https://docs.python.org/3/library/fnmatch.html"
        return

    def draw_help_regex(self, context, layout):
        col = layout.column(align = True)
        col.label(text = "Uses regular expressions")
        col.label(text = 'to check asset names')
        button = col.operator('wm.url_open', text = "Documentation")
        button.url = "https://docs.python.org/3/library/re.html#regular-expression-syntax"
        return

    def draw_filter(self, context, layout, prop_name, prop_owner):
        props = get_local_props()
        props_import = props.importer
        box = layout.box()
        row = box.row()
        row.prop(prop_owner, f'["{prop_name}"]', text = "",
            icon = "ERROR" if prop_owner[prop_name] == "" else 'NONE')
        row.context_pointer_set(name = "prop_owner", data = prop_owner)
        remove = row.operator('dt.edit_importer_filter', text = "", icon = 'TRASH')
        remove.index = int(prop_name)
        return

    def draw_filters(self, context, layout, prop_owner):
        column = layout.column(align = True)
        for key in prop_owner.keys():
            self.draw_filter(context, column, key, prop_owner)
        row = column.row()
        row.context_pointer_set(name = "prop_owner", data = prop_owner)
        add_button = row.operator('dt.edit_importer_filter', text = "Add", icon = 'ADD')
        add_button.index = -1
        return

    def draw_simple(self,context,layout):
        props = get_local_props()
        props_import = props.importer
        box = layout.box()

        toggles = box.column(align = True)

        row = toggles.row()
        row.prop(props_import, 'with_lods', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.with_lods else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'with_dps', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.with_dps else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'with_dmgs', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.with_dmgs else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'with_destr', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.with_destr else 'CHECKBOX_DEHLT')

        row = box.row()
        row.prop(props_import, "filepath")

        row = box.row()
        dirpath = dirname(props_import.filepath)
        row.active = row.enabled = is_dirpath_valid(dirpath)
        open_button = row.operator('wm.path_open', icon = 'FILE_FOLDER' if row.active else 'ERROR',
            text = 'open import directory')
        open_button.filepath = dirpath

        row = box.row()
        row.operator_context = 'EXEC_DEFAULT'  # No File/Import dialog, use sent params instead
        row.scale_y = 2
        row.active = row.enabled = is_filepath_valid(props_import.filepath)
        importer = row.operator('import_scene.dag', text='IMPORT', icon = 'IMPORT' if row.active else 'ERROR')

        importer.filepath = props_import.filepath
        importer.check_subdirs = props_import.check_subdirs
        importer.with_lods = props_import.with_lods
        importer.with_dps = props_import.with_dps
        importer.with_dmgs = props_import.with_dmgs
        importer.with_destr = props_import.with_destr

        importer.mopt = props_import.mopt
        importer.preserve_sg = props_import.preserve_sg
        importer.replace_existing = props_import.replace_existing
        importer.preserve_path = props_import.preserve_path
        return

    def draw_wildcard(self,context,layout):
        props = get_local_props()
        props_import = props.importer
        box = layout.box()
        row = box.row()
        row.prop(props_import, "dirpath")

        row = box.row()
        open_button = row.operator('wm.path_open', icon = 'FILE_FOLDER' if row.active else 'ERROR',
            text = 'open import directory')
        open_button.filepath = props_import.dirpath
        row.active = row.enabled = is_dirpath_valid(props_import.dirpath)

        filters = box.column(align = True)

        includes = filters.box()
        draw_custom_header(includes, "Includes", props_import, 'includes_maximized', icon = 'THREE_DOTS')
        if props_import.includes_maximized:
            self.draw_filters(context, includes, props_import.includes)

        excludes = filters.box()
        draw_custom_header(excludes, "Excludes", props_import, 'excludes_maximized', icon = 'THREE_DOTS')
        if props_import.excludes_maximized:
            self.draw_filters(context, excludes, props_import.excludes)

        row = box.row()
        row.operator_context = 'EXEC_DEFAULT'  # No File/Import dialog, use sent params instead
        row.scale_y = 2
        row.active = row.enabled = is_dirpath_valid(props_import.dirpath)
        importer = row.operator('import_scene.dag', text='IMPORT', icon = 'IMPORT' if row.active else 'ERROR')

        importer.dirpath = props_import.dirpath
        importer.check_subdirs = props_import.check_subdirs
        importer.includes = get_includes()
        importer.excludes = get_excludes()
        importer.includes_re = ""  # Blender tries to repeat last parameters if not specified on call
        importer.excludes_re = ""  # Blender tries to repeat last parameters if not specified on call
        importer.filepath = ""  # Blender tries to repeat last parameters if not specified on call

        importer.mopt = props_import.mopt
        importer.preserve_sg = props_import.preserve_sg
        importer.replace_existing = props_import.replace_existing
        importer.preserve_path = props_import.preserve_path
        return

    def draw_regex(self,context,layout):
        props = get_local_props()
        props_import = props.importer
        box = layout.box()
        row = box.row()
        row.prop(props_import, "dirpath")

        row = box.row()
        row.active = row.enabled = is_dirpath_valid(props_import.dirpath)
        open_button = row.operator('wm.path_open', icon = 'FILE_FOLDER' if row.active else 'ERROR',
            text = 'open import directory')
        open_button.filepath = props_import.dirpath

        filters = box.column(align = True)

        includes = filters.box()
        draw_custom_header(includes, "Includes", props_import, 'includes_re_maximized', icon = 'THREE_DOTS')
        if props_import.includes_re_maximized:
            self.draw_filters(context, includes, props_import.includes_re)

        excludes = filters.box()
        draw_custom_header(excludes, "Excludes", props_import, 'excludes_re_maximized', icon = 'THREE_DOTS')
        if props_import.excludes_re_maximized:
            self.draw_filters(context, excludes, props_import.excludes_re)

        row = box.row()
        row.operator_context = 'EXEC_DEFAULT'  # No File/Import dialog, use sent params instead
        row.scale_y = 2
        row.active = row.enabled = is_dirpath_valid(props_import.dirpath)
        importer = row.operator('import_scene.dag', text='IMPORT', icon = 'IMPORT' if row.active else 'ERROR')

        importer.dirpath = props_import.dirpath
        importer.check_subdirs = props_import.check_subdirs
        importer.includes_re = get_includes_re()
        importer.excludes_re = get_excludes_re()
        importer.includes = ""  # Blender tries to repeat last parameters if not specified on call
        importer.excludes = ""  # Blender tries to repeat last parameters if not specified on call
        importer.filepath = ""  # Blender tries to repeat last parameters if not specified on call

        importer.mopt = props_import.mopt
        importer.preserve_sg = props_import.preserve_sg
        importer.replace_existing = props_import.replace_existing
        importer.preserve_path = props_import.preserve_path
        return

    def draw_parameters(self,context,layout):
        props = get_local_props()
        prefs = get_preferences()
        props_import = props.importer
        box = layout.box()
        toggles = box.column(align = True)
        draw_custom_header(toggles, "Parameters", prefs, 'imp_props_maximized', icon = 'PROPERTIES')
        if prefs.imp_props_maximized:
            toggles.separator()

            row = toggles.row()
            row.prop(props_import, 'check_subdirs', toggle = True,
                icon = 'CHECKBOX_HLT' if props_import.check_subdirs else 'CHECKBOX_DEHLT')

            row = toggles.row()
            row.prop(props_import, 'mopt', toggle = True,
                icon = 'CHECKBOX_HLT' if props_import.mopt else 'CHECKBOX_DEHLT')

            row = toggles.row()
            row.prop(props_import, 'preserve_sg', toggle = True,
                icon = 'CHECKBOX_HLT' if props_import.preserve_sg else 'CHECKBOX_DEHLT')

            row = toggles.row()
            row.prop(props_import, 'replace_existing', toggle = True,
                icon = 'CHECKBOX_HLT' if props_import.replace_existing else 'CHECKBOX_DEHLT')

            row = toggles.row()
            row.prop(props_import, 'preserve_path', toggle = True,
                icon = 'CHECKBOX_HLT' if props_import.preserve_path else 'CHECKBOX_DEHLT')
        return

    def draw(self,context):
        props = get_local_props()
        props_import = props.importer
        layout = self.layout
        if props_import.show_help:
            self.draw_help(context, layout)
        self.draw_parameters(context, layout)
        column = layout.column(align = True)
        mode = column.row(align = True)
        mode.prop(props_import, 'mode', expand = True)
        if props_import.mode == 'SIMPLE':
            self.draw_simple(context, column)
        elif props_import.mode == 'WILDCARD':
            self.draw_wildcard(context, column)
        else:  # props_import.mode == 'REGEX'
            self.draw_regex(context, column)
        return
classes.append(DAGOR_PT_Import)

def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        unregister_class(cl)
    return
