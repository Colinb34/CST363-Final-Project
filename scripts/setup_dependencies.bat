@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "CPP_EXTERNAL_DIR=%PROJECT_DIR%\cpp\external"
set "GODOT_CPP_DIR=%CPP_EXTERNAL_DIR%\godot-cpp"

echo Preparing Godot C++ dependencies...
if not exist "%CPP_EXTERNAL_DIR%" mkdir "%CPP_EXTERNAL_DIR%"

if exist "%GODOT_CPP_DIR%" (
  echo godot-cpp already exists.
) else (
  echo Cloning godot-cpp into cpp\external\godot-cpp
  git clone https://github.com/godotengine/godot-cpp.git "%GODOT_CPP_DIR%"
)

echo Done.
