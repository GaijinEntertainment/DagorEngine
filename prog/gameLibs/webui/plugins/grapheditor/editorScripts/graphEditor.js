"use strict";

var pluginName = "";
var renderEnabled = false;
var typeConv = null;
var nodeDescriptions = null;
var editor = null;
var xmlns = "http://www.w3.org/2000/svg";
var draggingElementName = null;
var openGraphDialog = null;

var uidIncrementalPart = 1;

var globalLockGraphics = false;
var globalLockChange = false;
var globalLockInput = false;
var loadedWithErrors = false;

var S_FULL_INFO = 1 << 0;
var S_SELECTED_ONLY = 1 << 1;
var S_SKIP_GRAPH_ID = 1 << 2;
var S_COPY_INPUT_EDGES = 1 << 3;
var S_SKIP_POSTPROCESS = 1 << 4;
var S_RETURN_AS_OBJECT = 1 << 5;
var S_INOUT_PINS = 1 << 6;

var idxToChar = ['x', 'y', 'z', 'w'];

var cachedSizes = {}

function sign(x)
{
  return x < 0.0 ? -1.0 : x > 0.0 ? 1.0 : 0.0;
}

function isPluginMatching(pluginName, expectedName)
{
  if (!pluginName)
    return true;

  if (expectedName === "shader_editor")
  {
    if (pluginName !== "[[plugin:shader_editor]]" &&
        pluginName !== "[[plugin:fog_shader_editor]]" &&
        pluginName !== "[[plugin:gpu_object_shader_editor]]")
    return false;
  }
  else if (pluginName !== "[[plugin:" + expectedName + "]]")
    return false;

  return true;
}

function generateRandomUid()
{
  uidIncrementalPart++;
  return "" + uidIncrementalPart + "-" + Date.now();
}


function safeHtmlStr(s)
{
  return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}


function Connection()
{
  this.myIndex = -1;
  this.svgPath = null;
  this.elemFrom = -1;
  this.elemTo = -1;
  this.pinFrom = -1;
  this.pinTo = -1;

  this.pinFromUid = null;
  this.pinToUid = null;

  this.fromX = 0;
  this.fromY = 0;
  this.fromDir = 50;
  this.toX = 0;
  this.toY = 0;
  this.toDir = -50;

  this.hidden = false;

  this.create = function(myIndex, elemFrom, pinFrom, elemTo, pinTo, pinFromUid, pinToUid)
  {
    this.myIndex = myIndex;
    this.elemFrom = elemFrom;
    this.elemTo = elemTo;
    this.pinFrom = pinFrom;
    this.pinTo = pinTo;
    this.pinFromUid = pinFromUid;
    this.pinToUid = pinToUid;

    this.update();
  }

  this.kill = function()
  {
    if (this.myIndex >= 0)
    {
      editor.edges[this.myIndex] = null;
      this.myIndex = -1;

      if (this.svgPath && !globalLockGraphics && renderEnabled)
      {
        this.svgPath.parentNode.removeChild(this.svgPath);
        this.svgPath = null;
      }
    }
  }

  this.findPinIdByUid = function(elem, defaultIndex, pinUid)
  {
    if (!pinUid)
      return defaultIndex;

    for (var i = 0; i < elem.desc.pins.length; i++)
      if (elem.desc.pins[i].uid === pinUid)
        return i;

    return -1;
  }

  this.update = function()
  {
    var from = editor.elems[this.elemFrom];
    var to = editor.elems[this.elemTo];

    if (!from || !to)
    {
      alert("error in edge update");
      return;
    }

    this.pinFrom = this.findPinIdByUid(from, this.pinFrom, this.pinFromUid);
    this.pinTo = this.findPinIdByUid(to, this.pinTo, this.pinToUid);

    if (this.pinFrom === -1 || this.pinTo === -1)
    {
      this.kill();
      return;
    }

    if (!this.pinFromUid && from.desc.pins[this.pinFrom].uid)
      this.pinFromUid = from.desc.pins[this.pinFrom].uid;
    if (!this.pinToUid && to.desc.pins[this.pinTo].uid)
      this.pinToUid = to.desc.pins[this.pinTo].uid;

    if (globalLockGraphics || !renderEnabled)
      return;

    var pos = from.getRelativePinPos(this.pinFrom);
    this.fromX = pos[0] + from.x - from.leftOffset;
    this.fromY = pos[1] + from.y;
    this.fromDir = pos[2] ? 1 : -1;
    var pos = to.getRelativePinPos(this.pinTo);
    this.toX = pos[0] + to.x - to.leftOffset;
    this.toY = pos[1] + to.y;
    this.toDir = pos[2] ? 1 : -1;

    var dist = Math.min(Math.max(Math.abs(this.fromX - this.toX), Math.abs(this.fromY - this.toY)) * 0.5, 300) + 10;
    this.fromDir *= dist;
    this.toDir *= dist;

    if (!this.svgPath)
    {
      this.svgPath = document.createElementNS(xmlns, "path");
      editor.edgesSvg.appendChild(this.svgPath);
    }

    this.highlighted =
      editor.selected.indexOf(this.elemFrom) >= 0 ||
      editor.selected.indexOf(this.elemTo) >= 0;

    this.svgPath.setAttributeNS(null, "stroke-width", this.highlighted ? 6 : 2);
    this.svgPath.setAttributeNS(null, "stroke", "#4ef");

    this.svgPath.setAttributeNS(null, "fill", "transparent");
    this.svgPath.setAttributeNS(null, "d", "M" + this.fromX + " " + this.fromY +
      "C " + (this.fromX + this.fromDir) + " " + this.fromY + ", " + (this.toX + this.toDir) + " " + this.toY +
      ", " + this.toX + " " + this.toY);

  }
}

function Element()
{
  this.myIndex = -1;
  this.desc = null;
  this.svgElem = null;
  this.svgRectElem = null;
  this.svgComment = null;
  this.svgBlock = null;
  this.x = 0;
  this.y = 0;
  this.moveX = 0;
  this.moveY = 0;
  this.width = 1;
  this.height = 1;
  this.blockWidth = 0;
  this.blockHeight = 0;
  this.leftOffset = 0;
  this.propValues = [];
  this.leftPins = 0;
  this.rightPins = 0;
  this.pinTypes = []; // array of arrays
  this.connected = [];
  this.typeEmitter = [];
  this.pinComments = [];
  this.largestGroupTypes = {};

  this.isPinOut = function(pinIndex)
  {
    return (this.desc.pins[pinIndex].role !== "in");
  }

  this.getRelativePinPos = function(pinIndex)
  {
    var p = this.desc.pins[pinIndex];
    return [(p.isOut ? 1 : 0) * this.width, p.colIndex * 20 + 40, p.isOut];
  }

  this.determineIfOptional = function(idx)
  {
    var pin = this.desc.pins[idx];
    if(pin.optionalIdentifier)
    {
      for (var j = 0; j < this.desc.properties.length; j++)
      {
        var prop = this.desc.properties[j];
        if(pin.optionalIdentifier["prop"] == prop.name)
        {
          if (parseInt(pin.optionalIdentifier["idx"]) <= parseInt(this.propValues[j]))
          {
            return false;
          }
          else
          {
            return true;
          }
        }
      }
    }
    return false;
  }
  this.initPinTypes = function()
  {
    for (var i = 0; i < this.desc.pins.length; i++)
    {
      this.connected[i] = false;
      var pin = this.desc.pins[i];
      if (pin.selectableTypes) // For gbuffer property read
      {
        for (var j = 0; j < this.desc.properties.length; j++)
        {
          if(this.desc.properties[j].type == "multitype_combobox" && this.desc.properties[j].linkedPin == pin.name)
          {
            if (typeof this.propValues[j] === "undefined") // If propvalues undefined use this fallback
              this.pinTypes[i] = [pin.selectableTypes[Object.keys(pin.selectableTypes)[0]]];
            else
              this.pinTypes[i] = [pin.selectableTypes[this.propValues[j]]];
          }
        }
      }
      else if (pin.types)
        this.pinTypes[i] = pin.types.slice(0);
    }
  }

  this.hideOptionalPins = function()
  {
    if (!this.svgElem)
      return;

    var pinRectangles = this.svgElem.__pinsList;
    var visibleIdx = -1;
    for (var i = 0; i < this.desc.pins.length; i++)
    {
      var p = this.desc.pins[i];
      if (!p.hidden)
      {
        visibleIdx++;
        pinRectangles[visibleIdx].style.visibility = this.determineIfOptional(i) ? "hidden" : "visible";
        for (var j = 0; j < this.lodTexts.length; j++)
        {
          if(this.lodTexts[j].getAttributeNS(null, "id") == p.name)
            this.lodTexts[j].style.visibility = this.determineIfOptional(i) ? "hidden" : "visible";
        }
      }
    }
  }

  this.create = function(myIndex, desc_)
  {
    this.myIndex = myIndex;
    this.desc = desc_;
    this.width = this.desc.width;

    var haveExternalName = false;
    for (var i = 0; i < this.desc.properties.length; i++)
    {
      this.propValues.push(this.desc.properties[i].val);
      if (this.desc.properties[i].name === "external name")
        haveExternalName = true;
    }

    if (haveExternalName)
      this.externalUid = generateRandomUid();

    this.uid = generateRandomUid();


    if (myIndex < 0) // graph properties
      return;

    this.initPinTypes();

    if (globalLockGraphics || !renderEnabled)
      return;

    this.lodTexts = [];

    var group = document.createElementNS(xmlns, "g");
    group.__pinsList = [];
    group.setAttributeNS(null, "transform", "translate(" + this.x + " " + this.y + ")");

    if (this.desc.name === "block" && editor.elemsSvg.firstChild)
      editor.elemsSvg.insertBefore(group, editor.elemsSvg.firstChild);
    else
      editor.elemsSvg.appendChild(group);

    this.svgElem = group;

    if (this.desc.name === "block")
    {
      this.blockWidth = 500;
      this.blockHeight = 300;
      var elem = document.createElementNS(xmlns, "rect");
      elem.setAttributeNS(null, "x", 0);
      elem.setAttributeNS(null, "y", 0);
      elem.setAttributeNS(null, "width", this.blockWidth);
      elem.setAttributeNS(null, "height", this.blockHeight);
//      elem.setAttributeNS(null, "class", "blockRectClass");
      this.svgBlock = elem;
      group.appendChild(elem);
    }

    var elem = document.createElementNS(xmlns, "rect");
    elem.setAttributeNS(null, "x", 0);
    elem.setAttributeNS(null, "y", 0);
    elem.setAttributeNS(null, "width", this.width);
//    elem.setAttributeNS(null, "rx", 5);
//    elem.setAttributeNS(null, "ry", 5);
    elem.setAttributeNS(null, "class", "elemRectClass");
    this.svgRect = elem;

    group.appendChild(elem);

    var leftPins = 0;
    var rightPins = 0;
    var pos = [0, 0];
    for (var i = 0; i < this.desc.pins.length; i++)
    {
      if (this.desc.pins[i].hidden)
        continue;

      var isOut = this.isPinOut(i);
      if (isOut)
      {
        this.desc.pins[i].isOut = true;
        this.desc.pins[i].colIndex = rightPins++;
      }
      else
      {
        this.desc.pins[i].isOut = false;
        this.desc.pins[i].colIndex = leftPins++;
      }

      pos = this.getRelativePinPos(i);

      var pin = document.createElementNS(xmlns, "rect");
      group.appendChild(pin);

      var pinSize = this.desc.pins[i].main ? 4.5 : 4;
      pin.setAttributeNS(null, "x", pos[0] - pinSize);
      pin.setAttributeNS(null, "y", pos[1] - pinSize);
      pin.setAttributeNS(null, "width", pinSize * 2);
      pin.setAttributeNS(null, "height", pinSize * 2);
      pin.setAttributeNS(null, "class", this.desc.pins[i].main ? "elemMainPinClass" : "elemPinClass");
      group.__pinsList.push(pin);

      var txt = document.createElementNS(xmlns, "text");
      group.appendChild(txt);
      this.lodTexts.push(txt);

      txt.setAttributeNS(null, "x", pos[0] + (isOut ? -7 : 7));
      txt.setAttributeNS(null, "y", pos[1] - 16 / 2);
      txt.setAttributeNS(null, "text-anchor", isOut ? "end" : "start");
      txt.setAttributeNS(null, "class", "elemTextClass");
      txt.setAttributeNS(null, "id", this.desc.pins[i].name);
      txt.textContent = this.desc.pins[i].name;

      var comm = document.createElementNS(xmlns, "text");
      group.appendChild(comm);
      this.lodTexts.push(comm);

      comm.setAttributeNS(null, "x", pos[0] + (isOut ? 7 : -7));
      comm.setAttributeNS(null, "y", pos[1] - 16 / 2);
      comm.setAttributeNS(null, "text-anchor", isOut ? "start" : "end");
      comm.setAttributeNS(null, "class", "elemCommentClass");
      comm.setAttributeNS(null, "id", this.desc.pins[i].name + "__comment");
      comm.textContent = "";
    }


    var txt = document.createElementNS(xmlns, "text");
    group.appendChild(txt);
    this.lodTexts.push(txt);

    txt.setAttributeNS(null, "x", this.width / 2);
    txt.setAttributeNS(null, "y", 6);
    txt.setAttributeNS(null, "text-anchor", "middle");
    txt.setAttributeNS(null, "class", "elemTextClass");

    if (this.desc.name !== "comment" && this.desc.name !== "block")
      txt.textContent = this.desc.name;

    this.height = Math.max(leftPins, rightPins) * 20 + 40 + (this.desc.addHeight ? this.desc.addHeight : 0);
    this.leftPins = leftPins;
    this.rightPins = rightPins;
    elem.setAttributeNS(null, "height", this.height);

    var txt1 = document.createElementNS(xmlns, "text");
    group.appendChild(txt1);
    this.lodTexts.push(txt1);
    txt1.setAttributeNS(null, "x", this.width / 2);
    txt1.setAttributeNS(null, "y", this.height + 3);
    txt1.setAttributeNS(null, "text-anchor", "middle");
    txt1.setAttributeNS(null, "class", "elemOuterTextClass");
    txt1.textContent = "";

    var txt2 = document.createElementNS(xmlns, "text");
    group.appendChild(txt2);
    this.lodTexts.push(txt2);
    txt2.setAttributeNS(null, "x", this.width / 2);
    txt2.setAttributeNS(null, "y", this.height + 3 + 16);
    txt2.setAttributeNS(null, "text-anchor", "middle");
    txt2.setAttributeNS(null, "class", "elemOuterTextClass");
    txt2.textContent = "";

    this.outerText = [txt1, txt2];


    if (this.desc.name === "comment" || this.desc.name === "block")
    {
      this.svgComment = document.createElementNS(xmlns, "text");

      group.appendChild(this.svgComment);
      this.svgComment.setAttributeNS(null, "x", 10);
      this.svgComment.setAttributeNS(null, "y", 10);
      this.svgComment.setAttributeNS(null, "text-anchor", "left");
      this.svgComment.setAttributeNS(null, "class", "commentTextClass");
      this.svgComment.setAttributeNS(null, "id", "comment string");
    }

    this.updatePinCaptions();
    this.updatePinColors();
//    this.update();
  }

  this.setOuterText = function(line, text) // line = 0..1
  {
    if (renderEnabled && !globalLockGraphics && this.outerText)
      this.outerText[line].textContent = text;
  }


  this._findPinCommentIndex = function(pinIndex)
  {
    if (this.pinComments.length == 0)
      return -1;

    if (this.desc.pins[pinIndex].uid)
      for (var i = 0; i < this.pinComments.length; i++)
      {
        var comm = this.pinComments[i];
        if (comm && comm.pinUid && comm.pinUid === this.desc.pins[pinIndex].uid)
          return i;
      }

    for (var i = 0; i < this.pinComments.length; i++)
    {
      var comm = this.pinComments[i];
      if (comm && comm.pinIndex === pinIndex)
        return i;
    }

    return -1;
  }

  this.setPinComment = function(pinIndex, text)
  {
    var commentIndex = this._findPinCommentIndex(pinIndex);
    if (commentIndex >= 0)
    {
      this.pinComments[commentIndex].text = text;
      if (text.length == 0)
        this.pinComments.splice(commentIndex, 1);
    }
    else
    {
      var comm = {
        pinIndex: pinIndex,
        pinUid: this.desc.pins[pinIndex].uid,
        text: text,
      };

      this.pinComments.push(comm);
    }
  }

  this.getPinComment = function(pinIndex)
  {
    var commentIndex = this._findPinCommentIndex(pinIndex);
    if (commentIndex >= 0)
      return this.pinComments[commentIndex].text;
    else
      return "";
  }

  this.getPinVisualType = function(pinIndex)
  {
    if (this.pinTypes[pinIndex].length === 1)
      return this.pinTypes[pinIndex][0];

    if (this.desc.pins[pinIndex].group)
      return this.largestGroupTypes[this.desc.pins[pinIndex].group];
    else
      return null;
  }



  this.updatePinColors = function()
  {
    if (globalLockGraphics || !renderEnabled)
      return;

    if (editor.typeColors && this.svgElem)
    {
      var visibleIdx = -1;

      var circles = this.svgElem.__pinsList;
      if (!circles.length)
        return;

      for (var i = 0; i < this.desc.pins.length; i++)
      {
        if (!this.desc.pins[i].hidden)
        {
          visibleIdx++;
          var type = this.getPinVisualType(i);
          circles[visibleIdx].style.fill = type ? editor.typeColors[type] : "#fff";
        }
      }
    }
  }


  this.getCachedSize = function(svg, text, fontSize)
  {
    var key = text + "_#" + fontSize;
    if (!cachedSizes[key])
    {
      var box = svg.getBBox();
      cachedSizes[key] = {width: box.width,  height: box.height};
      return cachedSizes[key];
    }

    return cachedSizes[key];
  }


  this.updatePinCaptions = function()
  {
    if (!this.svgElem || globalLockGraphics || !renderEnabled)
      return;

    if (this.svgComment)
    {
      var text = "";
      var fontSize = 32;
      for (var i = 0; i < this.desc.properties.length; i++)
      {
        var name = this.desc.properties[i].name;
        var value = this.propValues[i];
        if (name === "comment string")
          text = "\xa0" + value + "\xa0";
        else if (name === "font size")
          fontSize = value;
      }

      this.svgComment.textContent = text;
      this.svgComment.setAttributeNS(null, "style", "font-size:" + fontSize + "px");

      var box = this.getCachedSize(this.svgComment, text, fontSize);

      this.width = box.width + 20;
      this.height = box.height + 20;
      this.svgRect.setAttributeNS(null, "width", box.width + 20);
      this.svgRect.setAttributeNS(null, "height", box.height + 20);
    }

    var maxSize = 0

    for (var i = 0; i < this.desc.pins.length; i++)
    {
      var commentText = this.getPinComment(i);
      var texts = this.svgElem.getElementsByTagName("text");

      for (var j = 0; j < texts.length; j++)
      {
        var textId = texts[j].getAttributeNS(null, "id");
        if (textId === this.desc.pins[i].name + "__comment")
          texts[j].textContent = commentText;
      }

      var caption = this.desc.pins[i].caption;
      if (caption)
      {
        for (var j = 0; j < this.propValues.length; j++)
        {
          var replaceFrom = "%" + this.desc.properties[j].name + "%";
          var replaceTo = this.propValues[j];
          caption = caption.split(replaceFrom).join(replaceTo);
        }

        for (var j = 0; j < texts.length; j++)
        {
          if (texts[j].getAttributeNS(null, "id") === this.desc.pins[i].name)
          {
            var captionLines = caption.split("\n");
            if (captionLines.length == 1)
            {
              texts[j].textContent = caption;
              if (this.desc.dynSizeToLeft || this.desc.dynSizeToRight)
              {
                var bbox = texts[j].getBBox();
                if (bbox.width > maxSize)
                  maxSize = bbox.width
              }
            }
            else
            {
              texts[j].textContent = "";
              var spans = this.svgElem.getElementsByTagName("tspan");
              for (var k = spans.length; k < captionLines.length; k++)
              {
                var sp = document.createElementNS(xmlns, "tspan");
                texts[j].appendChild(sp);

                sp.setAttributeNS(null, "x", texts[j].getAttributeNS(null, "x"));
                sp.setAttributeNS(null, "dy", k == 0 ? "0" : "1.2em");
              }

              for (var k = 0; k < spans.length; k++)
              {
                spans[k].textContent = captionLines[k];

                if (this.desc.dynSizeToLeft || this.desc.dynSizeToRight)
                {
                  var bbox = spans[k].getBBox();
                  if (bbox.width > maxSize)
                    maxSize = bbox.width
                }
              }

              for (var k = captionLines.length; k < spans.length; k++)
                spans[k].parentNode.removeChild(spans[k]);
            }
          }
        }
      }
    }

    if (this.desc.dynSizeToLeft || this.desc.dynSizeToRight && this.svgElem)
    {
      maxSize += 40;
      maxSize = Math.ceil(maxSize / 20.0) * 20.0;
      if (maxSize < this.desc.width)
        maxSize = this.desc.width;
      this.svgElem.setAttributeNS(null, "width", maxSize);
      this.svgRect.setAttributeNS(null, "width", maxSize);
      if (this.desc.dynSizeToLeft)
      {
        this.leftOffset = maxSize - this.desc.width;
        this.svgRect.setAttributeNS(null, "x", -this.leftOffset);
      }
      this.width = maxSize;
    }
  }

  this.update = function()
  {
    if (globalLockGraphics || !renderEnabled)
      return;

    if (this.svgBlock)
    {
      this.svgBlock.setAttributeNS(null, "width", this.blockWidth);
      this.svgBlock.setAttributeNS(null, "height", this.blockHeight);

      var props = this.desc.properties
      for (var i = 0; i < props.length; i++)
        if (props[i] && props[i].name == "color")
        {
          var col = [0, 0, 0];
          var v = this.propValues[i].split(",");
          for (var i = 0; i < 3; i++)
            col[i] = "" + Math.round((parseFloat(v[i]) * 255.0));

          this.svgBlock.setAttributeNS(null, "fill", "rgb(" + col[0] + "," + col[1] + "," + col[2] + ")");
          this.svgBlock.setAttributeNS(null, "fill-opacity", "0.2");

          this.svgBlock.setAttributeNS(null, "stroke-width", "2px");
          this.svgBlock.setAttributeNS(null, "stroke", "#000");

          break;
        }
    }

    if (this.svgElem)
    {
      if (editor.specialSelected === this.myIndex)
      {
        var outerBorder = editor.specialSelectSvg;
        outerBorder.setAttributeNS(null, "transform", "translate(" + this.x + " " + this.y + ")");
        outerBorder.style.visibility = "visible";
        outerBorder.setAttributeNS(null, "x", -19 - this.leftOffset);
        outerBorder.setAttributeNS(null, "y", -19);
        outerBorder.setAttributeNS(null, "width", this.width + 38);
        outerBorder.setAttributeNS(null, "height", this.height + 38);
      //  outerBorder.setAttributeNS(null, "rx", 9);
      //  outerBorder.setAttributeNS(null, "ry", 9);
      }
      this.svgElem.setAttributeNS(null, "transform", "translate(" + this.x + " " + this.y + ")");
    }
  }

  this.kill = function()
  {
    if (this.myIndex >= 0)
    {
      if (this.myIndex === editor.specialSelected)
        editor.specialSelect(-1);

      editor.elems[this.myIndex] = null;
      this.myIndex = -1;
      this.desc = null;
      this.propValues = null;

      if (this.svgElem && !globalLockGraphics && renderEnabled)
      {
        this.svgElem.parentNode.removeChild(this.svgElem);
        this.svgElem = null;
        this.svgRect = null;
      }
    }
  }
}



