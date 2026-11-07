options gen2

require material
require csg

struct Material {
    row : int
}

def material(id : int; name, shader : string) : Material => Material(row = material_add(id, name, shader))
def tex(m : Material; slot : int; name : string) : Material => Material(row = material_tex(m.row, slot, name))
def script(m : Material; line : string) : Material => Material(row = material_script(m.row, line))
def two_sided(m : Material) : Material => Material(row = material_two_sided(m.row))
def look(m : Material; name : string) : Material => Material(row = material_look(m.row, name))
def diffuse(m : Material; r, g, b : float) : Material => Material(row = material_diffuse(m.row, r, g, b))

def mat(a : Solid; name : string) : Solid => Solid(id = material_mat(a.id, name))
