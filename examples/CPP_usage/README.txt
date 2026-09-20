Using the C++ wrapper in MounRiver Studio
=========================================

The four numbered example projects are plain C projects, and they open and
build as-is. C++ needs a project that MounRiver created with the C++ nature,
so it takes four steps rather than a copy-paste.

1. File > New > MounRiver Project
     Chip series : CH32V003
     MCU         : CH32V003F4P6
     Project type: tick the C++ option
   Give it a name and let MRS generate the usual Core / Debug / Ld / Peripheral
   / Startup / User folders.

2. Link the driver into the project.
     right-click the project > New > Folder > Advanced
     > "Link to alternate location (Linked Folder)"
     > browse to  <this repo>\lib
     > name the folder  Lcd

   Or copy ch32v003_lcd1602.c/.h, lcd1602_config.h and LCD1602.hpp straight
   into the project's User folder if you would rather not share the library.

3. Add the include path.
     Project > Properties > C/C++ Build > Settings
     > GNU RISC-V Cross C Compiler   > Includes > add  ${ProjName}/Lcd
     > GNU RISC-V Cross C++ Compiler > Includes > add  ${ProjName}/Lcd

   Both compilers need it: the driver .c is compiled by the C compiler, your
   main.cpp by the C++ one.

4. Replace User/main.cpp with the main.cpp next to this file, then build.

Optional: to use lcd.printf(...), set
     C/C++ Build > Settings > GNU RISC-V Cross C++ Compiler > Language standard
to ISO C++11 (-std=gnu++11) or newer. Without it the rest of the class still
compiles - only the variadic-template printf is compiled out.

Note on ch32v003_lcd1602.h: it already carries extern "C" guards, so including
it (or LCD1602.hpp) from C++ needs nothing extra. debug.h in the WCH SRC tree
has its own guards too, but the example wraps it anyway so the file still works
if you drop it into an older EVT package.
