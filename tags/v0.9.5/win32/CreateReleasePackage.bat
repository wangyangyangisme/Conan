@echo off
title Creating Conan release package
REM Cleaning old docs
rd /S /Q "../doc/html"
REM qmake build output
rd /S /Q "../debug"
rd /S /Q "../release"
del /Q "..\Makefile*"
del /Q "..\*.debug"
del /Q "..\*.release"
del /Q "..\*.user"
REM vs2008 build output
vcbuild /clean /time Conan.vcproj
REM generate code doc
cd ../doc
doxygen Doxyfile
if NOT ERRORLEVEL == 0 goto exiterror
cd ../win32
REM build Conan
vcbuild /rebuild /time Conan.vcproj
if NOT ERRORLEVEL == 0 goto exiterror
REM copy Conan
cd ../../
xcopy /Y /S /V /I conan "tempConan/Conan"
if NOT ERRORLEVEL == 0 goto exiterror
REM clean Conan
cd tempConan/conan
del /Q bin
rd /S /Q int
cd win32
del /Q *.user
cd ../../../
if NOT ERRORLEVEL == 0 goto exiterror
REM clean old archives
del "conan.7z"
del "conan.zip"
REM create archives
cd tempConan
7z a -t7z -mx9 "../conan.7z" conan
if NOT ERRORLEVEL == 0 goto exiterror
7z a -tzip -mx9 "../conan.zip" conan
if NOT ERRORLEVEL == 0 goto exiterror
REM cleanup
cd ..
rd /S /Q tempConan
cd conan/win32
exit /b

:exiterror
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !!!!!!!!!!could not create a Conan release package!!!!!!!!!!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
exit /b 1