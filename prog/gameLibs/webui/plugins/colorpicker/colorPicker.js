

//function onColorChangedCallback(color4FloatStr, e3dcolorHexStr, varId)

var colorCallback = null;
var pipette_client = new XMLHttpRequest();


function pipette_query(query)
{
  pipette_client.onerror = function()
  {
    console.log("ERROR: pipette_query returned: " + pipette_client.status);
  }

  pipette_client.onload = function()
  {
    if (pipette_client.status === 200 && pipette_client.responseText != "")
    {
      if (pipette_client.responseText != "stop")
      {
        color_text_change_255(pipette_client.responseText, true);
        apply_color();
        ignore_input_255 = false;
        ignore_input_1 = false;
        color_to_text();
      }
      return;
    }
    window.setTimeout("on_pipette_timer()", 60);
  }

  pipette_client.open('GET', 'color_pipette?' + query, true);
  pipette_client.send();
}


function on_pipette_timer()
{
  pipette_query("get_result");
}


function request_pipette()
{
  pipette_query("start");
  window.setTimeout("on_pipette_timer()", 60);
}


function show_color_picker(elem, var_id, onColorChangedCallback, hideButtons)
{
  colorCallback = onColorChangedCallback;

  var colorStr = elem.dataset.color;
  if (colorStr)
    backToColor = colorStr;
  else
    backToColor = elem.name;

  show_color_picker_internal(elem, var_id, hideButtons);
}


function show_color_picker_hex(elem, var_id, onColorChangedCallback, hideButtons)
{
  colorCallback = onColorChangedCallback;

  var colorHex = elem.dataset.color;

  var rr = colorHex.substring(0, 2);
  var gg = colorHex.substring(2, 4);
  var bb = colorHex.substring(4, 6);
  var aa = colorHex.substring(6, 8);

  var color4 = parseInt(rr, 16) / 255.0 + "," +
               parseInt(gg, 16) / 255.0 + "," +
               parseInt(bb, 16) / 255.0 + "," +
               parseInt(aa, 16) / 255.0;

  backToColor = color4;
  backToColorHex = elem.dataset.color;
  show_color_picker_internal(elem, var_id, hideButtons);
}


function show_color_picker_internal(elem, var_id, hideButtons)
{
  current_var_id = var_id;
  current_color_picker_elem = elem;
  document.getElementById("color_picker_div").style.visibility = 'visible';

  document.getElementById("color_picker_revert_btn").style.visibility = hideButtons ? 'hidden' : 'visible';
  document.getElementById("color_picker_close_btn").style.visibility = hideButtons ? 'hidden' : 'visible';

  color_text_change_1(backToColor);
  ignore_input_1 = false;
  ignore_input_255 = false;
  hsvToRgb(currentHue, currentS, currentV);
  startR = _r;
  startG = _g;
  startB = _b;
  startA = Math.round(currentAlpha);
  render();
}

function hide_color_picker()
{
  document.getElementById("color_picker_div").style.visibility = 'hidden';
  document.getElementById("color_picker_revert_btn").style.visibility = 'hidden';
  document.getElementById("color_picker_close_btn").style.visibility = 'hidden';
  current_color_picker_elem = null;
}

function dec2hex(i)
{
  if (i < 0)
    i = 0;
  if (i > 255)
    i = 255;
  return (i + 0x10000).toString(16).substr(-2);
}

function revert_color()
{
  if (current_color_picker_elem.dataset.color && backToColorHex && backToColorHex.length > 1)
  {
    current_color_picker_elem.dataset.color = backToColorHex;
    show_color_picker_hex(current_color_picker_elem, current_var_id, colorCallback);
  }
  else
  {
    current_color_picker_elem.name = backToColor;
    current_color_picker_elem.dataset.color = backToColor;
    show_color_picker(current_color_picker_elem, current_var_id, colorCallback);
  }

  apply_color();

  var colorStr = dec2hex(_r) + dec2hex(_g) + dec2hex(_b);
  colorCallback(backToColor, colorStr, current_var_id);

//  var x = new XMLHttpRequest();
//  x.open('GET', 'shader_vars?color4=' + backToColor.replace(' ', '') + '&id=' + current_var_id, true);
//  x.send();
}

