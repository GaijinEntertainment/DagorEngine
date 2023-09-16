class NodeExp:
    def __init__(self):
        self.name = ""
        self.id = -1
        self.parent_id = -1
        self.children = list()
        self.mesh = None
        self.curves = None
        self.tm = None
        self.wtm = None
        self.renderable = True
        self.is_tmp_mesh = False
        self.materials_indices = list()
        self.objProps = ""
        self.objFlg = 0

        self.subMat = list()
