
                             Q3BSP v1.0


                                by

                           John W. Ratcliff
                          jratcliff@verant.com

                      Open Sourced December 5, 2000
                           Merry Christmas

Q3BSP is a Windows console application that will convert any valid
Quake 3 BSP file into an organized polygon mesh and export the results
into a standardized ASCII file format, VRML 1.0.  I encourage users of
this software to expand this tool to export in other popular ASCII file
formats, especially ones which support multiple U/V channels.


This utility will convert a Quake3 BSP into a valid polygon mesh and
output the results into two seperate VRML 1.0 files.  The first VRML
file contains all the U/V mapping and texture mapping information for
channel #1, and the second VRML file will contain all of the U/V
mapping and texture names for the second U/V channel, which contains
all lightmap information.  You can then directly import these files
into any number of 3d editing tools, including 3d Studio Max

This tool also extracts the lightmap data and saves it out as a
series of .BMP files.

Contact Id Software about using Quake 3 data files and Quake 3 editing
tools for commercial software development projects.

This tool will parse Quake 3 shader files to find the base texture name
used.  I have not included the Quake 3 shader files with this build, but
you can copy them into the working directory yourself.

This source code makes heavy use of C++ and STL.  It is mostly OS
neutral and should compile fairly easily on any system, with the exception
of the utility LOADBMP.CPP which is Windows specific.


This project was created with Microsoft Developer Studio 6.0 and all
you should have to do is load the workspace and compile it.

The files included in this project are:


arglist.h         Utility class to parse a string into a series of
arglist.cpp       arguments.

fload.h           Utility class to load a file from disk into memory.
fload.cpp

loadbmp.h         Utility class to load and save .BMP files.  This is
loadbmp.cpp       OS specific to Windows.

main.cpp          Main console application.

patch.h           Converts a Quake 3 Bezier patch into a set of
patch.cpp         triangles.

plane.h           Simple representation of a plane equation.

q3bsp.h           Class to load a Quake 3 BSP file
q3bsp.cpp

q3bsp.dsp         Dev Studio Project file
q3bsp.dsw         Dev Studio Workspace

q3def.h           Data definitions for Quake 3 data structures.

q3shader.h        Utility to parse Quake 3 shader files
q3shader.cpp

rect.h            Simple template class to represent an axis aligned
                  bounding region.

stable.h          Simple class to maintain a set of ascii strings with
                  no duplications.

stl.h             Includes common STL header files.

stringdict.h      Application global string table.
stringdict.cpp

vector.h          Simple template class to represent a 3d data point.

vformat.h         Class to create an organized mesh from a polygon soup.
vformat.cpp       Also saves output into VRML 1.0

