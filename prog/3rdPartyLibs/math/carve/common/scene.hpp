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

#include <carve/geom3d.hpp>

#include <gloop/gl.hpp>
#include <gloop/glu.hpp>
#include <gloop/glut.hpp>

class Option {
 public:
  bool isChecked();
  void setChecked(bool value);
};

class OptionGroup {
 public:
  Option* createOption(const char* caption, bool initialValue);
};

class Scene {
  static GLvoid s_drag(int x, int y);
  static GLvoid s_click(int button, int state, int x, int y);
  static GLvoid s_key(unsigned char k, int x, int y);
  static GLvoid s_draw();
  static GLvoid s_resize(int w, int h);

  carve::geom3d::Vector CAM_LOOK;
  double CAM_ROT;
  double CAM_ELEVATION;
  double CAM_DIST;
  carve::geom3d::Vector CAM_LOOK_REAL;
  double CAM_DIST_REAL;

  int WIDTH;
  int HEIGHT;

  bool initialised;
  bool disp_grid;
  bool disp_axes;

  GLvoid _drag(int x, int y);
  GLvoid _click(int button, int state, int x, int y);
  GLvoid _key(unsigned char k, int x, int y);
  GLvoid _draw();
  GLvoid _resize(int w, int h);
  virtual void _init();

  void updateDisplay();

 protected:
  virtual bool key(unsigned char k, int x, int y);
  virtual GLvoid draw();
  virtual void click(int button, int state, int x, int y);

 public:
  Scene(int argc, char** argv);
  virtual ~Scene();
  virtual void run();
  void init();

  carve::geom3d::Ray getRay(int x, int y);
  void zoomTo(carve::geom3d::Vector pos, double dist);
  OptionGroup* createOptionGroup(const char* title);
};