function apply_color()
{
  hsvToRgb(currentHue, currentS, currentV);
  var colorStr = dec2hex(_r) + dec2hex(_g) + dec2hex(_b);
  current_color_picker_elem.style.background = "#" + colorStr;
  var colorFloatStr = document.getElementById("color_text_1").value;

  colorStr += dec2hex(Math.round(currentAlpha));

  if (current_color_picker_elem.dataset.color)
    current_color_picker_elem.dataset.color = colorStr;
  else
    current_color_picker_elem.name = colorFloatStr;

  var elem = document.getElementById('sh_val_'+current_var_id);
  if (elem)
    elem.innerHTML = colorFloatStr;

  colorCallback(colorFloatStr, colorStr, current_var_id);

//  var x = new XMLHttpRequest();
//  x.open('GET', 'shader_vars?color4=' + colorFloatStr.replace(' ', '') + '&id=' + current_var_id, true);
//  x.send();
}

var current_var_id = -1;
var current_color_picker_elem = null;

var ctx = null;
var canv = null;
var ctxHue = null;
var canvHue = null;

var backToColor='';
var backToColorHex = '';
var _r = 0, _g = 0, _b = 0;
var _h = 0, _s = 0, _v = 0;
var currentHue = 0.0;
var currentAlpha = 255.0;
var currentS = 128.0;
var currentV = 128.0;
var startR = 128;
var startG = 128;
var startB = 128;
var startA = 255;

var ignore_input_1 = false;
var ignore_input_255 = false;

function color_text_change_255(s, keep_alpha = false)
{
  var components = s.split(",");
  if (components.length == 3)
  {
    if (keep_alpha)
      components.push("" + currentAlpha)
    else
      components.push("255");
  }

  if (components.length != 4)
    return;

  for (var i = 0; i < components.length; i++)
  {
    var n = parseInt(components[i]);
    if (n < 0)
      n = 0;
    if (n > 255)
      n = 255;
    if (isNaN(n))
      n = 0;
    components[i] = n;
  }

  rgbToHsv(components[0] / 255.0, components[1] / 255.0, components[2] / 255.0);

  ignore_input_255 = true;
  currentAlpha = components[3];
  currentHue = _h * 360.0;
  currentS = _s * 255.0;
  currentV = _v * 255.0;
  render();
}

function color_text_change_1(s)
{
  var components = s.split(",");
  if (components.length == 3)
    components.push("1");
  if (components.length != 4)
    return;

  for (var i = 0; i < components.length; i++)
  {
    var n = parseFloat(components[i]);
    if (n < 0)
      n = 0;
    if (n > 1)
      n = 1;
    if (isNaN(n))
      n = 0;
    components[i] = n;
  }

  rgbToHsv(components[0], components[1], components[2]);

  ignore_input_1 = true;
  currentAlpha = components[3] * 255.0;
  currentHue = _h * 360.0;
  currentS = _s * 255.0;
  currentV = _v * 255.0;
  render();
}

function rgbToHsv(r, g, b)
{
  r = Math.max(Math.min(r, 255), 0);
  g = Math.max(Math.min(g, 255), 0);
  b = Math.max(Math.min(b, 255), 0);

  var max = Math.max(Math.max(r, g), b),
      min = Math.min(Math.min(r, g), b);
  var h, s, v = max;

  var d = max - min;
  s = max === 0 ? 0 : d / max;

  if (max == min)
  {
    h = 0;
  }
  else
  {
    switch (max)
    {
      case r: h = (g - b) / d + (g < b ? 6 : 0); break;
      case g: h = (b - r) / d + 2; break;
      case b: h = (r - g) / d + 4; break;
    }
    h /= 6;
  }
  _h = h;
  _s = s;
  _v = v;
}


