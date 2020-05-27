@echo off

del *.exe
 
set incs=-DNDEBUG -DSTRICT -DNDEBUG -DWIN32 -DINC_FLEN -I../Lib3 -DTRACE_ON

set opts=-fwhole-program -fstrict-aliasing -fomit-frame-pointer -ffast-math ^
-fno-rtti -fno-exceptions -fpermissive  ^
-fno-stack-protector -fno-stack-check -fno-check-new

:set gcc=C:\MinGW710\bin\g++.exe -m32 
set gcc=C:\MinGW820x\bin\g++.exe
set gcc=C:\MinGW810\bin\g++.exe -m32
set gcc=C:\MinGW810x\bin\g++.exe 
set gcc=C:\MinGW510\bin\g++.exe -m32 -mno-sse
set gcc=C:\MinGW820\bin\g++.exe -m32 -mno-sse
set gcc=C:\MinGW820x\bin\g++.exe -m64 -march=k8 -mno-sse
set path=%gcc%\..\

del *.exe *.o

%gcc% -std=gnu++1z -O9 -s %incs% %opts% -static cdm.cpp -o cdm.exe

