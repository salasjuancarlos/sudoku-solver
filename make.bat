@echo off
setlocal enabledelayedexpansion

:: ===== Configuración =====
set CC=g++
set SOURCE=src/main.cpp
set TARGET=.\bin\main.exe
set INCLUDE=include
set FLAGS=-Ofast -march=native -mtune=native -flto -fomit-frame-pointer -funroll-loops -fno-plt ^
-DNDEBUG -std=c++20 -fno-exceptions -fno-rtti -ffast-math -fno-stack-protector -fwhole-program

:: ===== Compilación =====
%CC% %SOURCE% -o %TARGET% -I%INCLUDE% %FLAGS%

:: ===== Ejecución =====
if not exist %TARGET% (
    echo Ejecutando programa...
    %TARGET%
) else (
    echo Error: No se pudo compilar el programa
    exit /b 1
)

endlocal