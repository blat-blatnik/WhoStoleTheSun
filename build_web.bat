rem # Compile for the web using Emscripten.
rem # Assumes that emsdk is on you PATH.
rem # https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5)

@echo off
setlocal EnableDelayedExpansion

rem # Setup emsdk
rem call emsdk update
rem call emsdk install latest
call emsdk activate latest

rem # Collect all .c files for compilation into the 'input' variable.
for /f %%a in ('forfiles /s /m *.c /c "cmd /c echo @relpath"') do set input=!input! "%%~a"

rem # Compile and link
call emcc -o bin/web/index.html -Wall -L./lib -lraylib_web -s USE_GLFW=3 -s TOTAL_MEMORY=16777216 --shell-file webshell.html --preload-file res %input%

rem # Copy the favicon
rem copy /y "Pictomage.ico" "bin/web/favicon.ico"

pause
rem # Run with: $ emrun bin/web/index.html