import bpy
from   bpy.types    import Operator
from   bpy.props    import StringProperty,BoolProperty

from  .texts        import *

#returns screen.area of needed type (if it exists)
def get_area_by_type(type='TEXT_EDITOR'):
    found = None
    for area in bpy.context.screen.areas:
        if area.type == type:
            found = area
    return found

#retutns new area by splitting the "owner"
def add_area(owner = None, dir = 'VERTICAL', fac = 0.3):
    areas = bpy.context.screen.areas
    if owner is None:
        for area in areas:
            if area.type == 'VIEW_3D':
                owner = area
                break
    if owner is None:
        owner = bpy.context.area
    with bpy.context.temp_override(area = owner):
        bpy.ops.screen.area_split(direction=dir, factor=fac)
    return areas[-1]

#shows the text/info log
def show_log_area(text=False,info=False):
    text_area = None if not text else get_area_by_type('TEXT_EDITOR')
    info_area = None if not info else get_area_by_type('INFO')
    areas = bpy.context.screen.areas
    if text and info:
        if text_area is None and info_area is None:
            text_area = add_area()
            info_area = add_area(text_area, 'HORIZONTAL', 0.5)
        elif text_area is None:
            text_area = add_area(info_area, 'HORIZONTAL', 0.5)
        elif info_area is None:
            info_area = add_area(text_area, 'HORIZONTAL', 0.5)
        text_area.type = 'TEXT_EDITOR'
        text_area.spaces[0].text = get_text("log")
        info_area.type = 'INFO'
    elif text:
        if text_area is None:
            text_area = add_area()
        text_area.type = 'TEXT_EDITOR'
        text_area.spaces[0].text = get_text("log")
    elif info:
        if info_area is None:
            info_area = add_area()
            info_area.type = 'INFO'
    return

#updates text log/ info area when needed
def redraw_info(text = False, info = False):
    areas = bpy.context.screen.areas
    if info:
        for area in areas:
            if area.type=="INFO":
                area.tag_redraw()
    if text:
        for area in areas:
            if area.type=="TEXT_EDITOR":
                area.spaces[0].text = get_text("log")
                area.tag_redraw()
                return#no need to replace all texts
    return

class DAGOR_OT_Log(Operator):
    bl_idname = "dt.log"
    bl_label = "log"
    bl_description = "Writes info to log"

    str:        StringProperty(default = "")
    type:       StringProperty(default = 'INFO')
    to_text:    BoolProperty(default = True)
    to_info:    BoolProperty(default = True)

    def execute(self, context):
        if self.to_text:
            T = get_text("log")
            upd_text_cursor(T)
            T.write(f"'{self.type}': {self.str}\n")
        if self.to_info:
            if self.type in ['INFO', 'OPERATOR', 'PROPERTY', 'WARNING', 'ERROR']:#supported types
                self.report({self.type}, self.str)
            else:
                self.report({'ERROR'}, "incorrect log call!")
        show_log_area(text=self.to_text,info=self.to_info)
        redraw_info(self.to_text, self.to_info)
        return{'FINISHED'}

classes = [DAGOR_OT_Log]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    return

def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)
    return

if __name__ == "__main__":
    register()