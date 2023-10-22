
include "shader_global.sh"

texture velocity_pressure_tex;
texture density_tex;

texture next_velocity_pressure_tex;
texture next_density_tex;

texture solid_boundaries_tex;

float simulation_time = 0;
float simulation_dt = 0;
float simulation_dx = 1;

float standard_density = 1.225;
float4 standard_velocity = float4(10, 10, 0, 0);

texture initial_density_tex;
texture initial_velocity_pressure_tex;

int4 tex_size;

hlsl {
  float calc_P(float4 rho)
  {
    const float M = 0.0289644f; // molar mass of air
    const float R = 8.314462618f; // universal gas constant
    const float T = 300.0f; // temperature in Kelvin
    return dot(rho, float4(1, 1, 1, 1)) * R * T / M;
  }
}

shader fill_initial_conditions
{
  ENABLE_ASSERT(cs)

  (cs) {
    velocityPressure@uav = velocity_pressure_tex hlsl {
      RWTexture2D<float4> velocityPressure@uav;
    };
    density@uav = density_tex hlsl {
      RWTexture2D<float4> density@uav;
    };

    standard_density@f1 = (standard_density, 0, 0, 0);
    standard_velocity@f2 = (standard_velocity.x, standard_velocity.y, 0, 0);

    texSize@i2 = (tex_size.x, tex_size.y, 0, 0);
  }

  hlsl(cs) {
    [numthreads(8, 8, 1)]
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {
      uint2 size = texSize;
      if (tid.x >= size.x || tid.y >= size.y)
        return;

      uint2 coord = uint2(tid.x, tid.y);

      density[coord] = float4(standard_density, 0, 0, 0);
      velocityPressure[coord] = float4(standard_velocity, calc_P(float4(standard_density, 0, 0, 0)), 0);
    }
  }

  compile("cs_5_0", "cs_main")
}

shader fill_initial_conditions_from_tex
{
  ENABLE_ASSERT(cs)

  (cs) {
    velocityPressure@uav = velocity_pressure_tex hlsl {
      RWTexture2D<float4> velocityPressure@uav;
    };
    density@uav = density_tex hlsl {
      RWTexture2D<float4> density@uav;
    };

    initial_density@smp2d = initial_density_tex;
    initial_velocity_pressure@smp2d = initial_velocity_pressure_tex;

    texSize@i2 = (tex_size.x, tex_size.y, 0, 0);
  }

  hlsl(cs) {
    [numthreads(8, 8, 1)]
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {
      uint2 size = texSize;
      if (tid.x >= size.x || tid.y >= size.y)
        return;

      uint2 coord = uint2(tid.x, tid.y);

      density[coord] = tex2Dlod(initial_density, float4(float2(coord) / float2(size), 0, 0));
      velocityPressure[coord] = tex2Dlod(initial_velocity_pressure, float4(float2(coord) / float2(size), 0, 0));
    }
  }

  compile("cs_5_0", "cs_main")
}

