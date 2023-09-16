
from bpy.types import NodeInputs


class Material:
    mat = None
    name: str
    ambient = []
    diffuse = []
    specular = []
    emissive = []
    specular_power: float
    tex = None
    shader: str
    script: str
    flags = 0

    def __eq__(self, other):
        return self.name == other.name and self.ambient == other.ambient and self.diffuse == other.diffuse and self.specular == other.specular and self.emissive == other.emissive and self.specular_power == other.specular_power and self.shader == other.shader and self.script == other.script and self.tex == other.tex and self.flags == other.flags