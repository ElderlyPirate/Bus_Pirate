@echo "Building for WINDOWS"

@cl source/pirate-loader.c /DWIN32=1 /DNDEBUG=1 /D_CRT_SECURE_NO_DEPRECATE=1 >out.log 2>err.log || exit

@del pirate-loader.obj
@del out.log
@del err.log

pause