shader euler_simulation_cs
{
  ENABLE_ASSERT(cs)

  (cs) {
    tex_size@i2 = (tex_size.x, tex_size.y, 0, 0);

    simulation_time@f1 = (simulation_time, 0, 0, 0);
    dt@f1 = (simulation_dt, 0, 0, 0);
    h@f1 = (simulation_dx, 0, 0, 0);
    velocityPressure@smp2d = velocity_pressure_tex;
    density@smp2d = density_tex;

    standard_density@f1 = (standard_density, 0, 0, 0);
    standard_velocity@f2 = (standard_velocity.x, standard_velocity.y, 0, 0);

    nextVelocityPressure@uav = next_velocity_pressure_tex hlsl {
      RWTexture2D<float4> nextVelocityPressure@uav;
    };
    nextDensity@uav = next_density_tex hlsl {
      RWTexture2D<float4> nextDensity@uav;
    };

    solidBoundaries@smp2d = solid_boundaries_tex;
  }

  hlsl(cs) {
    #define x_ofs int2(1, 0)
    #define y_ofs uint2(0, 1)

    // Central difference derivative approximations
    inline float dvx_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).x -
              texelFetch(velocityPressure, coord - x_ofs, 0).x) / (2 * h);
    }
    inline float dvy_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).y -
              texelFetch(velocityPressure, coord - x_ofs, 0).y) / (2 * h);
    }
    inline float dvx_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).x -
              texelFetch(velocityPressure, coord - y_ofs, 0).x) / (2 * h);
    }
    inline float dvy_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).y -
              texelFetch(velocityPressure, coord - y_ofs, 0).y) / (2 * h);
    }
    inline float dP_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).z -
              texelFetch(velocityPressure, coord - x_ofs, 0).z) / (2 * h);
    }
    inline float dP_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).z -
              texelFetch(velocityPressure, coord - y_ofs, 0).z) / (2 * h);
    }
    inline float4 drho_dx(int2 coord)
    {
      return (texelFetch(density, coord + x_ofs, 0).xyzw -
              texelFetch(density, coord - x_ofs, 0).xyzw) / (2 * h);
    }
    inline float4 drho_dy(int2 coord)
    {
      return (texelFetch(density, coord + y_ofs, 0).xyzw -
              texelFetch(density, coord - y_ofs, 0).xyzw) / (2 * h);
    }

    // Set the desired conditions on the grid edges
    void applyFarFieldConditions(uint2 coord, inout float4 nextRho, inout float4 nextVelPressure)
    {
      // Inflow
      if (coord.x == 0)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(standard_velocity, calc_P(nextRho), 0);
      }
      if (coord.y == 0)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(standard_velocity, calc_P(nextRho), 0);
      }

      // Outflow
      if (coord.x == tex_size.x - 1)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(standard_velocity, calc_P(nextRho), 0);
      }
      if (coord.y == tex_size.y - 1)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(standard_velocity, calc_P(nextRho), 0);
      }

      // Based on https://web.stanford.edu/group/frg/course_work/AA214/CA-AA214-Ch8.pdf
      // Inflow
      // if (coord.x == 0)
      // {
      //   nextRho = float4(standard_density, 0, 0, 0);
      //   nextVelPressure.xy = standard_velocity;
      // }
      // if (coord.y == 0)
      // {
      //   nextRho = float4(standard_density, 0, 0, 0);
      //   nextVelPressure.xy = standard_velocity;
      // }

      // // Outflow
      // if (coord.x == tex_size.x - 1)
      // {
      //   nextVelPressure.z = calc_P(float4(standard_density, 0, 0, 0));
      // }
      // if (coord.y == tex_size.y - 1)
      // {
      //   nextVelPressure.z = calc_P(float4(standard_density, 0, 0, 0));
      // }
    }

    void applyCircleFriction(uint2 coord, float2 v, float2 center, float radius, inout float2 f)
    {
      float2 diff = coord.xy - center;
      float dist = length(diff);
      if (dist < radius)
      {
        f += -v * 1000.0;
      }
    }

    void applyWallFriction(uint2 coord, float2 v, float2 tl, float2 br, inout float2 f)
    {
      if (coord.x > tl.x && coord.x < br.x && coord.y > tl.y && coord.y < br.y)
      {
        f += -v * 10000.0;
      }
    }

    float2 frictionForce(uint2 coord, float2 v)
    {
      float boundaryData = tex2Dlod(solidBoundaries, float4(float2(coord) / float2(tex_size), 0, 0)).w;

      if (boundaryData > 0)
        return -v * 10000.0;


      return 0;
      // float2 ret = 0;
      // applyCircleFriction(coord, v, float2(100, 200), 40, ret);
      // applyCircleFriction(coord, v, float2(400, 200), 20, ret);
      // applyCircleFriction(coord, v, float2(450, 270), 10, ret);
      // applyCircleFriction(coord, v, float2(400, 330), 10, ret);
      // applyWallFriction(coord, v, float2(200, 100), float2(210, 300), ret);
      // applyWallFriction(coord, v, float2(400, 50), float2(500, 60), ret);
      // applyWallFriction(coord, v, float2(400, 100), float2(500, 150), ret);

      // return ret;
    }

    void solver(uint2 coord, float t)
    {
      float4 nextVelPressure = 0;
      float4 nextRho = 0;

      float4 rho = texelFetch(density, coord, 0).xyzw;
      float combinedRho = dot(rho, float4(1, 1, 1, 1));

      float v_x = texelFetch(velocityPressure, coord, 0).x;
      float v_y = texelFetch(velocityPressure, coord, 0).y;
      float dvx_dx_dvy_dy = (dvx_dx(coord) + dvy_dy(coord));

      // calculate rho
      nextRho = rho + dt * (float4(0, 0, 0, 0) - rho * dvx_dx_dvy_dy - v_x * drho_dx(coord) - v_y * drho_dy(coord));
      nextRho = float4(max(nextRho.x, 0), max(nextRho.y, 0), max(nextRho.z, 0), max(nextRho.w, 0));

      // calculate v
      float2 force = frictionForce(coord, float2(v_x, v_y));
      nextVelPressure.x = v_x + dt * ((force.x - dP_dx(coord)) / (combinedRho + 0.0001) - v_x * dvx_dx(coord) - v_y * dvx_dy(coord));
      nextVelPressure.y = v_y + dt * ((force.y - dP_dy(coord)) / (combinedRho + 0.0001) - v_x * dvy_dx(coord) - v_y * dvy_dy(coord));

      // calculate p
      nextVelPressure.z = calc_P(nextRho);

      applyFarFieldConditions(coord, nextRho, nextVelPressure);

      nextDensity[coord] = nextRho;
      nextVelocityPressure[coord] = nextVelPressure;
    }

    [numthreads(8, 8, 1)]
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {
      if (tid.x >= tex_size.x || tid.y >= tex_size.y)
        return;

      solver(tid.xy, simulation_time);
    }
  }
  compile("cs_5_0", "cs_main")
}

