#build with dsc2-compute-build.bat (if needed)
#for .exe building, PythonWin required (http://sourceforge.net/projects/pywin32/)

import os
import sys
import struct
import argparse
import subprocess

# parsing args

parser = argparse.ArgumentParser( description="compute shaders binary packer" )
parser.add_argument( "in_list_file", type=str, action="store" )
parser.add_argument( "out_obj_path", type=str, action="store" )
parser.add_argument( "out_bin_path", type=str, action="store" )
parser.add_argument( "platform", type=str, action="store" )
parser.add_argument( "cache_path", type=str, default="", nargs='?', action="store" ) #ps4 ast cache path

args = parser.parse_args()

# prepare out paths

if not os.path.exists( args.out_obj_path ) :
  os.makedirs( args.out_obj_path )

if not os.path.exists( os.path.dirname( args.out_bin_path ) ) :
  os.makedirs( os.path.dirname( args.out_bin_path ) )

# utils
def get_base_fname( f ) :
  return os.path.splitext( os.path.basename( f ) )[0]  

def get_full_obj_fname( f ) : 
  return os.path.join( args.out_obj_path, get_base_fname( f ) + ".obj" )

# input list

input_list_f = open( args.in_list_file, "r" )
input_list = input_list_f.read().splitlines()
input_list_f.close()

def compile_dx11( f ):
  res = subprocess.call(
   [ os.path.join( os.environ[ "GDEVTOOL" ], "dx11.sdk.win8.0/bin/x86/fxc.exe" ),
    "/Tcs_5_0",
    "/nologo",
    "/Fo",
    get_full_obj_fname( f ),
    f ] )
  if res != 0:
    print( "compilation error, file:" + file_base )
    sys.exit( 1 )

def compile_ps4( f ):

  psslc = os.path.join( os.environ[ "GDEVTOOL"], "ps4.sdk.170/host_tools/bin/orbis-psslc.exe" )

  if not args.cache_path == "" :
    if not os.path.exists(args.cache_path) :
      os.makedirs(args.cache_path)

  cmdl = [psslc, "-profile", "sce_cs_orbis", "-o", get_full_obj_fname(f)]
  if not args.cache_path == "" :
    cmdl = cmdl + ["-cachedir", args.cache_path, "-cache-ast"]

  res = subprocess.call(cmdl + [f])
  if res != 0:
    print( "compilation error, file:" + file_base )
    sys.exit( 1 )

# compile obj

for f in input_list :
  file_base = os.path.basename( f )
  out_file_base = get_base_fname( f ) + ".obj"

  if args.platform == "dx11" :
    print( "building dx11: " + file_base )
    compile_dx11( f )
  if args.platform == "ps4" :
    print( "building ps4: " + file_base )
    compile_ps4( f )
  else :
    print( "unknown platform" )
    sys.exit( 1 );

# pack binary

bin_offset = 0

res_file_n = args.out_bin_path + ".compute." + args.platform + ".bin"
res_file = open( res_file_n, "wb" )
res_file.write( "GJCS" )
bin_offset += 4

res_file.write( struct.pack( "<i", len( input_list ) ) )
bin_offset += 4

for f in input_list :
  bin_offset += len( get_base_fname( f ) ) + 1 + 4 # 1 for \0, 4 for offset to binary

for f in input_list :
  res_file.write( get_base_fname( f ) + '\0' )
  res_file.write( struct.pack( "<i", bin_offset ) )
  bin_offset += os.path.getsize( get_full_obj_fname( f ) ) + 4 # 4 - size

for f in input_list :
  res_file.write( struct.pack( "<i", os.path.getsize( get_full_obj_fname( f ) ) ) )
  obj_file = open( get_full_obj_fname( f ), "rb" )
  res_file.write( obj_file.read() )
  obj_file.close()

res_file.flush()
res_file.close()

print( "output binary: " + res_file_n )