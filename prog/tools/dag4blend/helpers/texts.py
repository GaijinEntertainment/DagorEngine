import bpy

#moves cursor to the end of the text
def upd_text_cursor(text):
    last_line = text.lines.__len__()-1
    last_char = text.lines[last_line].body.__len__()
    text.select_set(last_line,last_char,last_line,last_char)
    return

#returns the empty text object by name
def get_text_clear(text):
    if bpy.data.texts.get(text) is None:
        bpy.data.texts.new(name=text)
    text=bpy.data.texts[text]
    text.clear()
    return text

#returns text object by name, keep lines if exists
#moves cursor to the end
def get_text(text):
    if bpy.data.texts.get(text) is None:
        bpy.data.texts.new(name=text)
    text=bpy.data.texts[text]
    upd_text_cursor(text)
    return text

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
    with bpy.context.temp_override(area=owner):
        bpy.ops.screen.area_split( direction=dir, factor=fac)
    return areas[-1]

#shows text object in last found text_editor area
def show_text(text):
    text_area = get_area_by_type('TEXT_EDITOR')
    if text_area is None:
        text_area = add_area()
        text_area.type = 'TEXT_EDITOR'
    text_area.spaces[0].text = text
    return

def log(str, type = 'INFO', show = False):
    if str == "":
        return
    T = get_text("log")
    T.write(f"'{type}': {str}")
    if show:
        show_text(T)
    return