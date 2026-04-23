@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."

pushd "%PROJECT_DIR%\cpp"
scons platform=windows target=template_debug generate_bindings=yes
popd
