@echo off
rem AOEII-WXR - build the 32-bit d3d9 proxy.
rem
rem Win32 / Release only. The game is a 32-bit process; a 64-bit DLL will not
rem load into it and the failure is silent (CLAUDE.md, Hard constraints).
rem
rem Usage:  build\build.bat
rem Output: build\out\d3d9.dll

setlocal
set ROOT=%~dp0..
set OUT=%ROOT%\build\out

set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARS%" (
  echo ERROR: vcvarsall.bat not found at:
  echo   %VCVARS%
  echo Edit build\build.bat if Visual Studio lives somewhere else.
  exit /b 1
)

if not exist "%OUT%" mkdir "%OUT%"

echo === configuring x86 toolchain
call "%VCVARS%" x86 >nul
if errorlevel 1 (
  echo ERROR: vcvarsall x86 failed
  exit /b 1
)

echo === checking the compiler really is 32-bit
cl 2>&1 | findstr /C:"for x86" >nul
if errorlevel 1 (
  echo ERROR: cl.exe is not targeting x86. Refusing to build a DLL the game
  echo        cannot load.
  exit /b 1
)

echo === compiling
pushd "%OUT%"
cl /nologo /c /O2 /MT /W4 /EHsc /GS- /DNDEBUG /D_CRT_SECURE_NO_WARNINGS ^
   /DWIN32_LEAN_AND_MEAN "%ROOT%\src\log.cpp" "%ROOT%\src\d3d9_proxy.cpp"
if errorlevel 1 (
  popd
  echo ERROR: compile failed
  exit /b 1
)

echo === linking
link /nologo /DLL /MACHINE:X86 /OUT:d3d9.dll /DEF:"%ROOT%\src\d3d9.def" ^
     log.obj d3d9_proxy.obj kernel32.lib user32.lib advapi32.lib
if errorlevel 1 (
  popd
  echo ERROR: link failed
  exit /b 1
)
popd

echo.
echo === built %OUT%\d3d9.dll
powershell -NoProfile -Command "$f='%OUT%\d3d9.dll'; $h=(Get-FileHash $f -Algorithm SHA256).Hash; Write-Host ('    ' + (Get-Item $f).Length + ' bytes'); Write-Host ('    sha256 ' + $h)"
echo.
echo Next: build\deploy.ps1 -GameDir "<path to age2_x1>"
endlocal
