@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4505 -wd4996 -wd4312 -wd4661 -DQLIB_INTERNAL=0 -DQLIB_SLOW=0 -DSAVE_IMAGES=1 -DQLIB_WIN32=1 -DVSYNC=0 -FC -Z7

set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib 

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

cl %CommonCompilerFlags% -DQLIB_WINDOW=1 -DQLIB_OPENGL=1 C:\Snake\game\code\application.cpp -Fmwin32_application.map /link %CommonLinkerFlags%

cl %CommonCompilerFlags% -DQLIB_WINDOW=0 -DQLIB_OPENGL=0 C:\Snake\game\code\server.cpp -Fmwin32_application.map /link %CommonLinkerFlags%


popd