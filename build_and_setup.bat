@echo off
REM Abyss Restaurant - compile C++ and (re)build materials + map.
set UE=F:\Epic Games\UE_5.6
set PROJ=%~dp0AbyssRestaurant.uproject

echo === Compiling C++ ===
call "%UE%\Engine\Build\BatchFiles\Build.bat" AbyssRestaurantEditor Win64 Development "-Project=%PROJ%" -WaitMutex -NoHotReloadFromIDE
if errorlevel 1 (
  echo BUILD FAILED
  pause
  exit /b 1
)

echo === Building materials + level (editor will open and close) ===
"%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJ%" -ExecutePythonScript="%~dp0Scripts\setup_project.py" -AbyssSetupQuit -nosplash
echo Done. Open AbyssRestaurant.uproject and press Play.
pause