function hsvToRgb(h, s, v)
{
  h = (h / 360.0) % 1.0 * 6;
  s = s / 255.0;
  v = v / 255.0;

  var i = Math.floor(h),
      f = h - i,
      p = v * (1 - s),
      q = v * (1 - f * s),
      t = v * (1 - (1 - f) * s),
      mod = i % 6,
      r = [v, q, p, p, t, v][mod],
      g = [t, v, v, q, p, p][mod],
      b = [p, p, t, v, v, q][mod];

  _r = Math.round(r * 255);
  _g = Math.round(g * 255);
  _b = Math.round(b * 255);
}

function render()
{
  if (!canv)
    return;

  var pixels = ctx.getImageData(0, 0, canv.width, canv.height);
  var p = 0;
  for (var y = 0; y < 256; y++)
    for (var x = 0; x < 256; x++)
    {
      hsvToRgb(currentHue, x, 255 - y);
      pixels.data[p + 0] = _r;
      pixels.data[p + 1] = _g;
      pixels.data[p + 2] = _b;
      pixels.data[p + 3] = 255;
      p += 4;
    }
  ctx.putImageData(pixels, 0, 0);

  ctx.beginPath();
  ctx.arc(currentS, 255 - currentV, 5, 0, 2 * Math.PI, false);
  ctx.lineWidth = 2;
  ctx.strokeStyle = '#000';
  ctx.stroke();
  ctx.beginPath();
  ctx.arc(currentS, 255 - currentV, 6, 0, 2 * Math.PI, false);
  ctx.lineWidth = 1;
  ctx.strokeStyle = '#fff';
  ctx.stroke();


  var pixels = ctxHue.getImageData(0, 0, canvHue.width, canvHue.height);
  var p = 0;
  var iy = Math.round(255 - currentHue / 360.0 * 256);
  for (var y = 0; y < 256; y++)
  {
    hsvToRgb((255 - y) / 256.0 * 360.0, 255, 255);
    if (Math.abs(iy - y) <= 1)
    {
      _r = _g = _b = 255;
    }
    if (iy == y)
    {
      _r = _g = _b = 0;
    }

    for (var x = 0; x < 32; x++)
    {
      if (Math.abs(x - 15.5) > 14 && (Math.abs(iy - y) > 1))
      {
        pixels.data[p + 0] = 48;
        pixels.data[p + 1] = 48;
        pixels.data[p + 2] = 48;
        pixels.data[p + 3] = 255;
      }
      else
      {
        pixels.data[p + 0] = _r;
        pixels.data[p + 1] = _g;
        pixels.data[p + 2] = _b;
        pixels.data[p + 3] = 255;
      }
      p += 4;
    }
  }
  ctxHue.putImageData(pixels, 0, 0);


  var pixels = ctxS.getImageData(0, 0, canvHue.width, canvHue.height);
  var p = 0;
  var iy = Math.round(255 - currentS);
  for (var y = 0; y < 256; y++)
  {
    hsvToRgb(currentHue, 255 - y, currentV);
    if (Math.abs(iy - y) <= 1)
    {
      _r = _g = _b = 255;
    }
    if (iy == y)
    {
      _r = _g = _b = 0;
    }

    for (var x = 0; x < 32; x++)
    {
      if (Math.abs(x - 15.5) > 14 && (Math.abs(iy - y) > 1))
      {
        pixels.data[p + 0] = 48;
        pixels.data[p + 1] = 48;
        pixels.data[p + 2] = 48;
        pixels.data[p + 3] = 255;
      }
      else
      {
        pixels.data[p + 0] = _r;
        pixels.data[p + 1] = _g;
        pixels.data[p + 2] = _b;
        pixels.data[p + 3] = 255;
      }
      p += 4;
    }
  }
  ctxS.putImageData(pixels, 0, 0);


  var pixels = ctxV.getImageData(0, 0, canvHue.width, canvHue.height);
  var p = 0;
  var iy = Math.round(255 - currentV);
  for (var y = 0; y < 256; y++)
  {
    hsvToRgb(currentHue, currentS, 255 - y);
    if (Math.abs(iy - y) <= 1)
    {
      _r = _g = _b = 255;
    }
    if (iy == y)
    {
      _r = _g = _b = 0;
    }

    for (var x = 0; x < 32; x++)
    {
      if (Math.abs(x - 15.5) > 14 && (Math.abs(iy - y) > 1))
      {
        pixels.data[p + 0] = 48;
        pixels.data[p + 1] = 48;
        pixels.data[p + 2] = 48;
        pixels.data[p + 3] = 255;
      }
      else
      {
        pixels.data[p + 0] = _r;
        pixels.data[p + 1] = _g;
        pixels.data[p + 2] = _b;
        pixels.data[p + 3] = 255;
      }
      p += 4;
    }
  }
  ctxV.putImageData(pixels, 0, 0);



  var pixels = ctxAlpha.getImageData(0, 0, canvAlpha.width, canvAlpha.height);
  var p = 0;
  var iy = Math.round(255 - currentAlpha);
  for (var y = 0; y < 256; y++)
  {
    for (var x = 0; x < 32; x++)
    {
      _r = _g = _b = 255 - y;
      if (iy == y || Math.abs(x - 15.5) > 14)
      {
        _r = _g = _b = 48;
      }
      if (Math.abs(iy - y) == 1)
      {
        _r = _g = _b = 255;
      }

      pixels.data[p + 0] = _r;
      pixels.data[p + 1] = _g;
      pixels.data[p + 2] = _b;
      pixels.data[p + 3] = 255;
      p += 4;
    }
  }
  ctxAlpha.putImageData(pixels, 0, 0);


  hsvToRgb(currentHue, currentS, currentV);
  ctxColors.fillStyle = "rgba(" + _r + "," + _g + "," + _b + ",255)";
  ctxColors.fillRect(0, 0, 128, 128);
  var m = Math.round(currentAlpha);
  ctxColors.fillStyle = "rgba(" + m + "," + m + "," + m + ",255)";
  ctxColors.fillRect(120, 0, 8, 128);
  ctxColors.fillStyle = "rgba(" + startR + "," + startG + "," + startB + ",255)";
  ctxColors.fillRect(0, 128, 128, 128);
  var m = Math.round(startA);
  ctxColors.fillStyle = "rgba(" + m + "," + m + "," + m + ",255)";
  ctxColors.fillRect(120, 128, 8, 128);

  color_to_text();
}


