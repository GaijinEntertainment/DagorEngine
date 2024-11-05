if exist %GDEVTOOL%/python3/python3.exe (
    %GDEVTOOL%/python3/python3.exe __build_pack.py
) else (
    python3 __build_pack.py
)
