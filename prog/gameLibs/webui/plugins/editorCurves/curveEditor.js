"use strict";

var curve_is_global_mouse_up = true;
var curve_global_event_object = null;

function make_curve_editor_html(name, value, onchange_cb, stroke_style, bck_curves, interpolation, can_add_remove_points) // bck_curves - array of element names
{
  var curve_default_width = 182;
  var curve_default_height = 182;

  var innerHTML = "<canvas class='repaint_canvas' id='curve_editor_canv_" + name +
    "' width='"+curve_default_width+"' height='"+curve_default_height+"'"+
    ' ondragstart="return false;"' +
    " onmousedown='this._curve_mouse_down(event)'" +
    "></canvas>";

  if (value.length >= 1)
  {
    if (value.indexOf("P*/") >= 0)
      interpolation = "polynom";

    if (value.indexOf("L*/") >= 0)
      interpolation = "piecewise_linear";

    if (value.indexOf("M*/") >= 0)
      interpolation = "piecewise_monotonic";

    if (value.indexOf("S*/") >= 0)
      interpolation = "steps";
  }

  if (!interpolation)
    interpolation = "steps";

  if (!can_add_remove_points)
    can_add_remove_points = false;


  if (!value || value.length < 7)
    switch (interpolation)
    {
      case "polynom": value = "0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 P*/"; break;
      case "piecewise_linear": value = "0,0,0,0,0,0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 L*/"; break;
      case "piecewise_monotonic": value = "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 M*/"; break;
      case "steps": value = "0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 S*/"; break;
    }

  function find_steps_coefficients(p)
  {
    var a = [];
    p = p.slice().sort(function(a,b){return a[0] - b[0];});
    for (var i = 0; i < p.length; i++)
    {
      a.push(p[i][0]);
      a.push(p[i][1]);
    }
    return a;
  }

  function steps(x, a)
  {
    var idx = 0;
    for (var i = 0; i < a.length; i += 2)
      if (a[i] < x)
        idx = i;

    return a[idx + 1];
  }



  function find_piecewise_linear_coefficients(p)
  {
    var a = [];
    p = p.slice().sort(function(a,b){return a[0] - b[0];});
    for (var i = 0; i < p.length - 1; i++)
    {
      var k = (p[i + 1][1] - p[i][1]) / (p[i + 1][0] - p[i][0] + 1e-9);
      a.push(p[i][0]);
      a.push(p[i][1]);
      a.push(k);
    }

    return a;
  }

  function piecewise_linear(x, a)
  {
    var idx = 0;
    for (var i = 0; i < a.length; i+=3)
      if (a[i] < x)
        idx = i;

    return a[idx + 1] + a[idx + 2] * (x - a[idx]);
  }


  function piecewise_monotonic(x, a) // a = [ xs, ys, c1s, c2s, c3s, ... ]
  {
    var low = 0;
    var mid = 0;
    var high = (a.length / 5) - 1;
    while (low <= high)
    {
      mid = Math.floor(0.5 * (low + high));
      var xHere = a[mid * 5];
      if (xHere < x)
        low = mid + 1;
      else if (xHere > x)
        high = mid - 1;
      else
        return a[mid * 5 + 1];
    }
    var i = Math.max(0, high) * 5;
    
    var diff = x - a[i];
    var diffSq = diff * diff;
    return a[1 + i] + a[2 + i] * diff + a[3 + i] * diffSq + a[4 + i] * diff * diffSq;
  }

  function find_piecewise_monotonic_coefficients(p)
  {
    var a = [];
    p = p.slice().sort(function(a,b){return a[0] - b[0];});


    var i = 0;
    var length = p.length;
    var xs = [];
    var ys = [];

    for (i = 0; i < length; i++)
    {
      xs.push(p[i][0]);
      ys.push(p[i][1]);
    }

    var dys = []
    var dxs = []
    var ms = [];
    for (i = 0; i < length - 1; i++)
    {
      var dx = xs[i + 1] - xs[i];
      var dy = ys[i + 1] - ys[i];
      dxs.push(dx);
      dys.push(dy);
      ms.push(dy / (dx + 1e-9));
    }

    var c1s = [ms[0]];
    for (i = 0; i < dxs.length - 1; i++)
    {
      var m = ms[i];
      var mNext = ms[i + 1];
      if (m * mNext <= 0)
        c1s.push(0);
      else
      {
        var dx_ = dxs[i];
        var dxNext = dxs[i + 1];
        var common = dx_ + dxNext;
        c1s.push(3 * common / ((common + dxNext + 1e-9) / (m + 1e-9) + (common + dx_) / (mNext + 1e-9)));
      }
    }
    c1s.push(ms[ms.length - 1]);

    // Get degree-2 and degree-3 coefficients
    var c2s = [];
    var c3s = [];
    for (i = 0; i < c1s.length - 1; i++)
    {
      var c1 = c1s[i]
      var m_ = ms[i];
      var invDx = 1.0 / (dxs[i] + 1e-9);
      var common_ = c1 + c1s[i + 1] - m_ - m_;
      c2s.push((m_ - c1 - common_) * invDx);
      c3s.push(common_ * invDx * invDx);
    }


    for (i = 0; i < c3s.length; i++)
    {
      a.push(xs[i]);
      a.push(ys[i]);
      a.push(c1s[i]);
      a.push(c2s[i]);
      a.push(c3s[i]);
    }

    return a;
  }



  function poly(x, a)
  {
    var xp = 1.0;
    var sum = 0.0;
    for (var i = 0; i < a.length; i++)
    {
      sum += xp * a[i];
      xp *= x;
    }

    return sum;
  }

  function find_polynom_coefficients(p)
  {
    var matA = [];
    var vecY = [];
    for (var i = 0; i < p.length; i++)
    {
      vecY.push(p[i][1]);
      matA.push(new Array(p.length).fill(0.0));
    }

    for (var i = 0; i < p.length; i++)
    {
      var x = p[i][0];
      for (var j = 0; j < p.length; j++)
      {
        var one = new Array(p.length).fill(0.0);
        one[j] = 1.0;
        matA[i][j] = poly(x, one);
      }
    }

    function array_fill(i, n, v)
    {
      var a = [];
      for (; i < n; i++)
        a.push(v);
      return a;
    }

    function gauss(A, x)
    {
      var i, k, j;

      for ( i = 0; i < A.length; i++)
        A[i].push(x[i]);

      var n = A.length;

      for ( i = 0; i < n; i++)
      {
        var maxEl = Math.abs(A[i][i]),
            maxRow = i;

        for ( k = i + 1; k < n; k++)
          if (Math.abs(A[k][i]) > maxEl)
          {
            maxEl = Math.abs(A[k][i]);
            maxRow = k;
          }

        for ( k = i; k < n + 1; k++)
        {
          var tmp = A[maxRow][k];
          A[maxRow][k] = A[i][k];
          A[i][k] = tmp;
        }

        for ( k = i + 1; k < n; k++)
        {
          var c = (Math.abs(A[i][i]) > 1e-9) ? -A[k][i] / A[i][i] : 0.0;
          for (j = i; j < n + 1; j++)
          {
            if (i === j)
              A[k][j] = 0;
            else
              A[k][j] += c * A[i][j];
          }
        }
      }

      x = array_fill(0, n, 0);
      for (i = n - 1; i > -1; i--)
      {
        x[i] = (Math.abs(A[i][i]) > 1e-9) ? A[i][n] / A[i][i] : 0.0;
        for (k = i - 1; k > -1; k--)
          A[k][n] -= A[k][i] * x[i];
      }

      return x;
    }

    return gauss(matA, vecY);
  }

  function find_coefficients(p)
  {
    if (interpolation === "piecewise_linear")
      return find_piecewise_linear_coefficients(p);
    if (interpolation === "piecewise_monotonic")
      return find_piecewise_monotonic_coefficients(p);
    else if (interpolation === "polynom")
      return find_polynom_coefficients(p);
    else if (interpolation === "steps")
      return find_steps_coefficients(p);

    alert("Unsupported interpolation: " + interpolation);
  }


  function setup_curve_elem()
  {
    var elem = document.getElementById("curve_editor_canv_" + name);

    if (interpolation === "polynom")
      elem._get_value = function(x)
      {
        if (!this._curve_coefficients)
          return 0;
        return Math.min(Math.max(poly(x, this._curve_coefficients), 0.0), 1.0);
      }
    else if (interpolation === "steps")
      elem._get_value = function(x)
      {
        if (!this._curve_coefficients)
          return 0;
        return Math.min(Math.max(steps(x, this._curve_coefficients), 0.0), 1.0);
      }
    else if (interpolation === "piecewise_linear")
      elem._get_value = function(x)
      {
        if (!this._curve_coefficients)
          return 0;
        return Math.min(Math.max(piecewise_linear(x, this._curve_coefficients), 0.0), 1.0);
      }
    else if (interpolation === "piecewise_monotonic")
      elem._get_value = function(x)
      {
        if (!this._curve_coefficients)
          return 0;
        return Math.min(Math.max(piecewise_monotonic(x, this._curve_coefficients), 0.0), 1.0);
      }
    else
      alert("Unsupported interpolation: " + interpolation);

    elem._curve_render = function()
    {
      this._ctx.fillStyle = "black";
      this._ctx.fillRect(0, 0, 1000, 1000);

      if (this._bck_curves)
      {
        for (var k = 0; k < this._bck_curves.length; k++)
        {
          var e = document.getElementById(this._bck_curves[k]);
          if (e && e._curve_coefficients)
          {
            this._ctx.beginPath();
            for (var i = 0; i < curve_default_width; i += 2)
            {
              var x = i;
              var y = (1.0 - e._get_value(1.0 * i / (curve_default_width - 1))) * (curve_default_height - 1);
              if (i === 0)
                this._ctx.moveTo(x, y);
              else
                this._ctx.lineTo(x, y);
            }
            this._ctx.lineWidth = 2.0;
            this._ctx.strokeStyle = e._curve_style;
            this._ctx.stroke();
          }
        }

        this._ctx.fillStyle = "rgba(0, 0, 0, 0.5)";
        this._ctx.fillRect(0, 0, 1000, 1000);
      }

      for (var pass = 0; pass < 2; pass++)
      {
        this._ctx.beginPath();
        for (var i = 0; i < curve_default_width; i += 2)
        {
          var x = i;
          var y = (1.0 - this._get_value(1.0 * i / (curve_default_width - 1))) * (curve_default_height - 1);
          y = Math.max(Math.min(y, (curve_default_height - 1)), 0);
          if (i === 0)
            this._ctx.moveTo(x, y);
          else
            this._ctx.lineTo(x, y);
        }
        this._ctx.lineWidth = (pass === 1) ? 1 : 2;
        this._ctx.strokeStyle = (pass === 1) ? "white" : stroke_style;
        this._ctx.stroke();
      }


      this._ctx.fillStyle = "white";

      for (var i = 0; i < this._curve_points.length; i++)
        this._ctx.fillRect(
          this._curve_points[i][0] * (curve_default_width - 1) - 2,
          (1.0 - this._curve_points[i][1]) * (curve_default_height - 1) - 2,
          5, 5);
    }

    elem._curve_mouse_down = function(event)
    {
      curve_is_global_mouse_up = false;
      var x = 1.0 * event.offsetX / (curve_default_width - 1);
      var y = 1.0 - 1.0 * event.offsetY / (curve_default_height - 1);
      var bestIndex = 0;
      var bestDist = 1000000;
      for (var i = 0; i < this._curve_points.length; i++)
      {
        var dx = (this._curve_points[i][0] - x) * curve_default_width;
        var dy = (this._curve_points[i][1] - y) * curve_default_height * 0.3;
        var dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < bestDist)
        {
          bestIndex = i;
          bestDist = dist;
        }
      }

      curve_global_event_object = this;

      if (this._curve_add_remove_points && bestDist >= 10)
      {
        this._curve_points.push([x, y]);
        bestIndex = this._curve_points.length - 1;
        this._curve_coefficients = find_coefficients(this._curve_points);
      }

      this._curve_point_idx = bestIndex;
      this._curve_mouse_move(event.offsetX, event.offsetY);
    }

    elem._curve_mouse_move = function(offsetX, offsetY)
    {
      if (curve_is_global_mouse_up || this._curve_point_idx < 0)
      {
        this._curve_point_idx = -1;
        return;
      }

      var x = 1.0 * offsetX / (curve_default_width - 1);
      x = Math.min(Math.max(x, 0.0), 1.0);
      var y = 1.0 - 1.0 * offsetY / (curve_default_height - 1);
      y = Math.min(Math.max(y, 0.0), 1.0);

      if (this._curve_points.length > 2 && (offsetX < -100 || offsetY < -100 ||
        offsetX > curve_default_width + 100 || offsetY > curve_default_height + 100))
      {
        this._curve_points.splice(this._curve_point_idx, 1);
        this._curve_point_idx = -1;
      }
      else
        this._curve_points[this._curve_point_idx] = [x, y];

      this._curve_coefficients = find_coefficients(this._curve_points);
      this._curve_render();
    }

    var ctx = elem.getContext("2d");
    elem._ctx = ctx;

    var v = value.split("/*");
    if (v.length != 2)
      alert("Invalid curve value. Expected 'poly_coefficients /* points */>': " + value);
    var coeffs = v[0].split(",");
    var points = v[1].split(",");

    if (interpolation === "steps")
      if (coeffs.length != points.length)
        alert("Invalid curve value. coeffs.length != (points.length / 2 - 1) * 2: " + value + "\nexpected_coeffs=" + ((points.length/2-1)*3));

    if (interpolation === "polynom")
      if (coeffs.length < points.length / 2)
        alert("Invalid curve value. coeffs.length < points.length / 2: " + value + "\nexpected_coeffs=" + (points.length/2));

    if (interpolation === "piecewise_linear")
      if (coeffs.length != (points.length / 2 - 1) * 3)
        alert("Invalid curve value. coeffs.length != (points.length / 2 - 1) * 3: " + value + "\nexpected_coeffs=" + ((points.length/2-1)*3));

    if (interpolation === "piecewise_monotonic")
      if (coeffs.length != (points.length / 2 - 1) * 5)
        alert("Invalid curve value. coeffs.length != (points.length / 2 - 1) * 5: " + value + "\nexpected_coeffs=" + ((points.length/2-1)*5));

    elem._curve_points = [];
    for (var i = 0; i < points.length; i += 2)
      elem._curve_points.push([parseFloat(points[i]), parseFloat(points[i + 1])]);

    elem._bck_curves = bck_curves;
    elem._curve_coefficients = find_coefficients(elem._curve_points);
    elem._curve_callback = onchange_cb;
    elem._curve_point_idx = -1;
    elem._curve_name = name;
    elem._curve_style = stroke_style;
    elem._curve_interpolation = interpolation;
    elem._curve_add_remove_points = can_add_remove_points;
    setTimeout(function(){elem._curve_render(elem, ctx);}, 1);
    
  }

  setTimeout(setup_curve_elem, 1);
  return innerHTML;
}


