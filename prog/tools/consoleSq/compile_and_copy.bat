set Config=dev
jam -sConfig=%Config%
jam -sPlatform=win64 -sConfig=%Config%
copy ..\..\..\tools\dagor3_cdk\util\csq-%Config%.exe ..\..\..\tools\util\csq.exe
