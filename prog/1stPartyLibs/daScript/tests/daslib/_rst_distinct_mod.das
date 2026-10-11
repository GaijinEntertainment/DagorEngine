options gen2
options no_aot

module _rst_distinct_mod public

typedef distinct RstMeters = float //! Distance along a path, in meters.
typedef private distinct RstHidden = int

class RstBase {
    tag : int
}

class RstCamera : RstBase {
    pos : float3
    def RstCamera(p : float3, r : float4) {
        //! Places the camera at `p`, facing `r`.
        pos = p
    }
    def RstCamera(p : float3) {
        //! Places the camera at `p` with no rotation.
        pos = p
    }
}

class RstPlain {
    x : int
}

struct RstPoint {
    x : int
    def RstPoint(v : int) {
        //! Places the point at `v`.
        x = v
    }
}

def rst_walk(m : RstMeters) : RstMeters {
    //! Walks a distance.
    return m
}

def rst_peek(h : RstHidden) : RstHidden {
    //! Passes a hidden value through.
    return h
}
