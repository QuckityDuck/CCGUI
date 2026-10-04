#pragma once

#include <memory>
#include <string>

// CGUI+ is a normal C++ library. It adds its own API and does not change
// C++ syntax, the compiler, or the standard library.

struct CGUIDefaultSettings
{
};

// The name is intentionally DEFULT because that is the requested public API.
inline constexpr CGUIDefaultSettings DEFULT{};

class CGUI
{
public:
    CGUI();
    ~CGUI();

    CGUI(const CGUI&) = delete;
    CGUI& operator=(const CGUI&) = delete;

    // Windows
    bool window(const std::string& name, int width, int height, bool resizable);
    bool window(const CGUIDefaultSettings&);
    bool awindow(const std::string& name, int width, int height, bool resizable);
    bool usewindow(const std::string& name);

    // Sprites
    bool sprite(const std::string& name, const std::string& path);
    bool dsprite(const std::string& name, int x, int y);
    bool sposition(const std::string& name, int x, int y);
    bool sscale(const std::string& name, float x, float y);
    bool srotate(const std::string& name, float degrees);
    bool svisible(const std::string& name, bool visible);
    bool scolor(const std::string& name, int r, int g, int b);
    bool sflip(const std::string& name, bool horizontal, bool vertical);
    bool sdelete(const std::string& name);

    // Text / fonts
    bool font(const std::string& name, const std::string& path, int size);
    void text(const std::string& value, int x, int y,
              int r = 255, int g = 255, int b = 255);

    // Drawing
    void background(int r, int g, int b);
    void rect(int x, int y, int width, int height,
              int r = 255, int g = 255, int b = 255);
    void circle(int x, int y, int radius,
                int r = 255, int g = 255, int b = 255);
    void line(int x1, int y1, int x2, int y2,
              int r = 255, int g = 255, int b = 255);

    // Simple GUI
    void button(const std::string& name, int x, int y, int width, int height);
    bool clicked(const std::string& name) const;

    // Mouse
    int mousex() const;
    int mousey() const;
    bool mousedown(const std::string& button) const;
    bool mousepressed(const std::string& button) const;

    // Keyboard
    bool keypressed(const std::string& key) const;
    bool keydown(const std::string& key) const;

    // Main loop
    bool running() const;
    void clear();
    void update();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

// The simple public object used by examples such as cg.window(...).
extern CGUI cg;
