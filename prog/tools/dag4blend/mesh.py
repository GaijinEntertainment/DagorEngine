class MeshExp:
    def __init__(self):
        self.vertices = list()
        self.faces = list()
        self.uv = list()
        self.uv_poly = list()
        self.color_attributes = dict()
        self.color_attributes_poly = list()
        self.normals = list()

        self.normals_ver = dict()
        self.normals_faces = list()

        self.normals_ver_list = list()

    def addNormal(self, normal):
        n = normal.copy()
        n = n.freeze()
        if n in self.normals_ver:
            return self.normals_ver.get(n)
        self.normals_ver[n] = len(self.normals_ver)
        return len(self.normals_ver) - 1

    def addUV(self, uv, i):
        uv = uv.copy()
        uv = uv.freeze()
        if uv in self.uv[i]:
            return self.uv[i].get(uv)
        self.uv[i][uv] = len(self.uv[i])
        return len(self.uv[i]) - 1

    def addVertexColor(self, color):
        if color in self.color_attributes:
            return self.color_attributes.get(color)
        self.color_attributes[color] = len(self.color_attributes)
        return len(self.color_attributes) - 1
