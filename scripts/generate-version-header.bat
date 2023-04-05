@echo off
if "%1" == "" (
     echo "Usage: %0 <output-filename>" 1>&2
     exit 1
)

set output_file=%1
rem Accept forward slashes as directory separator for better portability.
set "output_file=%output_file:/=\%"
set output_file_tmp=%output_file%.tmp

for /f %%i in ('git describe --tags --always --dirty') do set version=%%i
set e=%ERRORLEVEL%
if "%version%" == "" (
    echo Failed to get version info via git. 1>&2
    echo Ensure git is in path and project is cloned by git. 1>&2
    exit %e%
)

echo /**>%output_file_tmp%
echo  * @brief Version string generated using git describe, @see BP_FIRMWARE_STRING>>%output_file_tmp%
echo  */>>%output_file_tmp%
echo #define BP_GIT_VERSION_STRING "%version%">>%output_file_tmp%


rem Only update file if it has changed (avoids unneccessary rebuilds)
fc /b %output_file% %output_file_tmp% > NUL
IF %ERRORLEVEL% NEQ 0 (
    copy %output_file_tmp% %output_file%
    echo Updated Git version string in %output_file%: %version%.
) else (
    echo No update needed for Git version string in %output_file%: %version%.
)

rem Cleanup
del %output_file_tmp% > NUL
