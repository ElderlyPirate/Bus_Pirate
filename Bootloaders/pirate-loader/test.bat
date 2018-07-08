@ECHO OFF

REM This file is part of the Bus Pirate project
REM (http://code.google.com/p/the-bus-pirate/).
REM
REM Written and maintained by the Bus Pirate project.
REM
REM To the extent possible under law, the project has
REM waived all copyright and related or neighboring rights to Bus Pirate. This
REM work is published from United States.
REM
REM For details see: http://creativecommons.org/publicdomain/zero/1.0/.
REM
REM This program is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

ECHO Running on Windows...

SET UPLOADER="pirate-loader.exe"

IF [%1]==[] SET PORT="COM3"
IF NOT [%1]==[] SET PORT=%1

IF EXIST %UPLOADER% (
	pirate-loader.exe --dev=%PORT% --hello
) ELSE (
	ECHO ERROR
	ECHO Cannot find bootloader update program!
)

ECHO DONE

PAUSE
