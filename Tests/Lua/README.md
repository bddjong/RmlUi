# Lua colour regression tests

These tests exercise Lua property dispatch. They are registered with CTest when
Lua bindings are enabled. With a C++17 compiler and Lua development package,
they can also be run independently:

```sh
c++ -std=c++17 -I Include $(pkg-config --cflags luajit) \
  Tests/Lua/colourb.cpp Source/Lua/Colourb.cpp Source/Lua/LuaType.cpp \
  $(pkg-config --libs luajit) -o /tmp/rmlui-colourb-test
/tmp/rmlui-colourb-test Tests/Lua/colourb.lua
c++ -std=c++17 -I Include $(pkg-config --cflags luajit) \
  Tests/Lua/colourf.cpp Source/Lua/Colourb.cpp Source/Lua/Colourf.cpp Source/Lua/LuaType.cpp \
  $(pkg-config --libs luajit) -o /tmp/rmlui-colourf-test
/tmp/rmlui-colourf-test Tests/Lua/colourf.lua
```

Run from the RmlUi root. Substitute the appropriate pkg-config package for
another supported Lua version. The `rgba` property reads and writes a snapshot
table with four required, 1-based RGBA components. Components use the existing
Lua integer conversion and narrow to bytes; numeric strings are accepted.
Invalid assignments leave the colour unchanged. Float components retain Lua's
numeric conversion without clamping.
