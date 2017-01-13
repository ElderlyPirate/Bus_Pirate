rem Drag and drop Bus Pirate firmware to this batch file
pause
pirate-loader.exe --dev=COM4 --hex=%1
pause