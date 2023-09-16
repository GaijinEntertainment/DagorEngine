"use strict";

var colorGradient = null;
var maxColorGradientPoints = 64;

function ColorGradient(createWithInterface)
{
  this.callback = null;
  this.array = []; // floats T,R,G,B,A, T,R,G,B,A
  this.nearest = false;
  this.buttons = [];
  this.canvas = null;
  this.ctx = null;
  this.gradPosInput = null;

  this.draggingButton = -1;
  this.lastSelected = -1;
  this.prevScreenX = 0;
  this.prevScreenY = 0;

  this.initialArray = [];
  this.initialNearest = false;

  if (createWithInterface)
  {
    var htmlBtn = "";
    for (var i = 0; i < maxColorGradientPoints; i++)
    {
      htmlBtn += "<button id='cgr_" + i + "'" +
          " style='background:rgb(0,0,0); width:9px; height:26px; border: 1px #000 solid; visibility:hidden; z-index: 1002;"+
          " position:absolute; top:71px; left:10px; -webkit-user-select: none; -moz-user-select: none; user-select: none;' " +
          " data-color='1,1,1,1'"+
          " data-pos='0'"+
          " name='" + i + "' " +
          " onmousedown='show_color_picker(this, \"" + i + "\", onGradientColorChanged, true); global_gradient_mouse_down(" + i + ", event);'>" +
          "</button>"
    }

    var div = document.createElement('div');
    div.innerHTML =
      "<div style=\"position: fixed; top:340px; left:0; z-index: 1001; -moz-user-select:-moz-none; font-size: 11pt; font-family: verdana;"+
      "  visibility:hidden;" +
      "  user-select:none; -webkit-user-select:none; background:#333; width:570px; height:130px; padding:20px; border: 2px solid #000;\""+
      "  id=\"color_gradient_div\">"+
      htmlBtn +
      "  <canvas id=\"gradient_canvas_\" width=\"512\" height=\"50\"></canvas>"+
      "  <canvas id=\"creation_canvas_\" width=\"512\" height=\"26\" onmousedown=\"global_create_gradient_color(this, event);\"></canvas>"+
      "  <br>"+
      "  <input id=\"gradient_color_pos_input\" size=\"25\" style='width:60px;' onkeyup=\"global_gradient_pos_change(this.value);\" value=\"0.0\"/>"+
      "  &nbsp;&nbsp;&nbsp;<input type=\"checkbox\" id=\"nearest_cbox\" name=\"nearest_cbox\" value=\"nearest\" onchange=\"onNearestChanged();\"> " + 
      "    <label for=\"nearest_cbox\">Nearest</label>" +
      " <br><br>"+
      "  <button style=\"width: 100px;\" onclick=\"revert_gradient();\">Revert</button>"+
      "  <button style=\"width: 100px;\" onclick=\"hide_gradient();\">Close</button>"+
      "</div>";

    document.body.appendChild(div);
  }


  this.sortByT = function()
  {
    var sorted = true;
    for (var i = this.array.length / 5 - 1; i >= 0; i--)
    {
      sorted = true;
      for (var j = 0; j < i; j++)
        if (this.array[j * 5] > this.array[(j + 1) * 5])
        {
          sorted = false;
          if (j == this.draggingButton)
            this.draggingButton = j + 1;
          else if (j + 1 == this.draggingButton)
            this.draggingButton = j;

          if (j == this.lastSelected)
            this.lastSelected = j + 1;
          else if (j + 1 == this.lastSelected)
            this.lastSelected = j;

          for (var k = j * 5; k < (j + 1) * 5; k++)
          {
            var tmp = this.array[k];
            this.array[k] = this.array[k + 5];
            this.array[k + 5] = tmp;
          }
        }

      if (sorted)
        break;
    }
  }

  this.getColor = function(t)
  {
    var len = this.array.length / 5;

    if (len == 0)
      return [0.5, 0.5, 0.5, 1];
    else if (len == 1)
      return [this.array[1], this.array[2], this.array[3], this.array[4]];
    else if (t <= this.array[0])
      return [this.array[1], this.array[2], this.array[3], this.array[4]];
    else if (t >= this.array[(len - 1) * 5])
      return [this.array[(len - 1) * 5 + 1], this.array[(len - 1) * 5 + 2], this.array[(len - 1) * 5 + 3], this.array[(len - 1) * 5 + 4]];

    for (var i = 1; i < len; i++)
      if (t < this.array[i * 5])
      {
        var k = this.nearest ? 0.0 : (t - this.array[(i - 1) * 5]) / (this.array[i * 5] - this.array[(i - 1) * 5] + 1e-6);
        var invK = 1.0 - k;

        var res = [0, 0, 0, 0];
        for (var n = 0; n < 4; n++)
        {
          res[n] = this.array[(i - 1) * 5 + n + 1] * invK + this.array[i * 5 + n + 1] * k;

          if (res[n] < 0.0)
            res[n] = 0.0;

          if (res[n] > 1.0)
            res[n] = 1.0;
        }

        return res;
      }

    return [this.array[1], this.array[2], this.array[3], this.array[4]];
  }

  this.renderGradient = function(canvas)
  {
    this.sortByT();

    if (canvas.width < 3)
      return;

    var ctx = canvas.getContext("2d");

    var pixels = ctx.getImageData(0, 0, canvas.width, canvas.height);
    var p = 0;
    var step = 4 * canvas.width;
    var alphaHeight = Math.round(0.25 * canvas.height);
    for (var x = 0; x < canvas.width; x++)
    {
      p = 4 * x;
      var color = this.getColor(x / (canvas.width - 1.0));
      var _r = Math.round(color[0] * 255.0);
      var _g = Math.round(color[1] * 255.0);
      var _b = Math.round(color[2] * 255.0);
      var _a = Math.round(color[3] * 255.0);

      for (var y = 0; y < canvas.height - alphaHeight; y++)
      {
        pixels.data[p + 0] = _r;
        pixels.data[p + 1] = _g;
        pixels.data[p + 2] = _b;
        pixels.data[p + 3] = 255;
        p += step;
      }

      for (var y = 0; y < alphaHeight; y++)
      {
        pixels.data[p + 0] = _a;
        pixels.data[p + 1] = _a;
        pixels.data[p + 2] = _a;
        pixels.data[p + 3] = 255;
        p += step;
      }
    }
    ctx.putImageData(pixels, 0, 0);
  }

  this.setupColorButtons = function(ignoreEditPos)
  {
    this.sortByT();

    for (var i = 0; i < maxColorGradientPoints; i++)
    {
      var btn = this.buttons[i];
      var idx = i * 5;
      if (idx >= this.array.length)
        btn.style.visibility = "hidden";
      else
      {
        btn.style.visibility = "visible";
        btn.style.left = "" + Math.round(20 - 6 + this.array[idx] * 512) + "px";
        btn.dataset.color = "" + this.array[idx + 1] + ", " + this.array[idx + 2] + ", " + this.array[idx + 3] + ", " + this.array[idx + 4];
        btn.style.background = "rgb(" + Math.round(this.array[idx + 1] * 255) + ", " +
                                Math.round(this.array[idx + 2] * 255) + ", " + Math.round(this.array[idx + 3] * 255) + ")";
        btn.dataset.pos = "" + this.array[idx];

        btn.style.borderColor = this.lastSelected == i ? "white" : "black";
        if (this.lastSelected == i && !ignoreEditPos)
        {
          this.gradPosInput.value = ("" + this.array[idx]).substring(0, 5);
        }
      }
    }
  }

  this.initFromGradientStr = function(gradientStr)
  {
    this.nearest = (gradientStr[0] === "N");
    if (this.nearest)
      gradientStr = gradientStr.slice(1);

    this.array = gradientStr.split(",");
    for (var i = 0; i < this.array.length; i++)
      this.array[i] = parseFloat(this.array[i]);

    if (this.array.length < 5)
      this.array = [0.0, 0.0, 0.0, 0.0, 1.0,  1.0, 1.0, 1.0, 1.0, 1.0];

    this.sortByT();
  }

  this.show = function(gradientStr, name, callback)
  {
    this.canvas = document.getElementById("gradient_canvas_");
    this.gradPosInput = document.getElementById("gradient_color_pos_input");
    this.nearestCheckbox = document.getElementById("nearest_cbox");
    document.getElementById("color_gradient_div").style.visibility = 'visible';

    this.ctx = this.canvas.getContext("2d");
    this.draggingButton = -1;
    this.lastSelected = 0;
    this.name = name;
    this.callback = callback;

    for (var i = 0; i < maxColorGradientPoints; i++)
      this.buttons[i] = document.getElementById("cgr_" + i);

    this.initFromGradientStr(gradientStr);

    this.setupColorButtons();
    this.renderGradient(this.canvas);

    this.nearestCheckbox.checked = this.nearest;

    this.initialArray = this.array.slice(0);
    this.initialNearest = this.nearest;

    show_color_picker(this.buttons[0], "0", onGradientColorChanged, true);
  }

  this.applyChanges = function()
  {
    if (this.callback)
      this.callback(this.name, (this.nearest ? "N" : "") + this.array.join(", "));
  }

  this.onMouseUp = function()
  {
    if (this.draggingButton >= 0)
    {
      this.draggingButton = -1;
      this.applyChanges();
    }
  }

  this.onMouseMove = function(e)
  {
    var prevDragging = this.draggingButton;

    if (this.draggingButton >= 0)
    {
      this.array[this.draggingButton * 5] += (e.screenX - this.prevScreenX) / 512.0;
      this.array[this.draggingButton * 5] = Math.min(Math.max(0.0, this.array[this.draggingButton * 5]), 1.0);

      this.setupColorButtons();
      this.renderGradient(this.canvas);

      this.prevScreenX = e.screenX;

      if (Math.abs(e.screenY - this.prevScreenY) > 200 && this.array.length > 5) // remove
      {
        this.array.splice(this.draggingButton * 5, 5);
        this.setupColorButtons();
        this.renderGradient(this.canvas);
        this.onMouseUp();
      }
    }

    if (prevDragging !== this.draggingButton)
    {
      var idx = Math.max(this.draggingButton, 0);
      this.lastSelected = idx;
      show_color_picker(this.buttons[idx], "" + idx, onGradientColorChanged, true);
      this.setupColorButtons();
    }
  }

  this.onCreateGradientColor = function(t)
  {
    if (this.array.length / 5 > 30)
      return;

    t = Math.min(Math.max(0.0, t), 1.0);
    var color = this.getColor(t);
    this.array.push(t);
    this.array.push(color[0]);
    this.array.push(color[1]);
    this.array.push(color[2]);
    this.array.push(color[3]);

    this.draggingButton = this.array.length / 5 - 1;
    this.lastSelected = this.draggingButton;
    this.sortByT();
    this.setupColorButtons();
    this.renderGradient(this.canvas);
    show_color_picker(this.buttons[this.draggingButton], "" + this.draggingButton, onGradientColorChanged, true);
  }

  this.hide = function()
  {
    colorGradient.onMouseUp();
    hide_color_picker();
    document.getElementById("color_gradient_div").style.visibility = 'hidden';

    for (var i = 0; i < maxColorGradientPoints; i++)
    {
      var btn = this.buttons[i];
      btn.style.visibility = "hidden";
    }
  }
}