var active_element = null;
var mouse_down = false;

function hue_mousedown(event)
{
  active_element = canvHue;
  event.preventDefault = true;
}

function hue_mousemove(event)
{
  var x = event.relative_coord_x;
  var y = event.relative_coord_y;
  currentHue = (255 - y) / 256.0 * 360.0;
  render();
  event.preventDefault = true;
}

function s_mousedown(event)
{
  active_element = canvS;
  event.preventDefault = true;
}

function s_mousemove(event)
{
  var x = event.relative_coord_x;
  var y = event.relative_coord_y;
  currentS = (255 - y);
  render();
  event.preventDefault = true;
}

function v_mousedown(event)
{
  active_element = canvV;
  event.preventDefault = true;
}

function v_mousemove(event)
{
  var x = event.relative_coord_x;
  var y = event.relative_coord_y;
  currentV = (255 - y);
  render();
  event.preventDefault = true;
}

function alpha_mousedown(event)
{
  active_element = canvAlpha;
  event.preventDefault = true;
}

function alpha_mousemove(event)
{
  var x = event.relative_coord_x;
  var y = event.relative_coord_y;
  currentAlpha = 255 - y;
  render();
  event.preventDefault = true;
}


function color_mousedown(event)
{
  active_element = canv;
  event.preventDefault = true;
}

