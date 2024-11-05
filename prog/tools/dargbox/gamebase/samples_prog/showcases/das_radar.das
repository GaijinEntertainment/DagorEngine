require darg
require DagorStdGuiRender
require math
require DagorMath

let start_time = ref_time_ticks()

let darkColor = float4(0.1, 0.5, 0.05, 0.8)
let lightColor = float4(0.4, 0.9, 0.2, 0.8)


struct TestParams
  x: int = 123
  y: float2 = float2(888, 999)
  textColor: E3DCOLOR = E3DCOLOR(0xFFFFFFFF)


[export]
def setup_data(props: Properties&; var storage: TestParams &)
  storage.x = 555
  storage.textColor = props |> getColor("dasTextColor", E3DCOLOR(0xFFFFFFFF))


[export]
def draw_with_params(var ctx: GuiContext&; rdata: ElemRenderData& const; rstate: RenderState& const; data: TestParams &)
  let ax = float2(1., 0.)
  let ay = float2(0., 1.)
  let o = rdata.pos
  ctx |> setViewTm(ax, ay, o, false)
  aircraft(ctx)


[export]
def draw_radar_b_scope(var ctx: GuiContext&; rdata: ElemRenderData& const; rstate: RenderState& const)
  let lt = rdata.pos
  let rb = rdata.pos+rdata.size
  let center = (lt + rb)*0.5
  let sz = rdata.size
  let radius = float2(sz.x*0.5, sz.y)
  let anglesExternal = float2(-PI/2.-PI*0.45, -PI/2.+PI*0.45)
  let anglesInternal = float2(-PI/2.-PI*0.25, -PI/2.+PI*0.25)
  let origin = float2(center.x, rb.y)

  let t = float(get_time_usec(start_time))*1e-3
  let azimuth = lerp(anglesInternal[0], anglesInternal[1], abs(t*0.0007 % 2. - 1.))
  let azumuthWidth = PI*0.01
  let azimuthStart = max(anglesInternal[0], azimuth-azumuthWidth)
  let azimuthEnd = min(anglesInternal[1], azimuth+azumuthWidth)

  let rounding = float4(20,20,20,20)
  let e3dDark = E3DCOLOR(Color4(darkColor))
  let e3dLight = E3DCOLOR(Color4(lightColor))
  ctx |> render_sector_aa(origin, radius, anglesExternal, 2., e3dLight, e3dDark)
  ctx |> render_sector_aa(origin, radius, float2(azimuthStart, azimuthEnd), 0., e3dLight, e3dLight)

  let midRanges <- [[float[3] 0.25; 0.5; 0.75]]
  for r in midRanges
    ctx |> render_sector_aa(origin, radius * r, anglesInternal, 2., e3dLight, E3DCOLOR(0u))
  let midSectors <- [[float[5] 0.0; 0.25; 0.5; 0.75; 1.0]]
  for r in midSectors
    let angle = lerp(anglesInternal[0], anglesInternal[1], r)
    let coord <- to_array_move([[float2[2] origin; origin+float2(cos(angle), sin(angle))*radius]])
    ctx |> render_line_aa(coord, false, 2., float2(0,0), e3dLight)



def detected_screen_target(var ctx: GuiContext&)
  let e3dLight = E3DCOLOR(Color4(lightColor))
  ctx |> set_color(e3dLight)
  ctx |> draw_line(0., 0., 40., 0., 1.)
  ctx |> draw_line(0., 0., 0., 40., 1.)
  ctx |> draw_line(100., 0., 60., 0., 1.)
  ctx |> draw_line(100., 0., 100., 40., 1.)
  ctx |> draw_line(100., 100., 60., 100., 1.)
  ctx |> draw_line(100., 100., 100., 60., 1.)
  ctx |> draw_line(0., 100., 40., 100., 1.)
  ctx |> draw_line(0., 100., 0., 60., 1.)


def not_detected_screen_target(var ctx: GuiContext&)
  let e3dLight = E3DCOLOR(Color4(lightColor))
  ctx |> draw_line(0., 0., 10., 0., 1.)
  ctx |> draw_line(0., 0., 0., 10., 1.)
  ctx |> draw_line(100., 0., 90., 0., 1.)
  ctx |> draw_line(100., 0., 100., 10., 1.)
  ctx |> draw_line(100., 100., 90., 100., 1.)
  ctx |> draw_line(100., 100., 100., 90., 1.)
  ctx |> draw_line(0., 100., 10., 100., 1.)
  ctx |> draw_line(0., 100., 0., 90., 1.)


def aircraft(var ctx: GuiContext&)
  let tailW = 25.
  let tailH = 10.
  let tailOffset1 = 10.
  let tailOffset2 = 5.
  let tailOffset3 = 25.
  let fuselageWHalf = 10.
  let wingOffset1 = 45.
  let wingOffset2 = 30.
  let wingW = 32.
  let wingH = 18.
  let wingOffset3 = 30.
  let noseOffset = 5.

  let linePts <- to_array_move([[float2[]
    // tail left
    float2(50., 100. - tailOffset1);
    float2(50. - tailW, 100. - tailOffset2);
    float2(50. - tailW, 100. - tailOffset2 - tailH);
    float2(50. - fuselageWHalf, 100. - tailOffset3);
    // wing left
    float2(50. - fuselageWHalf, 100. - wingOffset1);
    float2(50. - fuselageWHalf - wingW, 100. - wingOffset2);
    float2(50. - fuselageWHalf - wingW, 100. - wingOffset2 - wingH);
    float2(50. - fuselageWHalf, wingOffset3);
    // nose
    float2(50., noseOffset);
    // wing rigth
    float2(50. + fuselageWHalf, wingOffset3);
    float2(50. + fuselageWHalf + wingW, 100. - wingOffset2 - wingH);
    float2(50. + fuselageWHalf + wingW, 100. - wingOffset2);
    float2(50. + fuselageWHalf, 100. - wingOffset1);
    // tail right
    float2(50. + fuselageWHalf, 100. - tailOffset3);
    float2(50. + tailW, 100. - tailOffset2 - tailH);
    float2(50. + tailW, 100. - tailOffset2)
  ]])

  ctx |> render_line_aa(linePts, true, 2.0, float2(0., 0.), E3DCOLOR(0xFF20F020))


// def test(var ctx: GuiContext&)
//   var tm: float2[3]
//   ctx |> getViewTm(tm)
