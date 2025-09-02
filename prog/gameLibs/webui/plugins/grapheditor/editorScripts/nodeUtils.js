"use strict";

var randomFilterName = "elemFilterEdit" + Date.now();


function TypeConv()
{
  this.conversions = GE_conversions;

  this.implicitGroupConversionOrder = GE_implicitGroupConversionOrder;

  this.keepTypeOrConvert = function(type, typeList)
  {
    if (typeList.indexOf(type) >= 0)
      return type;

    var typeIdx = this.implicitGroupConversionOrder.indexOf(type);
    if (typeIdx < 0)
      return type;  // error

    var bestTypeIndex = this.implicitGroupConversionOrder.length;
    for (var i = 0; i < typeList.length; i++)
    {
      var index = this.implicitGroupConversionOrder.indexOf(typeList[i]);
      if (index < bestTypeIndex && index > typeIdx)
        bestTypeIndex = index;
    }

    if (bestTypeIndex < this.implicitGroupConversionOrder.length)
      return this.implicitGroupConversionOrder[bestTypeIndex];

    bestTypeIndex = -1;
    for (var i = 0; i < typeList.length; i++)
    {
      var index = this.implicitGroupConversionOrder.indexOf(typeList[i]);
      if (index > bestTypeIndex && index < typeIdx)
        bestTypeIndex = index;
    }

    if (bestTypeIndex >= 0)
      return this.implicitGroupConversionOrder[bestTypeIndex];

    return type; // error
  }

  this.getLargestType = function(typeList)
  {
    if (typeList.length == 0)
      return "error";
    var bestTypeIndex = -1;
    for (var i = 0; i < typeList.length; i++)
    {
      var index = this.implicitGroupConversionOrder.indexOf(typeList[i]);
      if (index > bestTypeIndex)
        bestTypeIndex = index;
    }

    return this.implicitGroupConversionOrder[bestTypeIndex];
  }

  this.isPossibleConversion = function(typeFrom, typeTo)
  {
    if (typeFrom === typeTo)
      return true;

    for (var i = 0; i < this.conversions.length; i++)
      if (typeFrom === this.conversions[i][0] && typeTo === this.conversions[i][1])
        return true;

    return false;
  }

  this.removeInconvertableTypes = function(constTypes, inOutTypes) // conversion: const -> inout
  {
    var changed = false;
    for (var i = inOutTypes.length - 1; i >= 0; i--)
    {
      if (constTypes.indexOf(inOutTypes[i]) >= 0)
        continue;

      var convertable = false;
      for (var j = 0; j < constTypes.length; j++)
      {
        if (this.isPossibleConversion(constTypes[j], inOutTypes[i]))
        {
          convertable = true;
          break;
        }
      }

      if (!convertable)
      {
        inOutTypes.splice(i, 1);
        changed = true;
      }
    }

    return changed;
  }

  this.removeNonEqualTypes = function(constTypes, inOutTypes) // conversion: const -> inout
  {
    var changed = false;
    for (var i = inOutTypes.length - 1; i >= 0; i--)
    {
      if (constTypes.indexOf(inOutTypes[i]) >= 0)
        continue;

      inOutTypes.splice(i, 1);
      changed = true;
    }

    return changed;
  }
}