function color_mousemove(event)
{
  var x = event.relative_coord_x;
  var y = event.relative_coord_y;
  currentS = x;
  currentV = 255 - y;
  render();
  event.preventDefault = true;
}



function color_to_text()
{
  hsvToRgb(currentHue, currentS, currentV);
  if (!ignore_input_255)
  {
    var txt = "" + _r + ", " + _g + ", " + _b + ", " + Math.round(currentAlpha);
    document.getElementById("color_text_255").value = txt;
    ignore_input_255 = false;
  }
  if (!ignore_input_1)
  {
    var txt = "" + (_r / 255.0).toFixed(3) + ", " + (_g / 255.0).toFixed(3) + ", " +
                     (_b / 255.0).toFixed(3) + ", " + (currentAlpha / 255.0).toFixed(3);
    document.getElementById("color_text_1").value = txt;
    ignore_input_1 = false;
  }
}

function window_mouse_move(event)
{
  ignore_input_1 = false;
  ignore_input_255 = false;

  if (active_element)
  {
    if (event.buttons == 0)
      active_element = null;
    else
    {
      event.relative_coord_x = event.clientX - active_element.offsetLeft;
      event.relative_coord_y = event.clientY - active_element.offsetTop;
      if (event.relative_coord_x < 0)
        event.relative_coord_x = 0;
      if (event.relative_coord_y < 0)
        event.relative_coord_y = 0;
      if (event.relative_coord_x >= active_element.width)
        event.relative_coord_x = active_element.width - 1;
      if (event.relative_coord_y >= active_element.height)
        event.relative_coord_y = active_element.height - 1;

      switch (active_element)
      {
        case canv: color_mousemove(event); break;
        case canvHue: hue_mousemove(event); break;
        case canvS: s_mousemove(event); break;
        case canvV: v_mousemove(event); break;
        case canvAlpha: alpha_mousemove(event); break;
      }
    }
  }
}

function window_mouse_up(event)
{
  if (!active_element)
    return;
  active_element = null;
  apply_color();
}

