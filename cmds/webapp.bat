@echo off

::set app here (replace "%1")
set app=%1

setlocal ENABLEDELAYEDEXPANSION
::List of browsers to try
set browsers[0]=chrome.exe
set browsers[1]="C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"
set browsers[2]="C:\Program Files (x86)\Google\Chrome\chrome.exe"
set browsers[3]=msedge
set "max_x=4"
set "x=0"

if /i "%app~0,4%"=="http" goto :start

set app=http://%app%

:start

:Loop
if not !x! == !max_x! (
	!!browsers[%x%]!! --new-window --app=!app!
	if not errorlevel 1 GOTO END
	set /a "x+=1"
	GOTO :Loop
)
:END
