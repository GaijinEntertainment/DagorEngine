struct Box
    min, max : float2

def operator . center ( var self:Box )
    return (self.min + self.max) * 0.5

def operator . size ( var self:Box )
    return self.max - self.min

def operator . center := ( var self:Box; at:float2 )
    var size = self.size
    self.min = at - size * 0.5
    self.max = at + size * 0.5

[export]
def main
    var box = [[Box min=float2(-1,-1), max=float2(1,1)]]
    print("box = {box}\n")
    print("box.center = {box.center}\n")
    box.center := float2(2,2)
    print("box = {box}\n")
    print("box.center = {box.center}\n")