function onGradientColorChanged(color4FloatStr, e3dcolorHexStr, name)
{
  var array = color4FloatStr.split(",");
  for (var i = 0; i < array.length; i++)
    array[i] = parseFloat(array[i]);

  var idx = parseInt(name) * 5;
  colorGradient.array[idx + 1] = array[0];
  colorGradient.array[idx + 2] = array[1];
  colorGradient.array[idx + 3] = array[2];
  colorGradient.array[idx + 4] = array[3];


  colorGradient.setupColorButtons();
  colorGradient.renderGradient(colorGradient.canvas);
  colorGradient.applyChanges();
}

function onNearestChanged()
{
  colorGradient.nearest = colorGradient.nearestCheckbox.checked;
  colorGradient.renderGradient(colorGradient.canvas);
  colorGradient.applyChanges();
}


function global_gradient_mouse_down(idx, e)
{
  if (e.button != 0)
    return;

  colorGradient.draggingButton = idx;
  colorGradient.lastSelected = idx;
  colorGradient.prevScreenX = e.screenX;
  colorGradient.prevScreenY = e.screenY;
  colorGradient.setupColorButtons();
}

function global_create_gradient_color(elem, e)
{
  if (e.button != 0)
    return;

  var t = (e.clientX - elem.offsetLeft) * 1.0 / (elem.width - 1);
  colorGradient.prevScreenX = e.screenX;
  colorGradient.prevScreenY = e.screenY;
  colorGradient.onCreateGradientColor(t);
}

