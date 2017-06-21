@echo off

setlocal

set LIBSDL2=G:\Program\mingw-w64\mingw64\x86_64-w64-mingw32\lib

set LIB=-L%LIBSDL2%


set INCLUDEROOT=src
set INCLUDESDL2=G:\Program\mingw-w64\mingw64\x86_64-w64-mingw32\include\SDL2

set INCLUDE=-I%INCLUDEROOT% -I%INCLUDESDL2%


set SRCIO=src\io\*.c
set SRCAUDIO=src\audio\*.c
set SRCROOT=src\*.c

set SRC=%SRCROOT% %SRCAUDIO% %SRCIO%


set BINDIR=bin
set OUTPUTFILENAME=audiosynth


@echo on

gcc -std=c11 -Wall -Wextra %SRC% %INCLUDE% %LIB% -lmingw32 -lSDL2main -lSDL2 -o %BINDIR%\%OUTPUTFILENAME%.exe

@echo off


endlocal
