echo off
rem use the release csq built from this repo (jam -sConfig=rel in the consoleSq tool)
set CSQ=..\..\..\..\..\..\tools\dagor_cdk\windows-x86_64\csq.exe
%CSQ% version.nut
%CSQ% fib_recursive.nut
%CSQ% fib_loop.nut
%CSQ% primes.nut
%CSQ% particles.nut
%CSQ% dict.nut
%CSQ% exp.nut
%CSQ% nbodies.nut
%CSQ% f2i.nut
%CSQ% f2s.nut
%CSQ% queen.nut
%CSQ% spectral-norm.nut
%CSQ% table-sort.nut