function NodeDescriptions()
{
  this.descriptions = GE_nodeDescriptions;

  this.colors = {};

  try
  {
    if (GE_preprocessDescription)
      for (var i = 0; i < this.descriptions.length; i++)
        GE_preprocessDescription(this.descriptions[i]);
  }
  catch (err) {}

  try
  {
    if (GE_categoryColors)
      this.colors = GE_categoryColors;
  }
  catch (err) {}
  

  this.elemFilterDiv = document.getElementById("elemFilterDiv");
  this.elemFilterItems = document.getElementById("elemFilterItems");
  this.elemFilterEdit = document.getElementById("elemFilterEdit");
  this.selectedIndex = -1;
  this.selectedName = "";
  this.prevValue = "";
  if (this.elemFilterEdit)
  {
    this.elemFilterEdit.id = randomFilterName
    this.elemFilterEdit.autocomplete = "off"
    this.elemFilterDiv.onclick = function() { document.getElementById(randomFilterName).focus(); }
  }
  
  this.update = function()
  {
    var catList = [];
    for (var i = 0; i < this.descriptions.length; i++)
    {
      var desc = this.descriptions[i];
      if (desc.hidden)
        continue;
      if (!desc.category || desc.category === "")
        desc.category = "-Unknown-";
      if (catList.indexOf(desc.category) < 0)
        catList.push(desc.category);
    }

    var catHtml = "";
    var elemHtml = "";

    for (var i = 0; i < catList.length; i++)
    {
      var s = "<div class='listClickableItem' id='" + catList[i] + "' onclick='doScrollToCategory(this.id)'>" + catList[i] + "</div>";
      catHtml += s;
      s = "<div id='cat:" + catList[i] + "' style='background:#777;'><center>" + catList[i] + "</center></div>";
      elemHtml += s;
      for (var j = 0; j < this.descriptions.length; j++)
      {
        var desc = this.descriptions[j];
        if (desc.hidden)
          continue;
        if (desc.category == catList[i])
        {
          var color = this.colors[desc.category];
          var style = "max-width:100%; overflow:hidden;";
          if (color)
            style += "color:" + color + ";";

          s = "<div id='" + desc.name + "' data-name='" + desc.name + "' draggable='true' ondragstart='doStartDragElem(event)'" +
            " style='" + style + "'>" +
            desc.name + "</div>";

          elemHtml += s;
        }
      }
    }

    var categories = document.getElementById("categoriesDiv");
    var elements = document.getElementById("elementsDiv");
    if (categories)
    {
      categories.innerHTML = catHtml;
      elements.innerHTML = elemHtml;
    }
  }

  this.findDescriptionByName = function(name)
  {
    for (var i = 0; i < this.descriptions.length; i++)
      if (this.descriptions[i].name == name)
        return this.descriptions[i];
    return null;
  }

  this.descriptionWithNameExistsGlobally = function(name)
  {
    name = name.toLowerCase();
    for (var i = 0; i < this.descriptions.length; i++)
    {
      if (this.descriptions[i].name.toLowerCase() == name)
        return true;
    }
    return false;
  }

  this.findByPartialName = function(partialName) // {names:[], categories:[]}  find in synonyms too
  {
    partialName = partialName.split(" ").join("");
    if (partialName.length === 0)
      return {names:[], categories:[]};

    var res = [];
    var cats = [];

    var listed = new Array(this.descriptions.length).fill(false);

    for (var i = 0; i < this.descriptions.length; i++)
    {
      if (!this.descriptions[i].synCache)
      {
        if (this.descriptions[i].hidden)
        {
          this.descriptions[i].synCache = "";
          continue;
        }

        var synonyms = this.descriptions[i].synonyms;
        var synList = synonyms ? synonyms.toLowerCase().split(" ").join("").split(",") : [];
        var nameList = [this.descriptions[i].name.toLowerCase()];
        nameList = nameList.concat(this.descriptions[i].name.toLowerCase().split("_").join(" ").split(" "));
        this.descriptions[i].synCache = "//" + nameList.join("//") + "./" + synList.join("./");
      }
    }

    var toFind = "//" + partialName.toLowerCase();

    for (var i = 0; i < this.descriptions.length; i++)
    {
      if (this.descriptions[i].synCache.indexOf(toFind) >= 0)
      {
        if (this.descriptions[i].hidden)
          continue;

        res.push(this.descriptions[i].name);
        cats.push(this.descriptions[i].category);
        listed[i] = true;
      }
    }

    var toFind = "./" + partialName.toLowerCase();

    for (var i = 0; i < this.descriptions.length; i++)
    {
      if (!listed[i] && this.descriptions[i].synCache.indexOf(toFind) >= 0)
      {
        if (this.descriptions[i].hidden)
          continue;

        res.push(this.descriptions[i].name);
        cats.push(this.descriptions[i].category);
        listed[i] = true;
      }
    }

    return {names:res, categories:cats};
  }

  this.showFilterDialog = function()
  {
    this.elemFilterDiv.style.visibility = "visible";
    this.elemFilterEdit.value = "";
    this.elemFilterItems.innerHTML = "Search<sup><big><big><big>&#10548</big></big></big></sup>";
    this.selectedName = "";
    this.prevValue = "";
    this.selectedIndex = -1;

    setTimeout(function()
      {
        nodeDescriptions.elemFilterEdit.focus();
      }, 10);
  }

  this.hideFilterDialog = function()
  {
    if (this.elemFilterDiv.style.visibility === "hidden")
      return;

    this.elemFilterDiv.style.visibility = "hidden";

    setTimeout(function()
      {
        if (document.activeElement)
          document.activeElement.blur();
        window.focus();
      }, 10);

  }

  this.onFilterKeyPress = function(event)
  {
    if (this.elemFilterEdit.value.trim() === "")
    {
      this.elemFilterEdit.value = "";
      return;
    }

    if (this.prevValue === this.elemFilterEdit.value)
      return;

    this.prevValue = this.elemFilterEdit.value;

    var found = this.findByPartialName(this.elemFilterEdit.value);
    var names = found.names;
    var categories = found.categories;
    this.selectedIndex = 0;//Math.max(names.indexOf(this.selectedName), 0);
    this.selectedName = "";
    var html = "";

    for (var i = 0; i < names.length; i++)
    {
      var color = this.colors[categories[i]];
      var cl = "";
      if (this.selectedIndex === i)
      {
        cl = "class = 'selectedFilteredElem'";
        this.selectedName = names[i];
      }

      var style = "max-width:100%; overflow:hidden;";
      if (color)
        style += "color:" + color + ";";

      html += "<div id='elemFilterItems." + i + "' " + cl + "style='" + style + "'" +
        " data-name='" + names[i] + "' draggable='true' ondragstart='doStartDragElem(event); document.getElementById(\"" + randomFilterName + "\").focus();'" +
        " onclick='globalOnFilterClick(event," + i + ")'" + ">" +
        names[i].replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") + "</div>";
    }

    this.elemFilterItems.innerHTML = html;
  }

  this.onFilterClick = function(event, index)
  {
    var prevSelElem = document.getElementById("elemFilterItems." + this.selectedIndex);
    this.selectedIndex = index;
    var selElem = document.getElementById("elemFilterItems." + this.selectedIndex);
    if (selElem)
    {
      if (prevSelElem)
        prevSelElem.className = "";

      selElem.className = "selectedFilteredElem";
      this.selectedName = selElem.innerHTML;
    }

    event.preventDefault();
    return;
  }

  this.onFilterKeyDown = function(event)
  {
    if (event.keyCode === 38) // up
    {
      var prevSelElem = document.getElementById("elemFilterItems." + this.selectedIndex);
      this.selectedIndex--;
      var selElem = document.getElementById("elemFilterItems." + this.selectedIndex);
      if (selElem)
      {
        if (prevSelElem)
          prevSelElem.className = "";

        selElem.className = "selectedFilteredElem";
        this.selectedName = selElem.innerHTML;
      }
      else
        this.selectedIndex++;

      event.preventDefault();
      return;
    }
    else if (event.keyCode === 40) // down
    {
      var prevSelElem = document.getElementById("elemFilterItems." + this.selectedIndex);
      this.selectedIndex++;
      var selElem = document.getElementById("elemFilterItems." + this.selectedIndex);
      if (selElem)
      {
        if (prevSelElem)
          prevSelElem.className = "";

        selElem.className = "selectedFilteredElem";
        this.selectedName = selElem.innerHTML;
      }
      else
        this.selectedIndex--;

      event.preventDefault();
      return;
    }
    else if (event.keyCode === 27) // esc
    {
      event.preventDefault();
      this.hideFilterDialog();
      return;
    }
    else if (event.keyCode === 13) // enter
    {
      if (this.selectedName !== "")
      {
        this.hideFilterDialog();

        var name = this.selectedName.replace(/&amp;/g, "&").replace(/&lt;/g, "<").replace(/&gt;/g, ">");
        var desc = this.findDescriptionByName(name);
        if (desc)
        {
          var mx = editor.lastMouseX;
          var my = editor.lastMouseY;
          var autoEdgeFromElem = -1;
          var autoEdgeFromPin = -1;
          var autoEdgeToPin = -1;

          var curEdgeIdx = -1;

          // add after selected elem
          if (editor.selected.length === 1)
          {
            var se = editor.elems[editor.selected[0]];
            if (se)
            {
              for (var i = 0; i < se.desc.pins.length; i++)
                if (se.desc.pins[i].role === "out" && !se.desc.pins[i].hidden)
                {
                  mx = se.x + se.desc.width + 60;
                  my = se.y;
                  autoEdgeFromElem = editor.selected[0];
                  autoEdgeFromPin = i;
                  break;
                }
            }
          }

          var idx = editor.addElem(mx, my, desc, -1);

          if (autoEdgeFromElem >= 0)
          {
            for (var i = 0; i < desc.pins.length; i++)
              if (desc.pins[i].main && desc.pins[i].role === "in" &&
                  editor.isValidConnection(autoEdgeFromElem, autoEdgeFromPin, idx, i))
              {
                autoEdgeToPin = i;
                break;
              }

            if (autoEdgeToPin < 0)
              for (var i = 0; i < desc.pins.length; i++)
                if (desc.pins[i].role === "in" && editor.isValidConnection(autoEdgeFromElem, autoEdgeFromPin, idx, i))
                {
                  autoEdgeToPin = i;
                  break;
                }

            if (autoEdgeToPin >= 0)
            {
              var edges = editor.findConnections(autoEdgeFromElem, autoEdgeFromPin);
              for (var j = 0; j < edges.length; j++)
              {
                var nextPin = editor.getOppositeConnection(edges[j], autoEdgeFromElem);

                if (nextPin)
                {
                  for (var i = 0; i < desc.pins.length; i++)
                    if (desc.pins[i].role === "out" && editor.isValidConnection(nextPin.elem, nextPin.pin, idx, i))
                    {
                      editor.edges[edges[j]].kill();
                      editor.addConnection(nextPin.elem, nextPin.pin, idx, i, -1, null, null);
                      break;
                    }
                }
              }

              editor.addConnection(autoEdgeFromElem, autoEdgeFromPin, idx, autoEdgeToPin, -1, null, null);

            }
          }

          editor.selected = [idx];
          editor.onSelectionChanged();


         // if (editor.checkForExplodeSubgraph(idx) == false)
          editor.onChanged();
        }
      }
      event.preventDefault();
      return;
    }
  }

  this.isDescriptionExists = function(name)
  {
    for (var i = 0; i < this.descriptions.length; i++)
    {
      var desc = this.descriptions[i];
      if (name === desc.name)
        return true;
    }

    return false;
  }


  this.changeOrAddDescription = function(descJson, isRemove)
  {
    if (!this.skipPreprocessDescription)
    {
      try
      {
        GE_preprocessDescription(descJson);
      }
      catch (err)
      {
        this.skipPreprocessDescription = true;
      }
    }

    var idx = -1;
    for (var i = 0; i < this.descriptions.length; i++)
    {
      var desc = this.descriptions[i];
      if (descJson.uid && desc.uid && descJson.uid === desc.uid)
      {
        idx = i;
        break;
      }
    }

    if (idx === -1)
      for (var i = 0; i < this.descriptions.length; i++)
      {
        var desc = this.descriptions[i];
        if (descJson.name === desc.name)
        {
          idx = i;
          break;
        }
      }

    if (idx === -1)
    {
      if (!isRemove)
        this.descriptions.push(descJson);

      this.update();
      return true;
    }
    else
    {
      if (isRemove)
        this.descriptions.splice(idx, 1);
      else
        this.descriptions[idx] = descJson;

      editor.updateConnections();
      this.update();
      return true;
    }

    return false;
  }

  this.replaceSubgraphDescriptions = function(category, descListStr)
  {
    var listJson = null;
    var list = null;
    try
    {
      listJson = JSON.parse(descListStr);
      list = listJson.descriptions;
    }
    catch(err)
    {
      return;
    }

    for (var i = 0; i < list.length; i++)
    {
      var desc = list[i];
      desc._in_list = true;
      this.changeOrAddDescription(desc, false);
      //TODO: rename elements in graph
    }

    for (var i = this.descriptions.length - 1; i >= 0; i--)
    {
      var desc = this.descriptions[i];
      if (desc.category === category)
      {
        if (!desc._in_list)
        {
          //TODO: remove elements from graph
          this.descriptions.splice(i, 1);
        }
        else
          desc._in_list = undefined;
      }
    }

    editor.updateConnections();
    this.update();
  }
}

