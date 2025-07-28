@echo off

rem -----------------------------------------------------
rem SET THE PATH TO YOUR GLFW LIBRARIES AND INCLUDES HERE
rem -----------------------------------------------------
set libDir=../OpenGLStuff/lib
set includeDir=../OpenGLStuff/includes

set commonCompilerFlags= -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4101 -wd4701 -wd4201 -wd4100 -wd4189 -FC -Z7 -DENABLE_GRID=0
set commonLinkerFlags= -incremental:no -opt:ref  
set libraries= user32.lib gdi32.lib opengl32.lib glfw3.lib shell32.lib

rem -GR- and -EHa- turn off exception handling stuff
rem -Oi turns on commpiler intrisics (replaces c functions if it knows how to do it in assembly)
rem -Z7 and -Zi are for debugging. -Z7 does not produce vc140.pdb
rem -nologo gets rid of compiler logo
rem -MT gets rid of multithread dll bug, -MTd is the debug version
rem -Gm- turns off minimal rebuild
rem -Od turns off all optimisation
rem -incremental:no turns off incremental build

mkdir 
pushd 

rem Debug build
cls
del *.pdb > NUL 2> NUL
cl  -DDEBUG=1 %commonCompilerFlags% -MDd /EHsc code/tetris.cpp /I%includeDir% /I/includes /link %commonLinkerFlags% /LIBPATH:%libDir% %libraries% /out:tetris.exe 

rem Release build
rem cl  -DDEBUG=0 %commonCompilerFlags% -MT /EHsc code\tetris.cpp /I %includeDirs% /link %commonLinkerFlags% -libpath:"..\libRelease" %libraries% /out:TCtetris_1.1.exe 

rem version Names
rem /out:TCtetris_1.0.exe 

popd

rem /EHsc adds in error handler
rem /LD builds as DLL when using "cd" command
rem /PDB:(name).pdb creates a pdb with the file name
rem %date:~-4,4%%date:~-10,2%%date:~-7,2% is noise for visual studio pdb bypassing