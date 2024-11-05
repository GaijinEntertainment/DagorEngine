.. _repl:

######################################
  Try Quirrel online
######################################

.. raw:: html

  <script>
    function loadCss(url) {
      var link = document.createElement("link")
      link.type = "text/css"
      link.rel = "stylesheet"
      link.href = url
      document.getElementsByTagName("head")[0].appendChild(link)
    }
    loadCss("../_static/repl-rst.css")
  </script>

  <div id="repl-root">
    <div class="editor-container container-source">
      <div class="editor-header">
        <span>Source code</span>
        <button id="btn-run">Run (Ctrl+Enter)</button>
      </div>
      <pre id="source-editor" class="editor-area"></pre>
    </div>
    <div class="editor-container container-output">
      <div class="editor-header">
        <span>Result</span>
      </div>
      <pre id="output" class="output"></pre>
    </div>
  </div>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.4.14/ace.js"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>

  <script>
    // Setup editor
    const sourceEditor = ace.edit("source-editor", {minLines:32, maxLines:32})
    sourceEditor.setShowPrintMargin(false)
    sourceEditor.setValue(localStorage.getItem('sourceCode') || 'println("Hello, Quirrel!")')
    sourceEditor.moveCursorTo(0, 0)


    function execute() {
      const sourceCode = sourceEditor.getValue()
      localStorage.setItem('sourceCode', sourceCode)
      const result = Module.runScript(sourceCode)
      $('#output').text(result)
    }

    $('#btn-run').on('click', execute)

    document.addEventListener('keyup', (e)=>{
      if (e.key === "Enter" && e.ctrlKey) {
        execute()
        e.preventDefault()
      }
    }, false)

    // Show page
    $('#repl-root').css({'display': 'block'})

    // WebAssembly
    var Module = {
      onRuntimeInitialized: function() {
        // Initialize here
      }
    }

  </script>

  <script src="../_static/sqjsrepl.js"></script>


Project source code is available at https://github.com/GaijinEntertainment/quirrel

Copyright (c) 2003-2016 Alberto Demichelis
Copyright (c) 2016-2024 Gaijin Games KFT
