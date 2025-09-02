// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define I_DAGORMAT 0x70a066e2

class IDagorMat
{
public:
  enum class Sides : int
  {
    OneSided,
    DoubleSided,
    RealDoubleSided
  };

  virtual Color get_amb() = 0;
  virtual Color get_diff() = 0;
  virtual Color get_spec() = 0;
  virtual Color get_emis() = 0;
  virtual float get_power() = 0;
  virtual Sides get_2sided() = 0;
  virtual const TCHAR *get_classname() = 0;
  virtual const TCHAR *get_script() = 0;
  virtual const TCHAR *get_texname(int) = 0;
  virtual float get_param(int) = 0;

  virtual void set_amb(Color) = 0;
  virtual void set_diff(Color) = 0;
  virtual void set_spec(Color) = 0;
  virtual void set_emis(Color) = 0;
  virtual void set_power(float) = 0;
  virtual void set_2sided(Sides) = 0;
  virtual void set_classname(const TCHAR *) = 0;
  virtual void set_script(const TCHAR *) = 0;
  virtual void set_texname(int, const TCHAR *) = 0;
  virtual void set_param(int, float) = 0;
};
