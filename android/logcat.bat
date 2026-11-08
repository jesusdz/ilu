@echo off

REM Configure environment variables
call environment.bat

if %errorlevel% neq 0 (
	echo Could not prepare the environment.
	exit /b 1
)

call %PLATFORM_TOOLS%\adb logcat tools:V *:S