int euler_cascade_no = 0;
interval euler_cascade_no: c128<1, c256<2, c512;

int euler_implicit_mode = 0;
interval euler_implicit_mode: hor<1, vert;

shader euler_simulation_explicit_implicit_cs
{
  ENABLE_ASSERT(cs)

  (cs) {
    tex_size@i2 = (tex_size.x, tex_size.y, 0, 0);

    simulation_time@f1 = (simulation_time, 0, 0, 0);
    dt@f1 = (simulation_dt, 0, 0, 0);
    h@f1 = (simulation_dx, 0, 0, 0);
    velocityPressure@smp2d = velocity_pressure_tex;
    density@smp2d = density_tex;

    standard_density@f1 = (standard_density, 0, 0, 0);
    standard_velocity@f2 = (standard_velocity.x, standard_velocity.y, 0, 0);

    nextVelocityPressure@uav = next_velocity_pressure_tex hlsl {
      RWTexture2D<float4> nextVelocityPressure@uav;
    };
    nextDensity@uav = next_density_tex hlsl {
      RWTexture2D<float4> nextDensity@uav;
    };

    solidBoundaries@smp2d = solid_boundaries_tex;
  }

  hlsl(cs) {

    #define x_ofs int2(1, 0)
    #define y_ofs uint2(0, 1)

    //#define ROW_SIZE 512
    #define ROW_SIZE 64

    groupshared float equation[ROW_SIZE];
    groupshared float rightPart[3][ROW_SIZE];
    groupshared float solution[3][ROW_SIZE];

    // Central difference derivative approximations
    inline float dvx_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).x -
              texelFetch(velocityPressure, coord - x_ofs, 0).x) / (2 * h);
    }
    inline float dvy_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).y -
              texelFetch(velocityPressure, coord - x_ofs, 0).y) / (2 * h);
    }
    inline float dvx_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).x -
              texelFetch(velocityPressure, coord - y_ofs, 0).x) / (2 * h);
    }
    inline float dvy_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).y -
              texelFetch(velocityPressure, coord - y_ofs, 0).y) / (2 * h);
    }
    inline float dP_dx(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + x_ofs, 0).z -
              texelFetch(velocityPressure, coord - x_ofs, 0).z) / (2 * h);
    }
    inline float dP_dy(int2 coord)
    {
      return (texelFetch(velocityPressure, coord + y_ofs, 0).z -
              texelFetch(velocityPressure, coord - y_ofs, 0).z) / (2 * h);
    }
    inline float4 drho_dx(int2 coord)
    {
      return (texelFetch(density, coord + x_ofs, 0).xyzw -
              texelFetch(density, coord - x_ofs, 0).xyzw) / (2 * h);
    }
    inline float4 drho_dy(int2 coord)
    {
      return (texelFetch(density, coord + y_ofs, 0).xyzw -
              texelFetch(density, coord - y_ofs, 0).xyzw) / (2 * h);
    }

    // Set the desired conditions on the grid edges
    void applyFarFieldConditions(uint2 coord, inout float4 nextRho, inout float4 nextVelPressure)
    {
      float2 velocity;
      velocity.x = cos(simulation_time*4)*15;
      velocity.y = sin(simulation_time*4)*15;

      velocity = standard_velocity;
      // Inflow
      if (coord.x == 0)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(velocity, calc_P(nextRho), 0);
      }
      if (coord.y == 0)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(velocity, calc_P(nextRho), 0);
      }

      // Outflow
      if (coord.x == tex_size.x - 1)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(velocity, calc_P(nextRho), 0);
      }
      if (coord.y == tex_size.y - 1)
      {
        nextRho = float4(standard_density, 0, 0, 0);
        nextVelPressure = float4(velocity, calc_P(nextRho), 0);
      }
    }

    float2 frictionForce(uint2 coord, float2 v)
    {
      float boundaryData = tex2Dlod(solidBoundaries, float4(float2(coord) / float2(tex_size), 0, 0)).w;

      if (boundaryData > 0)
        return -v * 10000.0;

      return 0;
    }

    void solver(uint2 coord, float t)
    {
      float4 nextVelPressure = 0;
      float4 nextRho = 0;

      float4 rho = texelFetch(density, coord, 0).xyzw;
      float combinedRho = dot(rho, float4(1, 1, 1, 1));

      float v_x = texelFetch(velocityPressure, coord, 0).x;
      float v_y = texelFetch(velocityPressure, coord, 0).y;
      float dvx_dx_dvy_dy = (dvx_dx(coord) + dvy_dy(coord));

      float2 force = frictionForce(coord, float2(v_x, v_y));

      ##if euler_implicit_mode == hor
        uint threadCoord = coord.x % ROW_SIZE;
      ##else
        uint threadCoord = coord.y % ROW_SIZE;
      ##endif

      // on scheme borders we use explicit method for calculations, inside - implicit calculations
      float2 derivWeights = (threadCoord == 0 || threadCoord == ROW_SIZE-1) ? float2(0, 1) : float2(0.25, 0.5);

      ##if euler_implicit_mode == hor // impliicit fox x derivative
        // v
        rightPart[0][threadCoord] = v_x / dt - v_y * dvx_dx(coord) + (force.x - dP_dx(coord)) / (combinedRho + 0.0001) - derivWeights.y * v_x * dvx_dx(coord);
        rightPart[1][threadCoord] = v_y / dt - v_y * dvy_dx(coord) + (force.y - dP_dy(coord)) / (combinedRho + 0.0001) - derivWeights.y * v_x * dvy_dx(coord);
        // rho
        rightPart[2][threadCoord] = rho.x * (1.0f / dt - dvx_dx_dvy_dy) - v_y * drho_dy(coord).x - derivWeights.y * v_x * drho_dx(coord).x;

        // equation params (we need only one value of 3 per matrix row)
        // and for all v_x, v_y and rho equation is the same!
        equation[threadCoord] = - derivWeights.x * v_x / h;
      ##else                          // impliicit fox y derivavtive
        // v
        rightPart[0][threadCoord] = v_x / dt - v_x * dvx_dx(coord) + (force.x - dP_dx(coord)) / (combinedRho + 0.0001) - derivWeights.y * v_y * dvx_dx(coord);
        rightPart[1][threadCoord] = v_y / dt - v_x * dvy_dx(coord) + (force.y - dP_dy(coord)) / (combinedRho + 0.0001) - derivWeights.y * v_y * dvy_dx(coord);
        // rho
        rightPart[2][threadCoord] = rho.x * (1.0f / dt - dvx_dx_dvy_dy) - v_x * drho_dx(coord).x - derivWeights.y * v_y * drho_dy(coord).x;

        // equation params (we need only one value of 3 per matrix row)
        // and for all v_x, v_y and rho equation is the same!
        equation[threadCoord] = - derivWeights.x * v_y / h;
      ##endif

      GroupMemoryBarrierWithGroupSync();

      // sweep method
      // https://pro-prof.com/forums/topic/sweep-method-for-solving-systems-of-linear-algebraic-equations
      if (threadCoord <= 2) // 0 for v_x, 1 for v_y, 2 for rho
      {
        derivWeights =  float2(0.25, 0.5);

        float equation_y = 1.0f / dt;

        float gamma;

        float alpha[ROW_SIZE];
        float beta[ROW_SIZE];

        gamma = equation_y;
        alpha[0] = equation[0] / gamma;
        beta[0] = rightPart[threadCoord][0] / gamma;

        // forward sweep
        for (int i = 1; i< ROW_SIZE; i++)
        {
          gamma = equation_y + equation[i] * alpha[i - 1];
          alpha[i] = equation[i] / gamma;
          beta[i] = (rightPart[threadCoord][i] - equation[i] * beta[i-1]) / gamma;
        }

        int j = ROW_SIZE - 1;
        solution[threadCoord][j] = beta[j];

        // back sweep
        for (int i = ROW_SIZE - 2; i >= 0; i--)
        {
          solution[threadCoord][i] = alpha[i] * solution[threadCoord][i+1] + beta[i];
        }
      }
      GroupMemoryBarrierWithGroupSync();

      nextVelPressure.x = solution[0][threadCoord];
      nextVelPressure.y = solution[1][threadCoord];
      nextRho = 0;
      nextRho.x = solution[2][threadCoord];

      // calculate p
      nextVelPressure.z = calc_P(nextRho);

      applyFarFieldConditions(coord, nextRho, nextVelPressure);

      nextDensity[coord] = nextRho;
      nextVelocityPressure[coord] = nextVelPressure;
    }

  ##if euler_implicit_mode == hor
    [numthreads(ROW_SIZE, 1, 1)]
  ##else
    [numthreads(1, ROW_SIZE, 1)]
  ##endif
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {

      solver(tid.xy, simulation_time);
    }
  }
  compile("cs_5_0", "cs_main")
}


