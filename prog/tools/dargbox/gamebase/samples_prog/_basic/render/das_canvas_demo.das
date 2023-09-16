require darg
require math
require DagorMath

let start_time = ref_time_ticks()

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
  //print("!@! draw() called, screen_width = {ctx |> screen_width()}, opacity = {rstate.opacity}, data.x = {data.x}")

  //var fontid = get_font_id("big")

  var fctx: StdGuiFontContext
  get_font_context(fctx, 0, 0, 0, 0)

  let lt = rdata.pos
  let rb = rdata.pos+rdata.size

  ctx |> set_color(200,20,20,200)
  ctx |> render_box(lt.x, lt.y, rb.x, rb.y)

  let linePts <- to_array_move([[float2[4] lt+float2(0., 50.); lt+float2(200., 150.); lt+float2(250., 100.); lt+float2(50., 0.)]])
  ctx |> render_line_aa(linePts, true, 3.0, float2(0., 0.), E3DCOLOR(0xFF20F020))

  let strBox = get_str_bbox("foo", 3, fctx)
  ctx |> set_color(0,0,0,255)
  ctx |> render_frame(lt.x+10., rb.y-20.-strBox.width.y, lt.x+10.+strBox.width.x, rb.y-20., 2.0)

  ctx |> goto_xy(lt.x+10., rb.y-20.)
  ctx |> set_color(data.textColor)
  ctx |> draw_str_scaled(1.0, "foo", 3)


[export]
def draw_without_params(var ctx: GuiContext&; rdata: ElemRenderData& const; rstate: RenderState& const)
  let lt = rdata.pos
  let rb = rdata.pos+rdata.size

  let t = float(get_time_usec(start_time))

  let rounding = float4(20,20,20,20)
  let color = E3DCOLOR(Color4(float4(0.50, 0.60+0.3*sin(t*3e-6), 0.80, 1.0)))
  ctx |> render_rounded_box(lt, rb, color, E3DCOLOR(0xFFF0FFF0), rounding, 3.)
