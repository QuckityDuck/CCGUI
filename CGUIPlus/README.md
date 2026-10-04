# CGUI+

CGUI+ is a small, normal C++ library for simple Windows GUI and 2D graphics.

It does **not** change C++ syntax, replace the C++ compiler, or replace the C++ standard library. You include one header and call ordinary C++ functions on the `cg` object.

## Backend

CGUI+ uses the Windows API directly with GDI+ for window creation, input, images, text, and 2D drawing.

It does **not** use raylib, SDL, SFML, GLFW, OpenGL helper frameworks, or CMake.

## Project structure

```text
CGUI+/
├── include/
│   └── cgui.hpp
├── src/
│   └── cgui.cpp
├── examples/
│   ├── main.cpp
│   └── assets/
│       └── player.png
├── build.bat
└── README.md
```

## Requirements

- Windows
- MinGW-w64 / MSYS2 with `g++`
- A normal C++17 compiler

No separate graphics framework is required. GDI+ and the other linked Windows system libraries are part of Windows.

## Build

Open a Command Prompt in the CGUI+ folder and run:

```bat
build.bat
```

The script creates:

```text
build\libcgui.a
build\cgui_example.exe
```

It links these Windows system libraries:

```text
user32
gdi32
gdiplus
```

## Run the example

From the CGUI+ folder:

```bat
build\cgui_example.exe
```

The example demonstrates a sprite, text, rectangles, a circle, a line, a button, keyboard input, and the main loop.

## Your own program

Create a normal C++ file such as `main.cpp`:

```cpp
#include <cgui.hpp>
#include <iostream>

int main()
{
    cg.window("My Window", 800, 600, true);

    while (cg.running())
    {
        cg.clear();

        cg.text("Hello World!", 100, 100);
        cg.rect(100, 150, 200, 100, 255, 0, 0);

        if (cg.keypressed("E"))
        {
            std::cout << "E pressed\n";
        }

        cg.update();
    }

    return 0;
}
```

Build it from the CGUI+ folder with:

```bat
g++ -std=c++17 -Iinclude main.cpp build\libcgui.a -o my_program.exe -luser32 -lgdi32 -lgdiplus
```

Then run:

```bat
my_program.exe
```

## Windows

Create a named window:

```cpp
cg.window("Window 1", 800, 600, true);
```

Create the default window:

```cpp
cg.window(DEFULT);
```

Change an existing window:

```cpp
cg.awindow("Window 1", 1000, 700, false);
```

Create another window:

```cpp
cg.window("Settings", 400, 300, false);
```

Select a window:

```cpp
cg.usewindow("Settings");
```

Then drawing calls target that named window.

## Sprites

Load an image:

```cpp
cg.sprite("player", "player.png");
```

Draw it:

```cpp
cg.dsprite("player", 100, 100);
```

Change it by name:

```cpp
cg.sposition("player", 200, 150);
cg.sscale("player", 2, 2);
cg.srotate("player", 45);
cg.svisible("player", true);
cg.scolor("player", 255, 0, 0);
cg.sflip("player", true, false);
cg.sdelete("player");
```

There is no selected/current sprite. Every sprite-manipulation function takes the sprite name.

## Text and drawing

```cpp
cg.text("Hello", 100, 100);
cg.text("Red", 100, 140, 255, 0, 0);

cg.rect(100, 200, 200, 100);
cg.circle(400, 250, 50);
cg.line(0, 0, 500, 500);
cg.background(30, 30, 30);
```

Font selection is deliberately simple: the last successful `font()` call becomes the font used by `text()` and `button()`.

For a normal system font:

```cpp
cg.font("Arial", "", 24);
```

For a font file:

```cpp
cg.font("MyFont", "myfont.ttf", 24);
```

## Buttons

```cpp
cg.button("Play", 100, 100, 200, 50);

if (cg.clicked("Play"))
{
    std::cout << "Play!\n";
}
```

Button names identify the button for the frame and for `clicked()`.

## Mouse

```cpp
int x = cg.mousex();
int y = cg.mousey();

if (cg.mousedown("LEFT"))
{
}

if (cg.mousepressed("LEFT"))
{
}
```

`LEFT` / `LMB` and `RIGHT` / `RMB` are accepted.

## Keyboard

```cpp
if (cg.keypressed("E"))
{
}

if (cg.keydown("SPACE"))
{
}
```

Examples of readable key names include:

```text
A-Z
0-9
SPACE
ENTER
ESC
TAB
BACKSPACE
SHIFT
CTRL
ALT
LEFT
RIGHT
UP
DOWN
F1-F24
```

## Errors

CGUI+ reports simple problems to `std::cerr` instead of deliberately terminating the application. For example:

```text
CGUI+ ERROR: Sprite 'player' was not found.
```

## Full example

```cpp
#include <cgui.hpp>
#include <iostream>

int main()
{
    cg.window("Game", 800, 600, true);

    cg.sprite("player", "player.png");

    cg.sposition("player", 100, 100);
    cg.sscale("player", 2, 2);

    while (cg.running())
    {
        cg.clear();

        cg.dsprite("player", 100, 100);
        cg.text("Hello!", 50, 50);

        if (cg.keypressed("E"))
        {
            cg.svisible("player", false);
        }

        cg.update();
    }

    return 0;
}
```
