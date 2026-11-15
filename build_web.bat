@echo off
setlocal
call C:\emsdk\emsdk_env.bat

if not exist dist mkdir dist
copy /Y web\index.html dist\index.html >nul

set SRCS=src\main.c src\data.c src\seg000.c src\seg001.c src\seg002.c src\seg003.c src\seg004.c src\seg005.c src\seg006.c src\seg007.c src\seg008.c src\seg009.c src\seqtbl.c src\replay.c src\options.c src\lighting.c src\screenshot.c src\menu.c src\midi.c src\opl3.c src\stb_vorbis.c src\fx.c

emcc -std=c99 -O2 -D_GNU_SOURCE=1 %SRCS% -o dist\prince.js ^
  -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=["png"] ^
  -s ASYNCIFY -s ASYNCIFY_STACK_SIZE=65536 ^
  -s ALLOW_MEMORY_GROWTH=1 ^
  -s EXPORTED_RUNTIME_METHODS=["FS"] -lidbfs.js ^
  --preload-file data@/data --preload-file SDLPoP.ini@/SDLPoP.ini -lm

if errorlevel 1 (
  echo.
  echo [ERROR] Fallo la compilacion WebAssembly.
  exit /b 1
)
echo.
echo [OK] Compilacion lista en .\dist\