window.addEventListener("load", function()
{
  var pipette_icon = "<svg fill=\"#000000\" height=\"14\" width=\"14\" version=\"1.1\" id=\"pipette_svg\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" " +
    "viewBox=\"0 0 296.135 296.135\" xml:space=\"preserve\">" +
    "<path d=\"M284.5,11.635C276.997,4.132,267.021,0,256.411,0s-20.586,4.132-28.089,11.635l-64.681,64.68l-6.658-6.658" +
    "c-2.777-2.777-6.2-4.512-9.786-5.206c-0.598-0.116-1.2-0.202-1.804-0.26s-1.211-0.087-1.817-0.087s-1.213,0.029-1.817,0.087" +
    "s-1.206,0.145-1.804,0.26c-3.585,0.694-7.009,2.43-9.786,5.206v0c-1.388,1.388-2.516,2.938-3.384,4.59" +
    "c-0.289,0.55-0.55,1.112-0.781,1.683c-0.694,1.712-1.128,3.505-1.302,5.317c-0.058,0.604-0.087,1.211-0.087,1.817" +
    "c0,1.213,0.116,2.426,0.347,3.621c0.347,1.793,0.954,3.545,1.822,5.196c0.868,1.651,1.996,3.201,3.384,4.59l4.319,4.319" +
    "L21.468,213.811c-1.434,1.434-2.563,3.143-3.316,5.025l-16.19,40.387c-3.326,8.298-2.338,17.648,2.644,25.013" +
    "c5.04,7.451,13.356,11.899,22.244,11.899c3.432,0,6.817-0.659,10.063-1.961L77.3,277.984c1.882-0.754,3.592-1.883,5.025-3.316" +
    "l113.021-113.021l4.318,4.318c0.463,0.463,0.944,0.897,1.44,1.302c0.993,0.81,2.049,1.504,3.15,2.083" +
    "c2.752,1.446,5.785,2.169,8.818,2.169l0,0c0.029,0,0.058-0.004,0.087-0.004c1.791-0.008,3.58-0.264,5.312-0.777" +
    "c2.345-0.694,4.583-1.851,6.569-3.471c0.497-0.405,0.977-0.839,1.44-1.302v0c2.314-2.314,3.905-5.077,4.772-8.009" +
    "c0.694-2.345,0.926-4.798,0.694-7.216c-0.116-1.209-0.347-2.408-0.694-3.581s-0.81-2.318-1.388-3.419" +
    "c-0.868-1.651-1.996-3.201-3.384-4.59l-6.658-6.658l64.68-64.68C299.988,52.326,299.988,27.124,284.5,11.635z M63.285,251.282" +
    "l-30.764,12.331l12.332-30.763l110.848-110.848l18.432,18.432L63.285,251.282z\"/>" +
    "</svg>"


  var div = document.createElement('div');
  div.innerHTML =
    "<div style=\"position: fixed; top:0px; left:0px; z-index: 1000; -moz-user-select:-moz-none; font-size: 11pt; font-family: verdana;"+
    "  visibility:hidden;" +
    "  user-select:none; -webkit-user-select:none; background:#333; width:570px; height:342px; padding:20px; border: 2px solid #000;\""+
    "  id=\"color_picker_div\">"+
    ""+
    "  <canvas id=\"color_picker_canvas\" width=\"256\" height=\"256\">Canvas is not supported</canvas>"+
    "  <canvas id=\"hue_canvas\" width=\"32\" height=\"256\"></canvas>"+
    "  <canvas id=\"s_canvas\" width=\"32\" height=\"256\"></canvas>"+
    "  <canvas id=\"v_canvas\" width=\"32\" height=\"256\"></canvas>"+
    "  <canvas id=\"alpha_canvas\" width=\"32\" height=\"256\"></canvas>"+
    "  <canvas id=\"colors_canvas\" width=\"128\" height=\"256\"></canvas>"+
    "  <br>"+
    "  <input id=\"color_text_255\" size=\"25\" onkeyup=\"color_text_change_255(this.value);apply_color();\" value=\"255, 255, 255, 255\"/>"+
    "  <input id=\"color_text_1\" size=\"25\" "+
    "    onkeyup=\"color_text_change_1(this.value);apply_color();\" value=\"1.000, 1.000, 1.000, 1.000\"/>"+
    "  <button id=\"request_pipette_btn\" style=\"width: 30px; \" onclick=\"request_pipette();\">" + pipette_icon + "</button>"+
    "  <font size=\"1\"><br><br></font>"+
    "  <button id=\"color_picker_revert_btn\" style=\"width: 100px;\" onclick=\"revert_color();\">Revert</button>"+
    "  <button id=\"color_picker_close_btn\" style=\"width: 100px;\" onclick=\"hide_color_picker();\">Close</button>"+
    "</div>";
	document.body.appendChild(div);

  canv = document.getElementById("color_picker_canvas");
  ctx = canv.getContext("2d");
  canv.addEventListener("mousedown", color_mousedown, false);

  canvHue = document.getElementById("hue_canvas");
  ctxHue = canvHue.getContext("2d");
  canvHue.addEventListener("mousedown", hue_mousedown, false);

  canvS = document.getElementById("s_canvas");
  ctxS = canvS.getContext("2d");
  canvS.addEventListener("mousedown", s_mousedown, false);

  canvV = document.getElementById("v_canvas");
  ctxV = canvV.getContext("2d");
  canvV.addEventListener("mousedown", v_mousedown, false);

  canvAlpha = document.getElementById("alpha_canvas");
  ctxAlpha = canvAlpha.getContext("2d");
  canvAlpha.addEventListener("mousedown", alpha_mousedown, false);

  canvColors = document.getElementById("colors_canvas");
  ctxColors = canvColors.getContext("2d");

  window.addEventListener("mousedown", window_mouse_move, false);
  window.addEventListener("mousemove", window_mouse_move, false);
  window.addEventListener("mouseup", window_mouse_up, false);

  render();
});

