for /D %%D in ("%USERPROFILE%\.vscode\extensions\ms-vscode.cpptools-*") do (
	md "%%D\debugAdapters"
	md "%%D\debugAdapters\vsdbg"
	md "%%D\debugAdapters\vsdbg\bin"
	md "%%D\debugAdapters\vsdbg\bin\Visualizers"
	copy /Y *.natvis "%%D\debugAdapters\vsdbg\bin\Visualizers"
	)