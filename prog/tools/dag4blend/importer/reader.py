import struct
from mathutils          import Matrix, Vector

from ..face             import FaceExp
from ..node             import NodeExp

from ..popup.popup_functions    import show_popup
from ..helpers.texts    import log

class DagChunk:
    def __init__(self):
        self.size = 0
        self.id = 0


class DagReader:
    file = None
    bo = '<'
    eof: bool

    def readByte(self):
        bytes = self.file.read(1)
        if len(bytes) < 1:
            self.eof = True
            return 0
        return struct.unpack("B", bytes)[0]

    def readDWord(self):
        bytes = self.file.read(4)
        if len(bytes) < 4:
            self.eof = True
            return 0
        return struct.unpack(self.bo + "I", bytes)[0]

    def readInt(self):
        bytes = self.file.read(4)
        if len(bytes) < 4:
            self.eof = True
            return 0
        return struct.unpack(self.bo + "i", bytes)[0]

    def readUInt(self):
        return self.readDWord()

    def readFloat(self):
        bytes = self.file.read(4)
        if len(bytes) < 4:
            self.eof = True
            return 0
        return struct.unpack(self.bo + "f", bytes)[0]

    def readUShort(self):
        bytes = self.file.read(2)
        if len(bytes) < 2:
            self.eof = True
            return 0
        return struct.unpack(self.bo + "H", bytes)[0]

    def readStr(self, length):
        res = self.file.read(length)
        if len(res) < length:
            self.eof = True
            return ""
        try:
            return bytes.decode(res, 'ascii')
        except UnicodeDecodeError:
            result = bytes.decode(res, 'Windows-1251')
            msg=f"String \n{result}\ncontains Cyrillic characters please avoid using it"
            log(f"{msg}\n",type = 'ERROR', show = True)
            return result

    def readDAGString(self):
        len = self.readByte()
        if self.eof:
            return ""
        return self.readStr(len)

    def readColor(self):
        return [self.readByte() / 255.0, self.readByte() / 255.0, self.readByte() / 255.0]

    def readMatrix4x3(self):
        res = Matrix()
        res[0].x = self.readFloat()
        res[0].y = self.readFloat()
        res[0].z = self.readFloat()
        res[1].x = self.readFloat()
        res[1].y = self.readFloat()
        res[1].z = self.readFloat()
        res[2].x = self.readFloat()
        res[2].y = self.readFloat()
        res[2].z = self.readFloat()
        res[3].x = self.readFloat()
        res[3].y = self.readFloat()
        res[3].z = self.readFloat()
        return res

    def readVertex(self):
        return (self.readFloat(), self.readFloat(), self.readFloat())

    def readVector(self):
        return [self.readFloat(), self.readFloat(), self.readFloat()]

    def readIndices(self):
        return [self.readDWord(), self.readDWord(), self.readDWord()]

    def readBigFace(self):
        f = FaceExp()
        f.f.append(self.readUInt())
        f.f.append(self.readUInt())
        f.f.append(self.readUInt())
        f.smgr = self.readUInt()
        f.mat = self.readUShort()
        return f

    def readFace(self):
        f = FaceExp()
        f.f.append(self.readUShort())
        f.f.append(self.readUShort())
        f.f.append(self.readUShort())
        f.smgr = self.readUInt()
        f.mat = self.readUShort()
        return f

    def readHeader(self, id):
        self.readDWord(id)

    def open(self, filename):
        self.file = open(filename, "rb")
        self.eof = False

    def beginChunk(self):
        chunk = DagChunk()
        chunk.size = self.readDWord()
        if self.eof:
            return None
        chunk.size -= 4
        chunk.id = self.readDWord()
        return chunk

    def endChunk(self, size):
        self.file.seek(size, 1)

    def close(self):
        self.file.close()

    def readDagNodeData(self, size):
        pos = self.file.tell()
        node = NodeExp()
        node.id = self.readUShort()
        self.readUShort()
        node.objFlg = self.readDWord()
        len = size + pos - self.file.tell()
        if len > 0:
            if len > 255:
                len = 255
            node.name = self.readStr(len)
        return node
