import struct
from ..helpers.texts    import get_text

class DagWriter:
    file = None
    bo = '<'

    def writeByte(self, value):
        self.file.write(struct.pack(self.bo + "B", value))

    def writeDWord(self, value):
        self.file.write(struct.pack(self.bo + "I", value))

    def writeInt(self, value):
        self.file.write(struct.pack(self.bo + "i", value))

    def writeUInt(self, value):
        self.file.write(struct.pack(self.bo + "I", value))

    def writeFloat(self, value):
        ba = bytearray(struct.pack(self.bo + "f", value))
        self.file.write(ba)

    def writeUShort(self, value):
        self.file.write(struct.pack(self.bo + "H", value))

    def writeStr(self, value):
        try:
            string = str.encode(value, 'ascii')
            err = None
        except UnicodeEncodeError:
            string = str.encode(value, 'Windows-1251')
            err=f'\nERROR! "String {value} contains Cyrillic characters please avoid using it"\n'
        self.file.write(string)
        return err

    def writeDAGString(self, value):
        self.writeByte(min(255, len(value)))
        try:
            string = str.encode(value[0:255], 'ascii')
            err = None
        except UnicodeEncodeError:
            string = str.encode(value[0:255], 'Windows-1251')
            err=f'\nERROR! "String {value} contains Cyrillic characters please avoid using it"\n'
        self.file.write(string)
        return err

    def writeColor(self, c):
        self.writeByte(int(c[0] * 255.0))
        self.writeByte(int(c[1] * 255.0))
        self.writeByte(int(c[2] * 255.0))

    def writeMatrix4x3(self, m):
        self.writeFloat(m[0].x)
        self.writeFloat(m[0].y)
        self.writeFloat(m[0].z)
        self.writeFloat(m[1].x)
        self.writeFloat(m[1].y)
        self.writeFloat(m[1].z)
        self.writeFloat(m[2].x)
        self.writeFloat(m[2].y)
        self.writeFloat(m[2].z)
        self.writeFloat(m[3].x)
        self.writeFloat(m[3].y)
        self.writeFloat(m[3].z)

    def writeVertex(self, v):
        self.writeFloat(v.x)
        self.writeFloat(v.y)
        self.writeFloat(v.z)

    def writeBigFace(self, indices, smgr, mat):
        self.writeUInt(indices[0])
        self.writeUInt(indices[1])
        self.writeUInt(indices[2])
        self.writeUInt(smgr)
        self.writeUShort(mat)

    def writeFace(self, indices, smgr, mat):
        self.writeUShort(indices[0])
        self.writeUShort(indices[1])
        self.writeUShort(indices[2])
        self.writeUInt(smgr)
        self.writeUShort(mat)

    def writeHeader(self, id):
        self.writeDWord(id)

    def open(self, filename):
        self.file = open(filename, "wb")

    def beginChunk(self, chunkId):
        beginChunkPos = self.file.tell()
        chunkSize = 0
        self.writeDWord(chunkSize)
        self.writeDWord(chunkId)
        return beginChunkPos

    def endChunk(self, beginChunkPos):
        pos = self.file.tell()
        self.file.seek(beginChunkPos)
        self.writeDWord(pos - beginChunkPos - 4)
        self.file.seek(pos)

    def close(self):
        self.file.close()

    def writeDagNodeData(self, id, cnum, flg):
        self.writeUShort(id)
        self.writeUShort(cnum)
        self.writeDWord(flg)
