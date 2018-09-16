@echo off
title Creating Conan demo package
set /p name= Package name? 
set /p qt-sdk= Qt SDK folder? 
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
xcopy /Y /S /V /I "ConanApp\doc\html" "%name%\doc\html\"
xcopy /Y /S /V /I "ConanApp\src\*.h" "%name%\src\"
xcopy /Y /S /V /I "ConanApp\src\*.cpp" "%name%\src\"
xcopy /Y /S /V /I "ConanApp\src\*.ui" "%name%\src\"
xcopy /Y /S /V /I "ConanApp\src\*.qrc" "%name%\src\"
xcopy /Y    /V /I "ConanApp\win32\ConanApp.vcproj" "%name%\win32\"
xcopy /Y    /V /I "Conan\README.txt" "%name%\"
xcopy /Y    /V /I "Conan\GPL.txt" "%name%\"
xcopy /Y    /V /I "ConanApp\ConanApp.pro" "%name%\"
xcopy /Y    /V /I "ConanApp\bin\ConanDemo.exe" "%name%\bin\"
xcopy /Y    /V /I "Conan\lib\Conan1.dll" "%name%\bin\"
xcopy /Y    /V /I "%qt-sdk%\qt\bin\mingwm10.dll" "%name%\bin\"
xcopy /Y    /V /I "%qt-sdk%\qt\bin\libgcc_s_dw2-1.dll" "%name%\bin\"
xcopy /Y    /V /I "%qt-sdk%\qt\bin\QtCore4.dll" "%name%\bin\"
xcopy /Y    /V /I "%qt-sdk%\qt\bin\QtGui4.dll" "%name%\bin\"
REM create archive
"C:\Program Files\7-Zip\7z" a -tzip -mx9 "%name%.zip" %name%
cd ConanApp/win32
if NOT ERRORLEVEL == 0 goto exiterror
exit /b

:exiterror
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !!!!!!!!!!could not create a Conan demo package!!!!!!!!!!
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
exit /b 1