function round_curve_number(x)
{
  return Math.round(x * 100000.0) / 100000.0
}

function global_curve_mouse_up(obj, event)
{
  obj._curve_point_idx = -1;
  curve_is_global_mouse_up = true;
  curve_global_event_object = null;

  if (obj._curve_callback)
  {
    var t = "";
    if (obj._curve_interpolation === "steps")
      t = "S";
    if (obj._curve_interpolation === "polynom")
      t = "P";
    if (obj._curve_interpolation === "piecewise_linear")
      t = "L";
    if (obj._curve_interpolation === "piecewise_monotonic")
      t = "M";



    var value = "";

    if (obj._curve_interpolation === "polynom")
      value = obj._curve_coefficients[i].join(", ");
    else
    {
      for (var i = 0; i < obj._curve_coefficients.length; i++)
        value += round_curve_number(obj._curve_coefficients[i]) + (i < obj._curve_coefficients.length - 1 ? ", " : "");
    }

    value += " /*";
    for (var i = 0; i < obj._curve_points.length; i++)
      value += round_curve_number(obj._curve_points[i][0])  + ", " +
               round_curve_number(obj._curve_points[i][1]) +
               (i < obj._curve_points.length - 1 ? ", " : t + "*/");

    obj._curve_callback(obj._curve_name, value);
  }

  setTimeout(function() 
  {
    var canvases = document.getElementsByClassName("repaint_canvas");
    for (var i = 0; i < canvases.length; i++)
      if (canvases[i]._curve_render)
        canvases[i]._curve_render();

  }, 1);

}


