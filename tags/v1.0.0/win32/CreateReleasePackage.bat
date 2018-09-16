@echo off
title Creating Conan release package
set /p name= Package name?
REM Cleaning old docs
rd /S /Q "..\doc\html"
REM generate code doc
cd ..\doc
doxygen Doxyfile
cd ..\win32
if NOT ERRORLEVEL == 0 goto exiterror
cd ..\..
REM copy files
md "%name%"
xcopy /Y /S /V /I "Conan\doc\html" "%name%\doc\html\"
xcopy /Y /S /V /I "Conan\doc\images\*.jpg" "%name%\doc\images\"
xcopy /Y    /V /I "Conan\doc\Doxyfile" "%name%\doc\"
xcopy /Y /S /V /I "Conan\icons\*.png" "%name%\icons\"
xcopy /Y /S /V /I "Conan\icons\*.txt" "%name%\icons\"
xcopy /Y /S /V /I "Conan\include\*.h" "%name%\include\"
xcopy /Y /S /V /I "Conan\src\*.h" "%name%\src\"
xcopy /Y /S /V /I "Conan\src\*.cpp" "%name%\src\"
xcopy /Y /S /V /I "Conan\src\*.ui" "%name%\src\"
xcopy /Y /S /V /I "Conan\src\*.qrc" "%name%\src\"
xcopy /Y    /V /I "Conan\win32\Conan.vcproj" "%name%\win32\"
xcopy /Y    /V /I "Conan\win32\QTRules.rules" "%name%\win32\"
xcopy /Y    /V /I "Conan\CHANGES.txt" "%name%\"
xcopy /Y    /V /I "Conan\GPL.txt" "%name%\"
xcopy /Y    /V /I "Conan\README.txt" "%name%\"
xcopy /Y    /V /I "Conan\TODO.txt" "%name%\"
xcopy /Y    /V /I "Conan\Conan.pro" "%name%\"
REM create archive
"C:\Program Files\7-Zip\7z" a -tzip -mx9 "%name%.zip" %name%
cd conan/win32
if NOT ERRORLEVEL == 0 goto exiterror
exit /b

:exiterror
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !!!!!!!!!!could not create a Conan release package!!!!!!!!!!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
exit /b 1