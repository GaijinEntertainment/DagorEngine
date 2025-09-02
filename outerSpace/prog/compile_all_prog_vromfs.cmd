call create_vfsroms.bat
if ERRORLEVEL 1 goto on_error

goto EOF

:on_error
echo "ERROR!!"
pause
exit /b 1

:EOF
