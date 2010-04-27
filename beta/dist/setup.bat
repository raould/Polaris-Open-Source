
rem  Copyright 2010 Hewlett-Packard under the terms of the MIT X license
rem  found at http://www.opensource.org/licenses/mit-license.html

@echo off
echo.


rem - Only administrators get full access.  Ordinary users can traverse
rem - the filesystem or read the Windows and Program Files directories.
rem - If users do not have traverse authority from root,
rem -  then Office apps crash opening file dialogs.

echo.
echo.
echo Setting filesystem permissions. This may take a while,
echo    depending on how many files you have and where they are
echo    located in your system....
echo.

rem can I make this all run in background?

rem wipe out all non admin authorities
cscript xcacls.vbs C:\ /g administrators:f /q

rem grant users list contents so office dialogs work
cscript xcacls.vbs c:\ /e /g users:l /q

cscript xcacls.vbs %windir% /e /g users:r /q
cscript xcacls.vbs "C:\Program Files" /e /g users:r /q

echo.
echo Installing Polaris files.
mkdir "c:\program files\Hewlett-Packard"
mkdir "c:\program files\Hewlett-Packard\Polaris"

xcopy /s /y /h *.*  "c:\program files\Hewlett-Packard\Polaris\*.*" >nul

echo.
echo Installing Excel virus demo/test
mkdir "c:\trash"
copy killer.xls "%USERPROFILE%\My Documents\killer.xls"


start "blah" /B "c:\program files\Hewlett-Packard\Polaris\PowerWindow.exe"

echo.
echo.
echo Automated portion of Polaris installation is done.  Further manual steps:

echo Use the Polarizer to secure individual applications.
echo Manually confirm that directory traverse checking is turned off.
echo If desired, Install outlook email attachment
echo     launcher using "setupOutlook"
echo.
echo Press enter to continue.
echo.
echo We will now reboot after you again press enter to continue.
echo.
pause
echo.
echo.
echo Let use be perfectly clear, we really
echo   will force a reboot after you press enter.
echo.
echo DO NOT press enter until the PowerWindow has appeared
echo in the system tray, if it has not done so already
echo.
echo If you want to postpone the forced reboot, just minimize this dialog box,
echo    and avoid pressing any keys with this window active.
echo.
echo Ok, the next key you type in this window will start the reboot.
echo.
pause
shutdown -r -t 10 -c "Shutdown to complete Polaris Installation"
