float animate_gi_demo_speed = 0.0;
float animate_gi_demo = 0.0;
macro NO_GI_DEMO(code)
  hlsl(code) {
    float get_gi_param(float tcX) {return 1;}
  }
endmacro

macro GI_DEMO_ON(code)
  (code) {gtime@f4 = (time_phase(0,0)*animate_gi_demo_speed - PI/2, 1-animate_gi_demo, animate_gi_demo, 0);}
  hlsl(code) {
    float get_gi_param(float tcX)
    {
      return saturate(gtime.y + saturate(sin(gtime.x)+1)*gtime.z) >= tcX;
    }
  }
endmacro

macro GI_DEMO(code)
  NO_GI_DEMO(code)
endmacro
