@echo off
SETLOCAL enabledelayedexpansion

:: Get current date and time
for /f "delims=" %%a in ('wmic OS Get localdatetime ^| find "."') do set datetime=%%a
set year=!datetime:~0,4!
set month=!datetime:~4,2!
set day=!datetime:~6,2!
set hour=!datetime:~8,2!
set minute=!datetime:~10,2!
set second=!datetime:~12,2!

:: Create a Git tag with the current date and time
set tag=v-%year%%month%%day%-%hour%%minute%%second%
git tag %tag%

:: Push the tag to the remote repository
git push origin %tag%

:: Display a success message
echo Tag %tag% created and pushed successfully!

:: End of script