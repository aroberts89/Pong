@echo off

set CommonCompilerFlags= -MT -WX -W4 -wd4100 -wd4189 -wd4505 -wd4201 -Oi -Od -GR- -EHa- -nologo -FC -Z7 
set CommonLinkerFlags= -opt:ref -incremental:no user32.lib gdi32.lib ole32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL
REM cl %CommonCompilerFlags% ..\gamekoo\code\gamekoo.cpp /LD /link -incremental:no -opt:ref -PDB:"gamekoo_%random%.pdb" -export:GameUpdateAndRender
cl %CommonCompilerFlags% ..\Pong\code\win32_Pong.cpp /link %CommonLinkerFlags%
popd