// Copyright 2006-2015 Tobias Sargeant (tobias.sargeant@gmail.com).
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <carve/carve.hpp>

#include <carve/pointset.hpp>
#include <carve/poly.hpp>
#include <carve/polyline.hpp>

#include <fstream>
#include <ostream>

void writePLY(std::ostream& out, const carve::mesh::MeshSet<3>* poly,
              bool ascii = false);
void writePLY(const std::string& out_file, const carve::mesh::MeshSet<3>* poly,
              bool ascii = false);

void writePLY(std::ostream& out, const carve::poly::Polyhedron* poly,
              bool ascii = false);
void writePLY(const std::string& out_file, const carve::poly::Polyhedron* poly,
              bool ascii = false);

void writePLY(std::ostream& out, const carve::line::PolylineSet* lines,
              bool ascii = false);
void writePLY(const std::string& out_file,
              const carve::line::PolylineSet* lines, bool ascii = false);

void writePLY(std::ostream& out, const carve::point::PointSet* points,
              bool ascii = false);
void writePLY(const std::string& out_file, const carve::point::PointSet* points,
              bool ascii = false);

void writeOBJ(std::ostream& out, const carve::mesh::MeshSet<3>* poly);
void writeOBJ(const std::string& out_file, const carve::mesh::MeshSet<3>* poly);

void writeOBJ(std::ostream& out, const carve::poly::Polyhedron* poly);
void writeOBJ(const std::string& out_file, const carve::poly::Polyhedron* poly);

void writeOBJ(std::ostream& out, const carve::line::PolylineSet* lines);
void writeOBJ(const std::string& out_file,
              const carve::line::PolylineSet* lines);

void writeVTK(std::ostream& out, const carve::mesh::MeshSet<3>* poly);
void writeVTK(const std::string& out_file, const carve::mesh::MeshSet<3>* poly);

void writeVTK(std::ostream& out, const carve::poly::Polyhedron* poly);
void writeVTK(const std::string& out_file, const carve::poly::Polyhedron* poly);

void writeVTK(std::ostream& out, const carve::line::PolylineSet* lines);
void writeVTK(const std::string& out_file,
              const carve::line::PolylineSet* lines);
