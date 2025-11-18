from ..ui.draw_elements         import draw_custom_header
from ..helpers.getters          import get_preferences
from os.path                    import exists


def is_project_configured(project):
    if project.name.replace(" ", "") == "":
        return False
    if not exists(project.path):
        return False
    if project.palette_global.replace(" ", "") == "":
        return False
    if not exists(project.palette_global):
        return False
    return True

def draw_project_manager(layout):
    pref = get_preferences()
    box = layout.box()
    header = draw_custom_header(box,"Projects", pref, 'projects_maximized', icon = 'SCENE_DATA')
    if not pref.projects_maximized:
        return
    if pref.projects.__len__() == 0:
        error_box = box.box()
        error_box.label(text = "NO PROJECTS FOUND!", icon = 'ERROR')
        error_box.label(text = "Please, configure at least one to use add-on", icon = 'ERROR')
    big_button = box.row()
    big_button.scale_y = 2
    big_button.operator('dt.add_project', text = "Add Project", icon = 'ADD')
    index = 0
    for project in pref.projects:
        draw_project_settings(box, project, index)
        index += 1
    return

def draw_project_settings(layout, project, index):
    pref = get_preferences()
    box = layout.box()
    named = project.name.replace(' ', '') != ""
    configured = is_project_configured(project)
    row = box.row(align = True)
    column = row.column()  # restores rounding of the unlock button
    column.prop(project, 'unlock', text = "", icon = 'UNLOCKED' if project.unlock else 'LOCKED')
    header = draw_custom_header(row, project.name if named else "NO NAME",
     project, "maximized", icon = 'ERROR' if not configured else 'BLANK1')
    del_button = header.column()
    del_button.operator('dt.remove_project', icon = 'TRASH', text = "").index = index
    del_button.active = del_button.enabled = project.unlock
    if not project.maximized:
        return
    main_props = box.split(factor = 0.2)
    prop_names = main_props.column(align = True)
    prop_values = main_props.column(align = True)
    prop_names.label(text = "Name:", icon = 'ERROR' if not named else 'NONE')
    prop_values.prop(project, 'name', text = "")
    prop_names.label(text = "Path:", icon = 'ERROR' if project.path == "" or not exists(project.path) else 'NONE')
    prop_values.prop(project, 'path', text = "")
    prop_names.label(text = "Shading:")
    shading_row = prop_values.row(align = True)
    shading_row.prop_tabs_enum(project, 'mode')
    prop_values.active = prop_values.enabled = project.unlock
    draw_project_palettes(box, project)
    box.operator('wm.save_userpref')
    return

def draw_project_palettes(layout, project):
    box = layout.box()
    draw_custom_header(box, "Palettes", project, 'palettes_maximized', icon = 'IMAGE')
    if not project.palettes_maximized:
        return
    paths = box.column(align = True)
    paths.prop(project, 'palette_global', text = "",
        icon = 'IMAGE' if exists(project.palette_global) else 'ERROR')
    paths.prop(project, 'palette_local', text = "",
        icon = 'IMAGE' if exists(project.palette_local) else 'ERROR')
    paths.active = paths.enabled = project.unlock
    return