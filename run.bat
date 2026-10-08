@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul || (echo CMake is required. Install CMake and add it to PATH.& exit /b 1)
cmake -S . -B build
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
where python >nul 2>nul
if errorlevel 1 (
  where py >nul 2>nul || (echo Python 3 is required.& exit /b 1)
  py -3 -m venv .venv
) else (
  if not exist .venv\Scripts\python.exe python -m venv .venv
)
if errorlevel 1 exit /b 1
if exist .venv\Scripts\python.exe (set "PY=.venv\Scripts\python.exe") else set "PY=.venv\bin\python.exe"
"%PY%" -c "import flask, flask_cors" >nul 2>nul
if errorlevel 1 "%PY%" -m pip install -r backend\requirements.txt
if errorlevel 1 exit /b 1
echo Open http://127.0.0.1:5000
"%PY%" backend\app.py
