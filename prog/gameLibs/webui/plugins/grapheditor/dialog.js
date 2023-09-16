/*
example:

<script src="dialog.js"></script>

<div id="place_for_dialogs"></div>

<script>
init_dialog("place_for_dialogs");

//...............................

show_dialog("Dialog message", null, [
   ["OK", function() {
     console.log("ok")
   }],
   ["Cancel", function() {
     console.log("cancel")
   }]
   ]);

show_dialog(null, "<div>HTML Dialog Message</div><div>Second line</div>", [
   ["OK", function(){
     console.log("ok")
   }]
   ]);
</script>

*/

var __button_callbacks = {};
var __dilaog_queue = [];

function __stop_event_propagation(e)
{
  if (!e)
    e = window.event;

  if (e.stopPropagation)
    e.stopPropagation();
  else
    e.cancelBubble = true;
}

function __close_dialog()
{
  var elemDialog = document.getElementById("fullscreen_dialog_div");
  elemDialog.style.visibility = 'hidden';

  if (__dilaog_queue.length > 0)
  {
    show_dialog(__dilaog_queue[0].innerText, __dilaog_queue[0].innerHTML, __dilaog_queue[0].buttons);
    __dilaog_queue.splice(0, 1);
  }
}

///////////////////////////


function init_dialog(element_id)
{
  var fs = document.getElementById("fullscreen_dialog_div");
  if (fs)
  {
    alert("Dialog is already inited");
    return;
  }

  var dialogHTML =
    '<div id="fullscreen_dialog_div" style="box-sizing:border-box; position:absolute; top:0%; left:0%; width:100%; height:100%; visibility:hidden; background-color:rgba(0, 0, 0, 0.7);">'+
    '  <div style="position: fixed; top: 50%; left: 50%; -webkit-transform: translate(-50%, -50%); transform: translate(-50%, -50%); border: 20px solid white; background-color:white;">'+
    '    <div id="fullscreen_dialog_text"></div>'+
    '    <br><br>'+
    '    <center><div id="fullscreen_dialog_buttons"></div></center>'+
    '  </div>'+
    '</div>';

  var elem = document.getElementById(element_id);
  if (!elem)
    alert("Cannot init dialogs, DIV with id='" + element_id + "' not found");
  elem.innerHTML = dialogHTML;
}


function show_dialog(innerText, innerHTML, buttons)
{
  var elemDialog = document.getElementById("fullscreen_dialog_div");

  if (elemDialog.style.visibility === "visible")
  {
    __dilaog_queue.push(
     {
       innerText: innerText,
       innerHTML: innerHTML,
       buttons: buttons,
     });
    return;
  }

  var elemText = document.getElementById("fullscreen_dialog_text");
  var elemButtons = document.getElementById("fullscreen_dialog_buttons");

  if (innerHTML)
    elemText.innerHTML = innerHTML;
  else
    elemText.innerText = innerText;

  if (buttons && buttons.length > 0)
  {
    var html = "";
    __button_callbacks = {};
    for (var i = 0; i < buttons.length; i++)
    {
      __button_callbacks[buttons[i][0]] = buttons[i][1];
      if (i > 0)
        html += "&nbsp;&nbsp;&nbsp;&nbsp;";
      html += '<button id="__btn_id' + buttons[i][0] +
        '" onclick="__stop_event_propagation(event);__button_callbacks[\'' + buttons[i][0] + '\']();__close_dialog();">&nbsp;&nbsp;' +
        buttons[i][0] + '&nbsp;&nbsp;</button>';
      elemButtons.innerHTML = html;
    }

    elemDialog.onclick = null;
  }
  else
  {
    elemButtons.innerHTML = "";
    elemDialog.onclick = function(){ elemDialog.style.visibility = 'hidden'; };
  }

  elemDialog.style.visibility = "visible";
}

function show_prompt(description, defaultText, callback)
{
  defaultText = ("" + defaultText).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&#34;");

  var html = '<div>&nbsp;' + description + '</div>';
  html += '&nbsp;<input id="fullscreen_dialog_input" type="text" size="60" value="' + defaultText + '">&nbsp;';

  show_dialog(null, html, [
     ["OK", function() {
       if (callback)
         callback(document.getElementById("fullscreen_dialog_input").value);
     }],
     ["Cancel", function() {
       if (callback)
         callback(null)
     }]
     ]);

}