float euler_centralBlurWeight = 1000.0;

// Blurs the solution textures to avoid aliasing caused by the derivative approximations
// TODO: fix this issue inside solver
shader blur_result_cs
{
  ENABLE_ASSERT(cs)

  (cs) {
    tex_size@i2 = (tex_size.x, tex_size.y, 0, 0);
    simulation_time@f1 = (simulation_time, 0, 0, 0)
    blurWeights@f2 = (euler_centralBlurWeight, 1.0/(euler_centralBlurWeight + 4.0), 0, 0);
    velocityPressure@smp2d = velocity_pressure_tex;
    density@smp2d = density_tex;

    nextVelocityPressure@uav = next_velocity_pressure_tex hlsl {
      RWTexture2D<float4> nextVelocityPressure@uav;
    };
    nextDensity@uav = next_density_tex hlsl {
      RWTexture2D<float4> nextDensity@uav;
    };
  }

  hlsl(cs) {
    #define x_ofs int2(1, 0)
    #define y_ofs uint2(0, 1)

    void blurer(int2 coord, float t)
    {
      float4 blurredVelPressure = 0;

      float v_x = blurWeights.y*(blurWeights.x*texelFetch(velocityPressure, coord, 0).x + texelFetch(velocityPressure, coord + x_ofs, 0).x +
                                                                                        + texelFetch(velocityPressure, coord - x_ofs, 0).x +
                                                                                        + texelFetch(velocityPressure, coord + y_ofs, 0).x +
                                                                                        + texelFetch(velocityPressure, coord - y_ofs, 0).x);
      float v_y = blurWeights.y*(blurWeights.x*texelFetch(velocityPressure, coord, 0).y + texelFetch(velocityPressure, coord + x_ofs, 0).y +
                                                                                        + texelFetch(velocityPressure, coord - x_ofs, 0).y +
                                                                                        + texelFetch(velocityPressure, coord + y_ofs, 0).y +
                                                                                        + texelFetch(velocityPressure, coord - y_ofs, 0).y);

      float  P  = blurWeights.y*(blurWeights.x*texelFetch(velocityPressure, coord, 0).z + texelFetch(velocityPressure, coord + x_ofs, 0).z +
                                                                                        + texelFetch(velocityPressure, coord - x_ofs, 0).z +
                                                                                        + texelFetch(velocityPressure, coord + y_ofs, 0).z +
                                                                                        + texelFetch(velocityPressure, coord - y_ofs, 0).z);

      blurredVelPressure = float4(v_x, v_y, P, texelFetch(velocityPressure, coord, 0).w);

      nextVelocityPressure[coord] = blurredVelPressure;


      float4 blurredDensity = 0.001f*(996*texelFetch(density, coord, 0).xyzw + texelFetch(density, coord + x_ofs, 0).xyzw +
                                                            + texelFetch(density, coord - x_ofs, 0).xyzw +
                                                              + texelFetch(density, coord + y_ofs, 0).xyzw +
                                                            + texelFetch(density, coord - y_ofs, 0).xyzw);

      nextDensity[coord] = blurredDensity;
    }

    [numthreads(8, 8, 1)]
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {
      if (tid.x >= tex_size.x || tid.y >= tex_size.y)
        return;

      blurer(tid.xy, simulation_time);
    }
  }
  compile("cs_5_0", "cs_main")
}