function OpenGraphDialog()
{
  this.openGraphDiv = document.getElementById("openGraphDiv");
  this.openGraphItems = document.getElementById("openGraphItems");
  this.openGraphEdit = document.getElementById("openGraphEdit");
  this.openGraphEdit.value = "";
  this.fileList = [];
  this.sfFileList = []; // search-friendly strings
  this.selectedIndex = 0;
  this.selectedName = "";
  this.openGraphItems.innerHTML = "loading...";

  this.showDialog = function()
  {
    this.openGraphDiv.style.visibility = "visible";

    setTimeout(function()
      {
        openGraphDialog.openGraphEdit.focus();
      }, 10);
  }

  this.hideDialog = function()
  {
    if (this.openGraphDiv.style.visibility === "hidden")
      return;

    this.openGraphDiv.style.visibility = "hidden";

    setTimeout(function()
      {
        if (document.activeElement)
          document.activeElement.blur();
        window.focus();
      }, 10);
  }

  this.setFileList = function(str)
  {
    this.fileList = str.split("%").filter(Boolean);

    this.fileList.sort(function (a, b) {
        a = a.toLowerCase();
        b = b.toLowerCase();
        if (a.indexOf("root:") === 0)
          a = '  ' + a;
        if (a.indexOf("incl:") === 0)
          a = ' ' + a;
        if (a.indexOf("root:") === 0)
          b = '  ' + b;
        if (b.indexOf("incl:") === 0)
          b = ' ' + b;
        return a > b ? 1 : a === b ? 0 : -1;
      });

    this.sfFileList = [];
    var code_a = "a".charCodeAt(0);
    var code_z = "z".charCodeAt(0);
    var code_A = "A".charCodeAt(0);
    var code_Z = "Z".charCodeAt(0);
    for (var i = 0; i < this.fileList.length; i++)
    {
      var s = this.fileList[i];
      var c = this.fileList[i];
      s = s.toLowerCase().split("-").join(" ").split("_").join(" ").split(".").join(" ");
      for (var j = s.length - 1; j >= 0; j--)
        if (c.charCodeAt(j + 1) >= code_A && c.charCodeAt(j + 1) <= code_Z &&
            c.charCodeAt(j) >= code_a && c.charCodeAt(j) <= code_z)
        {
          s = s.slice(0, j + 1) + " " + s.slice(j + 1);
        }

      s = this.fileList[i].toLowerCase() + "// " + s;
      this.sfFileList.push(s);
    }

    this.updateList();
  }

  this.updateList = function()
  {
    var html = "";
    var filter = this.openGraphEdit.value.trim().toLowerCase();
    var wordFilter = " " + filter;
    var names = [];

    var listed = new Array(this.fileList.length).fill(false);

    for (var i = 0; i < this.fileList.length; i++)
    {
      if (filter === "" || this.sfFileList[i].indexOf(filter) == 0)
      {
        listed[i] = true;
        names.push(this.fileList[i]);
      }
    }

    for (var i = 0; i < this.fileList.length; i++)
      if (!listed[i])
      {
        if (this.sfFileList[i].indexOf(wordFilter) >= 0)
        {
          listed[i] = true;
          names.push(this.fileList[i]);
        }
      }

    for (var i = 0; i < this.fileList.length; i++)
      if (!listed[i])
      {
        if (this.sfFileList[i].indexOf(filter) >= 0)
        {
          listed[i] = true;
          names.push(this.fileList[i]);
        }
      }

    var filterWords = filter.split(" ");
    if (filterWords.length > 1)
    {
      for (var i = 0; i < this.fileList.length; i++)
        if (!listed[i])
        {
          var allPresent = true;
          for (var j = 0; j < filterWords.length; j++)
            if (filterWords[j].length > 0 && this.sfFileList[i].indexOf(filterWords[j]) < 0)
              allPresent = false;

          if (allPresent)
          {
            listed[i] = true;
            names.push(this.fileList[i]);
          }
        }
    }


    for (var i = 0; i < names.length; i++)
    {
      var cl = "";
      if (this.selectedIndex === i)
      {
        cl = "class = 'selectedFilteredElem'";
        this.selectedName = names[i];
      }
      html += "<div id='openFileItems." + i + "' " + cl +
        "data-name='" + names[i] + "'" +
        " onclick='globalOnOpenClick(event," + i + ")'" +
        " ondblclick='globalOnOpenDblClick(event)'" +
        ">" +
        names[i].replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") + "</div>";
    }

    if (html === "")
      html = "--- empty ---";

    this.openGraphItems.innerHTML = html;
  }



  this.onFilterKeyPress = function(event)
  {
    if (this.openGraphEdit.value.trim() === "")
      this.openGraphEdit.value = "";

    if (this.prevValue === this.openGraphEdit.value)
      return;

    this.prevValue = this.openGraphEdit.value;

    this.selectedIndex = 0;
    this.selectedName = "";
    this.updateList();
  }

  this.onFilterClick = function(event, index)
  {
    var prevSelElem = document.getElementById("openFileItems." + this.selectedIndex);
    this.selectedIndex = index;
    var selElem = document.getElementById("openFileItems." + this.selectedIndex);
    if (selElem)
    {
      if (prevSelElem)
        prevSelElem.className = "";

      selElem.className = "selectedFilteredElem";
      this.selectedName = selElem.innerHTML;
    }

    event.preventDefault();
    return;
  }

  this.onFilterKeyDown = function(event)
  {
    var ctrlO = (event.keyCode == 79 && event.ctrlKey && !event.shiftKey);
    if (ctrlO)
      event.preventDefault();

    if (event.keyCode === 38) // up
    {
      var prevSelElem = document.getElementById("openFileItems." + this.selectedIndex);
      this.selectedIndex--;
      var selElem = document.getElementById("openFileItems." + this.selectedIndex);
      if (selElem)
      {
        if (prevSelElem)
          prevSelElem.className = "";

        selElem.className = "selectedFilteredElem";
        this.selectedName = selElem.innerHTML;
        selElem.scrollIntoView({ block: 'nearest', inline: 'nearest' });
      }
      else
        this.selectedIndex++;

      event.preventDefault();
      return;
    }
    else if (event.keyCode === 40) // down
    {
      var prevSelElem = document.getElementById("openFileItems." + this.selectedIndex);
      this.selectedIndex++;
      var selElem = document.getElementById("openFileItems." + this.selectedIndex);
      if (selElem)
      {
        if (prevSelElem)
          prevSelElem.className = "";

        selElem.className = "selectedFilteredElem";
        this.selectedName = selElem.innerHTML;
        selElem.scrollIntoView({ block: 'nearest', inline: 'nearest' });
      }
      else
        this.selectedIndex--;

      event.preventDefault();
      return;
    }
    else if (event.keyCode === 27) // esc
    {
      event.preventDefault();
      this.hideDialog();
      return;
    }
    else if (event.keyCode === 13) // enter
    {
      if (this.selectedName !== "")
      {
        this.hideDialog();

        var name = this.selectedName;
        if (name && name.length > 0)
          query("command&" + clientId + "&open:" + name, null);
      }
      event.preventDefault();
      return;
    }
  }

  this.updateList();
}


function globalOnFilterKeyPress(event)
{
  nodeDescriptions.onFilterKeyPress(event);
}

function globalOnFilterKeyDown(event)
{
  nodeDescriptions.onFilterKeyDown(event);
}

function globalOnFilterClick(event, index)
{
  nodeDescriptions.onFilterClick(event, index);
}


function globalOnOpenKeyPress(event)
{
  openGraphDialog.onFilterKeyPress(event);
}

function globalOnOpenKeyDown(event)
{
  openGraphDialog.onFilterKeyDown(event);
}

function globalOnOpenClick(event, index)
{
  openGraphDialog.onFilterClick(event, index);
}

function globalOnOpenDblClick(event)
{
  event.keyCode = 13; // enter
  openGraphDialog.onFilterKeyDown(event);
}


