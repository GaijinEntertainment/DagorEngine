// Setup editor
const sourceEditor = ace.edit("source-editor")
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
  if (e.key === 'F9')
    execute()
}, false)

// Show page
$('#repl-root').css({'display': 'flex'})

// WebAssembly
var Module = {
  onRuntimeInitialized: function() {
    // Initialize here
  }
}
