class FaceExp:
    def __init__(self):
        self.f = list()
        self.smgr = 0
        self.mat = 0
        self.flg = None

        self.neighbors = [[-1, -1, 0], [-1, -1, 0], [-1, -1, 0]]  # (face no., index no., been there)
