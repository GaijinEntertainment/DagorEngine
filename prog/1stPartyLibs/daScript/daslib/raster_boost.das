module raster_boost private

require math
require raster public

bitfield RasterFlags
    column_major

struct public SurfaceData
    //! permanent surface data, can be moved context to context
    data : uint8?
    size : uint64
    flags : RasterFlags

def public bitmap8_size ( size:int2 )
    return uint64(size.x*size.y)+1024ul

[unsafe_operation]
def public SurfaceData ( size:uint64 )
    //! allocate surface data
    unsafe
        return [[SurfaceData data=unsafe(reinterpret<uint8?> malloc(size)), size=size]]

def public finalize ( var data:SurfaceData )
    //! free surface data
    if data.data != null
        unsafe
            free(data.data)
            data.data = null

def public make_bitmap8 ( var surface:SurfaceData; size:int2; blk:block<(var arg:Bitmap8#):void> ) : void
    //! create a temporary bitmap8 out of surface data
    unsafe
        let numpix = size.x*size.y
        var bmp : Bitmap8
        bmp.size = size
        _builtin_make_temp_array(bmp.data, surface.data, numpix)
        _builtin_make_temp_array(bmp.palette, surface.data+numpix, 256)
        __builtin_array_lock(bmp.data)
        __builtin_array_lock(bmp.palette)
        blk |> invoke(reinterpret<Bitmap8#> bmp)
        surface.flags = bmp.flags
        __builtin_array_unlock(bmp.data)
        __builtin_array_unlock(bmp.palette)

struct public Bitmap8
    //! single surface
    size : int2
    flags : RasterFlags
    data : array<uint8>
    palette : array<uint>

def public lock_u8 ( dim:auto[]; blk:block<(var arg:array<uint8>#):void> )
    //! lock a 2D array of uint8
    let num_elem = typeinfo(sizeof dim)
    unsafe
        map_to_array(addr(dim[0]), num_elem, blk)

[sideeffects,jit,hint(unsafe_range_check,noalias=bmp)]
def public rast_fillRect ( var bmp:Bitmap8#; x,y,w,h:int; color:uint8 )
    //! fill a rectangle with a color
    let xmin = max(0,x)
    let xmax = min(bmp.size.x,x+w)
    if xmax <= xmin
        return
    let ymin = max(0,y)
    let ymax = min(bmp.size.y,y+h)
    if ymax <= ymin
        return
    if bmp.flags.column_major
        let H = ymax-ymin
        for X in range(xmin,xmax)
            unsafe
                memset8(addr(bmp.data[X*bmp.size.y+y]),color,H)
    else
        let W = xmax-xmin
        for Y in range(ymin,ymax)
            unsafe
                memset8(addr(bmp.data[Y*bmp.size.x+x]),color,W)