function GraphEditor()
{
  this.mainSvg = null;
  this.edgesSvg = null;
  this.elemsSvg = null;
  this.selectSvg = null;
  this.pinSelectSvg = null;
  this.lineSvg = null;
  this.specialSelectSvg = null;
  this.backgroundDiv = null;
  this.hintFrom = null;
  this.hintTo = null;
  this.bigTextArea = null;

  this.emptyGraphStr = "";

  this.specialSelected = -1;

  this.viewPosX = 0;
  this.viewPosY = 0;
  this.viewWidth = 1024;

  this.dragging = false;
  this.prevDragX = 0;
  this.prevDragY = 0;

  this.moving = false;
  this.prevMoveX = 0;
  this.prevMoveY = 0;

  this.selecting = false;
  this.selectStartX = 0;
  this.selectStartY = 0;
  this.selectEndX = 0;
  this.selectEndY = 0;

  this.resizing = false;
  this.resizeElem = -1;
  this.resizePoint = [1, 1]; // 0 - min, 1 - max

  this.connecting = false;
  this.connectFromElem = -1;
  this.connectFromPin = -1;
  this.connectFromX = 0;
  this.connectFromY = 0;
  this.connectToX = 0;
  this.connectToY = 0;

  this.lastMouseOverElem = -1;
  this.lastMouseOverPin = -1;
  this.lastMouseX = 200;
  this.lastMouseY = 100;

  this.changedTimerId = null;

  this.elems = [];
  this.edges = [];
  this.selected = [];

  this.graphElem = null;
  this.additionalIncludes = [];

  try { this.typeColors = GE_typeColors; } catch (err) {};

  this.init = function()
  {
    this.hintFrom = document.getElementById("hintFrom");
    this.hintTo = document.getElementById("hintTo");
    this.backgroundDiv = document.getElementById("backgroundDiv");
    this.mainSvg = document.getElementById("mainSvg");
    this.edgesSvg = document.getElementById("edgesSvg");
    this.elemsSvg = document.getElementById("elemsSvg");
    this.selectSvg = document.getElementById("selectSvg");
    this.pinSelectSvg = document.getElementById("pinSelectSvg");
    this.resizeSvg = document.getElementById("resizeSvg");
    this.lineSvg = document.getElementById("lineSvg");
    this.specialSelectSvg = document.getElementById("specialSelectSvg");
    this.bigTextArea = document.getElementById("bigTextArea");
    this.statusElemCount = document.getElementById("statusElemCount");
    this.statusSelectedCount = document.getElementById("statusSelectedCount");
    this.statusFileName = document.getElementById("statusFileName");
    this.splashScreenDiv = document.getElementById("splashScreenDiv");
    this.resetView();
    this.hideSelectBox();

    this.lastMouseOverElem = -1;
    this.lastMouseOverPin = -1;
    this.connecting = false;
    this.selecting = false;
    this.dragging = false;
    this.resizing = false;

    this.graphId = null;

    this.graphElem = null;
    var graphDesc = nodeDescriptions.findDescriptionByName("<graph properties>");
    if (graphDesc)
    {
      this.graphElem = new Element();
      this.graphElem.create(-1, graphDesc);
      this.graphElem.update();
    }

    this.emptyGraphStr = this.stringifyGraph(S_SKIP_GRAPH_ID);

    if (renderEnabled)
      if (this.splashScreenDiv.innerText.indexOf("__SPLASH_SCREEN_TEXT") == 0)
        this.splashScreenDiv.style.visibility = "hidden";

    this.resetHistory();
  }

  this.resetView = function()
  {
    this.viewPosX = 100;
    this.viewPosY = 0;
    this.viewWidth = 1024;
    this.applyView();
  }

  this.applyView = function()
  {
    if (this.mainSvg)
    {
      this.mainSvg.setAttribute("viewBox", "" + Math.round(this.viewPosX) + " " + Math.round(this.viewPosY) + " " +
        Math.round(this.viewWidth) + " " + Math.round(this.viewWidth));

      var w = window.innerWidth ||
        document.documentElement.clientWidth ||
        document.body.clientWidth;

      var h = window.innerHeight ||
        document.documentElement.clientHeight ||
        document.body.clientHeight;

      var aspect = w / (h + 1.0);

      if (this.viewWidth > w * 3)
      {
        for (var i = 0; i < this.elems.length; i++)
        {
          var e = this.elems[i];
          if (!e)
            continue;

          for (var j = 0; j < e.lodTexts.length; j++)
            e.lodTexts[j].style.display = "none";
        }
      }
      else
      {
        var testW = this.viewWidth * aspect;
        var testH = this.viewWidth;
        var diff = testW - testH;

        for (var i = 0; i < this.elems.length; i++)
        {
          var e = this.elems[i];
          if (!e)
            continue;

          var vis = ((e.x - e.leftOffset) + e.width + 300 < this.viewPosX - diff || (e.x - e.leftOffset) - 200 > this.viewPosX + testW ||
                     e.y + e.height + 300 < this.viewPosY || e.y - 200 > this.viewPosY + testH) ? "none" : "block";

          for (var j = 0; j < e.lodTexts.length; j++)
            e.lodTexts[j].style.display = vis;
        }
      }
    }
  }

  this.hideSelectBox = function()
  {
    if (this.selectSvg)
    {
      this.selectSvg.setAttribute("width", 0);
      this.selectSvg.setAttribute("height", 0);
      this.selectSvg.setAttribute("fill", "transparent");
    }
  }

  this.updateSelectBox = function()
  {
    if (this.selecting)
    {
      var minX = Math.min(this.selectStartX, this.selectEndX);
      var maxX = Math.max(this.selectStartX, this.selectEndX);
      var minY = Math.min(this.selectStartY, this.selectEndY);
      var maxY = Math.max(this.selectStartY, this.selectEndY);
      this.selectSvg.setAttribute("x", minX);
      this.selectSvg.setAttribute("y", minY);
      this.selectSvg.setAttribute("width", maxX - minX);
      this.selectSvg.setAttribute("height", maxY - minY);
      this.selectSvg.setAttribute("fill", "#fff");
      this.selectSvg.setAttribute("fill-opacity", "0.4");
    }
    else
    {
      this.hideSelectBox();
    }
  }

  this.findEmptyOrAdd = function(array)
  {
    var idx = array.indexOf(null);
    if (idx >= 0)
      return idx;

    array.push(null);
    return array.length - 1;
  }

  this.addElem = function(x, y, desc, forceIndex)
  {
    var elem = new Element();
    var idx = forceIndex >= 0 ? forceIndex : this.findEmptyOrAdd(this.elems);
    this.elems[idx] = elem;
    elem.create(idx, desc);
    elem.x = Math.round(x * 0.05) * 20;
    elem.y = Math.round(y * 0.05) * 20;
    elem.update();
    return idx;
  }

  this.checkForExplodeSubgraph = function(elemIndex)
  {
    var e = this.elems[elemIndex];
    if (e && e.desc && e.desc.generated && e.desc.plugin === pluginName)
    {
      this._explodedSubgraphs = [];
      this._explodedDepths = [];

      this.explodeSubgraph(elemIndex, [], function(ok)
        {
          if (ok)
          {
            editor.onSelectionChanged();
            editor.hideSelectBox();
            editor.applyView();
            editor.onChanged();
          }
        });
      return true;
    }
    else
      return false;
  }

  this.explodeAllSubgraphs = function(startIndex, ondone)
  {
    if (startIndex === 0)
    {
      this._explodedSubgraphs = [];
      this._explodedDepths = [];

      for (var i = 0; i < this.additionalIncludes.length; i++)
        this._pasteInternal(this.additionalIncludes[i]);
    }

    for (var i = startIndex; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e && e.desc && e.desc.generated && e.desc.plugin === pluginName)
      {
        var start = i;
        this.explodeSubgraph(i, [], function(ok)
          {
            if (ok)
              editor.explodeAllSubgraphs(start + 1, ondone);
            else
              ondone(false);
          });
        return;
      }
    }
    ondone(true);
  }

  this.getElemUnderCursor = function(worldX, worldY)
  {
    for (var i = this.elems.length - 1; i >= 0; i--)
    {
      var e = this.elems[i];
      if (e)
        if (worldX >= (e.x - e.leftOffset) + 2 && worldX <= (e.x - e.leftOffset) + e.width - 2 &&
            worldY >= e.y + 2 && worldY <= e.y + e.height - 2)
        {
          return i;
        }
    }
    return -1;
  }

  this.getSelectedElems = function(x1, y1, x2, y2)
  {
    var minX = Math.min(x1, x2);
    var maxX = Math.max(x1, x2);
    var minY = Math.min(y1, y2);
    var maxY = Math.max(y1, y2);
    var res = [];

    if (Math.max(maxX - minX, maxY - minY) <= 10)
    {
      var i = this.getElemUnderCursor(x1, y1);
      if (i >= 0)
        res.push(i);

      return res;
    }

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
        if ((e.x - e.leftOffset) < maxX && (e.x - e.leftOffset) + e.width > minX && e.y < maxY && e.y + e.height > minY)
          res.push(i);
    }
    return res;
  }

  this.specialSelect = function(index)
  {
    var e = this.elems[index];
    this.specialSelected = index;
    if (index === -1 || !e)
    {
      if (renderEnabled)
        this.specialSelectSvg.style.visibility = "hidden";
    }
    else
      this.elems[index].update();
  }

  this.findNearestElemAndPin = function(worldX, worldY, excludeElem)
  {
    var res = [-1, 0, 0, 0]; // elemIndex, pin, x, y
    var bestLenSq = 20 * 20;

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e && excludeElem != i)
        if (worldX >= (e.x - e.leftOffset) - 20 && worldX <= (e.x - e.leftOffset) + e.width + 20 &&
            worldY >= e.y - 20 && worldY <= e.y + e.height + 20)
        {
          var leftPins = 0;
          var rightPins = 0;
          var pos = [0, 0];
          for (var j = 0; j < e.desc.pins.length; j++)
          {
            pos = e.getRelativePinPos(j);

            pos[0] += (e.x - e.leftOffset);
            pos[1] += e.y;

            var dx = pos[0] - worldX;
            var dy = pos[1] - worldY;
            var lenSq = dx * dx + dy * dy;
            if (lenSq < bestLenSq && !e.desc.pins[j].hidden)
            {
              bestLenSq = lenSq;
              res = [i, j, pos[0], pos[1]];
            }
          }
        }
    }

    return res;
  }

  this.hideSameConnection = function(elemFrom, pinFrom, elemTo, pinTo)
  {
    var res = false;
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i] && !this.edges[i].hidden)
      {
        if (this.edges[i].elemFrom == elemFrom &&
            this.edges[i].elemTo == elemTo &&
            this.edges[i].pinFrom == pinFrom &&
            this.edges[i].pinTo == pinTo)
        {
          this.edges[i].hidden = true;
          res = true;
        }
        else
        if (this.edges[i].elemFrom == elemTo &&
            this.edges[i].elemTo == elemFrom &&
            this.edges[i].pinFrom == pinTo &&
            this.edges[i].pinTo == pinFrom)
        {
          this.edges[i].hidden = true;
          res = true;
        }
      }

    return res;
  }

  this.findConnection = function(elem, pin)
  {
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i] && !this.edges[i].hidden)
      {
        if ((this.edges[i].elemFrom == elem && this.edges[i].pinFrom == pin) ||
            (this.edges[i].elemTo == elem && this.edges[i].pinTo == pin))
          return i;
      }

    return -1;
  }

  this.findConnections = function(elem, pin)
  {
    var res = [];
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i] && !this.edges[i].hidden)
      {
        if ((this.edges[i].elemFrom == elem && this.edges[i].pinFrom == pin) ||
            (this.edges[i].elemTo == elem && this.edges[i].pinTo == pin))
          res.push(i);
      }

    return res;
  }

  this.getOppositeConnection = function(edgeIndex, elemIndex)
  {
    var e = this.edges[edgeIndex];
    if (!e)
      return null;

    if (e.elemFrom === elemIndex)
      return {elem:e.elemTo, pin:e.pinTo};
    else if (e.elemTo === elemIndex)
      return {elem:e.elemFrom, pin:e.pinFrom};
    else
      return null;
  }

  this.getConnectionPin = function(edgeIndex, elemIndex)
  {
    var e = this.edges[edgeIndex];
    if (!e)
      return null;

    if (e.elemFrom === elemIndex)
      return e.pinFrom;
    else if (e.elemTo === elemIndex)
      return e.pinTo;
    else
      return null;
  }

  this.hideSingleConnections = function(elemFrom, pinFrom, elemTo, pinTo)
  {
    var res = false;

    if (this.elems[elemFrom] && this.elems[elemFrom].desc.pins.length <= pinFrom)
      return;

    if (this.elems[elemFrom] && this.elems[elemFrom].desc.pins[pinFrom].singleConnect)
    {
      var idx = this.findConnection(elemFrom, pinFrom);
      if (idx >= 0)
      {
        this.edges[idx].hidden = true;
        res = true;
      }
    }

    if (this.elems[elemTo] && this.elems[elemTo].desc.pins.length <= pinTo)
      return;

    if (this.elems[elemTo] && this.elems[elemTo].desc.pins[pinTo].singleConnect)
    {
      var idx = this.findConnection(elemTo, pinTo);
      if (idx >= 0)
      {
        this.edges[idx].hidden = true;
        res = true;
      }
    }

    return res;
  }

  this.killHiddenConnections = function()
  {
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i] && this.edges[i].hidden)
        this.edges[i].kill();
  }

  this.unhideConnections = function()
  {
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i] && this.edges[i].hidden)
        this.edges[i].hidden = false;
  }

  this.updateConnections = function()
  {
    for (var i = 0; i < this.edges.length; i++)
      if (this.edges[i])
        this.edges[i].update();
  }


  this.testForLoops = function()
  {
    var outCnt = new Uint8Array(this.elems.length);
    var inCnt = new Uint8Array(this.elems.length);
    var anyCnt = new Uint8Array(this.elems.length);
    var exEdges = new Uint8Array(this.edges.length);
    var terminals = new Uint8Array(this.elems.length);

    // list of inputs:
    {
      for (var i = 0; i < this.edges.length; i++)
      {
        var c = this.edges[i];
        if (c && !c.hidden)
        {
          var connectElemFrom = this.elems[c.elemFrom];
          var connectElemTo = this.elems[c.elemTo];
          var pinRoleFrom = connectElemFrom.desc.pins[c.pinFrom].role;
          var pinRoleTo = connectElemTo.desc.pins[c.pinTo].role;
          if (pinRoleFrom === "in")
            inCnt[c.elemFrom]++;
          if (pinRoleFrom === "out")
            outCnt[c.elemFrom]++;
          if (pinRoleFrom === "any")
            anyCnt[c.elemFrom]++;

          if (pinRoleTo === "in")
            inCnt[c.elemTo]++;
          if (pinRoleTo === "out")
            outCnt[c.elemTo]++;
          if (pinRoleTo === "any")
            anyCnt[c.elemTo]++;
        }
        else
        {
          exEdges[i] = 1;
        }
      }
    }


    var order = 1;
    var changed = false;
    do
    {
      changed = false;

      // mark terminals
      for (var i = 0; i < this.elems.length; i++)
        if (terminals[i] != order)
        {
          var sum = anyCnt[i] + (inCnt[i] ? 1 : 0) + (outCnt[i] ? 1 : 0);
          if (sum == 1)
            terminals[i] = order;
        }

      // exclude connections to terminals
      for (var i = 0; i < this.edges.length; i++)
      {
        var c = this.edges[i];
        if (c && !c.hidden && exEdges[i] == 0)
        {
          if (terminals[c.elemFrom] == order || terminals[c.elemTo] == order)
          {
            changed = true;
            exEdges[i] = 1;
            var connectElemFrom = this.elems[c.elemFrom];
            var pinRoleFrom = connectElemFrom.desc.pins[c.pinFrom].role;
            if (pinRoleFrom === "in")
              inCnt[c.elemFrom]--;
            if (pinRoleFrom === "out")
              outCnt[c.elemFrom]--;
            if (pinRoleFrom === "any")
              anyCnt[c.elemFrom]--;

            var connectElemTo = this.elems[c.elemTo];
            var pinRoleTo = connectElemTo.desc.pins[c.pinTo].role;
            if (pinRoleTo === "in")
              inCnt[c.elemTo]--;
            if (pinRoleTo === "out")
              outCnt[c.elemTo]--;
            if (pinRoleTo === "any")
              anyCnt[c.elemTo]--;
          }
        }
      }

      order++;
    }
    while (changed && order < 255);


    for (var i = 0; i < this.elems.length; i++)
      if (this.elems[i] && terminals[i] == 0 && !this.elems[i].desc.allowLoop)
      {
        var sum = anyCnt[i] + (inCnt[i] ? 1 : 0) + (outCnt[i] ? 1 : 0);
        if (sum > 1)
          return true;
      }

    return false;
  }


  this.checkTypeCorrect = function()
  {
    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
        e.initPinTypes();
    }

    var changed = false;
    var iteration = 0;
    do
    {
      changed = false;

      for (var i = 0; i < this.edges.length; i++)
      {
        var c = this.edges[i];
        if (c && !c.hidden)
        {
          var from = this.elems[c.elemFrom];
          var to = this.elems[c.elemTo];

          from.connected[c.pinFrom] = true;
          to.connected[c.pinTo] = true;

          var pinTypesFrom = from.pinTypes[c.pinFrom];
          var pinTypesTo = to.pinTypes[c.pinTo];

          if (from.desc.pins[c.pinFrom].role === "out" || to.desc.pins[c.pinTo].role === "in")
          {
            changed |= typeConv.removeInconvertableTypes(pinTypesFrom, pinTypesTo);
            if (pinTypesTo.length == 0)
              return false;
          }

          if (from.desc.pins[c.pinFrom].role === "in" || to.desc.pins[c.pinTo].role === "out")
          {
            changed |= typeConv.removeInconvertableTypes(pinTypesTo, pinTypesFrom);
            if (pinTypesFrom.length == 0)
              return false;
          }
        }
      }

      for (var i = 0; i < this.elems.length; i++)
      {
        var elem = this.elems[i];
        if (elem)
        {
          var pdesc = elem.desc.pins;

          for (var j = 0; j < pdesc.length; j++)
            if (pdesc[j].typeGroup)
            {
              var allInTypes = [];
              var found = false;

              for (var k = 0; k < pdesc.length; k++)
                if (k != j && pdesc[k].typeGroup === pdesc[j].typeGroup && pdesc[k].role !== "out")
                {
                  if (elem.connected[k])
                  {
                    allInTypes = allInTypes.concat(elem.pinTypes[k]);
                    found = true;
                  }
                }

              if (!found)
                allInTypes = elem.pinTypes[j].slice(0);

              for (var k = 0; k < pdesc.length; k++)
                if (k != j && pdesc[k].typeGroup === pdesc[j].typeGroup)
                {
                  changed |= typeConv.removeNonEqualTypes(allInTypes, elem.pinTypes[j]);

                  if (elem.pinTypes[j].length == 0)
                    return false;
                }
            }

        }
      }

      iteration++;
      if (iteration > 400)
      {
        return false;
      }
    }
    while (changed);

    return true;
  }


  this.propagateExternalStuff = function()
  {
    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
      {
        e.propagatedStuff = null;
        e.externalStuffOnPins = [];
        e.connected = [];

        e.setOuterText(0, "");
        e.setOuterText(1, "");
      }
    }

    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
      {
        var from = this.elems[c.elemFrom];
        var to = this.elems[c.elemTo];

        from.connected[c.pinFrom] = true;
        to.connected[c.pinTo] = true;
      }
    }

    var changed = false;
    var iteration = 0;
    do
    {
      changed = false;

      for (var i = 0; i < this.elems.length; i++)
      {
        var e = this.elems[i];
        if (!e)
          continue;


        if (!e.propagatedStuff && e.desc.GE_propagateExternalStuff)
          changed |= e.desc.GE_propagateExternalStuff(i);
      }

      for (var i = 0; i < this.edges.length; i++)
      {
        var c = this.edges[i];
        if (c && !c.hidden)
        {
          var from = this.elems[c.elemFrom];
          var to = this.elems[c.elemTo];

          if (from.desc.pins[c.pinFrom].role === "out")
            if (from.propagatedStuff && !to.externalStuffOnPins[c.pinTo])
            {
              to.externalStuffOnPins[c.pinTo] = from.propagatedStuff;
              changed = true;
            }

          if (to.desc.pins[c.pinTo].role === "out")
            if (to.propagatedStuff && !from.externalStuffOnPins[c.pinFrom])
            {
              from.externalStuffOnPins[c.pinFrom] = to.propagatedStuff;
              changed = true;
            }
        }
      }


      iteration++;
      if (iteration > 200)
        return false;
    }
    while (changed);
  }


  this.updateAllPinColors = function()
  {
    if (globalLockGraphics || !renderEnabled)
      return;

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
        e.updatePinColors();
    }

    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
      {
        var type = null;
        var from = this.elems[c.elemFrom];
        var to = this.elems[c.elemTo];
        if (from.desc.pins[c.pinFrom].role === "out")
          type = from.getPinVisualType(c.pinFrom);
        if (to.desc.pins[c.pinTo].role === "out")
          type = to.getPinVisualType(c.pinTo);

        if (c.svgPath)
          c.svgPath.style.stroke = type ? editor.typeColors[type] : "#fff";
      }
    }
  }

  this.propagateTypes = function()
  {

    this.propagateExternalStuff();

 //   return this.checkTypeCorrect();

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
      {
        e.initPinTypes();
        if(clientId != "offline") //There is no render when its in offline mode
          e.hideOptionalPins();
        e.typeInferenced = new Array(e.desc.pins.length);
        for (var k = 0; k < e.typeInferenced.length; k++)
          e.typeInferenced[k] = false;
      }
    }

    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
        c.typeInferenced = false;
    }


    var changed = false;
    var iteration = 0;
    do
    {
      changed = false;

      for (var i = 0; i < this.edges.length; i++)
      {
        var c = this.edges[i];
        if (c && !c.hidden && !c.typeInferenced)
        {
          var from = this.elems[c.elemFrom];
          var to = this.elems[c.elemTo];

          from.connected[c.pinFrom] = true;
          to.connected[c.pinTo] = true;

          var pinTypesFrom = from.pinTypes[c.pinFrom];
          var pinTypesTo = to.pinTypes[c.pinTo];

          if (from.desc.pins[c.pinFrom].role === "out" || to.desc.pins[c.pinTo].role === "in")
          {
            if (from.typeInferenced[c.pinFrom])
            {
              c.typeInferenced = true;
              to.typeInferenced[c.pinTo] = true;
              to.pinTypes[c.pinTo] = [typeConv.keepTypeOrConvert(from.pinTypes[c.pinFrom][0], to.pinTypes[c.pinTo])];
              changed = true;
            }
          }

          if (from.desc.pins[c.pinFrom].role === "in" || to.desc.pins[c.pinTo].role === "out")
          {
            if (to.typeInferenced[c.pinTo])
            {
              c.typeInferenced = true;
              from.typeInferenced[c.pinFrom] = true;
              from.pinTypes[c.pinFrom] = [typeConv.keepTypeOrConvert(to.pinTypes[c.pinTo][0], from.pinTypes[c.pinFrom])];
              changed = true;
            }
          }
        }
      }

      for (var i = 0; i < this.elems.length; i++)
      {
        var elem = this.elems[i];
        if (elem)
        {
          var pdesc = elem.desc.pins;
          var groups = [];

          for (var j = 0; j < pdesc.length; j++)
            if (!elem.typeInferenced[j])
            {
              if (!pdesc[j].typeGroup && (pdesc[j].role === "out" || elem.pinTypes[j].length == 1))
              {
                elem.typeInferenced[j] = true;
                elem.pinTypes[j].length = 1;
                changed = true;
              }

              if (pdesc[j].typeGroup)
                groups.push(pdesc[j].typeGroup);
            }

          for (var g = 0; g < groups.length; g++)
          {
            var group = groups[g];
            var allInInferenced = true;
            var typeList = [];
            for (var j = 0; j < pdesc.length; j++)
              if (pdesc[j].typeGroup == group)
              {
                if (pdesc[j].role !== "out")
                {
                  if (elem.typeInferenced[j])
                    typeList.push(elem.pinTypes[j][0]);
                  else
                    allInInferenced = false;
                }
              }


            if (allInInferenced)
            {
              var outType = typeConv.getLargestType(typeList);
              if (!outType)
                outType = typeList[0];

              for (var j = 0; j < pdesc.length; j++)
                if (pdesc[j].typeGroup == group)
                {
                  if (pdesc[j].role !== "in")
                  {
                    elem.typeInferenced[j] = true;
                    elem.pinTypes[j] = [outType];
                    changed = true;
                  }
                }

              elem.largestGroupTypes[group] = outType;
            }
            else if (typeList.length > 0)
            {
              var largestType = typeConv.getLargestType(typeList);
              elem.largestGroupTypes[group] = largestType ? largestType : typeList[0];
            }

          }
        }
      }

      iteration++;
      if (iteration > 200)
      {
        return false;
      }
    }
    while (changed);

    return true;
  }


  this.isValidConnection = function(elemFrom, pinFrom, elemTo, pinTo)
  {
    var from = editor.elems[elemFrom];
    var to = editor.elems[elemTo];

    var pin1 = from.desc.pins[pinFrom];
    var pin2 = to.desc.pins[pinTo];

    var inOutOk = (pin1.role == "any" || pin2.role == "any" || pin1.role != pin2.role);
    if (!inOutOk)
      return false;

    var res = true;

    // create temporary connection
    var tmp = this.addConnection(elemFrom, pinFrom, elemTo, pinTo, -1, null, null);

    if (tmp >= 0)
    {
      if (this.testForLoops())
        res = false;

      if (this.checkTypeCorrect() == false)
        res = false;

      this.edges[tmp].kill();
    }

    this.unhideConnections();

    this.checkTypeCorrect(); // just calculate pin types;

    return res;
  }



  this.addConnection = function(elemFrom, pinFrom, elemTo, pinTo, forceIndex, uidFrom, uidTo)
  {
    if (this.hideSameConnection(elemFrom, pinFrom, elemTo, pinTo))
      return -1;

    this.hideSingleConnections(elemFrom, pinFrom, elemTo, pinTo);

    var conn = new Connection();
    var idx = forceIndex >= 0 ? forceIndex : this.findEmptyOrAdd(this.edges);
    this.edges[idx] = conn;
    conn.create(idx, elemFrom, pinFrom, elemTo, pinTo, uidFrom, uidTo);

    return idx;
  }

  this.addConnectionOnLoad = function(elemFrom, pinFrom, elemTo, pinTo, forceIndex, uidFrom, uidTo)
  {
    var conn = new Connection();
    var idx = forceIndex >= 0 ? forceIndex : this.findEmptyOrAdd(this.edges);
    this.edges[idx] = conn;
    conn.create(idx, elemFrom, pinFrom, elemTo, pinTo, uidFrom, uidTo);

    return idx;
  }

  this.screenToWorld = function(pageX, pageY)
  {
    var x = pageX + (this.mainSvg ? this.mainSvg.scrollLeft : 0);
    var y = pageY + (this.mainSvg ? this.mainSvg.scrollTop : 0);

    var wx = window.innerWidth;
    var wy = window.innerHeight;

    if (wy < wx)
      x -= (wx - wy) / 2;
    else
      y -= (wy - wx) / 2;

    var minSize = Math.min(wx, wy);
    var scale = 1.0 * this.viewWidth / minSize;

    x *= scale;
    y *= scale;
    x += this.viewPosX;
    y += this.viewPosY;
    return [x, y];
  }

  this.displayPropValues = function()
  {
    if (editor.selected.length != 1 || !this.elems[editor.selected[0]])
      return;

    var propValuesDiv = document.getElementById("propValuesDiv");
    var elem = this.elems[editor.selected[0]];
    var props = elem.desc.properties;

    if (props.length === 0)
    {
      propValuesDiv.innerHTML = "No properties";
      return;
    }

    if (this._displayPropValues(elem, props) === 0)
      propValuesDiv.innerHTML = "No properties";
  }

  this.displayGraphProperties = function()
  {
    var propValuesDiv = document.getElementById("propValuesDiv");
    if (!editor.graphElem || editor.graphElem.desc.properties.length === 0)
    {
      propValuesDiv.innerHTML = "No properties";
      return;
    }

    var elem = this.graphElem;
    var props = elem.desc.properties;
    if (this._displayPropValues(elem, props) === 0)
      propValuesDiv.innerHTML = "No properties";
  }


  this._displayPropValues = function(elem, props)
  {
    var count = 0;
    var propValuesDiv = document.getElementById("propValuesDiv");

    function quotes(text)
    {
      return "\"" + text + "\"";
    }

    function div(panel, variable, text)
    {
      return "<div id='" + makeComponentId("div", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "'>" + text + "</div>\n"
    }

    function edit(panel, variable, min_v, max_v, def_v, step)
    {
      return "<input class='editbox' autocomplete='off' lang='en-US' id='" + makeComponentId("edit", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "' type='number' min='" + min_v +
        "' max='" + max_v + "' value='" + def_v + "' step='" + step + "'" +
        " onchange='onEditChange(this.value, " + quotes(panel) + ", " + quotes(variable) + " );'/>";
    }

    function editIdx(panel, variable, min_v, max_v, def_v, step, idx)
    {
      return "<input class='editbox' autocomplete='off' lang='en-US' id='" + makeComponentIdWithIdx("edit", panel, variable, idx) + "'" +
        " name='" + makeComponentNameWithIdx(panel, variable, idx) + "' type='number' min='" + min_v +
        "' max='" + max_v + "' value='" + def_v + "' step='" + step + "'" +
        " onchange='onEditChangeWithIdx(this.value, " + quotes(panel) + ", " + quotes(variable) + "," + idx +");'/>";
    }
    function editStr(panel, variable, def_v)
    {
      return "<input class='editbox' autocomplete='off' lang='en-US' id='" + makeComponentId("edit", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "' value='" + def_v + "' " +
        " onchange='onEditChange(this.value, " + quotes(panel) + ", " + quotes(variable) + " );'/>";
    }

    function slider(panel, variable, min_v, max_v, def_v, step)
    {
      return "<input class='slider' id='" + makeComponentId("slider", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "' type='range' min='" + min_v +
        "' max='" + max_v + "' value='" + def_v + "' step='" + step + "'" +
        " onchange='onSliderChange(this.value, " + quotes(panel) + ", " + quotes(variable) + " );'/>";
    }

    function checkbox(panel, variable, def_v)
    {
      return "<label><input id='" + makeComponentId("checkbox", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "' type='checkbox' " +
        (def_v == "0" ? "" : "checked") +
        " onchange='onCheckboxChange(this.checked, " + quotes(panel) + ", " + quotes(variable) + " );'/>"+variable+"</label>";
    }

    function button(panel, variable, def_v)
    {
      return "<button id='" + makeComponentId("button", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "' " +
        " onclick='onButtonChange(" + def_v + ", " + quotes(panel) + ", " + quotes(variable) + " );'>"+variable+"</button>";
    }

    function select(panel, variable, def_v, values)
    {
      var res = "<select id='" + makeComponentId("select", panel, variable) + "'" +
        " name='" + makeComponentName(panel, variable) + "'" +
        " style='max-width:95%;'" +
        " onchange='onSelectChange(this[this.selectedIndex].innerHTML, " + quotes(panel) + ", " + quotes(variable) + " );'>";

      for (var i = 0; i < values.length; i++)
      {
        var v = values[i].trim();
        res += "<option " + (v == def_v ? "selected" : "") + " value='" + v + "'>" + v + "</option>";
      }
      res += "</select>";
      return res;
    }

    function colorPicker(panel, variable, def_v)
    {
      var col = def_v.split(",");
      for (var i = 0; i < 3; i++)
        col[i] = "" + Math.round((parseFloat(col[i]) * 255.0));

      return "<button id='" + makeComponentId("color", panel, variable) + "'" +
        " style='background:rgb(" + col[0] + "," + col[1] + "," + col[2] + ");'  " +
        " data-color='" + def_v + "'"+
        " name='" + makeComponentName(panel, variable) + "' " +
        " onclick='show_color_picker(this, \"" + makeComponentName(panel, variable) + "\",onColorChanged);'>" +
        "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</button>";
    }

    function curveEditor(panel, variable, def_v, style, bck_curves, interpolation, add_remove)
    {
      return make_curve_editor_html(makeComponentName(panel, variable), def_v, onCurveChanged, style, bck_curves, interpolation, add_remove);
    }


    function gradientEditor(id_, def_v)
    {
      var id = id_;
      var value = def_v;

      setTimeout(function() {
          var canv = document.getElementById("grad_canv_" + id);
          var grad = new ColorGradient(false);
          grad.initFromGradientStr(value);
          grad.renderGradient(canv);
        }, 1);

      var command = "show_gradient(\"" + value + "\", \"" + id + "\", onGradientChanged);";

      return "<canvas id='grad_canv_" + id + "' "+
             " style='width:182px; height:32px;' "+
             " onclick='" + command + "'"+
             ">Canvas not supported</canvas>";
    }


    function floatStep(step, min_v, max_v)
    {
      return step;
      //var d = max_v - min_v;
      //return Math.max(step, d / 1000);
    }


    var panelName = "" + elem.myIndex;
    var sum = "";

    for (var i = 0; i < props.length; i++)
      if (props[i])
      {
        var prop = props[i];

        if (prop.hidden)
          continue;

        count++;
        var html = "";

        if (prop.type == "string")
        {
          var v = prop.name;
          var def_v = elem.propValues[i];

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + editStr(panelName, v, def_v) + "<br>"
            //+slider(panelName, v, min_v, max_v, def_v, 1)
            );
        }
        else if (prop.type == "int")
        {
          var v = prop.name;
          var min_v = prop.minVal;
          var max_v = prop.maxVal;
          var def_v = elem.propValues[i];

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + edit(panelName, v, min_v, max_v, def_v, 1) + "<br>"
            //+slider(panelName, v, min_v, max_v, def_v, 1)
            );
        }
        else if (prop.type == "float")
        {
          var v = prop.name;
          var min_v = prop.minVal;
          var max_v = prop.maxVal;
          var def_v = elem.propValues[i];
          var step = floatStep(prop.step, min_v, max_v);

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + edit(panelName, v, min_v, max_v, def_v, step) + "<br>"
            //+slider(panelName, v, min_v, max_v, def_v, step)
            );
        }
        else if (prop.type == "float2" || prop.type == "float3" || prop.type == "float4")
        {
          var v = prop.name;
          var min_v = prop.minVal;
          var max_v = prop.maxVal;
          var def_v = elem.propValues[i];
          var step = floatStep(prop.step, min_v, max_v);
          var def_values = def_v.split(",");

          var editString = "";
          for (var j = 0; j < parseInt(prop.type.charAt(5)); j++)
            editString += idxToChar[j] + ": " + editIdx(panelName, v, min_v, max_v, def_values[j].trim(), step, j)  + "<br>";

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + editString);
        }
        else if (prop.type == "bool")
        {
          var v = prop.name;
          var def_v = elem.propValues[i];
          var values = ["false", "true"];

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + select(panelName, v, def_v, values));
        }
        else if (prop.type == "combobox" || prop.type == "multitype_combobox")
        {
          var v = prop.name;
          var def_v = elem.propValues[i];
          var values = prop.items;

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + select(panelName, v, def_v, values));
        }
        else if (prop.type == "color")
        {
          var v = prop.name;
          var def_v = elem.propValues[i];

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + colorPicker(panelName, v, def_v));
        }
        else if (prop.type == "polynom_curve" || prop.type == "steps_curve" || prop.type == "linear_curve" || prop.type == "monotonic_curve" ||
                 prop.type == "polynom2" || prop.type == "polynom3" || prop.type == "polynom4" || prop.type == "polynom5" ||
                 prop.type == "polynom6" || prop.type == "polynom7" || prop.type == "polynom8" ||
                 prop.type == "linear2" || prop.type == "linear3" || prop.type == "linear4" || prop.type == "linear5" ||
                 prop.type == "linear6" || prop.type == "linear7" || prop.type == "linear8" ||
                 prop.type == "steps2" || prop.type == "steps3" || prop.type == "steps4" || prop.type == "steps5" ||
                 prop.type == "steps6" || prop.type == "steps7" || prop.type == "steps8" ||
                 prop.type == "monotonic2" || prop.type == "monotonic3" || prop.type == "monotonic4" || prop.type == "monotonic5" ||
                 prop.type == "monotonic6" || prop.type == "monotonic7" || prop.type == "monotonic8"
                 )
        {
          var v = prop.name;
          var def_v = elem.propValues[i];
          var style = prop.style ? prop.style : "gray";

          var bck = [];
          if (prop.background)
            for (var k = 0; k < prop.background.length; k++)
              bck.push("curve_editor_canv_" + makeComponentName(panelName, prop.background[k]));

          var interpolation = "steps";
          if (prop.type.indexOf("linear") === 0)
            interpolation = "piecewise_linear";
          if (prop.type.indexOf("monotonic") === 0)
            interpolation = "piecewise_monotonic";
          if (prop.type.indexOf("polynom") === 0)
            interpolation = "polynom";

          var add_remove = (prop.type.indexOf("_curve") >= 0);

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + curveEditor(panelName, v, def_v, style, bck, interpolation, add_remove));
        }
        else if (prop.type == "gradient_preview")
        {
          var v = prop.name;
          var bck = [];
          if (prop.background)
            for (var k = 0; k < prop.background.length; k++)
              bck.push("curve_editor_canv_" + makeComponentName(panelName, prop.background[k]));

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + make_curve_gradient_html(makeComponentName(panelName, "p"), bck));
        }
        else if (prop.type == "gradient_editor")
        {
          var v = prop.name;
          var def_v = elem.propValues[i];

          html += div(panelName, v, safeHtmlStr(v) + "<br>" + gradientEditor(makeComponentName(panelName, v), def_v));
        }

        sum += html;
      }

    propValuesDiv.innerHTML = sum;
    return count;
  }


  this.selectInsideBlocks = function()
  {
    for (var changed = true; changed;)
    {
      changed = false;
      for (var s = 0; s < this.selected.length; s++)
      {
        var e = this.elems[editor.selected[s]];
        if (e && e.desc.name === "block")
        {
          for (var i = 0; i < this.elems.length; i++)
          {
            var em = this.elems[i];
            if (em)
            {
              var cx = em.x - em.leftOffset + em.width * 0.5;
              var cy = em.y + em.height * 0.5;
              if (cx > e.x && cx < e.x + e.blockWidth && cy > e.y && cy < e.y + e.blockHeight && i != editor.selected[s])
              {
                if (editor.selected.indexOf(i) < 0)
                {
                  editor.selected.push(i);
                  changed = true;
                }
              }
            }
          }
        }
      }
    }
  }


  this.onSelectionChanged = function()
  {
    this.replaceWithSubgraph = undefined;

    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
        c.update();
    }

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e && e.svgRect)
        e.svgRect.setAttributeNS(null, "class", "elemRectClass");
    }

    for (var i = 0; i < this.selected.length; i++)
    {
      var e = this.elems[editor.selected[i]];
      if (e && e.svgRect)
        e.svgRect.setAttributeNS(null, "class", "selectedElemRectClass");
    }

    var categories = document.getElementById("categoriesDiv");
    var elements = document.getElementById("elementsDiv");
    var propValues = document.getElementById("propValuesDiv");

    if (!globalLockChange && renderEnabled)
    {
      if (this.selected.length == 1)
        this.displayPropValues();
      else
        this.displayGraphProperties();
    }

    if (this.statusSelectedCount)
      this.statusSelectedCount.innerHTML = this.selected.length ? "&nbsp;&nbsp;Selected: " + this.selected.length : "";
  }

  this.selectAll = function()
  {
    this.selected = [];
    for (var i = 0; i < this.elems.length; i++)
      if (this.elems[i])
        this.selected.push(i);

    this.onSelectionChanged();
  }

  this.onMouseDblClick = function(event)
  {
    var worldPos = editor.screenToWorld(event.pageX, event.pageY);
    var x = worldPos[0];
    var y = worldPos[1];

    if (event.button == 0) // left
    {
      var sel = -1;
      if (editor.selected.length === 1 && editor.elems[editor.selected[0]])
        sel = editor.selected[0];

      if (editor.selected.length >= 1)
      {
        var elem = editor.getElemUnderCursor(x, y);
        if (editor.elems[elem])
          if (editor.elems[elem].desc.name === "block")
          {
            sel = elem;
            if (editor.selected.indexOf(sel) < 0)
              editor.selected.push(sel);
          }
      }

      var e = editor.elems[sel]
      if (e && (e.desc.name === "comment" || e.desc.name === "block"))
      {
        editor.selectInsideBlocks();
        sel = -1
      }

      editor.specialSelect(sel);
      editor.onChanged();
    }
  }

  this.xorSelection = function(newSelection)
  {
    for (var i = 0; i < newSelection.length; i++)
    {
      var idx = this.selected.indexOf(newSelection[i]);
      if (idx < 0)
        this.selected.push(newSelection[i]);
      else
        this.selected.splice(idx, 1);
    }
  }


  this.getResizeCornerUnderCursor = function(x, y)
  {
    var best = null;
    var bestDist = 40;
    for (var i = 0; i < this.elems.length; i++)
      if (this.elems[i] && this.elems[i].desc.name === "block")
      {
        var e = this.elems[i];
        for (var ix = 0; ix <= 1; ix++)
          for (var iy = 0; iy <= 1; iy++)
          {
            var px = e.x + ix * e.blockWidth;
            var py = e.y + iy * e.blockHeight;
            var dist = Math.sqrt((px - x) * (px - x) + (py - y) * (py - y));
            if (dist < bestDist)
            {
              bestDist = dist;
              best = { elem:i, ix:ix, iy:iy };
            }
          }
      }

    return best
  }


  this.onMouseDown = function(event)
  {
    if (!renderEnabled)
      return;

    var worldPos = editor.screenToWorld(event.pageX, event.pageY);
    var x = worldPos[0];
    var y = worldPos[1];

    if (event.button == 0) // left
    {
      editor.resizing = false;
      nodeDescriptions.hideFilterDialog();
      openGraphDialog.hideDialog();
      editor.bigTextArea.style.visibility = "hidden";

      var resizeCorner = editor.getResizeCornerUnderCursor(x, y);

      var underCursor = editor.getElemUnderCursor(x, y);
      var f = editor.findNearestElemAndPin(x, y, -1);

      if (!editor.connecting)
      {
        if (event.ctrlKey)
        {
          editor.selecting = true;
          editor.selectStartX = x;
          editor.selectStartY = y;
          editor.selectEndX = x;
          editor.selectEndY = y;
        }
        else if (resizeCorner)
        {
          editor.resizing = true;
          editor.resizeElem = resizeCorner.elem;
          editor.resizeCorner = [resizeCorner.ix, resizeCorner.iy];
          editor.prevMoveX = x;
          editor.prevMoveY = y;
          var e = editor.elems[editor.resizeElem];
          e.moveX = 0.0;
          e.moveY = 0.0;
        }
        else
        {
          if (f[0] < 0 && underCursor >= 0 && editor.selected.indexOf(underCursor) < 0)
          {
            editor.selected = [underCursor];
            editor.onSelectionChanged();
            editor.onChanged(false, false, true);
          }

          if (editor.selected.indexOf(underCursor) >= 0)
          {
            editor.moving = true;
            editor.prevMoveX = x;
            editor.prevMoveY = y;

            for (var i = 0; i < editor.selected.length; i++)
            {
              var e = editor.elems[editor.selected[i]];
              e.moveX = e.x;
              e.moveY = e.y;
            }
          }
          else
          {
            if (f[0] >= 0)
            {
              editor.connecting = true;
              editor.connectFromElem = f[0];
              editor.connectFromPin = f[1];
              editor.connectFromX = f[2];
              editor.connectFromY = f[3];
              editor.connectToX = f[2];
              editor.connectToY = f[3];
            }
            else
            {
              editor.selecting = true;
              editor.selectStartX = x;
              editor.selectStartY = y;
              editor.selectEndX = x;
              editor.selectEndY = y;
            }
          }
        }
      }
    }

    if (event.button == 1) // middle
    {
      editor.dragging = true;
      editor.prevDragX = x;
      editor.prevDragY = y;
    }
  }

  this.removeEdgesUnderCursor = function()
  {
    if (this.lastMouseOverElem < 0)
      return;

    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
        if ((c.elemFrom == this.lastMouseOverElem && c.pinFrom == this.lastMouseOverPin) ||
            (c.elemTo == this.lastMouseOverElem && c.pinTo == this.lastMouseOverPin))
        {
          c.kill();
        }
    }
  }

  this.jumpToOppositePin = function()
  {
    var f = this.findNearestElemAndPin(this.lastMouseX, this.lastMouseY, -1);
    var overElem = f[0];
    var overPin = f[1];

    if (overElem < 0)
      return;

    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
        if ((c.elemFrom == overElem && c.pinFrom == overPin) ||
            (c.elemTo == overElem && c.pinTo == overPin))
        {
          var fromElem = editor.elems[overElem];
          var fromPos = fromElem.getRelativePinPos(overPin);
          fromPos[0] += fromElem.x - fromElem.leftOffset;
          fromPos[1] += fromElem.y;

          var op = editor.getOppositeConnection(i, overElem);
          var toElem = editor.elems[op.elem];
          var toPos = toElem.getRelativePinPos(op.pin);
          toPos[0] += toElem.x - toElem.leftOffset;
          toPos[1] += toElem.y;

          editor.viewPosX += toPos[0] - fromPos[0];
          editor.viewPosY += toPos[1] - fromPos[1];
          editor.applyView();
          this.onMouseMoveInternal(null, toPos);

          return;
        }
    }
  }

  this.modifyEdgeUnderCursor = function()
  {
    if (this.lastMouseOverElem < 0 || this.lastMouseOverPin < 0 || this.connecting || this.moving)
      return;

    for (var i = editor.edges.length - 1; i >= 0; i--)
    {
      var c = this.edges[i];
      if (c && !c.hidden)
        if ((c.elemFrom == this.lastMouseOverElem && c.pinFrom == this.lastMouseOverPin) ||
            (c.elemTo == this.lastMouseOverElem && c.pinTo == this.lastMouseOverPin))
        {
          var op = editor.getOppositeConnection(i, this.lastMouseOverElem);
          var e = editor.elems[op.elem];
          var pos = e.getRelativePinPos(op.pin);

          editor.connectFromElem = op.elem;
          editor.connectFromPin = op.pin;

          pos[0] += e.x - e.leftOffset;
          pos[1] += e.y;

          editor.connectFromX = pos[0];
          editor.connectFromY = pos[1];
          editor.connectToX = this.lastMouseX;
          editor.connectToY = this.lastMouseY;
          editor.connecting = true;

          editor.lineSvg.setAttributeNS(null, "x1", editor.connectFromX);
          editor.lineSvg.setAttributeNS(null, "y1", editor.connectFromY);
          editor.lineSvg.setAttributeNS(null, "x2", editor.connectToX);
          editor.lineSvg.setAttributeNS(null, "y2", editor.connectToY);
          editor.lineSvg.setAttributeNS(null, "stroke", "#fff");

          c.kill();

          return;
        }
    }
  }

  this.commentPinUnderCursor = function()
  {
    if (this.lastMouseOverElem < 0 || this.lastMouseOverPin < 0)
      return;

    var e = this.elems[this.lastMouseOverElem];
    if (e)
    {
      var commentText = prompt("Enter pin comment:", e.getPinComment(this.lastMouseOverPin));
      lastRespondTime = Date.now();
      if (commentText !== null)
      {
        e.setPinComment(this.lastMouseOverPin, commentText.trim());
        e.updatePinCaptions();
      }
    }
  }


  this.onMouseMove = function(event)
  {
    if (!renderEnabled)
      return;

    var worldPos = editor.screenToWorld(event.pageX, event.pageY);
    this.onMouseMoveInternal(event, worldPos);
  }


  this.onMouseMoveInternal = function(event, worldPos)
  {
    if (!renderEnabled)
      return;

//    var worldPos = editor.screenToWorld(event.pageX, event.pageY);
    editor.hideResize();

    var x = worldPos[0];
    var y = worldPos[1];

    this.lastMouseX = x;
    this.lastMouseY = y;

    if (editor.dragging && event)
    {
      editor.viewPosX += editor.prevDragX - x;
      editor.viewPosY += editor.prevDragY - y;
      editor.applyView();

      worldPos = editor.screenToWorld(event.pageX, event.pageY);
      x = worldPos[0];
      y = worldPos[1];
      editor.prevDragX = x;
      editor.prevDragY = y;
    }

    if (editor.resizing)
    {
      var dx = x - editor.prevMoveX;
      var dy = y - editor.prevMoveY;
      editor.prevMoveX = x;
      editor.prevMoveY = y;

      var e = editor.elems[editor.resizeElem];
      if (e)
      {
        e.moveX += dx;
        e.moveY += dy;
        var offsetX = Math.floor(Math.abs(e.moveX) * 0.05) * sign(e.moveX) * 20.0;
        var offsetY = Math.floor(Math.abs(e.moveY) * 0.05) * sign(e.moveY) * 20.0;
        e.moveX -= offsetX;
        e.moveY -= offsetY;

        if (offsetX || offsetY)
        {
          if (editor.resizeCorner[0] == 0)
          {
            e.x += offsetX;
            e.blockWidth = Math.max(e.blockWidth - offsetX, 200);
          }
          else
            e.blockWidth = Math.max(e.blockWidth + offsetX, 200);

          if (editor.resizeCorner[1] == 0)
          {
            e.y += offsetY;
            e.blockHeight = Math.max(e.blockHeight - offsetY, 200);
          }
          else
            e.blockHeight = Math.max(e.blockHeight + offsetY, 200);

          e.update();
        }
      }
    }

    if (!editor.moving && !editor.dragging && !editor.selecting && !editor.connecting)
    {
      var resizeCorner = editor.getResizeCornerUnderCursor(x, y);
      if (resizeCorner)
      {
        var e = editor.elems[resizeCorner.elem]
        editor.showResizeAt(e.x + resizeCorner.ix * e.blockWidth, e.y + resizeCorner.iy * e.blockHeight);
      }
    }


    if (editor.moving)
    {
      var dx = x - editor.prevMoveX;
      var dy = y - editor.prevMoveY;

      for (var i = 0; i < editor.selected.length; i++)
      {
        var e = editor.elems[editor.selected[i]];
        e.moveX += dx;
        e.moveY += dy;
        e.x = Math.round(e.moveX * 0.05) * 20.0;
        e.y = Math.round(e.moveY * 0.05) * 20.0;
        e.update();
      }

      for (var i = 0; i < editor.edges.length; i++)
      {
        var c = editor.edges[i];
        if (c && (editor.selected.indexOf(c.elemFrom) >= 0 || editor.selected.indexOf(c.elemTo) >= 0))
        {
          c.update();
        }
      }

      editor.prevMoveX = x;
      editor.prevMoveY = y;
    }

    if (editor.selecting)
    {
      editor.selectEndX = x;
      editor.selectEndY = y;
      editor.updateSelectBox();
    }

    if (editor.connecting)
    {
      editor.lineSvg.setAttributeNS(null, "x1", editor.connectFromX);
      editor.lineSvg.setAttributeNS(null, "y1", editor.connectFromY);
      editor.lineSvg.setAttributeNS(null, "x2", x);
      editor.lineSvg.setAttributeNS(null, "y2", y);
      editor.lineSvg.setAttributeNS(null, "stroke", "#fff");
    }
    else
      editor.lineSvg.setAttributeNS(null, "stroke", "transparent");


    var hidePinSelect = true;
    if (!editor.selecting && !editor.dragging)
    {
      var exclude = editor.connecting ? editor.connectFromElem : -1;
      var f = editor.findNearestElemAndPin(x, y, exclude);
      if (f[0] >= 0 && (editor.selected.indexOf(f[0]) < 0 || editor.getElemUnderCursor(x, y) != f[0]))
      {
        var typeListStr = editor.elems[f[0]].pinTypes[f[1]].join(", ");
        if (!editor.connecting)
          hintFrom.innerText = typeListStr;
        else
          hintTo.innerText = typeListStr;

        editor.lastMouseOverElem = f[0];
        editor.lastMouseOverPin = f[1];

        if (!editor.connecting ||
            editor.isValidConnection(editor.connectFromElem, editor.connectFromPin, f[0], f[1]))
        {
          editor.pinSelectSvg.setAttribute("fill", "#8f8");
          editor.pinSelectSvg.setAttribute("fill-opacity", "0.7");
        }
        else
        {
          editor.pinSelectSvg.setAttribute("fill", "#f65");
          editor.pinSelectSvg.setAttribute("fill-opacity", "0.7");
        }

        editor.pinSelectSvg.setAttributeNS(null, "cx", f[2]);
        editor.pinSelectSvg.setAttributeNS(null, "cy", f[3]);
        hidePinSelect = false;
      }

    }

    if (hidePinSelect)
    {
      editor.lastMouseOverElem = -1;
      editor.lastMouseOverPin = -1;

      editor.pinSelectSvg.setAttributeNS(null, "fill", "transparent");
      if (!editor.connecting)
        hintFrom.innerText = "";
      else
        hintTo.innerText = "";
    }
  }


  this.hideResize = function()
  {
    if (editor.resizeSvg && renderEnabled)
    {
      editor.resizeSvg.setAttributeNS(null, "fill", "transparent");
    }
  }


  this.showResizeAt = function(x, y)
  {
    if (editor.resizeSvg && renderEnabled)
    {
      editor.resizeSvg.setAttributeNS(null, "fill", "transparent");
      editor.resizeSvg.setAttribute("fill", "#8f8");
      editor.resizeSvg.setAttribute("fill-opacity", "0.7");
      editor.resizeSvg.setAttributeNS(null, "cx", x);
      editor.resizeSvg.setAttributeNS(null, "cy", y);
    }
  }


  this.onMouseUp = function(event)
  {
    if (!renderEnabled)
      return;

    var worldPos = editor.screenToWorld(event.pageX, event.pageY);
    var x = worldPos[0];
    var y = worldPos[1];

    if (event.button == 0)
    {
      if (editor.resizing)
      {
        editor.resizing = false;
      }

      if (editor.selecting)
      {
        var s = editor.getSelectedElems(editor.selectStartX, editor.selectStartY, editor.selectEndX, editor.selectEndY);
        if (event.ctrlKey)
          editor.xorSelection(s);
        else
          editor.selected = s;

        editor.onSelectionChanged();

        editor.hideSelectBox();
        editor.selecting = false;
        editor.onChanged(false, false, true);
      }

      if (editor.connecting)
      {
        var exclude = editor.connectFromElem;
        var f = editor.findNearestElemAndPin(x, y, -1);
        var hide = false;
        if (f[0] >= 0 && exclude != f[0])
        {
          if (editor.isValidConnection(editor.connectFromElem, editor.connectFromPin, f[0], f[1]))
          {
            editor.addConnection(editor.connectFromElem, editor.connectFromPin, f[0], f[1], -1, null, null);
            editor.killHiddenConnections();

            editor.propagateTypes();
            editor.onChanged();

            hide = true;
          }
        }

        if (hide || editor.connectFromElem != f[0] || editor.connectFromPin != f[1])
        {
          editor.connecting = false;
          editor.lineSvg.setAttributeNS(null, "stroke", "transparent");
        }
      }

      if (editor.moving)
      {
        editor.moving = false;
        if (editor.selected.length > 0)
          editor.onChanged(false, false, true);
      }

      hintFrom.innerText = "";
      hintTo.innerText = "";
    }

    if (event.button == 1)
    {
      editor.dragging = false;
      editor.resizing = false;
    }
  }

  this.onWheel = function(event)
  {
    if (this.dragging)
      return;

    if (!renderEnabled)
      return;

    var centerBefore = this.screenToWorld(event.pageX, event.pageY);

    if (event.deltaY < 0)
      this.viewWidth *= 0.9;
    else if (event.deltaY > 0)
      this.viewWidth /= 0.9;

    this.viewWidth = Math.max(Math.min(this.viewWidth, 50000), 500);
    var center = this.screenToWorld(event.pageX, event.pageY);

    this.viewPosX += centerBefore[0] - center[0];
    this.viewPosY += centerBefore[1] - center[1];

    this.applyView();
  }

  this.onNewGraph = function()
  {
    var name = prompt("New graph name:", pluginName);
    if (name)
    {
      name = this.makeSafeFilename(name);
      lastRespondTime = Date.now();
      if (name !== null)
      {
        this.graphId = null;
        query("new_subgraph&" + clientId + "&" + name, null, " ");
        query("command&" + clientId + "&open:" + name, null);
      }
    }
  }


  this.keepOnlySingleExternals_ = function()
  {
    function getElemProperty(e, propName, def)
    {
      for (var i = 0; i < e.propValues.length; i++)
        if (e.desc.properties[i].name === propName)
          return e.propValues[i];
      return def;
    }

    var processedItems = [];
    var connectionRemap = [];

    for (var i = 0; i < editor.elems.length; i++)
    {
      var e = editor.elems[i];
      if (!e || !e.desc.isExternal || e.desc.pins.length != 1)
        continue;

      var hash = e.desc.name + " # " + getElemProperty(e, "name", null);
      if (processedItems.indexOf(hash) < 0)
      {
        processedItems.push(hash);
        for (var j = i + 1; j < editor.elems.length; j++)
        {
          var other = editor.elems[j];
          if (!other || !other.desc.isExternal || other.desc.pins.length != 1)
            continue;
          var otherHash = other.desc.name + " # " + getElemProperty(other, "name", null);
          if (otherHash === hash)
          {
            connectionRemap.push([j, i]); // j -> i
          }
        }
      }
    }

    if (connectionRemap.length > 0)
    {
      for (var i = 0; i < editor.edges.length; i++)
      {
        var c = editor.edges[i];
        if (!c)
          continue;

        for (var j = 0; j < connectionRemap.length; j++)
        {
          if (c.elemFrom === connectionRemap[j][0])
            editor.addConnection(connectionRemap[j][1], c.pinFrom, c.elemTo, c.pinTo, -1, null, null);

          if (c.elemTo === connectionRemap[j][0])
            editor.addConnection(c.elemFrom, c.pinFrom, connectionRemap[j][1], c.pinTo, -1, null, null);
        }
      }

      for (var j = 0; j < connectionRemap.length; j++)
        editor.elems[connectionRemap[j][0]].kill();
    }
  }

  this.deleteBypassKeepConnections_ = function(idx)
  {
    var e = editor.elems[idx];
    if (!e)
      return;

    var leftElem = -1;
    var leftPin = -1;
    var bestPin = 1e6;
    var rightPins = [];
    var connectedIn = -1;
    var connectedOut = -1;

    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = editor.edges[i];
      if (!c)
        continue;

      if (connectedIn === -1)
      {
        if (c.elemFrom === idx)
          if (editor.elems[c.elemFrom].desc.pins[c.pinFrom].role === "in")
            connectedIn = i;

        if (c.elemTo === idx)
          if (editor.elems[c.elemTo].desc.pins[c.pinTo].role === "in")
            connectedIn = i;
      }

      if (connectedOut === -1)
      {
        if (c.elemFrom === idx)
          if (editor.elems[c.elemFrom].desc.pins[c.pinFrom].role === "out")
            connectedOut = i;

        if (c.elemTo === idx)
          if (editor.elems[c.elemTo].desc.pins[c.pinTo].role === "out")
            connectedOut = i;
      }
    }

    if (connectedIn === -1 || connectedOut === -1)
      return;


    if (e.propagatedStuff)
      for (var i = 0; i < e.desc.pins.length; i++)
        if (e.desc.pins[i].role === "in")
        {
          var opp = editor.getOppositeConnection(connectedIn, idx);
          if (opp)
          {
            var oppElem = editor.elems[opp.elem];
            if (oppElem && oppElem.propagatedStuff)
            {
              var s1 = JSON.stringify(e.propagatedStuff);
              var s2 = JSON.stringify(oppElem.propagatedStuff);
              if (s1 != s2)
                return;        // don't remove this bypass. This node will change data type
            }
          }
        }


    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = editor.edges[i];
      if (!c)
        continue;

      var selectFrom = (c.elemFrom == idx);
      var selectTo = (c.elemTo == idx);

      if (selectFrom != selectTo)
      {
        var roleFrom = editor.elems[c.elemFrom].desc.pins[c.pinFrom].role;
        var roleTo = editor.elems[c.elemTo].desc.pins[c.pinTo].role;

        if (selectFrom && roleFrom === "out")
          rightPins.push({elem:c.elemTo, pin:c.pinTo});

        if (selectTo && roleTo === "out")
          rightPins.push({elem:c.elemFrom, pin:c.pinFrom});

        if (selectFrom && roleFrom === "in" && c.pinFrom < bestPin)
        {
          leftElem = c.elemTo;
          leftPin = c.pinTo;
          bestPin = c.pinFrom;
        }

        if (selectTo && roleTo === "in" && c.pinTo < bestPin)
        {
          leftElem = c.elemFrom;
          leftPin = c.pinFrom;
          bestPin = c.pinTo;
        }
      }

      if (selectFrom || selectTo)
        c.kill();
    }

    e.kill();

    if (leftElem >= 0)
      for (var i = 0; i < rightPins.length; i++)
      {
        //if (editor.isValidConnection(leftElem, leftPin, rightPins[i].elem, rightPins[i].pin))
        editor.addConnection(leftElem, leftPin, rightPins[i].elem, rightPins[i].pin, -1, null, null);
      }
  }

  this.removeAllBypassNodes = function()
  {
    var deleted = false;

    for (var t = editor.elems.length - 1; t >= 0; t--)
    {
      var e = editor.elems[t];
      if (e && e.desc && e.desc.bypass && editor.specialSelected != t)
      {
        editor.deleteBypassKeepConnections_(t);
        deleted = true;
      }
    }

    if (deleted)
      editor.propagateTypes();
  }


  this.centerGraphView = function()
  {
    var minX = 1e6;
    var minY = 1e6;
    var maxX = -1e6;
    var maxY = -1e6;

    for (var t = editor.elems.length - 1; t >= 0; t--)
    {
      var e = editor.elems[t];
      if (e)
      {
        minX = Math.min(minX, e.x);
        minY = Math.min(minY, e.y);
        maxX = Math.max(maxX, e.x + e.width);
        maxY = Math.max(maxY, e.y + e.height);
      }
    }

    if (minX < maxX)
    {
      this.viewWidth = Math.max(maxX - minX, maxY - minY) * 1.1;
      this.viewWidth = Math.max(Math.min(this.viewWidth, 50000), 500);
      this.viewPosX = minX;
      this.viewPosY = minY - this.viewWidth * 0.1;

      this.applyView();
    }
  }


  this.zoomToNode = function(nodeIndex)
  {
    var e = editor.elems[nodeIndex];
    if (e)
    {
      this.viewWidth = Math.max(e.width, e.height) * 1.1;
      this.viewWidth = Math.max(Math.min(this.viewWidth, 1500), 700);
      this.viewPosX = e.x - this.viewWidth * 0.5 + e.width * 0.5;
      this.viewPosY = e.y - this.viewWidth * 0.5 + e.height * 0.5;

      this.applyView();
    }
  }


  this.selectionTargetIndex = -1;

  this.findAndSelectNode = function(text)
  {
    // remove selection
    editor.selected = [];

    this.selectionTargetIndex = -1;

    for (var t = editor.elems.length - 1; t >= 0; t--)
    {
      var e = editor.elems[t];
      if (e && e.desc && e.desc.name.indexOf(text) >= 0)
      {
        editor.selected.push(t);
        if (this.selectionTargetIndex < 0)
          {
            this.selectionTargetIndex = t;
            this.zoomToNode(t);
          }
      }
    }

    editor.onSelectionChanged();
  }


  this.targetNextSelectedNode = function()
  {
    for (var i = this.selectionTargetIndex + 1; i < this.elems.length; i++)
      if (this.selected.indexOf(i) >= 0)
      {
        this.selectionTargetIndex = i;
        this.zoomToNode(i);
        return;
      }

    for (var i = 0; i < this.selectionTargetIndex; i++)
      if (this.selected.indexOf(i) >= 0)
      {
        this.selectionTargetIndex = i;
        this.zoomToNode(i);
        return;
      }

    this.selectionTargetIndex = -1;
  }


  this.selectNodesWithNoOutputs = function()
  {
    this.selected = [];
    var hasOutputConnection = [];
    hasOutputConnection.length = this.elems.length;
    hasOutputConnection.fill(false);

    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      if (!c)
        continue;

      if (this.elems[c.elemFrom] && this.elems[c.elemFrom].desc && this.elems[c.elemFrom].desc.pins[c.pinFrom].role === "out")
        hasOutputConnection[c.elemFrom] = true;

      if (this.elems[c.elemTo] && this.elems[c.elemFrom].desc && this.elems[c.elemTo].desc.pins[c.pinTo].role === "out")
        hasOutputConnection[c.elemTo] = true;
    }

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e && e.desc && !hasOutputConnection[i])
      {
        for (var j = 0; j < e.desc.pins.length; j++)
          if (e.desc.pins[j].role === "out")
          {
            this.selected.push(i);
            break;
          }
      }
    }

    this.onSelectionChanged();
  }


  this.showHelp = function()
  {
    if (!renderEnabled)
      return;

    editor.bigTextArea.style.visibility = "visible";
    editor.bigTextArea.value =
      " In Editor:\n"
      +"\n     F1 - This window"
      +"\n      N - New graph of current type"
      +"\n Ctrl+O - Open graph"
      +"\n  Space - Show element filter, element will be placed at cursor position"
      +"\n    LMB - Select element, create edge, move selected elements when pressed"
      +"\n Ctrl+LMB - Toggle selection"
      +"\n DClick - Special select (for node preview etc.)"
      +"\n MWheel - Zoom, pan when pressed"
      +"\n    Del - Remove"
      +"\n BackSp - Remove, keep connection"
      +"\n      D - Duplicate (Ctrl+D works too)"
      +"\n Ctrl+C - Copy"
      +"\n Ctrl+X - Cut"
      +"\n Ctrl+V - Paste"
      +"\n      X - Remove edges (cursor must be over pin)"
      +"\n      A - Modity edge (cursor must be over pin)"
      +"\n    Tab - Jump to opposite pin (cursor must be over pin)"
      +"\n Ctrl+A - Select all"
      +"\n Ctrl+Z - Undo (Ctrl+Shift+Z - Redo)"
      +"\n Ctrl+G - Make subgraph from selection"
      +"\n Ctrl+E - Explode subgraph"
      +"\n      C - Add comment to pin (if comment is empty or comment equals '#' then pin will not be exported)"
      +"\n Ctrl+Space - Center view on graph"
      +"\n Shift+Q - Toggle autoupdate"
      +"\n Ctrl+R - Force rebuild"
      +"\n     F4 - Show generated code"
      +"\n Ctrl+F - Find and select node"
      +"\n Shift+F - Show next selected node"
      +"\n Ctrl+M - Select nodes with no outputs connected"
      ;

    setTimeout(function(){editor.bigTextArea.focus(); editor.bigTextArea.setSelectionRange(0, 0);}, 10);
  }

  this.onKeyDown = function(event)
  {
    var focused = document.activeElement;
    if (focused && focused.nodeName === "INPUT")
      return;

    if (editor.bigTextArea.style.visibility === "visible")
      return;

    var del = (event.keyCode == 46 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyF1 = (event.keyCode == 112 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyF4 = (event.keyCode == 115 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyX = (event.keyCode == 88 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyA = (event.keyCode == 65 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyD = (event.keyCode == 68 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyC = (event.keyCode == 67 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyN = (event.keyCode == 78 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyO = (event.keyCode == 79 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyTab = (event.keyCode == 9 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keyBackSpace = (event.keyCode == 8 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var keySpace = (event.keyCode == 32 && !event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlSpace = (event.keyCode == 32 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlD = (event.keyCode == 68 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlA = (event.keyCode == 65 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlX = (event.keyCode == 88 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlC = (event.keyCode == 67 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlV = (event.keyCode == 86 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlZ = (event.keyCode == 90 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlO = (event.keyCode == 79 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlG = (event.keyCode == 71 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlE = (event.keyCode == 69 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlR = (event.keyCode == 82 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var ctrlShiftE = (event.keyCode == 69 && event.ctrlKey && event.shiftKey && !event.altKey);
    var ctrlShiftZ = (event.keyCode == 90 && event.ctrlKey && event.shiftKey && !event.altKey);
    var shiftQ = (event.keyCode == 81 && !event.ctrlKey && event.shiftKey && !event.altKey);
    var shiftT = (event.keyCode == 84 && !event.ctrlKey && event.shiftKey && !event.altKey);
    var ctrlF = (event.keyCode == 70 && event.ctrlKey && !event.shiftKey && !event.altKey);
    var shiftF = (event.keyCode == 70 && !event.ctrlKey && event.shiftKey && !event.altKey);
    var ctrlM = (event.keyCode == 77 && event.ctrlKey && !event.shiftKey && !event.altKey);

    if (ctrlF)
    {
      event.preventDefault();
      var text = prompt("Find", "");
      if (text !== null && text.length > 0)
        this.findAndSelectNode(text);
    }

    if (shiftF)
    {
      event.preventDefault();
      this.targetNextSelectedNode();
    }

    if (ctrlM)
    {
      event.preventDefault();
      this.selectNodesWithNoOutputs();
    }

    if (ctrlR)
    {
      event.preventDefault();
      query("command&" + clientId + "&force_rebuild", null);
    }

    if (shiftT)
    {
      // FOR TEST:
      //this.removeAllBypassNodes();
    }

    if (keyF1)
    {
      event.preventDefault();
      this.showHelp();
    }

    if (keyF4)
    {
      event.preventDefault();
      query("command&" + clientId + "&get_generated_code", null);
    }

    if (keySpace)
    {
      event.preventDefault();
      nodeDescriptions.showFilterDialog();
    }

    if (ctrlSpace)
    {
      event.preventDefault();
      this.centerGraphView();
    }

    if (ctrlO || keyO)
    {
      event.preventDefault();
      query("onfocus&" + clientId); // will refresh graphs' descriptions
      openGraphDialog.showDialog();
    }

    if (keyN)
    {
      event.preventDefault();
      this.onNewGraph();
    }

    if (ctrlC)
    {
      this.copyToClipboard();
    }

    if (ctrlV)
    {
      this.paste(null);
    }

    if (ctrlG)
    {
      event.preventDefault();
      this.makeSubgraphFromSelection();
    }

    if (ctrlE)
    {
      event.preventDefault();
      if (editor.selected.length === 1)
        if (editor.checkForExplodeSubgraph(editor.selected[0]) === false)
          editor.onChanged();
    }

    if (ctrlShiftE)
    {
      event.preventDefault();
      globalLockInput = true;
      setTimeout(function(){ globalLockInput = false; }, 1500);
      editor.explodeAllSubgraphs(0,
        function(ok)
        {
          if (ok)
            editor.onChanged();

          globalLockInput = false;
        });
    }

    if (ctrlA)
    {
      event.preventDefault();
      this.selectAll();
      this.onChanged(false, false, true);
    }

    if (keyD || ctrlD)
    {
      event.preventDefault();
      this.duplicateSelected();
      this.onChanged();
    }

    if (ctrlZ)
    {
      event.preventDefault();
      this.undo(null);
      this.onChanged(true);
    }

    if (ctrlShiftZ)
    {
      event.preventDefault();
      this.redo(null);
      this.onChanged(true);
    }

    if (shiftQ)
    {
      event.preventDefault();
      query("command&" + clientId + "&toggle_autoupdate", null);
    }

    if (keyX)
    {
      event.preventDefault();
      this.removeEdgesUnderCursor();
      this.propagateTypes();
      this.onChanged();
    }

    if (keyA)
    {
      event.preventDefault();
      this.modifyEdgeUnderCursor();
      this.propagateTypes();
    }

    if (keyTab)
    {
      event.preventDefault();
      this.jumpToOppositePin();
    }

    if (keyC)
    {
      event.preventDefault();
      this.commentPinUnderCursor();
      this.onChanged();
    }

    if (del || ctrlX || keyBackSpace)
    {
      if (ctrlX)
        this.copyToClipboard();

      if (keyBackSpace)
        event.preventDefault();

      editor.connecting = false;
      editor.lineSvg.setAttributeNS(null, "stroke", "transparent");

      var leftElem = -1;
      var leftPin = -1;
      var bestPin = 1e6;
      var rightPins = [];


      for (var i = 0; i < editor.edges.length; i++)
      {
        var c = editor.edges[i];
        if (!c)
          continue;

        var selectFrom = editor.selected.indexOf(c.elemFrom) >= 0;
        var selectTo = editor.selected.indexOf(c.elemTo) >= 0;

        if (keyBackSpace && selectFrom != selectTo)
        {
          var roleFrom = editor.elems[c.elemFrom].desc.pins[c.pinFrom].role;
          var roleTo = editor.elems[c.elemTo].desc.pins[c.pinTo].role;

          if (selectFrom && roleFrom === "out")
            rightPins.push({elem:c.elemTo, pin:c.pinTo});

          if (selectTo && roleTo === "out")
            rightPins.push({elem:c.elemFrom, pin:c.pinFrom});

          if (selectFrom && roleFrom === "in" && c.pinFrom < bestPin)
          {
            leftElem = c.elemTo;
            leftPin = c.pinTo;
            bestPin = c.pinFrom;
          }

          if (selectTo && roleTo === "in" && c.pinTo < bestPin)
          {
            leftElem = c.elemFrom;
            leftPin = c.pinFrom;
            bestPin = c.pinTo;
          }
        }

        if (selectFrom || selectTo)
          c.kill();
      }

      for (var i = 0; i < editor.selected.length; i++)
      {
        var e = editor.elems[editor.selected[i]];
        if (e)
          e.kill();
      }

      if (leftElem >= 0)
        for (var i = 0; i < rightPins.length; i++)
          if (editor.isValidConnection(leftElem, leftPin, rightPins[i].elem, rightPins[i].pin))
            editor.addConnection(leftElem, leftPin, rightPins[i].elem, rightPins[i].pin, -1, null, null);

      editor.propagateTypes();
      editor.selected = [];
      editor.onSelectionChanged();
      editor.onChanged();
    }

  }

  this.onKeyUp = function(event)
  {
    if (this.onKeyUpFunction)
    {
      var e = document.getElementById('clipboardEditBox');
      if (e.style.visibility !== "hidden")
      {
        setTimeout( function()
          {
            if (editor.onKeyUpFunction)
              editor.onKeyUpFunction();
            editor.onKeyUpFunction = null;
            document.getElementById('clipboardEditBox').style.visibility = 'hidden';
            if (document.activeElement)
              document.activeElement.blur();
            window.focus();
          }, 10);
      }
    }
  }

  this.fastHash = function(str)
  {
    var hash = 5381 >>> 0;
    var len = str.length;

    for (var i = 0; i < len; i++)
      hash = ((hash * 33) ^ str.charCodeAt(i)) & 0xFFFF;

    return hash >>> 0;
  }

  this.makeSafeFilename = function(name)
  {
    var code_0 = "0".charCodeAt(0);
    var code_9 = "9".charCodeAt(0);
    var code_a = "a".charCodeAt(0);
    var code_z = "z".charCodeAt(0);
    var code_A = "A".charCodeAt(0);
    var code_Z = "Z".charCodeAt(0);
    var code_underscore = "_".charCodeAt(0);

    for (var j = name.length - 1; j >= 0; j--)
    {
      var ch = name.charCodeAt(j);
      var good = (ch >= code_0 && ch <= code_9) || (ch >= code_a && ch <= code_z) ||
                 (ch >= code_A && ch <= code_Z) || ch == code_underscore;

      if (!good)
      {
        name = name.slice(0, j) + "_" + name.slice(j + 1);
      }
    }

    return name;
  }

  this.makeSubgraphFromSelection = function()
  {
    if (this.selected.length <= 1)
      return;

    this.replaceWithSubgraph = undefined;

    var subgraphStr = this.stringifyGraph(S_FULL_INFO | S_SELECTED_ONLY | S_SKIP_GRAPH_ID | S_INOUT_PINS);
    var subgraphName = prompt("Enter subgraph name:", "g-" + generateRandomUid());
    lastRespondTime = Date.now();

    if (subgraphName)
      subgraphName = this.makeSafeFilename(subgraphName.trim());

    if (subgraphName && subgraphName.length > 0)
    {
      if (this.changedTimerId)
        clearTimeout(this.changedTimerId);

      globalLockInput = true;
      setTimeout(function(){ globalLockInput = false; }, 1200);

      for (var i = 0; i < this.edges.length; i++)
        if (this.elems[i])
          this.elems[i]._toSubgraph = undefined;

      for (var j = 0; j < this.selected.length; j++)
        if (this.elems[this.selected[j]])
          this.elems[this.selected[j]]._toSubgraph = true;

      this.replaceWithSubgraph = subgraphName;

      query("new_subgraph&" + clientId + "&" + subgraphName, null, subgraphStr);
    }
  }

  this.replaceSelectedWithSubgraph = function(subgraphName)
  {
    var desc = nodeDescriptions.findDescriptionByName(subgraphName);
    if (!desc)
      return;

    var cx = 0.0;
    var cy = 0.0;
    var cnt = 0.0;
    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e && e._toSubgraph)
      {
        cx += e.x;
        cy += e.y;
        cnt++;
      }
    }

    if (cnt == 0)
      return;

    cx /= cnt;
    cy /= cnt;

    var idx = this.addElem(cx, cy, desc, -1);
    var newElem = this.elems[idx];

    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      if (!c)
        continue;

      if (this.elems[c.elemTo]._toSubgraph != this.elems[c.elemFrom]._toSubgraph)
      {
        if (this.elems[c.elemTo]._toSubgraph)
        {
          for (var j = 0; j < newElem.desc.pins.length; j++)
          {
            var p = newElem.desc.pins[j];
            if (p.subgraphElem === c.elemTo && p.subgraphPin === c.pinTo)
            {
              this.addConnection(c.elemFrom, c.pinFrom, idx, j, -1, null, null);
              break;
            }
          }
        }
        else if (this.elems[c.elemFrom]._toSubgraph)
        {
          for (var j = 0; j < newElem.desc.pins.length; j++)
          {
            var p = newElem.desc.pins[j];
            if (p.subgraphElem === c.elemFrom && p.subgraphPin === c.pinFrom)
            {
              this.addConnection(c.elemTo, c.pinTo, idx, j, -1, null, null);
              break;
            }
          }
        }
      }
    }

    for (var i = this.elems.length - 1; i >= 0; i--)
    {
      var e = this.elems[i];
      if (e && e._toSubgraph)
        e.kill();
    }

    for (var i = 0; i < editor.edges.length; i++)
    {
      var c = editor.edges[i];
      if (c && (!editor.elems[c.elemFrom] || !editor.elems[c.elemTo]))
        c.kill();
    }

    editor.propagateTypes();
    editor.selected = [];
    editor.onSelectionChanged();

    globalLockInput = false;
    editor.onChanged();
  }

  this.parseJsonProperty = function(s)
  {
    try
    {
      var obj = JSON.parse("{" + s + "}");
      return obj;
    }
    catch (err)
    {
      console.log("Obj in comment: cannot parse {" + s + "}");
    }

    return null;
  }

  this.stringifyGraph = function(flags)
  {
    var fullInfo = (flags & S_FULL_INFO) != 0;
    var selectedOnly = (flags & S_SELECTED_ONLY) != 0;
    var skipGraphId = (flags & S_SKIP_GRAPH_ID) != 0;
    var copyInputEdges = (flags & S_COPY_INPUT_EDGES) != 0;
    var skipPostprocress = (flags & S_SKIP_POSTPROCESS) != 0;
    var asObject = (flags & S_RETURN_AS_OBJECT) != 0;
    var exportInOutPins = (flags & S_INOUT_PINS) != 0;

    if (fullInfo)
      this.propagateTypes();

    if (!skipGraphId && !this.graphId)
      this.graphId = "[[GID:0A-" + Date.now() + "]]";

    var graph = {};
    graph.graphId = selectedOnly ? ("[[GID:0A-" + Date.now() + "]]") : this.graphId;
    graph.pluginId = "[[plugin:" + pluginName + "]]";
    graph.elemCount = this.elems.length;
    graph.edgeCount = this.edges.length;
    graph.hint = "[[generated_graph_hint]]";
    graph.view =
    {
      posX: this.viewPosX,
      posY: this.viewPosY,
      width: this.viewWidth
    };

    if (!selectedOnly)
    {
      graph.selected = this.selected.slice(0);
      graph.specialSelected = this.specialSelected;
    }
    else
    {
      graph.selected = [];
      graph.specialSelected = -1;
    }

    // graph properties
    if (this.graphElem)
    {
      var t = {};
      var e = this.graphElem;
      for (var j = 0; j < e.propValues.length; j++)
        t[e.desc.properties[j].name] = e.propValues[j];

      graph.graphElemProps = t;
    }

    graph.elems = [];
    graph.edges = [];

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      var skipElem = (selectedOnly && this.selected.indexOf(i) < 0);
      if (skipElem && e && e.desc.inOutMarker && exportInOutPins)
      {
        var edgeIndex = this.findConnection(i, 0);
        if (edgeIndex >= 0)
        {
          var c = this.edges[edgeIndex];
          if (this.selected.indexOf(c.elemFrom) >= 0 || this.selected.indexOf(c.elemTo) >= 0)
            skipElem = false;
        }
      }

      if (!e || skipElem)
        graph.elems.push(null);
      else
      {
        var propList = [];
        var pinList = [];
        for (var j = 0; j < e.desc.properties.length; j++)
        {
          var v = e.desc.properties[j];
          var p = {
            name: v.name,
            type: v.type,
            value: e.propValues[j],
          };

          propList.push(p);
        }


        if (fullInfo)
        {
          for (var j = 0; j < e.desc.pins.length; j++)
          {
            var p = e.desc.pins[j];
            var graphPin =
              {
                name: p.name,
                role: p.role,
                group: p.typeGroup,
                types: e.pinTypes[j].slice(0),
                data: p.data,
                makeInternalVar: p.makeInternalVar ? p.makeInternalVar : false,
                optional: e.determineIfOptional(j),
              };

            pinList.push(graphPin);
          }
        }

        var elem = {
          id: e.myIndex,
          descName: e.desc.name,
          properties: propList,
          uid: e.uid,
          view: {x: e.x, y: e.y},
          pinComments: e.pinComments.length > 0 ? e.pinComments.slice(0) : undefined,
        };

        if (e.desc.name === "block")
        {
          elem.blockWidth = e.blockWidth;
          elem.blockHeight = e.blockHeight;
        }

        if (e.desc.bypass)
          elem.bypass = true;

        if (e.desc.generated)
          elem.generated = true;

        if (e.desc.plugin)
          elem.plugin = e.desc.plugin;

        if (e.desc.nativeShader)
          elem.nativeShader = e.desc.nativeShader;

        if (e.desc.isExternal)
          elem.isExternal = e.desc.isExternal;

        if (e.propagatedStuff)
          elem.propagatedStuff = e.propagatedStuff;

        if (e.externalUid)
        {
          elem.externalUid = e.externalUid;

          for (var k = 0; k < e.desc.properties.length; k++)
            if (e.desc.properties[k].name === "external name")
            {
              elem.externalName = e.propValues[k].trim();
              break;
            }
        }

        if (fullInfo)
        {
          elem.pins = pinList;
          elem.groupTypes = e.largestGroupTypes;
        }

        graph.elems.push(elem);
      }
    }

    var extraEdges = [];
    var externals = [];
    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (!e)
        continue;
      if (selectedOnly && this.selected.indexOf(i) < 0)
        continue;

      var elemExternalName = "";

      if (e.externalUid)
      {
        for (var j = 0; j < e.propValues.length; j++)
        {
          if (e.desc.properties[j].name === "external name")
            elemExternalName = e.propValues[j].trim();
        }

        if (elemExternalName.length > 0)
        {
          var haveIns = false;
          var haveOuts = false;
          for (var j = 0; j < e.desc.pins.length; j++)
          {
            var p = e.desc.pins[j];

            if (!haveIns && p.role === "in")
              haveIns = true;

            if (!haveOuts && p.role === "out")
              haveOuts = true;
          }


          if (!haveIns || !haveOuts)                // only for input nodes or output nodes
            for (var j = 0; j < e.desc.pins.length; j++)
            {
              var subgraphPins = [];

              if (e.desc.pins.length == 1)
              {
                var conns = this.findConnections(i, j);
                for (var k = 0; k < conns.length; k++)
                {
                  var opp = this.getOppositeConnection(conns[k], i);
                  subgraphPins.push({elem: opp.elem, pin: opp.pin, uid: (e.desc.pins[opp.pin] ? e.desc.pins[opp.pin].uid : undefined) });
                }
              }


              var p = e.desc.pins[j];
              var externalPin = {
                name: elemExternalName,
                yPos: e.y + j * 0.01,
                uid: e.externalUid + "-" + i,
                types: e.pinTypes[j].slice(0),
                role: p.role === "out" ? "in" : "out",
                singleConnect: p.role === "out" ? true : false,
                data: {},
              };

              if (subgraphPins.length > 0)
                externalPin.subgraphPins = subgraphPins;

              externals.push(externalPin);
            }
        }
      }

      // subgraph in/out will be converted to external pins
      if (!e.desc.inOutMarker)
      {
        var overrideY = null;

        for (var j = 0; j < e.desc.pins.length; j++)
        {
          var makeExternal = false;
          var createSubgraphNode = true;

          var connectionName = e.getPinComment(j);

          if (e.connected)
          {
            // is opposite element not selected
            var idx = this.findConnection(i, j);
            if (idx >= 0)
            {
              var c = this.edges[idx];
              if (selectedOnly)
              {
                if (c.elemFrom == i && this.selected.indexOf(c.elemTo) < 0)
                {
                  makeExternal = true;
                  createSubgraphNode = true;
                }

                if (c.elemTo == i && this.selected.indexOf(c.elemFrom) < 0)
                {
                  makeExternal = true;
                  createSubgraphNode = true;
                }
              }

              var oppositeElem = c.elemFrom == i ? c.elemTo : c.elemFrom;
              if (this.elems[oppositeElem].desc.inOutMarker && exportInOutPins)
              {
                makeExternal = true;
                createSubgraphNode = false;
                if (this.elems[oppositeElem].propValues[0].trim().length > 0)
                {
                  connectionName = this.elems[oppositeElem].propValues[0].trim();
                  overrideY = this.elems[oppositeElem].y + j * 0.01;
                }
              }
            }
          }

          var p = e.desc.pins[j];

          if (connectionName.length === 0)
            connectionName = elemExternalName.length > 0 ? elemExternalName : p.name


          if (!exportInOutPins)
            createSubgraphNode = false;

          if (createSubgraphNode)
          {
            var expectedDescName = "";
            var graphDesc = null;
            if (p.role === "out")
            {
              expectedDescName = "subgraph out";
              graphDesc = nodeDescriptions.findDescriptionByName("subgraph out");
            }
            else if (p.role === "in")
            {
              var typeName = e.pinTypes[j][0];
              graphDesc = nodeDescriptions.findDescriptionByName("subgraph in: " + typeName);
              if (!graphDesc)
                makeExternal = false;
              else
                expectedDescName = "subgraph in: " + typeName;
            }

            if (makeExternal)
            {
              var pinList = [];
              for (var k = 0; k < graphDesc.pins.length; k++)
              {
                var px = graphDesc.pins[k];
                var graphPin =
                  {
                    name: px.name,
                    role: px.role,
                    group: px.typeGroup,
                    types: graphDesc.pins[0].types.slice(0),
                    data: px.data,
                  };

                pinList.push(graphPin);
              }

              var newElem = {
                id: graph.elems.length,
                descName: expectedDescName,
                properties: [{
                  "name": "name",
                  "type": "string",
                  "value": connectionName,
                }],
                uid: "in-out-" + connectionName,
                view: {x: e.x + (p.role == "out" ? 450 : -800), y: e.y + j * 80},
                pinComments: undefined,
                pins: pinList,
              };

              overrideY = newElem.view.y;

              graph.elems.push(newElem);

              extraEdges.push({
                id: extraEdges.length,
                elemA: i,
                pinA: j,
                elemB: newElem.id,
                pinB: 0,
              });
            }
          }


          if (p.role === "out" && p.hidden) // have hidden out pin
          {
            var ptype = p.types[0];
            if (ptype === "texture1D" || ptype === "texture2D" || ptype === "texture3D" || ptype === "particles")
              makeExternal = true;

            for (var k = 0; k < e.propValues.length; k++)
            {
              if (e.desc.properties[k].name === "external name")
              {
                connectionName = e.propValues[k].trim();
                makeExternal = true;
              }
            }
          }


          if (makeExternal)
          {
            var group = p.typeGroup;
            var singleType = group && graph.elems[i].groupTypes ? graph.elems[i].groupTypes[group] : null;

            var externalPin = {
              name: connectionName,
              yPos: overrideY !== null ? overrideY : e.y + j * 0.01,
              uid: "in-out-uid--" + connectionName,
              types: singleType ? [singleType] : e.pinTypes[j].slice(0),
              role: p.role,
              singleConnect: p.singleConnect,

              subgraphElem: i,
              subgraphPin: j,
              subgraphPinUid: e.desc.pins[j].uid ? e.desc.pins[j].uid : undefined, // optional

              data: {},
            };

            if (p.data)
            {
              externalPin.data = Object.assign({}, p.data);
            }

            externals.push(externalPin);
          }
        }
      }
    }

    // if (externals.length > 0)
    {
      // automatically create pins' typeGroups
      {
        var compareTypes = function(a, b)
        {
          if (a.length != b.length)
            return false;

          for (var i = 0; i < a.length; i++)
            if (a[i] !== b[i])
              return false;

          return true;
        }

        for (var i = 0; i < externals.length; i++)
          if (externals[i].types.length > 1 && !externals[i].typeGroup)
            for (var j = i + 1; j < externals.length; j++)
              if (compareTypes(externals[i].types, externals[j].types))
              {
                var group = "group" + i;
                externals[i].typeGroup = group;
                externals[j].typeGroup = group;
              }
      }


      externals.sort(function(a, b){ return a.yPos - b.yPos; });

      graph.description = {
        name: "[[description-name]]",
        category: "[[description-category]]",
        uid: skipGraphId ? undefined : this.graphId.slice(8),
        pins: externals,
        properties: [],
        allowLoop: false,
        generated: true,
        plugin: pluginName,
        width: 200,
      };

      if (fullInfo)
        for (var i = 0; i < this.elems.length; i++)
        {
          var e = this.elems[i];
          if (!e || (e.desc.name !== "comment" && e.desc.name !== "block"))
            continue;

          for (var j = 0; j < e.desc.properties.length; j++)
          {
            var name = e.desc.properties[j].name;
            var value = e.propValues[j];
            if (name === "comment string")
            {
              if (value.length > 4 && value[0] === '@' && value.indexOf(':') > 0) // substitution to description
              {
                var obj = this.parseJsonProperty(value.slice(1));
                if (obj)
                  for (var key in obj)
                  {
                    if (key === "pins" || key === "properties")
                      graph.description[key].push(obj[key]);
                    else
                      graph.description[key] = obj[key];
                  }
              }

              break;
            }
          }
        }
    }


    for (var i = 0; i < this.edges.length; i++)
    {
      var c = this.edges[i];
      var ok = c && !c.hidden;
      var inputPinFrom = false;
      var inputPinTo = false;
      if (ok && selectedOnly)
      {
        ok = (this.selected.indexOf(c.elemFrom) >= 0 && this.selected.indexOf(c.elemTo) >= 0);
        if (!ok && exportInOutPins)
        {
          if (this.selected.indexOf(c.elemFrom) >= 0 && this.elems[c.elemTo].desc.inOutMarker)
            ok = true;
          if (this.selected.indexOf(c.elemTo) >= 0 && this.elems[c.elemFrom].desc.inOutMarker)
            ok = true;
        }

        if (!ok && copyInputEdges)
        {
          var e = this.selected.indexOf(c.elemFrom) >= 0 ? this.elems[c.elemFrom] : null;
          if (e && e.desc.pins[c.pinFrom].role === "in" && this.elems[c.elemTo] && !this.elems[c.elemTo].desc.inOutMarker)
          {
            ok = true;
            inputPinFrom = true;
          }

          e = this.selected.indexOf(c.elemTo) >= 0 ? this.elems[c.elemTo] : null;
          if (e && e.desc.pins[c.pinTo].role === "in" && this.elems[c.elemFrom] && !this.elems[c.elemFrom].desc.inOutMarker)
          {
            ok = true;
            inputPinTo = true;
          }
        }
      }


      if (!ok)
        graph.edges.push(null);
      else
      {
        var ed = {
          id: c.myIndex,
          elemA: c.elemFrom,
          pinA: c.pinFrom,
          elemB: c.elemTo,
          pinB: c.pinTo,
          uidA: c.pinFromUid ? c.pinFromUid : undefined,
          uidB: c.pinToUid ? c.pinToUid : undefined
        };

        if (inputPinFrom)
          ed.inputPinFrom = true;

        if (inputPinTo)
          ed.inputPinTo = true;

        graph.edges.push(ed);
      }
    }

    for (var i = 0; i < extraEdges.length; i++)
    {
      var ee = extraEdges[i];
      ee.id += graph.edges.length;
      graph.edges.push(ee);
    }


    if (fullInfo && !skipPostprocress && GE_beforeStringifyGraph)
      GE_beforeStringifyGraph(graph);

    if (graph.description)
      graph.descriptionText = "/*DESCRIPTION_TEXT_BEGIN*/" + JSON.stringify(graph.description, null) + "/*DESCRIPTION_TEXT_END*/";

    return asObject ? graph : JSON.stringify(graph, null, "  ");
  }

  this._copyElem = function(fromGraphElem, newElem, keepUids)
  {
    var e = fromGraphElem;
    var typesChecked = false;
    var desc = newElem.desc;

    if (newElem.propValues.length === e.properties.length)
    {
      typesChecked = true;

      for (var j = 0; j < desc.properties.length; j++)
        if (e.properties[j].name !== desc.properties[j].name || e.properties[j].type !== desc.properties[j].type)
        {
          typesChecked = false;
          break;
        }
    }

    if (typesChecked)
    {
      for (var j = 0; j < desc.properties.length; j++)
        newElem.propValues[j] = e.properties[j].value;
    }
    else
    {
      for (var j = 0; j < desc.properties.length; j++)
        for (var k = 0; k < e.properties.length; k++)
          if (e.properties[k].name === desc.properties[j].name && e.properties[k].type === desc.properties[j].type)
            newElem.propValues[j] = e.properties[k].value;
    }

    if (keepUids)
    {
      newElem.externalUid = e.externalUid;
      newElem.uid = e.uid;
    }

    if (e.pinComments)
      newElem.pinComments = e.pinComments.slice(0);

    newElem.blockWidth = e.blockWidth ? e.blockWidth : 1
    newElem.blockHeight = e.blockHeight ? e.blockHeight : 1

    if (renderEnabled && e.blockWidth)
      newElem.update();

    newElem.updatePinCaptions();
  }

  this.parseGraph = function(graphStr, keepUids)
  {
    this.replaceWithSubgraph = undefined;
    loadedWithErrors = true;
    var anyErrors = false;

    var graph = null;
    try
    {
      graph = JSON.parse(graphStr);
    }
    catch (err)
    {
      console.log("this.parseGraph: cannot parse graphStr");
      return;
    }

    if (!graph)
      return;

    if (!isPluginMatching(graph.pluginId, pluginName))
    {
      console.log("Parsing error: Invalid plugin id: found " + graph.pluginId + ", expexted [[plugin:" + pluginName + "]]");
      return;
    }

    this.viewPosX = graph.view.posX;
    this.viewPosY = graph.view.posY;
    this.viewWidth = graph.view.width;
    this.dragging = false;
    this.moving = false;
    this.selecting = false;
    this.connecting = false;
    this.resizing = false;
    this.graphId = graph.graphId ? graph.graphId : "[[GID:0A-" + Date.now() + "]]";

    if (renderEnabled)
    {
      this.edgesSvg.innerHTML = "";
      this.elemsSvg.innerHTML = "";
    }

    this.elems = new Array(graph.elemCount);
    this.edges = new Array(graph.edgeCount);

    for (var k = 0; k < this.elems.length; k++)
      this.elems[k] = null;
    for (var k = 0; k < this.edges.length; k++)
      this.edges[k] = null;

    this.selected = graph.selected.slice(0);
    this.specialSelected = (graph.specialSelected === undefined) ? -1 : graph.specialSelected;

    if (this.specialSelected === -1)
      this.specialSelect(-1);

    // graph properties
    if (this.graphElem && Array.isArray(graph.graphElemProps) &&
        this.graphElem.propValues.length === graph.graphElemProps.length)
    {
      this.graphElem.propValues = graph.graphElemProps.slice(0);
    }
    else if (this.graphElem && graph.graphElemProps)
    {
      for (var key in graph.graphElemProps)
      {
        for (var i = 0; i < this.graphElem.desc.properties.length; i++)
          if (this.graphElem.desc.properties[i].name === key)
            this.graphElem.propValues[i] = graph.graphElemProps[key];
      }
    }

    var missedDescriptions = [];

    for (var i = 0; i < graph.elems.length; i++)
    {
      var e = graph.elems[i];
      if (e)
      {
        var desc = nodeDescriptions.findDescriptionByName(e.descName);
        if (desc)
        {
          this.addElem(e.view.x, e.view.y, desc, e.id);
          var newElem = this.elems[e.id];
          this._copyElem(e, newElem, keepUids);
        }
        else
        {
          var desc = nodeDescriptions.findDescriptionByName("<dummy>");
          if (desc)
          {
            this.addElem(e.view.x, e.view.y, desc, e.id);
            var newElem = this.elems[e.id];
            this._copyElem(e, newElem, keepUids);
            newElem.propValues[0] = e.descName;
            newElem.updatePinCaptions();
          }

          if (missedDescriptions.indexOf(e.descName) < 0)
            missedDescriptions.push(e.descName);
        }
      }
    }

    if (missedDescriptions.length > 0)
    {
      anyErrors = true;
      alert("Missed descriptions: " + missedDescriptions.join(", "));
    }

    for (var i = 0; i < graph.edges.length; i++)
    {
      var c = graph.edges[i];
      if (c && this.elems[c.elemA] && this.elems[c.elemB])
        this.addConnectionOnLoad(c.elemA, c.pinA, c.elemB, c.pinB, c.id, c.uidA, c.uidB);
    }

    this.updateConnections();
    this.hideSelectBox();
    this.applyView();
    this.propagateTypes();
    this.updateAllPinColors();
    this.onSelectionChanged();

    loadedWithErrors = anyErrors;
  }


  this.clipboardHeader = "[graph0xFA12]";

  this.copyToClipboard = function()
  {
    if (!renderEnabled)
      return;

    var e = document.getElementById("clipboardEditBox");
    e.style.visibility = "visible";
    e.value = this.clipboardHeader + this.stringifyGraph(S_SELECTED_ONLY);
    e.focus();
    e.select();
    document.execCommand("copy");
    this.onKeyUpFunction = function(){};
  }

  this._pasteInternal = function(table_or_text, custom_paste_center)
  {
    var graph = (typeof(table_or_text) === "string") ? JSON.parse(table_or_text) : table_or_text; 

    if (!graph)
      return;

    if (graph.pluginId && graph.pluginId !== "[[plugin:" + pluginName + "]]")
      return;

    this.selected = [];

    var remap = new Array(graph.elems.length);
    for (var k = 0; k < remap.length; k++)
      remap[k] = -1;

    var missedDescriptions = [];

    var xoffs = 0;
    var yoffs = 0;

    if (custom_paste_center)
    {
      var count = 0;

      for (var i = 0; i < graph.elems.length; i++)
      {
        var e = graph.elems[i];
        if (e)
        {
          xoffs += e.view.x;
          yoffs += e.view.y;
          count++;
        }
      }

      if (count > 0)
      {
        xoffs /= count;
        yoffs /= count;
        xoffs = custom_paste_center[0] - xoffs - 200;
        yoffs = custom_paste_center[1] - yoffs;
      }
    }


    for (var i = 0; i < graph.elems.length; i++)
    {
      var e = graph.elems[i];
      if (e)
      {
        var desc = nodeDescriptions.findDescriptionByName(e.descName);
        if (desc)
        {
          var idx = this.addElem(e.view.x + 100 + xoffs, e.view.y + 100 + yoffs, desc, -1);
          remap[e.id] = idx;
          this.selected.push(idx);
          var newElem = this.elems[idx];
          this._copyElem(e, newElem, false);
        }
        else
        {
          if (missedDescriptions.indexOf(e.descName) < 0)
            missedDescriptions.push(e.descName);
        }
      }
    }

    if (missedDescriptions.length > 0)
      alert("Missed descriptions: " + missedDescriptions.join(", "));

    for (var i = 0; i < graph.edges.length; i++)
    {
      var c = graph.edges[i];
      if (c)
      {
        if (c.inputPinFrom || c.inputPinTo)
        {
          this.addConnectionOnLoad(c.inputPinTo ? c.elemA : remap[c.elemA], c.pinA,
                                   c.inputPinFrom ? c.elemB : remap[c.elemB], c.pinB, -1);
        }
        else
        {
          if (remap[c.elemA] == -1 || remap[c.elemB] == -1)
            alert("ERROR: invalid remap");

          this.addConnectionOnLoad(remap[c.elemA], c.pinA, remap[c.elemB], c.pinB, -1, c.uidA, c.uidB);
        }
      }
    }
  }

  this.paste = function(asText) // if asText == null then paste from clipboard
  {
    this.replaceWithSubgraph = undefined;

    var buf = "";
    if (asText != null)
    {
      buf = asText;
    }
    else
    {
      var e = document.getElementById("clipboardEditBox");
      e.value = "";
      e.style.visibility = "visible";
      e.focus();
      document.execCommand("paste");

      this.onKeyUpFunction = function()
      {
        var e = document.getElementById('clipboardEditBox');
        editor.paste(e.value);
        editor.onChanged();
      }

      return;
    }

    if (buf.indexOf(this.clipboardHeader) != 0)
      return;

    buf = buf.slice(this.clipboardHeader.length);

    this._pasteInternal(buf, [this.viewPosX + this.viewWidth * 0.5, this.viewPosY + this.viewWidth * 0.5]);

    this.hideSelectBox();
    this.applyView();
    this.propagateTypes();
    this.updateAllPinColors();
    this.onSelectionChanged();
  }

  this.getCenterOfElems = function(graph)
  {
    var centerX = 0.0;
    var centerY = 0.0;
    var cnt = 0;

    for (var i = 0; i < graph.elems.length; i++)
    {
      var e = graph.elems[i];
      if (e)
      {
        centerX += e.view.x;
        centerY += e.view.y;
        cnt++;
      }
    }

    if (cnt > 0)
    {
      centerX /= cnt;
      centerY /= cnt;
    }

    return [centerX, centerY];
  }

  this.explodeSubgraph = function(explodeId, explodeHistory_, onfinish)
  {
    var finishCb = onfinish ? onfinish : function(){};
    var explode = this.elems[explodeId];
    if (!explode || !explode.desc || !explode.desc.generated || explode.desc.plugin !== pluginName)
    {
      finishCb(false);
      return;
    }

    var desc = explode.desc;
    var fileNames = "SAVE-DIR/" + desc.name + ".json";

    if (explodeHistory_.indexOf(desc.name) >= 0)
    {
      alert("Recursion is not allowed (for subgraph '" + desc.name + "')");
      finishCb(false);
      return;
    }

    var explodeHistory = explodeHistory_.slice(0);
    explodeHistory.push(desc.name);

    query("get_json_files&" + clientId, function(text)
      {
        if (!text || !text.length)
        {
          finishCb(false);
          return;
        }

        var parsed;

        try {
          parsed = JSON.parse('{"jsons":[' + text + ']}');
        } catch (e) {
          if (text.indexOf(">>>>>>") >= 0 && text.indexOf("======") >= 0 && text.indexOf("<<<<<<") >= 0)
            console.log("\nLOOKS LIKE JSON FILE HAS MERGE CONFLICTS\n");

          throw e;
        }


        if (!parsed.jsons || !parsed.jsons[0])
        {
          finishCb(false);
          return;
        }

        var graph = parsed.jsons[0];

        if (graph.pluginId && graph.pluginId !== "[[plugin:" + pluginName + "]]")
        {
          finishCb(false);
          return;
        }

        editor.selected = [];

        var remap = new Array(graph.elems.length);
        for (var k = 0; k < remap.length; k++)
          remap[k] = -1;


        var center = editor.getCenterOfElems(graph);
        center[0] = explode.x - center[0];
        center[1] = explode.y - center[1];

        var toExplode = [];
        var missedDescriptions = [];

        for (var i = 0; i < graph.elems.length; i++)
        {
          var e = graph.elems[i];

          if (e)
          {
            if (explodeHistory.indexOf(e.descName) >= 0)
            {
              alert("Recursion is not allowed (for subgraph '" + e.descName + "')");
              finishCb(false);
              return;
            }

            var desc = nodeDescriptions.findDescriptionByName(e.descName);
            if (desc)
            {
              if (desc.inOutMarker)
              {
                remap[e.id] = -2;
              }
              else
              {
                var idx = editor.addElem(e.view.x + center[0], e.view.y + center[1], desc, -1);
                remap[e.id] = idx;
                editor.selected.push(idx);
                var newElem = editor.elems[idx];
                editor._copyElem(e, newElem, true);

                var nextElem = editor.elems[idx];
                if (nextElem && nextElem.desc && nextElem.desc.generated && nextElem.desc.plugin === pluginName)
                  toExplode.push(idx);
              }
            }
            else
            {
              if (missedDescriptions.indexOf(e.descName) < 0)
                missedDescriptions.push(e.descName);
            }
          }
        }

        if (missedDescriptions.length > 0)
        {
          alert("Missed descriptions: " + missedDescriptions.join(", "));
          finishCb(false);
          return;
        }

        for (var i = 0; i < graph.edges.length; i++)
        {
          var c = graph.edges[i];
          if (c)
          {
            if (remap[c.elemA] === -1 || remap[c.elemB] === -1)
              alert("ERROR: invalid remap");

            if (remap[c.elemA] >= 0 && remap[c.elemB] >= 0)
              editor.addConnectionOnLoad(remap[c.elemA], c.pinA, remap[c.elemB], c.pinB, -1, c.uidA, c.uidB);
          }
        }


        for (var i = 0; i < editor.edges.length; i++)
        {
          var c = editor.edges[i];
          if (!c)
            continue;

          if (c.elemFrom === explodeId || c.elemTo === explodeId)
          {
            var pinIdx = editor.getConnectionPin(i, explodeId);
            var descPin = explode.desc.pins[pinIdx];
            if (descPin.subgraphElem !== undefined)
            {
              var opp = editor.getOppositeConnection(i, explodeId);
              editor.addConnectionOnLoad(opp.elem, opp.pin, remap[descPin.subgraphElem], descPin.subgraphPin, -1,
                null, descPin.subgraphPinUid);
            }
            else
            {
              if (descPin.subgraphPins)
              {
                var opp = editor.getOppositeConnection(i, explodeId);

                for (var j = 0; j < descPin.subgraphPins.length; j++)
                {
                  var p = descPin.subgraphPins[j];
                  editor.addConnection(opp.elem, opp.pin, remap[p.elem], p.pin, -1, null, p.uid);
                }
              }
            }

            c.kill();
          }
        }

        if (editor.specialSelected === explodeId)
        {
          for (var i = 0; i < explode.desc.pins.length; i++)
          {
            var descPin = explode.desc.pins[i];
            if (descPin.role === "out" && descPin.subgraphElem != undefined)
            {
              editor.specialSelected = remap[descPin.subgraphElem];
              break;
            }
          }
        }

        editor.killHiddenConnections();
        explode.kill();

        editor.propagateTypes();
        editor.updateAllPinColors();

        if (toExplode.length === 0)
          finishCb(true);
        else
          for (var i = 0; i < toExplode.length; i++)
            editor.explodeSubgraph(toExplode[i], explodeHistory, onfinish);
      },
      fileNames);
  }

  this.duplicateSelected = function()
  {
    var buf = this.clipboardHeader + this.stringifyGraph(S_SELECTED_ONLY | S_COPY_INPUT_EDGES);
    this.paste(buf);
  }


  this._descriptionNames = [];
  this.saveDescriptionNames = function()
  {
    for (var i = 0; i < this.elems.length; i++)
      this._descriptionNames[i] = this.elems[i] ? this.elems[i].desc.name : null;
  }

  this.restoreDescriptions = function()
  {
    for (var i = 0; i < this.elems.length; i++)
      if (this.elems[i] && this.saveDescriptionNames[i])
      {
        var desc = nodeDescriptions.findDescriptionByName(subgraphName);
        if (desc)
          this.elems[i].desc = desc;
      }
  }


  this.historyList = [];
  this.historyIndex = -1;

  this.resetHistory = function()
  {
    queryFileCache = {};
    this.lastCode = null;
    this.lastDesc = null;
    this.historyList = [];
    this.historyIndex = -1;
    this.saveHistoryPoint();
  }

  this.saveHistoryPoint = function()
  {
    var buf = this.stringifyGraph(0);
    if (this.historyIndex >= 0 && this.historyList[this.historyIndex] === buf)
      return false;

    this.historyIndex++;
    this.historyList.length = this.historyIndex + 1;
    this.historyList[this.historyIndex] = buf;
    if (this.historyIndex >= 100)
      this.historyList[this.historyIndex - 100] = null;

    return true;
  }

  this.undo = function()
  {
    if (this.historyIndex > 0 && this.historyList[this.historyIndex])
    {
      this.historyIndex--;
      this.parseGraph(this.historyList[this.historyIndex], true);
    }
  }

  this.redo = function()
  {
    if (this.historyIndex + 1 < this.historyList.length && this.historyList[this.historyIndex + 1])
    {
      this.historyIndex++;
      this.parseGraph(this.historyList[this.historyIndex], true);
    }
  }

  this.onCommand = function(cmd)
  {
    if (cmd.indexOf("id:") == 0)
    {
      if (cmd != clientId)
        onDetach();
    }
    else if (cmd.indexOf("error:") == 0)
    {
      lastErrorString = cmd;
      if (lastErrorString.length > 160)
        lastErrorString = lastErrorString.substring(0, 157) + "...";
    }
    else if (cmd.indexOf("message:") == 0)
    {
      lastErrorString = cmd.slice("message:".length);
      if (lastErrorString.length > 160)
        lastErrorString = lastErrorString.substring(0, 157) + "...";
    }
    else if (cmd.indexOf("compiled") == 0)
      lastErrorString = "";
    else if (cmd.indexOf("touch") == 0)
      editor.onChanged(true, true);
    else if (cmd.indexOf("filename:") == 0)
    {
      var s = cmd.slice("filename:".length).split("/");
      if (s.length == 1)
        s = s[0].split("\\");

      if (renderEnabled)
        this.statusFileName.innerText = s.join(" / ");
    }
    else if (cmd.indexOf("replace_with_subgraph") == 0)
    {
      if (this.replaceWithSubgraph)
      {
        this.replaceSelectedWithSubgraph(this.replaceWithSubgraph);
        this.replaceWithSubgraph = undefined;
      }
    }
  }

  this.parseAdditionalIncludes = function(text)
  {
    this.additionalIncludes = []; // array of strings that can be used as clipboard content
    if (!text || !text.length)
      return;

    var parsed;

    try {
      parsed = JSON.parse('{"jsons":[' + text + ']}');
    } catch (e) {
      if (text.indexOf(">>>>>>") >= 0 && text.indexOf("======") >= 0 && text.indexOf("<<<<<<") >= 0)
        console.log("\nLOOKS LIKE JSON FILE HAS MERGE CONFLICTS\n");

      throw e;
    }

    if (!parsed || !parsed.jsons || !parsed.jsons[0])
    {
      alert("ERROR: Parse Additional Includes: cannot parse JSON");
      return;
    }

    this.additionalIncludes = parsed.jsons;
  }

  this.makeUniqIdentifiers = function()
  {
    var identifiersList = [];
    var curIdentifierSuffix = 1;

    function makeUniqueIdentifier(ident)
    {
      if (ident.length == 0)
        ident = "n";

      ident = ident.split(" ").join("_");
      ident = ident.split("-").join("_");
      ident = ident.split(".").join("_");
      ident = ident.split(",").join("_");

      if (identifiersList.indexOf(ident) < 0)
      {
        identifiersList.push(ident);
        return ident;
      }

      var clip = false;
      var i = ident.indexOf("__");
      if (i > 0 && i < ident.length - 1)
      {
        clip = true;
        for (var j = i + 2; j < ident.length; j++)
        {
          var code = ident.charCodeAt(j);
          if (code < 48 || code > 57)
          {
            clip = false;
            break;
          }
        }
      }

      if (clip)
        ident = ident.slice(0, i);

      for (; curIdentifierSuffix < 99999; curIdentifierSuffix++)
      {
        var extendedIdent = ident + "__" + curIdentifierSuffix;
        if (identifiersList.indexOf(extendedIdent) < 0)
        {
          identifiersList.push(extendedIdent);
          return extendedIdent;
        }
      }

      return ident; // error
    }

    for (var i = 0; i < this.elems.length; i++)
    {
      var e = this.elems[i];
      if (e)
      {
        var propDesc = e.desc.properties;
        for (var j = 0; j < propDesc.length; j++)
          if (propDesc[j].unique)
          {
            var newValue = makeUniqueIdentifier(e.propValues[j]);
            if (newValue !== e.propValues[j])
            {
              e.propValues[j] = newValue;
              e.updatePinCaptions();
            }
          }
      }
    }
  }


  this.countElems = function()
  {
    var cnt = 0;
    for (var i = 0; i < this.elems.length; i++)
      if (this.elems[i])
        cnt++;

    return cnt;
  }


  this.onChanged = function(undoOrRedo, forceChanged, inessential)
  {
    this.makeUniqIdentifiers();

    var changed = true;
    if (!undoOrRedo)
      changed = this.saveHistoryPoint();

    if (changed || forceChanged)
    {
      if (this.changedTimerId)
        clearTimeout(this.changedTimerId);

      this.changedTimerId = setTimeout(function()
        {
          editor.changedTimerId = null;

          if (globalLockInput)
            return;

          if (loadedWithErrors)
            return;

          editor.updateAllPinColors();

          if (inessential)
          {
            var obj = editor.stringifyGraph(S_FULL_INFO | S_RETURN_AS_OBJECT | S_INOUT_PINS);
            obj.code = editor.lastCode;
            if (editor.lastDesc)
              obj.description = editor.lastDesc;
            var buf = JSON.stringify(obj, null, "  ");
            buf = buf.replace("[[generated_graph_hint]]", "/*__INESSENTIAL_CHANGE*/");
            query("graph&" + clientId, null, buf);
          }
          else
          {
            globalLockGraphics = true;
            globalLockChange = true;

            if (renderEnabled)
              editor.backgroundDiv.style.cursor = 'wait';

            var obj = editor.stringifyGraph(S_FULL_INFO | S_RETURN_AS_OBJECT | S_INOUT_PINS);
            editor.explodeAllSubgraphs(0, function (ok)
              {
                if (ok)
                {
                  editor.removeAllBypassNodes();
                  editor.keepOnlySingleExternals_();
                }

                function rollback()
                {
                  if (editor.historyIndex >= 0 && editor.historyList[editor.historyIndex])
                    editor.parseGraph(editor.historyList[editor.historyIndex], true);
                }

                if (!ok)
                {
                  globalLockGraphics = false;
                  rollback();
                  if (renderEnabled)
                    editor.backgroundDiv.style.cursor = 'default';
                  globalLockChange = false;
                  return;
                }


                var objWithCode = editor.stringifyGraph(S_FULL_INFO | S_RETURN_AS_OBJECT | S_INOUT_PINS);
                obj.code = objWithCode.code;
                editor.lastCode = obj.code;
                editor.lastDesc = JSON.parse(JSON.stringify(obj.description));
                var buf = JSON.stringify(obj, null, "  ");

                globalLockGraphics = false;

                rollback();

                globalLockChange = false;
                if (renderEnabled)
                  editor.backgroundDiv.style.cursor = 'default';

                query("graph&" + clientId, null, buf);
              }
            );

          }

        }, 10);
    }

    if (renderEnabled)
      this.statusElemCount.innerText = this.countElems();
  }
}

