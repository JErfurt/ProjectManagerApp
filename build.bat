@echo off
setlocal

set "BUILD_DIR=build"
set "PUBLISH_DIR=publish"

set "COMPILER_ARGS="
where gcc >nul 2>&1
if not errorlevel 1 (
    set "COMPILER_ARGS=-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"
)

cmake -S . -B "%BUILD_DIR%" -G Ninja %COMPILER_ARGS% -DCMAKE_BUILD_TYPE=Release -DPROJECTMANAGER_STATIC_BUILD=ON
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --parallel
if errorlevel 1 exit /b %errorlevel%

if not exist "%PUBLISH_DIR%" mkdir "%PUBLISH_DIR%"
copy /Y "%BUILD_DIR%\ProjectManagerApp.exe" "%PUBLISH_DIR%\ProjectManagerApp.exe" >nul
if errorlevel 1 exit /b %errorlevel%

echo Build completed: %PUBLISH_DIR%\ProjectManagerApp.exe
endlocal