function global_gradient_pos_change(val)
{
  if (colorGradient.lastSelected >= 0 && colorGradient.lastSelected * 5 < colorGradient.array.length)
  {
    if (isNaN(parseFloat(val)))
      val = "0";

    colorGradient.array[colorGradient.lastSelected * 5] = Math.min(Math.max(parseFloat(val), 0.0), 1.0);
    colorGradient.setupColorButtons(true);
    colorGradient.renderGradient(colorGradient.canvas);
    colorGradient.applyChanges();
  }
}

function revert_gradient()
{
  colorGradient.array = colorGradient.initialArray.slice(0);
  colorGradient.nearest = colorGradient.initialNearest;
  colorGradient.setupColorButtons();
  colorGradient.nearestCheckbox.checked = colorGradient.nearest;
  colorGradient.renderGradient(colorGradient.canvas);
  colorGradient.applyChanges();
}

function hide_gradient()
{
  if (colorGradient)
    colorGradient.hide();
}

function show_gradient(gradientStr, name, callback)
{
  if (colorGradient)
    colorGradient.show(gradientStr, name, callback);
}

window.addEventListener("mouseup", function()
{
  if (colorGradient)
    colorGradient.onMouseUp();
});

window.addEventListener("mousemove", function(e)
{
  if (colorGradient)
    colorGradient.onMouseMove(e);
});

window.addEventListener("load", function()
{
  colorGradient = new ColorGradient(true);
});