function make_curve_gradient_html(name, bck_curves) // bck_curves - array of element names
{
  var curve_default_width = 182;
  var curve_default_height = 32;

  var innerHTML = "<canvas class='repaint_canvas' id='display_gradient_canv_" + name +
    "' width='"+curve_default_width+"' height='"+curve_default_height+"'"+
    "></canvas>";


  function setup_curve_elem()
  {
    var elem = document.getElementById("display_gradient_canv_" + name);

    elem._curve_render = function()
    {
      this._ctx.fillStyle = "black";
      this._ctx.fillRect(0, 0, 1000, 1000);

      var curveElems = [null, null, null];
      for (var k = 0; k < this._bck_curves.length; k++)
        curveElems[k] = document.getElementById(this._bck_curves[k]);

      if (this._bck_curves && this._bck_curves.length >= 3)
      {
        var components = [0, 0, 0];

        for (var i = 0; i < curve_default_width; i += 1)
        {
          var x = 1.0 * i / (curve_default_width - 1);
          for (var k = 0; k < 3; k++)
          {
            var e = curveElems[k];
            components[k] = Math.round(e._get_value(x) * 255);
          }
          this._ctx.fillStyle = "rgb("+components[0]+", "+components[1]+", "+components[2]+")";
          this._ctx.fillRect(i, 0, 3, 100);
        }
      }
    }

    var ctx = elem.getContext("2d");
    elem._ctx = ctx;

    elem._bck_curves = bck_curves;
    elem._curve_point_idx = -1;
    elem._curve_name = name;
    setTimeout(function(){elem._curve_render(elem, ctx);}, 1);
    
  }

  setTimeout(setup_curve_elem, 1);

  return innerHTML;
}



// document.getElementById("place_editor_here").innerHTML = make_curve_editor_html(
//   "curve1", "0.0, 0.0, 0.0, 0.0 /*0.0, 0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0*/", null, "gray");

window.addEventListener('mousemove',
  function(event)
  {
    try
    {
      if (curve_global_event_object)
      {
        var rect = curve_global_event_object.getBoundingClientRect();
        var offsetX = event.pageX - rect.left;
        var offsetY = event.pageY - rect.top;
        curve_global_event_object._curve_mouse_move(offsetX, offsetY);
      }
    }
    catch (err)
    {
      curve_global_event_object = null;
    }
  });

window.addEventListener('mouseup',
  function(event)
  {
    try
    {
      if (curve_global_event_object)
        global_curve_mouse_up(curve_global_event_object, event);
    }
    catch (err)
    {
      curve_global_event_object = null;
    }
  });
